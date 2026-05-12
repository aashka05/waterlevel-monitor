#define BLYNK_TEMPLATE_ID "TMPL3HrH4KUPw"
#define BLYNK_TEMPLATE_NAME "Water Level System"
#define BLYNK_AUTH_TOKEN "NzRQJ5sRmorb14V9Tc_PP7LNl_UG2H8o" 

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <HTTPClient.h> 
#include <Preferences.h>
#include <WebServer.h>
#include <vector>
#include <algorithm>

const char* ugTankToken = "Ee5OjUZT-0K33JBvV7SrMbyBCQxe0cqE"; 

#define TRIGGER_PIN_OH 7 
#define ECHO_PIN_OH 6   

#include "index.h"
#include "config_page.h"

WebServer serverOH(80);
Preferences pref;
BlynkTimer timer;

float overheadTankHeightCm = 150.0;
bool apActive = false;

float getFilteredLevel() {
    std::vector<float> readings;
    for (int i = 0; i < 15; i++) {
        digitalWrite(TRIGGER_PIN_OH, LOW); delayMicroseconds(2);
        digitalWrite(TRIGGER_PIN_OH, HIGH); delayMicroseconds(10);
        digitalWrite(TRIGGER_PIN_OH, LOW);
        long duration = pulseIn(ECHO_PIN_OH, HIGH, 30000);
        float distance = (duration * 0.034) / 2;
        if (distance > 2.0 && distance < 400.0) readings.push_back(distance);
        delay(20); 
    }
    if (readings.empty()) return -1.0;
    std::sort(readings.begin(), readings.end());
    float medianDist = readings[readings.size() / 2];
    float waterDepth = overheadTankHeightCm - medianDist;
    float percentage = (waterDepth / overheadTankHeightCm) * 100.0;
    return std::max(0.0f, std::min(100.0f, percentage));
}

void sendData() {
    float level = getFilteredLevel();
    if (level >= 0) {
        if (Blynk.connected()) Blynk.virtualWrite(V1, level);
        
        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            String url = "https://blr1.blynk.cloud/external/api/update?token=" + String(ugTankToken) + "&v12=" + String(level, 1);
            http.begin(url);
            int httpCode = http.GET();
            http.end();
        }
    }
}

// Function to kill the AP once we are safely online
void checkConnectionStability() {
    if (WiFi.status() == WL_CONNECTED && apActive) {
        Serial.println("WiFi Stable. Shutting down Access Point for security.");
        WiFi.softAPdisconnect(true); // Shut down AP
        WiFi.mode(WIFI_STA);         // Switch to Station mode only
        apActive = false;
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(TRIGGER_PIN_OH, OUTPUT);
    pinMode(ECHO_PIN_OH, INPUT);

    pref.begin("wifi_cfg", false);
    String s = pref.getString("ssid", "");
    String p = pref.getString("pass", "");
    pref.end();

    if (s != "" && s != "NULL") {
        WiFi.mode(WIFI_AP_STA); // Start in Dual Mode
        WiFi.softAP("Gajiwala-OH-Setup", "12345678");
        apActive = true;
        
        WiFi.begin(s.c_str(), p.c_str());
        Blynk.config(BLYNK_AUTH_TOKEN, "blr1.blynk.cloud", 80);
        
        // Wait briefly to see if it connects immediately
        int timeout = 0;
        while (WiFi.status() != WL_CONNECTED && timeout < 20) {
            delay(500);
            Serial.print(".");
            timeout++;
        }
    } else {
        WiFi.mode(WIFI_AP);
        WiFi.softAP("Gajiwala-OH-Setup", "12345678");
        apActive = true;
    }

    serverOH.on("/", []() { serverOH.send(200, "text/html", INDEX_HTML); });
    serverOH.on("/config", []() {
        int n = WiFi.scanNetworks();
        String list = "";
        for (int i = 0; i < n; i++) list += "<option value='"+WiFi.SSID(i)+"'>"+WiFi.SSID(i)+"</option>";
        String html = CONFIG_PAGE_HTML;
        html.replace("", list);
        serverOH.send(200, "text/html", html);
    });
    serverOH.on("/save", HTTP_POST, []() {
        pref.begin("wifi_cfg", false);
        pref.putString("ssid", serverOH.arg("ssid"));
        pref.putString("pass", serverOH.arg("pass"));
        pref.end();
        serverOH.send(200, "text/html", "Saved. Restarting...");
        delay(2000); ESP.restart();
    });
    
    serverOH.begin();
    timer.setInterval(120000L, sendData); 
    
    // Check every 30 seconds if we can turn off the AP
    timer.setInterval(30000L, checkConnectionStability); 
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) Blynk.run();
    timer.run();
    serverOH.handleClient();
}