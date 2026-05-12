#define BLYNK_TEMPLATE_ID "TMPL3HrH4KUPw"
#define BLYNK_TEMPLATE_NAME "Water Level System"
#define BLYNK_AUTH_TOKEN "Ee5OjUZT-0K33JBvV7SrMbyBCQxe0cqE"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Preferences.h>
#include <WebServer.h>
#include <vector>
#include <algorithm>

// -------- PINS --------
#define TRIG_PIN 7
#define ECHO_PIN 6
#define PUMP_1_PIN 10
#define PUMP_2_PIN 20
#define STATUS_LED 8
#define RESET_WIFI_PIN 3   // PUSH BUTTON

WebServer server(80);
Preferences pref;

// -------- HTML PAGES --------
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head><title>Water System</title></head>
<body>
<h2>Water Level System</h2>
<a href="/config">WiFi Settings</a>
</body>
</html>
)rawliteral";

const char CONFIG_PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>WiFi Setup</title>
<style>
body { font-family: Arial; text-align: center; }
input { padding: 10px; margin: 5px; width: 200px; }
button { padding: 10px 20px; margin: 10px; }
</style>
</head>
<body>

<h2>WiFi Configuration</h2>

<p>Connected SSID: <b>%SSID%</b></p>

<form action="/save" method="POST">
<input name="ssid" placeholder="WiFi Name"><br>
<input name="pass" placeholder="Password" type="password"><br>
<button type="submit">Save</button>
</form>

<hr>

<h3>Reset WiFi</h3>
<button onclick="location.href='/reset'">Reset & Reboot</button>

</body>
</html>
)rawliteral";

// -------- SYSTEM VARIABLES --------
float ugHeight = 150.0;
float ugLevel = 0.0;
float currentOHLevel = 100.0;

bool pump1Active = false;
bool pump2Active = false;
bool autoModeActive = false;

unsigned long lastScanTime = 0;
unsigned long scanInterval = 600000;
unsigned long lastOHUpdate = 0;

unsigned long wifiConnectStart = 0;
bool wifiConnected = false;

// -------- BLYNK --------
BLYNK_WRITE(V12) {
    currentOHLevel = param.asFloat();
    lastOHUpdate = millis();
    Blynk.virtualWrite(V1, currentOHLevel);
}

BLYNK_WRITE(V11) {
    autoModeActive = param.asInt();
    runSystemLogic();
}

BLYNK_WRITE(V2) {
    int btn = param.asInt();
    if (btn == 1 && (currentOHLevel >= 75.0 || ugLevel < 15.0)) {
        Blynk.virtualWrite(V2, 0);
        pump1Active = false;
    } else {
        pump1Active = btn;
    }
    digitalWrite(PUMP_1_PIN, pump1Active ? LOW : HIGH);
    runSystemLogic();
}

BLYNK_WRITE(V3) {
    int btn = param.asInt();
    if (btn == 1 && ugLevel < 15.0) {
        Blynk.virtualWrite(V3, 0);
        pump2Active = false;
    } else {
        pump2Active = btn;
    }
    digitalWrite(PUMP_2_PIN, pump2Active ? LOW : HIGH);
    runSystemLogic();
}

// -------- CORE LOGIC --------
void runSystemLogic() {
    std::vector<float> readings;

    for (int i = 0; i < 10; i++) {
        digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
        digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
        digitalWrite(TRIG_PIN, LOW);

        long duration = pulseIn(ECHO_PIN, HIGH, 25000);
        float d = (duration * 0.034 / 2);

        if (duration > 0 && d < 400) readings.push_back(d);
        delay(15);
    }

    if (!readings.empty()) {
        std::sort(readings.begin(), readings.end());
        float medianDist = readings[readings.size() / 2];

        ugLevel = constrain(((ugHeight - medianDist) / ugHeight) * 100.0, 0, 100);
        Blynk.virtualWrite(V0, (int)ugLevel);
    }

    bool connectionLost = (millis() - lastOHUpdate > 180000);

    if (pump1Active && (ugLevel < 15.0 || currentOHLevel >= 75.0 || connectionLost)) {
        pump1Active = false;
        Blynk.virtualWrite(V2, 0);
    }

    if (pump2Active && ugLevel < 15.0) {
        pump2Active = false;
        Blynk.virtualWrite(V3, 0);
    }

    if (autoModeActive && !pump1Active && !connectionLost) {
        if (currentOHLevel <= 30.0 && ugLevel > 20.0) {
            pump1Active = true;
            Blynk.virtualWrite(V2, 1);
        }
    }

    digitalWrite(PUMP_1_PIN, pump1Active ? LOW : HIGH);
    digitalWrite(PUMP_2_PIN, pump2Active ? LOW : HIGH);
    digitalWrite(STATUS_LED, (pump1Active || pump2Active));

    scanInterval = (pump1Active || pump2Active) ? 60000 : 600000;
    lastScanTime = millis();
}

// -------- SETUP --------
void setup() {
    Serial.begin(115200);

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(PUMP_1_PIN, OUTPUT);
    pinMode(PUMP_2_PIN, OUTPUT);
    pinMode(STATUS_LED, OUTPUT);
    pinMode(RESET_WIFI_PIN, INPUT_PULLUP);

    digitalWrite(PUMP_1_PIN, HIGH);
    digitalWrite(PUMP_2_PIN, HIGH);

    pref.begin("wifi_cfg", false);
    String s = pref.getString("ssid", "");
    String p = pref.getString("pass", "");
    pref.end();

    // -------- BUTTON FORCE AP --------
    if (digitalRead(RESET_WIFI_PIN) == LOW) {
        Serial.println("Button pressed → AP Mode");

        pref.begin("wifi_cfg", false);
        pref.clear();
        pref.end();

        WiFi.softAP("Gajiwala-Setup");
    }
    else if (s != "") {
        WiFi.begin(s.c_str(), p.c_str());
        wifiConnectStart = millis();
    }
    else {
        WiFi.softAP("Gajiwala-Setup");
    }

    // -------- WEB SERVER --------
    server.on("/", []() {
        server.send(200, "text/html", INDEX_HTML);
    });

    server.on("/config", []() {
        String page = CONFIG_PAGE_HTML;
        page.replace("%SSID%", WiFi.SSID());
        server.send(200, "text/html", page);
    });

    server.on("/save", HTTP_POST, []() {
        String ssid = server.arg("ssid");
        String pass = server.arg("pass");

        pref.begin("wifi_cfg", false);
        pref.putString("ssid", ssid);
        pref.putString("pass", pass);
        pref.end();

        server.send(200, "text/html", "Saved! Restarting...");
        delay(1000);
        ESP.restart();
    });

    server.on("/reset", []() {
        pref.begin("wifi_cfg", false);
        pref.clear();
        pref.end();

        server.send(200, "text/html", "Reset Done! Restarting...");
        delay(1000);
        ESP.restart();
    });

    server.begin();

    runSystemLogic();
}

// -------- LOOP --------
void loop() {
    server.handleClient();

    // -------- WIFI FALLBACK --------
    if (!wifiConnected && WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Blynk.config(BLYNK_AUTH_TOKEN);
    }

    if (!wifiConnected && WiFi.status() != WL_CONNECTED) {
        if (millis() - wifiConnectStart > 15000) {
            WiFi.disconnect(true);
            WiFi.softAP("Gajiwala-Setup");
            wifiConnected = true;
        }
    }

    if (WiFi.status() == WL_CONNECTED) Blynk.run();

    unsigned long now = millis();

    if (now - lastScanTime >= scanInterval) {
        runSystemLogic();
    }

    if (!pump1Active && autoModeActive && currentOHLevel <= 30.0) {
        if (now - lastScanTime > 30000) {
            runSystemLogic();
        }
    }
}