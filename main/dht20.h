#pragma once

void dht20_init();
bool dht20_measure(int *temperature, int *humidity);
void DHT20_Task(void *params);
