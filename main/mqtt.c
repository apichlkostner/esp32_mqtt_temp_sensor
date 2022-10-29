#include <string.h>
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"

#include "led.h"
#include "mqtt.h"
#include "ota.h"

static const char *TAG = "MQTT";
static esp_mqtt_client_handle_t client;

extern const uint8_t mqtt_self_ca_pem_start[]   asm("_binary_mqtt_self_ca_pem_start");

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected");
        if (esp_mqtt_client_subscribe(client, "/io/led", 0) == -1)
            ESP_LOGI(TAG, "Error subscribing to /io/led");
        if (esp_mqtt_client_subscribe(client, "/update/esp32", 0) == -1)
            ESP_LOGI(TAG, "Error subscribing to /update/esp32");
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "topic: %.*s\r\n", event->topic_len, event->topic);
        ESP_LOGI(TAG, "data: =%.*s\r\n", event->data_len, event->data);

        // led data
        if (strncmp(event->topic, "/io/led", event->topic_len) == 0) {
            if (strncmp(event->data, "on", event->data_len) == 0) {
                ESP_LOGI(TAG, "Turning on the LED");
                set_led_state(1);
            } else {
                ESP_LOGI(TAG, "Turning off the LED");
                set_led_state(0);
            }
        }

        // update request
        if (strncmp(event->topic, "/update/esp32", event->topic_len) == 0) {
            ota_update(event->data, event->data_len);
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT error");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            ESP_LOGI(TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                     strerror(event->error_handle->esp_transport_sock_errno));
        } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
            ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
        } else {
            ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
        }
        break;
    default:
        break;
    }
}

void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = CONFIG_MQTT_BROKER_URI,
            .verification.certificate = (const char *)mqtt_self_ca_pem_start
        },
        .credentials = {
            .username = CONFIG_MQTT_USERNAME,
            .authentication = {
                .password = CONFIG_MQTT_PASSWORD
            }
        }
    };

    ESP_LOGI(TAG, "[APP] Free memory: %lu bytes", esp_get_free_heap_size());
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void mqtt_publish_temp(int temp)
{
    char temp_str[14];
    snprintf(temp_str, sizeof(temp_str), "%d.%d", temp / 10, temp % 10);
    esp_mqtt_client_publish(client, "/pos1/temperature", temp_str, 0, 0, 0);
}

void mqtt_publish_humidity(int humidity)
{
    char humidity_str[14];
    snprintf(humidity_str, sizeof(humidity_str), "%d.%d", humidity / 10, humidity % 10);
    esp_mqtt_client_publish(client, "/pos1/humidity", humidity_str, 0, 0, 0);
}
