
#include <WiFi.h>
#include <WebServer.h>

// ESP32 creates its own Wi-Fi network
const char* ssid = "ReveilleToTheRescue";
const char* password = "technothonGOATS";

WebServer server(80);

// L298N control pins
const int IN1 = 25;
const int IN2 = 26;
const int IN3 = 27;
const int IN4 = 14;

unsigned long lastCommandTime = 0;
const unsigned long COMMAND_TIMEOUT = 1000;

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void forward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void backward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void left() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void right() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void handleCommand(const String& command) {
  if (command == "forward") {
    forward();
  } else if (command == "backward") {
    backward();
  } else if (command == "left") {
    left();
  } else if (command == "right") {
    right();
  } else {
    stopMotors();
  }

  lastCommandTime = millis();
  server.send(200, "text/plain", "Command: " + command);
}

const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Reveille To The Rescue</title>
<style>
body {
  font-family: Arial, sans-serif;
  text-align: center;
  background: #f0f3f6;
  margin: 20px;
}
h1 { color: #243447; }
.grid {
  display: grid;
  grid-template-columns: repeat(3, 95px);
  gap: 10px;
  justify-content: center;
  margin-top: 25px;
}
button {
  height: 75px;
  font-size: 18px;
  font-weight: bold;
  border: none;
  border-radius: 14px;
  background: #2474d2;
  color: white;
  touch-action: manipulation;
}
.stop { background: #d93636; }
.empty { visibility: hidden; }
</style>
</head>
<body>
<h1>DisasterDog</h1>
<p>Tap a direction to drive</p>
<div class="grid">
  <div class="empty"></div>
  <button onclick="sendCmd('forward')">▲<br>Forward</button>
  <div class="empty"></div>

  <button onclick="sendCmd('left')">◀<br>Left</button>
  <button class="stop" onclick="sendCmd('stop')">STOP</button>
  <button onclick="sendCmd('right')">▶<br>Right</button>

  <div class="empty"></div>
  <button onclick="sendCmd('backward')">▼<br>Backward</button>
  <div class="empty"></div>
</div>
<script>
function sendCmd(cmd) {
  fetch('/' + cmd).catch(() => {});
}
</script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  stopMotors();

  WiFi.softAP(ssid, password);

  Serial.println();
  Serial.println("DisasterDog Wi-Fi started");
  Serial.print("Connect to Wi-Fi: ");
  Serial.println(ssid);
  Serial.print("Open this address: http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", []() {
    server.send_P(200, "text/html", webpage);
  });

  server.on("/forward", []() {
    handleCommand("forward");
  });
  server.on("/backward", []() {
    handleCommand("backward");
  });
  server.on("/left", []() {
    handleCommand("left");
  });
  server.on("/right", []() {
    handleCommand("right");
  });
  server.on("/stop", []() {
    handleCommand("stop");
  });

  server.begin();
  lastCommandTime = millis();
}

void loop() {
  server.handleClient();

  // Stop if the phone stops sending commands
  if (millis() - lastCommandTime > COMMAND_TIMEOUT) {
    stopMotors();
  }
}
