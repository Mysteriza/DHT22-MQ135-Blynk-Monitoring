# Air Quality Monitoring with ESP8266, DHT22, and MQ-135

This project is an air quality monitoring system built using an ESP8266 microcontroller, a DHT22 sensor for temperature and humidity, and an MQ-135 sensor for gas detection. It integrates with the Blynk IoT platform to provide real-time data visualization and includes robust connectivity features. The system is designed for indoor use, such as monitoring air quality in a room, with a light-friendly night mode for the onboard LED.

## [Web App | Air Quality, Temperature & Humidity Monitoring](https://github.com/Mysteriza/WebApp-TempHum-Monitoring)

## Features

- **Temperature and Humidity Monitoring**: Uses a DHT22 sensor to measure temperature and humidity, updated every 20 seconds.
- **Gas Detection**: Employs an MQ-135 sensor to detect gases (e.g., NH3, CO2, benzene) with averaging over 5 samples for stability.
- **Air Quality Status**: Calculates a compensated gas value based on temperature and humidity, then categorizes air quality into:
  - Very Good (≤ 200)
  - Good (≤ 400)
  - Fair (≤ 600)
  - Poor (≤ 800)
  - Very Poor (> 800)
- **Blynk Integration**: Sends data to Blynk virtual pins for real-time monitoring:
  - V0: Temperature
  - V1: Humidity
  - V2: System status (e.g., "Latest Update!", "Failed to read from DHT22 sensor!")
  - V3: Manual update button
  - V4: Current time (HH:MM)
  - V5: Raw gas value
  - V6: Compensated gas value
  - V7: Air quality status

- **Night Mode for LED**: Disables the onboard LED (GPIO D0) from 21:00 to 06:00 to prevent light disturbance at night. Outside these hours:
  - Blinks 3 times (500ms on/off) if DHT22 or MQ-135 fails to read data.
  - Stays on for 3 seconds if air quality is poor (compensated gas > 600).
- **Robust Connectivity**:
  - **WiFi Reconnection**: Checks WiFi every 5 seconds and attempts reconnection if disconnected.
  - **Blynk Reconnection**: Monitors Blynk connection every 10 seconds and reinitializes if the server link drops, even with active WiFi.
- **Error Handling**: Reports sensor failures to Blynk (V2) and continues operation without crashing.

## Hardware Requirements

- ESP8266 (e.g., NodeMCU or Wemos D1 Mini)
- DHT22 temperature and humidity sensor
- MQ-135 gas sensor
- Jumper wires and a breadboard or custom PCB
- USB power supply (always connected)

## Software Requirements

- Arduino IDE or PlatformIO
- Libraries:
  - `ESP8266WiFi` (pre-installed with ESP8266 board support)
  - `Blynk` (install via Library Manager)
  - `DHT` by Adafruit (install via Library Manager)
  - `TimeLib` (install via Library Manager)
  - `WidgetRTC` (included with Blynk)

## Notes
- Night Mode: The LED is disabled from 21:00 to 06:00. Adjust the hours in isNightTime() if needed.
- Sensor Stability: The 20-second update interval reduces DHT22 errors. Increase to 30 seconds if failures persist.
  

<img src="https://github.com/user-attachments/assets/34303268-10ba-4395-a387-1a26e75ad294" alt="image" width="300">
<img src="https://github.com/user-attachments/assets/fe96864b-4750-4365-a52a-b79724ae8521" alt="image" width="350">



