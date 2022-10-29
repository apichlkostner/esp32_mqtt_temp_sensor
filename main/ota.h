#pragma once

#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

struct ota_msg {
    char *url;
    size_t url_len;
};

extern QueueHandle_t queue_ota;
void Ota_Task(void *params);
void ota_update(char *url, size_t url_len);