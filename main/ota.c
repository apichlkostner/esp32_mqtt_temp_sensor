#include "ota.h"
#include "esp_ota_ops.h"
#include "esp_log.h"
#include "esp_https_ota.h"
#include "config.h"

QueueHandle_t queue_ota;

static const char *TAG = "OTA";

#define URL_MAX_SIZE 512
char url_buffer[URL_MAX_SIZE];
static struct ota_msg msg = {url_buffer, 0};
static bool update_running = false;


extern const uint8_t mqtt_self_ca_pem_start[]   asm("_binary_mqtt_self_ca_pem_start");


void Ota_Task(void *params)
{
    struct ota_msg msg;

    queue_ota = xQueueCreate(OTA_QUEUE_SIZE, sizeof(struct ota_msg));

    while(1) {
        ESP_LOGI(TAG, "Waiting for command from MQTT");
        if (xQueueReceive(queue_ota, &msg, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "MQTT received...");

            esp_http_client_config_t config = {
                .url = msg.url,
                .cert_pem = (char *)mqtt_self_ca_pem_start
            };
            esp_https_ota_config_t ota_config = {
                .http_config = &config,
            };
            esp_err_t ret = esp_https_ota(&ota_config);
            if (ret == ESP_OK) {
                esp_restart();
            }
            update_running = false;
        }
    }
}

void ota_update(char *url, size_t url_len)
{
    if (!update_running && (url_len < URL_MAX_SIZE)) {
        update_running = true;
        strncpy(msg.url, url, URL_MAX_SIZE - 1);
        msg.url[url_len + 1] = 0;
        msg.url_len = url_len;
        xQueueSend(queue_ota, &msg, 200 / portTICK_PERIOD_MS);
    }
}
