# Temperature sensor with ESP32

Build with the ESP-IDF toolchain.

- Measures temperature and humidity
- Publishes the data to a MQTT server
- Reads a button
- Controls a led with button or by MQTT message
- Received OTA update command with MQTT
- Performs OTA update by downloading the image from a HTTPS server
- Connection to MQTT and HTTPS server secured with self signed certificates
- Power safe mode for WiFi configurable