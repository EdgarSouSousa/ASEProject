#include "dht11.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define DHT11_PIN 4


void delay_us(uint32_t us) {
    const uint32_t freq = 80; // 80 MHz
    uint32_t count = (us * freq) / 2;
    for (uint32_t i = 0; i < count; i++) {
        asm volatile ("nop");
    }
}

void dht11_read(uint8_t *data) {
    gpio_set_direction(DHT11_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_PIN, 0);
    //delay_us(20000); // 20 ms
    gpio_set_level(DHT11_PIN, 1);
    //delay_us(40000);
    gpio_set_direction(DHT11_PIN, GPIO_MODE_INPUT);
    while (gpio_get_level(DHT11_PIN) == 0) {}
    while (gpio_get_level(DHT11_PIN) == 1) {}
    for (int i = 0; i < 40; i++) {
        uint32_t start = esp_timer_get_time();
        while (gpio_get_level(DHT11_PIN) == 0) {}
        uint32_t duration = esp_timer_get_time() - start;
        data[i / 8] <<= 1;
        if (duration > 40) {
            data[i / 8] |= 1;
        }
    }
}