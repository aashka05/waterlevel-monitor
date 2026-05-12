#ifndef CONFIG_PAGE_H
#define CONFIG_PAGE_H

const char CONFIG_PAGE_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>WiFi Setup</title>
  <style>
    body { font-family: sans-serif; text-align: center; background: #f0f2f5; padding: 20px; }
    .container { background: white; padding: 25px; border-radius: 15px; display: inline-block; width: 100%; max-width: 350px; box-shadow: 0 4px 10px rgba(0,0,0,0.1); }
    h2 { color: #333; margin-bottom: 20px; }
    select, input { width: 100%; padding: 12px; margin: 8px 0; border: 1px solid #ddd; border-radius: 8px; box-sizing: border-box; font-size: 16px; }
    button { width: 100%; padding: 14px; background: #28a745; color: white; border: none; border-radius: 8px; cursor: pointer; font-weight: bold; margin-top: 15px; font-size: 16px; }
    button:hover { background: #218838; }
    label { display: block; text-align: left; font-size: 14px; color: #666; margin-top: 10px; }
  </style>
</head>
<body>
  <div class="container">
    <h2>WiFi Setup</h2>
    <form action="/save" method="POST">
      <label>Select available network:</label>
      <select onchange="document.getElementById('ssid_input').value = this.value">
        <option value="">-- Scan Results --</option>
        </select>
      
      <label>WiFi Name (SSID):</label>
      <input type="text" name="ssid" id="ssid_input" placeholder="Enter SSID" required>
      
      <label>WiFi Password:</label>
      <input type="password" name="pass" placeholder="Enter Password">
      
      <button type="submit">SAVE & RESTART</button>
    </form>
  </div>
</body>
</html>
)=====";

#endif