#include <WiFi.h>
#include <WebServer.h>

// ESP32 creates its own Wi-Fi network
const char* ssid = "ReveilleToTheRescue";
const char* password = "technothonGOATS";

WebServer server(80);
HardwareSerial SensorSerial(2);  // UART2: ESP32 RX=16, TX=17

// L298N control pins
const int IN1 = 25;
const int IN2 = 26;
const int IN3 = 27;
const int IN4 = 14;

const int SENSOR_RX_PIN = 16; // Connect to Mega TX1 (pin 18) THROUGH a level shifter/divider
const int SENSOR_TX_PIN = 17; // Optional: connect to Mega RX1 (pin 19)
unsigned long lastCommandTime = 0;
const unsigned long COMMAND_TIMEOUT = 1000;

String sensorLine;
String latestSensorJson =
  "{\"temperature\":null,\"humidity\":null,\"distance\":null,"
  "\"emergencyPressed\":false,\"alert\":true,\"status\":\"Waiting for Mega sensor data\"}";

void addCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.sendHeader("Cache-Control", "no-store");
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void forward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void backward() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}

void left() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void right() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}

void handleCommand(const String& command) {
  if (command == "forward") forward();
  else if (command == "backward") backward();
  else if (command == "left") left();
  else if (command == "right") right();
  else stopMotors();

  lastCommandTime = millis();
  addCorsHeaders();
  server.send(200, "text/plain", "Command: " + command);
}

void handleSensorData() {
  addCorsHeaders();
  server.send(200, "application/json", latestSensorJson);
}

const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Reveille To The Rescue</title></head><body>
<h1>Reveille To The Rescue</h1>
<p>ESP32 Wi-Fi controller is online.</p>
<p>For the full sensor dashboard, connect to this Wi-Fi and open the updated
reveilletotherescue.html file on your phone or computer.</p>
<p>ESP32 address: <b>http://192.168.4.1</b></p>
<p><a href="/sensors">View latest sensor JSON</a></p>
</body></html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  SensorSerial.begin(115200, SERIAL_8N1, SENSOR_RX_PIN, SENSOR_TX_PIN);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  stopMotors();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.println();
  Serial.println("ESP32 Wi-Fi started");
  Serial.print("Wi-Fi: "); Serial.println(ssid);
  Serial.print("Open: http://"); Serial.println(WiFi.softAPIP());
  Serial.println("Waiting for sensor JSON from Mega on UART2...");

  server.on("/", HTTP_GET, []() {
    addCorsHeaders();
    server.send_P(200, "text/html", webpage);
  });

  server.on("/sensors", HTTP_GET, handleSensorData);
  server.on("/forward", HTTP_GET, []() { handleCommand("forward"); });
  server.on("/backward", HTTP_GET, []() { handleCommand("backward"); });
  server.on("/left", HTTP_GET, []() { handleCommand("left"); });
  server.on("/right", HTTP_GET, []() { handleCommand("right"); });
  server.on("/stop", HTTP_GET, []() { handleCommand("stop"); });

  server.onNotFound([]() {
    if (server.method() == HTTP_OPTIONS) {
      addCorsHeaders();
      server.send(204);
    } else {
      addCorsHeaders();
      server.send(404, "text/plain", "Not found");
    }
  });

  server.begin();
  lastCommandTime = millis();
}

void loop() {
  server.handleClient();

  // Read complete newline-terminated JSON records sent by the Mega.
  while (SensorSerial.available()) {
    char c = (char)SensorSerial.read();
    if (c == '\n') {
      sensorLine.trim();
      if (sensorLine.length() > 0) {
        Serial.print("UART2 received: ");
        Serial.println(sensorLine);
      }
      if (sensorLine.length() > 0 && sensorLine.startsWith("{") &&
          sensorLine.endsWith("}")) {
        latestSensorJson = sensorLine;
        Serial.println("Accepted sensor JSON; /sensors updated.");
      } else if (sensorLine.length() > 0) {
        Serial.println("Rejected line: expected a complete JSON object.");
      }
      sensorLine = "";
    } else if (c != '\r') {
      if (sensorLine.length() < 300) sensorLine += c;
      else sensorLine = ""; // discard oversized/corrupt line
    }
  }

  // Safety timeout: motors stop if no fresh movement command arrives.
  if (millis() - lastCommandTime > COMMAND_TIMEOUT) stopMotors();
}
