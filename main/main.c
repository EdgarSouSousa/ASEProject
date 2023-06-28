#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_http_client.h"
#include "esp_app_desc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_http_client.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "wifi_init.h"
#include "adc.h"
#include "dht11.h"
#include "http_request.h"
#include "esp_sleep.h"
#include "1602A.h"
#include "OTA.h"


#define TAG "final-project"

#define I2C_PORT I2C_NUM_0
#define SDA_PIN 21
#define SCL_PIN 22
#define CLK_SPEED_HZ 100000
#define DISPLAY_ADDR 0x27

SemaphoreHandle_t xSemaphore;
SemaphoreHandle_t xSemaphoreUpdate;
float temperature, humidity;

void dht11_task(void *pvParameters)
{
    

    while (1) {
        xSemaphoreTake(xSemaphoreUpdate, portMAX_DELAY); // Take the semaphore
        // Read the DHT11 data returned by the sensor
        uint8_t* dht11_data = dht11_read();
        
        uint8_t checksum = dht11_data[0] + dht11_data[1] + dht11_data[2] + dht11_data[3];
        if (checksum == dht11_data[4]) {
            temperature = dht11_data[2] + (dht11_data[3] / 10.0);
            humidity = dht11_data[0] + (dht11_data[1] / 10.0);
            ESP_LOGI(TAG, "Temperature: %.1f C, Humidity: %.1f %%", temperature, humidity);
        } else {
            ESP_LOGE(TAG, "Checksum error");
        }

        xSemaphoreGive(xSemaphore); // Release the semaphore
        xSemaphoreGive(xSemaphoreUpdate); // Give the semaphore
        vTaskDelay(pdMS_TO_TICKS(10000)); // Wait for 5 seconds before reading again
    }
}

void adc_task(void *pvParameters)
{
    // Initialize the ADC
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // Configure the ADC channels
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = EXAMPLE_ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN0, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN1, &config));

    // Calibration
    adc_cali_handle_t adc1_cali_handle = NULL;
    bool do_calibration1 = example_adc_calibration_init(ADC_UNIT_1, EXAMPLE_ADC_ATTEN, &adc1_cali_handle);

    while (1) {
        // Read the ADC values
        xSemaphoreTake(xSemaphoreUpdate, portMAX_DELAY); // Take the semaphore
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN0, &adc_raw[0][0]));
        ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN0, adc_raw[0][0]);
        if (do_calibration1) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle, adc_raw[0][0], &voltage[0][0]));
            ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN0, voltage[0][0]);
        }

        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN1, &adc_raw[0][1]));
        ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN1, adc_raw[0][1]);
        if (do_calibration1) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle, adc_raw[0][1], &voltage[0][1]));
            ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN1, voltage[0][1]);
        }

        xSemaphoreGive(xSemaphoreUpdate); // Give the semaphore
        xSemaphoreGive(xSemaphore); // Release the semaphore
        vTaskDelay(pdMS_TO_TICKS(10000)); // Wait for 1 second before reading again
    }
}


void http_request_task(void *pvParameters)
{
    while (1) {
        xSemaphoreTake(xSemaphoreUpdate, portMAX_DELAY); // Take the semaphore
         //normalise the voltage
        voltage[0][0] = (voltage[0][0]- 142) * 100 / 4095;
        voltage[0][1] = (voltage[0][1]-142) * 100 / 4095;


          // convert volage[0][0] to a string to be displayed on the LCD
        char voltage1[20]; 
        sprintf(voltage1, "%d", voltage[0][0]);
        strcat(voltage1, " Humidity ");
        _1602A_display_string(voltage1);


        xSemaphoreTake(xSemaphore, portMAX_DELAY);

        // Wait for the Wi-Fi module to be connected
        wifi_ap_record_t ap_info;
        esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);
        while (err != ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            err = esp_wifi_sta_get_ap_info(&ap_info);
        }

       

        // Send the data via HTTP request
        send_http_request(voltage[0][0], voltage[0][1], temperature, humidity);

      
        xSemaphoreGive(xSemaphoreUpdate); // Give the semaphore
        // Give the semaphore back to allow the tasks to continue
        xSemaphoreGive(xSemaphore);

        //esp_sleep_enable_timer_wakeup(9000000);

        vTaskDelay(pdMS_TO_TICKS(10000)); // Wait for 10 seconds before sending again
    }
}

void update_check_task(void *pvParameters)
{
    while (1) {
        xSemaphoreTake(xSemaphoreUpdate, portMAX_DELAY); // Take the semaphore
        ESP_LOGI(TAG, "Checking for updates...");
        checkForUpdates();
        xSemaphoreGive(xSemaphoreUpdate); // Give the semaphore
        vTaskDelay(pdMS_TO_TICKS(16000)); // Check for updates every hour
    }
}


void app_main(void)
{
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    i2c_init(I2C_PORT, SDA_PIN, SCL_PIN, CLK_SPEED_HZ);
    _1602A_init(I2C_PORT, DISPLAY_ADDR, "final", 1000);


    // Initialize the default event loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize the Wi-Fi connection
    wifi_init_sta();


    // Create the semaphore
    xSemaphore = xSemaphoreCreateBinary();
    xSemaphoreUpdate = xSemaphoreCreateBinary();

    //xSemaphoreGive(xSemaphore); // Give the semaphore
    xSemaphoreGive(xSemaphoreUpdate); // Give the semaphore
    

    xTaskCreate(update_check_task, "update_check_task", 8192, NULL, 2, NULL);


    // Create the DHT11 task
    xTaskCreate(dht11_task, "dht11_task", 2048, NULL, 5, NULL);

    // Create the ADC task
    xTaskCreate(adc_task, "adc_task", 2048, NULL, 4, NULL);

    // Create the HTTP request task
    xTaskCreate(http_request_task, "http_request_task", 8192, NULL, 3, NULL);
}
