
#include <DHT.h>

// Mirror sensor output to USB Serial Monitor and to the ESP32 over Serial1.
#define BOTH_PRINT(...) do { Serial.print(__VA_ARGS__); Serial1.print(__VA_ARGS__); } while (0)
#define BOTH_PRINTLN(...) do { Serial.println(__VA_ARGS__); Serial1.println(__VA_ARGS__); } while (0)

// EchoPaw - Dashboard Sensor Data

const int DHT_PIN = A0;
const int TRIG_PIN = 7;
const int ECHO_PIN = 8;
const int EMERGENCY_BUTTON = 2;

const float TEMP_WARNING_C = 30.0;
const float HUMIDITY_WARNING = 75.0;
const float OBSTACLE_DISTANCE_CM = 20.0;

DHT dht(DHT_PIN, DHT11);

unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL_MS = 2000;

bool lastButtonPressed = false;

void setup() {
  Serial.begin(115200);   // USB debugging
  Serial1.begin(115200);  // Sends sensor data to ESP32

  dht.begin();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  pinMode(EMERGENCY_BUTTON, INPUT_PULLUP);

  lastSensorRead = millis() - SENSOR_INTERVAL_MS;
}

void loop() {
  // Check emergency button continuously
  bool buttonPressed =
    digitalRead(EMERGENCY_BUTTON) == LOW;

  if (buttonPressed != lastButtonPressed) {
    delay(15);
    buttonPressed =
      digitalRead(EMERGENCY_BUTTON) == LOW;

    lastButtonPressed = buttonPressed;
  }

  // Send a sensor record every two seconds
  if (millis() - lastSensorRead >= SENSOR_INTERVAL_MS) {
    lastSensorRead = millis();

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    bool dhtValid =
      !isnan(humidity) && !isnan(temperature);

    // Read ultrasonic sensor
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    unsigned long duration =
      pulseIn(ECHO_PIN, HIGH, 25000);

    float distanceCm = duration * 0.0343 / 2.0;

    bool distanceValid =
      duration != 0 &&
      distanceCm >= 2.0 &&
      distanceCm <= 400.0;

    bool tempWarning =
      dhtValid && temperature >= TEMP_WARNING_C;

    bool humidityWarning =
      dhtValid && humidity >= HUMIDITY_WARNING;

    bool obstacleWarning =
      !distanceValid ||
      distanceCm <= OBSTACLE_DISTANCE_CM;

    bool alert =
      !dhtValid ||
      tempWarning ||
      humidityWarning ||
      obstacleWarning ||
      buttonPressed;

    // Output one JSON object per line
    BOTH_PRINT("{\"temperature\":");
    if (dhtValid) BOTH_PRINT(temperature, 1);
    else BOTH_PRINT("null");

    BOTH_PRINT(",\"humidity\":");
    if (dhtValid) BOTH_PRINT(humidity, 1);
    else BOTH_PRINT("null");

    BOTH_PRINT(",\"distance\":");
    if (distanceValid) BOTH_PRINT(distanceCm, 1);
    else BOTH_PRINT("null");

    BOTH_PRINT(",\"emergencyPressed\":");
    BOTH_PRINT(buttonPressed ? "true" : "false");

    BOTH_PRINT(",\"alert\":");
    BOTH_PRINT(alert ? "true" : "false");

    BOTH_PRINTLN("}");
  }
}
