#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "button.h"
#include "dht20.h"
#include "isr.h"
#include "led.h"
#include "mqtt.h"
#include "ota.h"

#include "protocol_examples_common.h"

#include <sys/param.h>


static const char* TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Startup..");
    ESP_LOGI(TAG, "Free memory: %lu bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());

    isr_init();
    led_init();
    button_init();
    dht20_init();

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    mqtt_app_start();

    if (xTaskCreate(LED_Control_Task, "LED_Control_Task", 2048, NULL, 1, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Could not create LED_Control_Task ");
    }
    if (xTaskCreate(DHT20_Task, "DHT20_Task", 3072, NULL, 1, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Could not create DHT20_Task ");
    }
    if (xTaskCreate(Ota_Task, "Ota_Task", 8192, NULL, 2, NULL)!= pdPASS) {
        ESP_LOGE(TAG, "Could not create Ota_Task ");
    }

    ESP_LOGI(TAG, "Free memory: %lu bytes", esp_get_free_heap_size());

    // while (1) {
    //     ESP_LOGI(TAG, "Main loop");
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
}
