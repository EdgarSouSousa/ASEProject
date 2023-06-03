#include "dht11.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp32/rom/ets_sys.h"

#define DHT11_PIN 4

void dht11_read(uint8_t *data) {
    gpio_config_t io_conf;
    io_conf.pin_bit_mask = (1ULL << DHT11_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    gpio_set_intr_type(DHT11_PIN, GPIO_INTR_ANYEDGE);

    gpio_set_level(DHT11_PIN, 0);
    ets_delay_us(20000); // 20 ms
    gpio_set_level(DHT11_PIN, 1);
    ets_delay_us(10);

    uint32_t start = xTaskGetTickCount();
    while (gpio_get_level(DHT11_PIN) == 0) {
        if (xTaskGetTickCount() - start > pdMS_TO_TICKS(5)) {
            printf("Timeout waiting for start signal\n");
            return;
        }
    }

    start = xTaskGetTickCount();
    while (gpio_get_level(DHT11_PIN) == 1) {
        if (xTaskGetTickCount() - start > pdMS_TO_TICKS(5)) {
            printf("Timeout waiting for response signal\n");
            return;
        }
    }

    for (int i = 0; i < 40; i++) {
        uint32_t start = xTaskGetTickCount();
        while (gpio_get_level(DHT11_PIN) == 0) {
            if (xTaskGetTickCount() - start > pdMS_TO_TICKS(5)) {
                printf("Timeout waiting for bit signal\n");
                return;
            }
        }

        uint32_t duration = xTaskGetTickCount() - start;
        data[i / 8] <<= 1;
        if (duration > pdMS_TO_TICKS(1)) { // assume 1 tick = 1 us
            data[i / 8] |= 1;
        }

        start = xTaskGetTickCount();
        while (gpio_get_level(DHT11_PIN) == 1) {
            if (xTaskGetTickCount() - start > pdMS_TO_TICKS(5)) {
                printf("Timeout waiting for bit signal\n");
                return;
            }
        }
    }

    // Calculate checksum
    uint8_t sum = 0;
    for (int i = 0; i < 4; i++) {
        sum += data[i];
    }
    if (sum != data[4]) {
        printf("Checksum error\n");
    }

    // Print sensor data and checksum
    printf("Sensor data: %d.%d %%RH, %d.%d °C\n", data[0], data[1], data[2], data[3]);
    printf("Checksum: %d\n", data[4]);
}
