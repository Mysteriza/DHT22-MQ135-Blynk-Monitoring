#define BLYNK_TEMPLATE_ID "TMPLxyk260wi"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <TimeLib.h>
#include <WidgetRTC.h>

// Pin definitions
#define DHTPIN D7          // GPIO pin for DHT22 sensor
#define DHTTYPE DHT22      // DHT22 sensor type
#define MQ135_PIN A0       // Analog pin for MQ-135 sensor
#define LED_PIN D0         // Onboard LED pin (active low)

// WiFi and Blynk credentials
char auth[] = "mnBmj5aj0LEKUdghiHnG81HKlt1aDFMR";
char ssid[] = "Kosan bu nata";
char pass[] = "immodium";

// Initialize objects
DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;
WidgetRTC rtc;

// Constants
const float GAS_TEMP_COMPENSATION_FACTOR = 0.015;
const float GAS_HUMIDITY_COMPENSATION_FACTOR = 0.008;
const unsigned long RECONNECT_INTERVAL = 5000;      // 5 seconds for WiFi
const unsigned long BLYNK_RECONNECT_INTERVAL = 10000; // 10 seconds for Blynk

// Reconnection variables
unsigned long lastWiFiReconnectAttempt = 0;
unsigned long lastBlynkReconnectAttempt = 0;

// LED control variables
unsigned long ledStartTime = 0;
bool ledActive = false;

// Check and reconnect WiFi
void checkWiFi() {
  if (WiFi.status() != WL_CONNECTED && millis() - lastWiFiReconnectAttempt > RECONNECT_INTERVAL) {
    lastWiFiReconnectAttempt = millis();
    Serial.println("WiFi disconnected. Attempting to reconnect...");
    WiFi.disconnect();
    WiFi.begin(ssid, pass);
    Blynk.virtualWrite(V2, "WiFi Disconnected - Reconnecting...");
    timer.setTimeout(5000L, []() {
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi reconnected successfully!");
        Blynk.virtualWrite(V2, "WiFi Reconnected!");
      }
    });
  }
}

// Check and reconnect Blynk
void checkBlynkConnection() {
  if (!Blynk.connected() && millis() - lastBlynkReconnectAttempt > BLYNK_RECONNECT_INTERVAL) {
    lastBlynkReconnectAttempt = millis();
    Serial.println("Blynk disconnected. Attempting to reconnect...");
    Blynk.disconnect();
    Blynk.begin(auth, ssid, pass);
    Blynk.virtualWrite(V2, "Blynk Disconnected - Reconnecting...");
  }
}

// Read MQ-135 with averaging
float readMQ135() {
  const int SAMPLES = 5;
  int total = 0;
  for (int i = 0; i < SAMPLES; i++) {
    total += analogRead(MQ135_PIN);
    delay(10);
  }
  return total / (float)SAMPLES;
}

// Get air quality status
String getAirQualityStatus(float compensated_gas) {
  if (compensated_gas <= 200) return "Very Good";
  if (compensated_gas <= 400) return "Good";
  if (compensated_gas <= 600) return "Fair";
  if (compensated_gas <= 800) return "Poor";
  return "Very Poor";
}

// Check if it's night time (19:00 to 07:00)
bool isNightTime() {
  if (!timeStatus() == timeSet) return false; // Default to daytime if time not synced
  int currentHour = hour();
  return (currentHour >= 19 || currentHour < 7);
}

// Handle LED blinking or solid on (non-blocking)
void updateLED(bool sensorFailure, bool poorAirQuality) {
  if (isNightTime() || ledActive) return; // Skip if night mode or LED already active

  if (sensorFailure) {
    ledActive = true;
    ledStartTime = millis();
    timer.setTimer(500L, []() {
      static int blinkCount = 0;
      digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Toggle LED
      blinkCount++;
      if (blinkCount >= 6) { // 3 full blinks (6 toggles)
        digitalWrite(LED_PIN, HIGH);
        ledActive = false;
        blinkCount = 0;
        return false; // Stop timer
      }
      return true; // Continue timer
    }, 6);
  } else if (poorAirQuality) {
    ledActive = true;
    digitalWrite(LED_PIN, LOW);
    timer.setTimeout(3000L, []() {
      digitalWrite(LED_PIN, HIGH);
      ledActive = false;
    });
  }
}

// Send sensor data to Blynk
void sendSensorData() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT22 sensor!");
    Blynk.virtualWrite(V2, "Failed to read from DHT22 sensor!");
    updateLED(true, false);
    return;
  }

  float raw_gas = readMQ135();
  if (raw_gas < 0 || raw_gas > 1023) {
    Serial.println("Failed to read from MQ-135 sensor!");
    Blynk.virtualWrite(V2, "Invalid MQ-135 sensor reading!");
    updateLED(true, false);
    return;
  }

  float compensated_gas = raw_gas * 
                          (1.0 + ((temperature - 25.0) * GAS_TEMP_COMPENSATION_FACTOR)) * 
                          (1.0 + ((humidity - 40.0) * GAS_HUMIDITY_COMPENSATION_FACTOR));
  String airQualityStatus = getAirQualityStatus(compensated_gas);

  updateLED(false, compensated_gas > 600);

  Blynk.virtualWrite(V0, temperature);
  Blynk.virtualWrite(V1, humidity);
  Blynk.virtualWrite(V5, raw_gas);
  Blynk.virtualWrite(V6, compensated_gas);
  Blynk.virtualWrite(V7, airQualityStatus);

  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", hour(), minute());
  Blynk.virtualWrite(V4, timeStr);

  Blynk.virtualWrite(V2, "Latest Update!");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  Blynk.begin(auth, ssid, pass);
  dht.begin();
  rtc.begin();

  timer.setInterval(20000L, sendSensorData);
  timer.setInterval(5000L, checkWiFi);
  timer.setInterval(10000L, checkBlynkConnection);
}

void loop() {
  Blynk.run();
  timer.run();
}

BLYNK_WRITE(V3) {
  if (param.asInt() == 1) sendSensorData();
}

BLYNK_CONNECTED() {
  rtc.begin();
  Serial.println("Connected to Blynk!");
  Blynk.virtualWrite(V2, "Connected to Blynk!");
}
