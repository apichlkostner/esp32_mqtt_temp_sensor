#include "esp_log.h"
#include "driver/i2c.h"
#include "config.h"
#include "mqtt.h"
#include "dht20.h"

static const char *TAG = "i2c";

#define I2C_MASTER_SCL_IO           CONFIG_I2C_MASTER_SCL
#define I2C_MASTER_SDA_IO           CONFIG_I2C_MASTER_SDA
#define I2C_MASTER_NUM              0
#define I2C_MASTER_FREQ_HZ          400000
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TIMEOUT_MS       1000

#define DHT20_SENSOR_ADDR                   0x38

#define DHT20_MEASURE_CMD                   0xAC
#define DHT20_WRITE_REGISTER_CMD            0x70
#define DHT20_STATUS_CMD                    0x71

static esp_err_t dht20_register_read(uint8_t reg_addr, uint8_t *data, size_t len);
static esp_err_t dht20_register_write(uint8_t *data, size_t len);
static esp_err_t i2c_master_init(void);
static void reset_register(uint8_t dht20_reg);
static void DHT20_init_Task(void *params);

struct dht20_data_s {
    bool initialized;
};

static volatile struct dht20_data_s dht20_data;

void dht20_init()
{
    xTaskCreate(DHT20_init_Task, "DHT20_Init_Task", 2048, NULL, 1, NULL);
}

bool dht20_measure(int *temperature, int *humidity)
{
    uint8_t data[20];

    while (!dht20_data.initialized) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    // datasheet 7.4.2: wait 10ms
    vTaskDelay(10 / portTICK_PERIOD_MS);
        
    // send read command
    uint8_t command1[] = {DHT20_WRITE_REGISTER_CMD, 0xac, 0x33, 0x00};
    dht20_register_write(command1, sizeof(command1));

    // 7.4.3: wait 80ms
    vTaskDelay(80 / portTICK_PERIOD_MS);

    // read status till busy indication is false
    int cnt;
    for (cnt = 0; cnt < DHT20_MEASURE_NUM_TRIES; cnt++) {
        //ESP_LOGI(TAG, "Read register, try %d", cnt);
        ESP_ERROR_CHECK(dht20_register_read(DHT20_STATUS_CMD, data, 1));
        if ((data[0] & 0x80) != 0x80)
            break;
        // check every 2ms, from sample code
        vTaskDelay(2 / portTICK_PERIOD_MS);
    }

    // try again the measurement sequence
    if (cnt == DHT20_MEASURE_NUM_TRIES)
       return false;

    ESP_ERROR_CHECK(dht20_register_read(DHT20_STATUS_CMD, data, 7));

    int val_hum = 0;
    val_hum = (val_hum | data[1]) << 8;
    val_hum = (val_hum | data[2]) << 4;
    val_hum = (val_hum | (data[3] >> 4));

    int val_temp = 0;
    val_temp = (val_temp | (data[3] & 0x0f)) << 8;
    val_temp = (val_temp | data[4]) << 8;
    val_temp = (val_temp | data[5]);

    // integer (value in % * 10)
    // rh = (value / 2^20) * 100%
    *humidity = val_hum * 100 * 10 / 0x100000;
    // integer (value in degree celsius * 10)
    // t = (value in degree celsius / 2^20) * 200 - 50
    *temperature = val_temp * 200 * 10 / 0x100000 - 50 * 10;

    return true;
}

void DHT20_Task(void *params)
{
    while (true) {
        int temperature, humidity;

        if (dht20_measure(&temperature, &humidity)) {
            ESP_LOGI(TAG, "Temperature: %d.%d", temperature / 10, temperature % 10);
            ESP_LOGI(TAG, "Humidity: %d.%d", humidity / 10, humidity % 10);

            mqtt_publish_temp(temperature);
            mqtt_publish_humidity(humidity);
        } else {
            ESP_LOGI(TAG, "Error reading measurement from sensor");
        }

        vTaskDelay(MQTT_TEMP_PUBLISH_INTERVAL_MS / portTICK_PERIOD_MS);
    }
}

static void DHT20_init_Task(void *params)
{
    uint8_t data[20];
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    // from Datasheet 7.4
    // 1. after power on wait at least 100ms
    vTaskDelay(200 / portTICK_PERIOD_MS);

    // 2. read status
    ESP_LOGI(TAG, "Read DHT20 status register");
    ESP_ERROR_CHECK(dht20_register_read(DHT20_STATUS_CMD, data, 1));

    // 3. check status & 0x18 == 0x18
    if ((data[0] & 0x18) != 0x18) {
        ESP_LOGI(TAG, "Reset registers");
        // 4. if not condition, reset registers
        reset_register(0x1b);
        reset_register(0x1c);
        reset_register(0x1e);
    }

    dht20_data.initialized = true;
    ESP_LOGI(TAG, "DHT20 initialized successfully");

    vTaskDelete(NULL);
}

static esp_err_t dht20_register_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, DHT20_SENSOR_ADDR, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

static esp_err_t dht20_register_write(uint8_t *data, size_t len)
{
    int ret;

    ret = i2c_master_write_to_device(I2C_MASTER_NUM, DHT20_SENSOR_ADDR, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    return ret;
}

static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

static void reset_register(uint8_t dht20_reg)
{
    uint8_t data[5];

    uint8_t command1[] = {DHT20_WRITE_REGISTER_CMD, dht20_reg, 0x00, 0x00};
    dht20_register_write(command1, sizeof(command1));
    vTaskDelay(5 / portTICK_PERIOD_MS);
    dht20_register_read(DHT20_STATUS_CMD, data, 3);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    uint8_t command2[] = {DHT20_WRITE_REGISTER_CMD, 0xb0 | dht20_reg, data[1], data[2]};
    dht20_register_write(command2, sizeof(command2));
}
