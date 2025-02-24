// Definitions and Initial Configuration
#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <TimeLib.h>
#include <WidgetRTC.h>

// Pin definitions
#define DHTPIN D7          // GPIO pin for DHT22 sensor
#define DHTTYPE DHT22      // Type of DHT sensor (DHT22)
#define MQ135_PIN A0       // Analog pin for MQ-135 sensor
#define LED_PIN D0         // Onboard LED pin (active low)

// WiFi and Blynk credentials
char auth[] = "";  // Blynk authentication token
char ssid[] = "";                        // WiFi SSID
char pass[] = "";                        // WiFi password

// Initialize objects
DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;
WidgetRTC rtc;

// Constants for gas compensation (adjusted for general accuracy)
const float GAS_TEMP_COMPENSATION_FACTOR = 0.015;   // Temperature compensation factor
const float GAS_HUMIDITY_COMPENSATION_FACTOR = 0.008; // Humidity compensation factor

// WiFi reconnection variables
unsigned long lastReconnectAttempt = 0;
const unsigned long RECONNECT_INTERVAL = 5000; // 5 seconds between reconnect attempts

// LED blinking variables
bool sensorFailed = false; // Flag to track sensor failure
bool ledState = HIGH;      // Current state of LED for blinking (HIGH = off, LOW = on)

// Function: Check and Reconnect WiFi (Adaptive and Stable)
void checkWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastReconnectAttempt > RECONNECT_INTERVAL) {
      lastReconnectAttempt = millis();
      Serial.println("WiFi disconnected. Attempting to reconnect...");
      WiFi.disconnect(); // Ensure clean disconnect before reconnect
      WiFi.begin(ssid, pass);

      // Update status on Blynk
      Blynk.virtualWrite(V2, "WiFi Disconnected - Reconnecting...");

      // Check connection status after 5 seconds
      timer.setTimeout(5000L, []() {
        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("WiFi reconnected successfully!");
          Blynk.virtualWrite(V2, "WiFi Reconnected!");
        } else {
          Serial.println("Still trying to reconnect...");
        }
      });
    }
  }
}

// Function: Read MQ-135 with Averaging
float readMQ135() {
  const int SAMPLES = 5;
  int total = 0;
  for (int i = 0; i < SAMPLES; i++) {
    total += analogRead(MQ135_PIN);
    delay(10); // Small delay for ADC stability
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

// Function: Blink LED when sensor fails
void blinkLED() {
  if (sensorFailed) {
    ledState = !ledState; // Toggle LED state
    digitalWrite(LED_PIN, ledState);
  }
}

// Function: Send Sensor Data to Blynk
void sendSensorData() {
  // Read temperature and humidity from DHT22
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Validate DHT22 readings
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT22 sensor!");
    Blynk.virtualWrite(V2, "Failed to read from DHT22 sensor!");
    sensorFailed = true; // Set failure flag
    return;
  }

  // Read raw gas value from MQ-135 with averaging
  float raw_gas = readMQ135();

  // Validate MQ-135 reading
  if (raw_gas < 0 || raw_gas > 1023) {
    Serial.println("Failed to read from MQ-135 sensor!");
    Blynk.virtualWrite(V2, "Invalid MQ-135 sensor reading!");
    sensorFailed = true; // Set failure flag
    return;
  }

  // If we reach here, sensors are working, so reset failure flag
  sensorFailed = false;

  // Compensate gas value based on temperature and humidity
  float compensated_gas = raw_gas * 
                          (1.0 + ((temperature - 25.0) * GAS_TEMP_COMPENSATION_FACTOR)) * 
                          (1.0 + ((humidity - 40.0) * GAS_HUMIDITY_COMPENSATION_FACTOR));

  // Determine air quality status
  String airQualityStatus = getAirQualityStatus(compensated_gas);

  // Control LED based on air quality if no sensor failure
  if (!sensorFailed) {
    digitalWrite(LED_PIN, compensated_gas > 600 ? LOW : HIGH); // LED on if air quality is poor
  }

  // Send data to Blynk virtual pins
  Blynk.virtualWrite(V0, temperature);           // Temperature
  Blynk.virtualWrite(V1, humidity);             // Humidity
  Blynk.virtualWrite(V5, raw_gas);              // Raw gas value
  Blynk.virtualWrite(V6, compensated_gas);      // Compensated gas value
  Blynk.virtualWrite(V7, airQualityStatus);     // Air quality status

  // Send current time to Blynk
  String timeStr = String(hour()) + ":" + (minute() < 10 ? "0" + String(minute()) : String(minute()));
  Blynk.virtualWrite(V4, timeStr);              // Formatted time

  // Update status to "Latest Update!" on successful read
  Blynk.virtualWrite(V2, "Latest Update!");
}

// Setup Function
void setup() {
  Serial.begin(115200);  // Initialize Serial Monitor
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // LED off (active low)
  
  // Connect to Blynk and WiFi
  Blynk.begin(auth, ssid, pass);  
  dht.begin();                    // Initialize DHT22 sensor
  rtc.begin();                    // Initialize real-time clock
  
  // Schedule tasks
  timer.setInterval(10000L, sendSensorData); // Update every 10 seconds
  timer.setInterval(5000L, checkWiFi);       // Check WiFi every 5 seconds
  timer.setInterval(300L, blinkLED);         // Blink LED every 0,3 seconds if sensor fails
}

// Loop Function
void loop() {
  Blynk.run();  // Process Blynk communication
  timer.run();  // Run scheduled tasks
}

// Callback: Handle Button Press on Blynk App
BLYNK_WRITE(V3) {
  int buttonState = param.asInt();  // Read button state
  if (buttonState == 1) {
    sendSensorData();  // Send sensor data immediately
  }
}

// Callback: Synchronize Time When Connected to Blynk
BLYNK_CONNECTED() {
  rtc.begin();  // Synchronize real-time clock
  Serial.println("Connected to Blynk!");
  Blynk.virtualWrite(V2, "Connected to Blynk!");
}
