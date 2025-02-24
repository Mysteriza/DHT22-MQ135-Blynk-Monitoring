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
char pass[] = "";                             // WiFi password

// Initialize objects
DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;
WidgetRTC rtc;

// Constants for gas compensation (adjusted for general accuracy)
const float GAS_TEMP_COMPENSATION_FACTOR = 0.015;   // Temperature compensation factor
const float GAS_HUMIDITY_COMPENSATION_FACTOR = 0.008; // Humidity compensation factor
const float TEMP_OFFSET = -1.0;                     
// Temperature offset to compensate MQ-135 heat (Since the two sensors are placed close together, the heat from the MQ-135 can make the DHT22 reading 1 degree higher. So it must be compensated -1 degree.)

// WiFi reconnection variables
unsigned long lastReconnectAttempt = 0;
const unsigned long RECONNECT_INTERVAL = 5000; // 5 seconds between reconnect attempts

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

// Function: Send Sensor Data to Blynk
void sendSensorData() {
  // Read temperature and humidity from DHT22
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Validate DHT22 readings
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT22 sensor!");
    Blynk.virtualWrite(V2, "Failed to read from DHT22 sensor!");
    digitalWrite(LED_PIN, LOW); // LED on for error
    return;
  }

  // Apply temperature offset to compensate for MQ-135 heat
  temperature += TEMP_OFFSET;

  // Read raw gas value from MQ-135 with averaging
  float raw_gas = readMQ135();

  // Validate MQ-135 reading
  if (raw_gas < 0 || raw_gas > 1023) {
    Serial.println("Failed to read from MQ-135 sensor!");
    Blynk.virtualWrite(V2, "Invalid MQ-135 sensor reading!");
    digitalWrite(LED_PIN, LOW); // LED on for error
    return;
  }

  // Compensate gas value based on temperature and humidity
  float compensated_gas = raw_gas * 
                          (1.0 + ((temperature - 25.0) * GAS_TEMP_COMPENSATION_FACTOR)) * 
                          (1.0 + ((humidity - 40.0) * GAS_HUMIDITY_COMPENSATION_FACTOR));

  // Determine air quality status
  String airQualityStatus = getAirQualityStatus(compensated_gas);

  // Control LED based on air quality
  digitalWrite(LED_PIN, compensated_gas > 600 ? LOW : HIGH); // LED on if air quality is poor

  // Send data to Blynk virtual pins
  Blynk.virtualWrite(V0, temperature);           // Temperature with offset
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
