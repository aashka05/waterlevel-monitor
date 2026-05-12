#ifndef INDEX_H
#define INDEX_H

const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Gajiwala Home</title>
  <style>
    body { font-family: sans-serif; text-align: center; padding: 50px; background-color: #f4f4f9; }
    h1 { color: #333; }
    .btn { display: inline-block; padding: 20px 40px; background: #007bff; color: white; text-decoration: none; border-radius: 10px; font-size: 20px; font-weight: bold; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }
    .btn:hover { background: #0056b3; }
  </style>
</head>
<body>
  <h1>Gajiwala Home Water Level System</h1>
  <p>OH Tank Controller</p>
  <br><br>
  <a href="/config" class="btn">Configure WiFi Settings</a>
</body>
</html>
)=====";

#endif