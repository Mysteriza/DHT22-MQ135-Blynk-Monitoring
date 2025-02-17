// ===============================
// DEFINITIONS AND INITIAL CONFIGURATION
// ===============================
#define BLYNK_TEMPLATE_ID "TMPLxyk260wi"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <TimeLib.h>
#include <WidgetRTC.h>

// Sensor pin definitions
#define DHTPIN D7          // GPIO pin for DHT22 sensor
#define DHTTYPE DHT22      // Type of DHT sensor (DHT22)
#define MQ135_PIN A0       // Analog pin for MQ-135 sensor

// WiFi and Blynk credentials
char auth[] = "mnBmj5aj0LEKUdghiHnG81HKlt1aDFMR";  // Blynk authentication token
char ssid[] = "Kosan bu nata";                     // WiFi SSID
char pass[] = "immodium";                          // WiFi password

// Initialize objects
DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;
WidgetRTC rtc;

// Constants for gas compensation
const float GAS_TEMP_COMPENSATION_FACTOR = 0.02;   // Temperature compensation factor
const float GAS_HUMIDITY_COMPENSATION_FACTOR = 0.01; // Humidity compensation factor

// Function to reconnect WiFi
void reconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {  // Check if WiFi is disconnected
    Serial.println("WiFi disconnected. Attempting to reconnect...");
    WiFi.disconnect();                  // Disconnect from WiFi
    WiFi.begin(ssid, pass);             // Reconnect to WiFi
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {  // Retry up to 10 times
      delay(1000);
      Serial.print(".");
      attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi reconnected successfully!");
    } else {
      Serial.println("\nFailed to reconnect to WiFi. Restarting...");
      ESP.restart();  // Restart ESP8266 if reconnect fails
    }
  }
}

// ===============================
// SETUP FUNCTION
// ===============================
void setup() {
  Serial.begin(115200);  // Initialize Serial Monitor for debugging
  Blynk.begin(auth, ssid, pass);  // Connect to Blynk server
  dht.begin();                    // Initialize DHT22 sensor
  rtc.begin();                    // Initialize real-time clock

  // Schedule the `sendSensorData` function to run every 60 seconds
  timer.setInterval(60000L, sendSensorData);

  // Schedule a task to check WiFi connection every 30 seconds
  timer.setInterval(30000L, reconnectWiFi);

  // Synchronize time every 10 minutes
  setSyncInterval(10 * 60);
}

// ===============================
// LOOP FUNCTION
// ===============================
void loop() {
  Blynk.run();  // Process Blynk communication
  timer.run();  // Run scheduled tasks
}

// ===============================
// FUNCTION: GET AIR QUALITY STATUS
// ===============================
String getAirQualityStatus(float compensated_gas) {
  if (compensated_gas <= 200) return "Very Good";
  else if (compensated_gas <= 400) return "Good";
  else if (compensated_gas <= 600) return "Fair";
  else if (compensated_gas <= 800) return "Poor";
  else return "Very Poor";
}

// ===============================
// FUNCTION: SEND SENSOR DATA TO BLYNK
// ===============================
void sendSensorData() {
  // Read temperature and humidity from DHT22
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Check if DHT22 readings are valid
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT22 sensor!");
    Blynk.virtualWrite(V2, "Failed to read from DHT22 sensor!");
    return;
  }

  // Read raw gas value from MQ-135
  int raw_gas = analogRead(MQ135_PIN);

  // Validate MQ-135 reading
  if (raw_gas < 0 || raw_gas > 1023) {
    Serial.println("Failed to read from MQ-135 sensor!");
    Blynk.virtualWrite(V2, "Invalid MQ-135 sensor reading!");
    return;
  }

  // Compensate gas value based on temperature and humidity
  float compensated_gas = raw_gas * 
                          (1.0 + (temperature - 20.0) * GAS_TEMP_COMPENSATION_FACTOR) * 
                          (1.0 + (humidity - 50.0) * GAS_HUMIDITY_COMPENSATION_FACTOR);

  // Determine air quality status
  String airQualityStatus = getAirQualityStatus(compensated_gas);

  // Send data to Blynk virtual pins
  Blynk.virtualWrite(V0, temperature);           // Send temperature data
  Blynk.virtualWrite(V1, humidity);             // Send humidity data
  Blynk.virtualWrite(V5, raw_gas);              // Send raw gas data
  Blynk.virtualWrite(V6, compensated_gas);      // Send compensated gas data
  Blynk.virtualWrite(V7, airQualityStatus);     // Send air quality status

  // Send "Data Updated!" to Blynk
  Blynk.virtualWrite(V2, "Data Updated!");

  // Print confirmation message to Serial Monitor
  Serial.println("Data Updated!");

  // Schedule a task to change the text back to "Refresh to Update" after 1 second
  timer.setTimeout(2500L, []() {
    Blynk.virtualWrite(V2, "Refresh to Update!");
  });

  // Get current time and send to Blynk
  int hours = hour();
  int minutes = minute();
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", hours, minutes);
  Blynk.virtualWrite(V4, timeStr);  // Send formatted time to virtual pin V4
}

// ===============================
// CALLBACK: HANDLE BUTTON PRESS ON BLYNK APP
// ===============================
BLYNK_WRITE(V3) {
  int buttonState = param.asInt();  // Read button state (1 = pressed, 0 = released)
  if (buttonState == 1) {
    sendSensorData();  // Immediately send sensor data when the button is pressed
  }
}

// ===============================
// CALLBACK: SYNCHRONIZE TIME WHEN CONNECTED TO BLYNK
// ===============================
BLYNK_CONNECTED() {
  rtc.begin();  // Synchronize real-time clock
}
