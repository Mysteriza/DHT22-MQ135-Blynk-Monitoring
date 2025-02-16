#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_TEMPLATE_NAME"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <TimeLib.h>    // Include TimeLib for time synchronization
#include <WidgetRTC.h>  // Include WidgetRTC for Blynk's real-time clock

#define DHTPIN D7      // Pin where the DHT22 sensor is connected
#define DHTTYPE DHT22  // Type of DHT sensor (DHT22)
#define MQ135_PIN A0   // Pin where the MQ-135 sensor is connected

char auth[] = "YOUR_BLYNK_AUTH";  // Blynk authentication token
char ssid[] = "YOUR WIFI SSID";                     // Your WiFi SSID
char pass[] = "YOUR WIFI PASSWORD";                          // Your WiFi password

DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;  // Use BlynkTimer for scheduling tasks
WidgetRTC rtc;     // Object for Blynk's real-time clock

void setup() {
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);  // Initialize Blynk
  dht.begin();                    // Initialize DHT22 sensor
  rtc.begin();                    // Initialize real-time clock

  // Set a timer to call the sendSensorData function every 60 seconds
  timer.setInterval(60000L, sendSensorData);
  setSyncInterval(10 * 60);  // Synchronize time every 10 minutes
}

void loop() {
  Blynk.run();  // Run Blynk
  timer.run();  // Run BlynkTimer
}

// Function to determine air quality status based on compensated gas value
String getAirQualityStatus(float compensated_gas) {
  if (compensated_gas <= 200) {
    return "Very Good";
  } else if (compensated_gas <= 400) {
    return "Good";
  } else if (compensated_gas <= 600) {
    return "Fair";
  } else if (compensated_gas <= 800) {
    return "Poor";
  } else {
    return "Very Poor";
  }
}

void sendSensorData() {
  // Read data from DHT22
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Read data from MQ-135
  int raw_gas = analogRead(MQ135_PIN);
  // Compensate MQ-135 readings based on temperature and humidity
  float compensated_gas = raw_gas * (1.0 + (t - 20.0) * 0.02) * (1.0 + (h - 50.0) * 0.01);

  // Determine air quality status
  String airQualityStatus = getAirQualityStatus(compensated_gas);

  // Check if sensor readings failed
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    Blynk.virtualWrite(V2, "Failed to read from DHT sensor!");
    return;
  }

  // Display data on Serial Monitor
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" *C\t");
  Serial.print("Gas (Raw): ");
  Serial.print(raw_gas);
  Serial.print(", Gas (Compensated): ");
  Serial.print(compensated_gas);
  Serial.print(" ppm, Air Quality: ");
  Serial.println(airQualityStatus);

  // Send data to Blynk
  Blynk.virtualWrite(V0, t);  // Send temperature data to virtual pin V0
  Blynk.virtualWrite(V1, h);  // Send humidity data to virtual pin V1
  Blynk.virtualWrite(V5, raw_gas);  // Send raw gas data to virtual pin V5
  Blynk.virtualWrite(V6, compensated_gas);  // Send compensated gas data to virtual pin V6
  Blynk.virtualWrite(V7, airQualityStatus);  // Send air quality status to virtual pin V7

  Blynk.virtualWrite(V2, "Data Updated!");  // Send a message to virtual pin V2 indicating data was updated
  delay(1000);
  Blynk.virtualWrite(V2, "Refresh to Update!");

  // Get current time
  int hours = hour();
  int minutes = minute();

  // Format time as hh:mm
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", hours, minutes);

  // Send time to virtual pin V4
  Blynk.virtualWrite(V4, timeStr);
}

// Callback function triggered when a button in Blynk is pressed
BLYNK_WRITE(V3) {
  int buttonState = param.asInt();  // Read button state (1 = pressed, 0 = released)
  if (buttonState == 1) {
    sendSensorData();  // Immediately send sensor data when the button is pressed
  }
}

// Function for time synchronization
BLYNK_CONNECTED() {
  rtc.begin();
}
