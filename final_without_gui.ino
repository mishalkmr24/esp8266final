#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "zzz";
const char* password = "12345678";

ESP8266WebServer server(80);

int irPin = D2; // IR sensor pin
int irValue;

unsigned long timerDuration = 10000; // 10 s in ms
unsigned long startTime = 0;
bool timerRunning = false;
bool machineBusy = false; // tracks if machine is in use

String status = "Available";

// JSON API for AJAX
void handleStatus() {
  String json = "{";
  json += "\"status\":\"" + status + "\"";
  if (timerRunning) {
    unsigned long elapsed = millis() - startTime;
    unsigned long remaining = (elapsed < timerDuration) ? (timerDuration - elapsed) / 1000 : 0;
    json += ",\"time\":" + String(remaining);
  } else {
    json += ",\"time\":0";
  }
  json += "}";
  server.send(200, "application/json", json);
}

// Webpage with auto-refresh
void handleRoot() {
  String page = R"rawliteral(
  <html>
  <head>
    <title>Washing Machine</title>
    <style>
      body { font-family: Arial; text-align: center; margin-top: 50px; }
      .status { font-size: 24px; font-weight: bold; }
      .available { color: green; }
      .inuse { color: red; }
      .ended { color: orange; }
    </style>
    <script>
      function updateStatus(){
        fetch('/status')
        .then(response => response.json())
        .then(data => {
          let statusText = data.status;
          let statusClass = "";
          if(statusText.includes("Available")) statusClass = "available";
          else if(statusText.includes("Done Washing")) statusClass = "ended";
          else statusClass = "inuse";
          document.getElementById('status').innerHTML = statusText;
          document.getElementById('status').className = "status " + statusClass;
          document.getElementById('timer').innerHTML = data.time + "s";
        });
      }
      setInterval(updateStatus, 500); // update every 0.5s
      window.onload = updateStatus;
    </script>
  </head>
  <body>
    <h1>Washing Machine Status</h1>
    <p>Status: <span id="status" class="status">Loading...</span></p>
    <p>Time Left: <span id="timer">0s</span></p>
  </body>
  </html>
  )rawliteral";
  server.send(200, "text/html", page);
}

void setup() {
  pinMode(irPin, INPUT);
  WiFi.softAP(ssid, password);
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.begin();
}

void loop() {
  irValue = digitalRead(irPin);

  // IR HIGH -> machine is Available, reset everything
  if (irValue == HIGH) {
    status = "Available";
    machineBusy = false;
    timerRunning = false;
  }

  // IR LOW -> someone started using machine
  if (irValue == LOW && !machineBusy) {
    startTime = millis();
    timerRunning = true;
    machineBusy = true; // lock until cycle done
    status = "In Use";
  }

  // While timer running
  if (timerRunning) {
    unsigned long elapsed = millis() - startTime;

    if (elapsed < timerDuration) {
      status = "In Use";
    } else {
      // Timer finished
      timerRunning = false;
      if (irValue == LOW) {
        status = "In Use (Done Washing)";
      }
    }
  }

  server.handleClient();
}