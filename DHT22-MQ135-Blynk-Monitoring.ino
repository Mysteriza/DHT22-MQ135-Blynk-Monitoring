#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""

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
char auth[] = "";
char ssid[] = "";
char pass[] = "";

// Initialize objects
DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;
WidgetRTC rtc;

// Constants for gas compensation
const float GAS_TEMP_COMPENSATION_FACTOR = 0.015;
const float GAS_HUMIDITY_COMPENSATION_FACTOR = 0.008;

// Reconnection variables
unsigned long lastWiFiReconnectAttempt = 0;
unsigned long lastBlynkReconnectAttempt = 0;
const unsigned long RECONNECT_INTERVAL = 5000;  // 5 seconds for WiFi
const unsigned long BLYNK_RECONNECT_INTERVAL = 10000;  // 10 seconds for Blynk

// Function: Check and Reconnect WiFi
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

// Function: Check and Reconnect Blynk
void checkBlynkConnection() {
  if (!Blynk.connected() && millis() - lastBlynkReconnectAttempt > BLYNK_RECONNECT_INTERVAL) {
    lastBlynkReconnectAttempt = millis();
    Serial.println("Blynk disconnected. Attempting to reconnect...");
    Blynk.disconnect();
    Blynk.begin(auth, ssid, pass);
    Blynk.virtualWrite(V2, "Blynk Disconnected - Reconnecting...");
  }
}

// Function: Read MQ-135 with Averaging
float readMQ135() {
  const int SAMPLES = 5;
  int total = 0;
  for (int i = 0; i < SAMPLES; i++) {
    total += analogRead(MQ135_PIN);
    delay(10);
  }
  return total / (float)SAMPLES;
}

// Function: Get Air Quality Status
String getAirQualityStatus(float compensated_gas) {
  if (compensated_gas <= 200) return "Very Good";
  else if (compensated_gas <= 400) return "Good";
  else if (compensated_gas <= 600) return "Fair";
  else if (compensated_gas <= 800) return "Poor";
  else return "Very Poor";
}

// Function: Blink LED 3 times for sensor failure
void blinkLEDForFailure() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, LOW);  // LED on
    delay(500);                  // Wait 500ms
    digitalWrite(LED_PIN, HIGH); // LED off
    delay(500);                  // Wait 500ms
  }
}

// Function: Turn LED on for 3 seconds for poor air quality
void ledOnForPoorAirQuality() {
  digitalWrite(LED_PIN, LOW);  // LED on
  delay(3000);                 // Stay on for 3 seconds
  digitalWrite(LED_PIN, HIGH); // LED off
}

// Function: Check if it's night time (21:00 to 06:00)
bool isNightTime() {
  int currentHour = hour();
  return (currentHour >= 21 || currentHour < 6);
}

// Function: Send Sensor Data to Blynk
void sendSensorData() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT22 sensor!");
    Blynk.virtualWrite(V2, "Failed to read from DHT22 sensor!");
    if (!isNightTime()) blinkLEDForFailure(); // Blink LED only outside night hours
    return;
  }

  float raw_gas = readMQ135();
  if (raw_gas < 0 || raw_gas > 1023) {
    Serial.println("Failed to read from MQ-135 sensor!");
    Blynk.virtualWrite(V2, "Invalid MQ-135 sensor reading!");
    if (!isNightTime()) blinkLEDForFailure(); // Blink LED only outside night hours
    return;
  }

  float compensated_gas = raw_gas * 
                          (1.0 + ((temperature - 25.0) * GAS_TEMP_COMPENSATION_FACTOR)) * 
                          (1.0 + ((humidity - 40.0) * GAS_HUMIDITY_COMPENSATION_FACTOR));

  String airQualityStatus = getAirQualityStatus(compensated_gas);

  // LED control only outside night hours
  if (!isNightTime() && compensated_gas > 600) {
    ledOnForPoorAirQuality(); // LED on for 3 seconds if air quality is poor
  }

  Blynk.virtualWrite(V0, temperature);
  Blynk.virtualWrite(V1, humidity);
  Blynk.virtualWrite(V5, raw_gas);
  Blynk.virtualWrite(V6, compensated_gas);
  Blynk.virtualWrite(V7, airQualityStatus);

  String timeStr = String(hour()) + ":" + (minute() < 10 ? "0" + String(minute()) : String(minute()));
  Blynk.virtualWrite(V4, timeStr);

  Blynk.virtualWrite(V2, "Latest Update!");
}

// Setup Function
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // LED off (active low)
  Blynk.begin(auth, ssid, pass);
  dht.begin();
  rtc.begin();

  timer.setInterval(20000L, sendSensorData);      // Update every 20 seconds
  timer.setInterval(5000L, checkWiFi);           // Check WiFi every 5 seconds
  timer.setInterval(10000L, checkBlynkConnection); // Check Blynk every 10 seconds
}

void loop() {
  Blynk.run();
  timer.run();
}

// Callback: Handle Button Press on Blynk App
BLYNK_WRITE(V3) {
  if (param.asInt() == 1) {
    sendSensorData();
  }
}

// Callback: Synchronize Time When Connected to Blynk
BLYNK_CONNECTED() {
  rtc.begin();
  Serial.println("Connected to Blynk!");
  Blynk.virtualWrite(V2, "Connected to Blynk!");
}
