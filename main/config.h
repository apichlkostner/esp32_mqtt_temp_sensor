#pragma once


#define LED_GPIO 2
#define BUTTON_GPIO 15

#define ESP_INTR_FLAG_DEFAULT 0

#define ISR_QUEUE_SIZE 10
#define OTA_QUEUE_SIZE 2

// recommended measurement intervall is 2s because of temperature rise
#define MQTT_TEMP_PUBLISH_INTERVAL_MS 10000
#define DHT20_MEASURE_NUM_TRIES 40