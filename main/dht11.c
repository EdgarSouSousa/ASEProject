#include <string.h>
#include "dht11.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp32/rom/ets_sys.h"
#include "esp_timer.h"

#define DHT11_PIN 4

uint8_t reading[5] = {0,0,0,0,0};
static gpio_num_t dht_gpio = DHT11_PIN;

int waitAndCount(int timeOut) {
    int count = 0;
    while(gpio_get_level(dht_gpio) == 1) { 
        ets_delay_us(1);
        count++;
        if(count == timeOut) {
            count = -1;
            break;
        }
    }
    return count;
}

uint8_t* dht11_read() {

    //reset data
    memset(reading, 0, sizeof(reading));

    //send dht11 start signal
    gpio_set_direction(dht_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(dht_gpio, 0);
    ets_delay_us(20 * 1000);
    gpio_set_level(dht_gpio, 1);
    ets_delay_us(40);
    gpio_set_direction(dht_gpio, GPIO_MODE_INPUT);

    int count = 0;
    while(gpio_get_level(dht_gpio) == 0) { 
        ets_delay_us(1);
        count++;
        if(count == 80) {
            printf("Timeout waiting for response\n");
            return reading;
        }
    }
 
    //wait for dht11 response
    if(waitAndCount(80) == -1) {
        printf("Timeout waiting for response\n");
        return reading; 
    }

    
    //read response, each response is 50us followed by 26-28us for 0 and 70us for 1
    //so we wait for 40us and check if the pin is still low, if it is then it is a 0
    //if it is high then it is a 1
    //we do this for 5 bytes of data
    for(int i = 0; i < 40; i++) {
        int count = 0;
        while(gpio_get_level(dht_gpio) == 0) { 
            ets_delay_us(1);
            count++;
            if(count == 50) {
            printf("Timeout waiting start bit\n");
            return reading;
        }
    }
        if(waitAndCount(70) > 28) {
            reading[i/8] |= (1 << (7-(i%8)));
        }
    }

    
    if(reading[4]!= (reading[0] + reading[1] + reading[2] + reading[3])) {
        printf("Checksum failed\n");
        return reading;
    }

    printf("Temperature: %d\n", reading[2]);
    printf("Humidity: %d\n", reading[0]);
    
    return reading;
}

