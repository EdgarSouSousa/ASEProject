#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#define TAG "final-project"

SemaphoreHandle_t xSemaphore;
float temperature, humidity;

void dht11_task(void *pvParameters)
{
    uint8_t dht11_data[5] = {0}; // Initialize the DHT11 data array

    while (1) {
        // Read the DHT11 data
        dht11_read(dht11_data);
        uint8_t checksum = dht11_data[0] + dht11_data[1] + dht11_data[2] + dht11_data[3];
        if (checksum == dht11_data[4]) {
            temperature = dht11_data[2] + (dht11_data[3] / 10.0);
            humidity = dht11_data[0] + (dht11_data[1] / 10.0);
            ESP_LOGI(TAG, "Temperature: %.1f C, Humidity: %.1f %%", temperature, humidity);
        } else {
            ESP_LOGE(TAG, "Checksum error");
        }

        xSemaphoreGive(xSemaphore); // Release the semaphore
        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for 5 seconds before reading again
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

        xSemaphoreGive(xSemaphore); // Release the semaphore
        vTaskDelay(pdMS_TO_TICKS(5000)); // Wait for 1 second before reading again
    }
}


void http_request_task(void *pvParameters)
{
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(3000));

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

        // Give the semaphore back to allow the tasks to continue
        xSemaphoreGive(xSemaphore);

        vTaskDelay(pdMS_TO_TICKS(3000)); // Wait for 5 seconds before sending again
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

    // Initialize the default event loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize the Wi-Fi connection
    wifi_init_sta();
    

    // Create the semaphore
    xSemaphore = xSemaphoreCreateBinary();

    // Create the DHT11 task
    xTaskCreate(dht11_task, "dht11_task", 2048, NULL, 5, NULL);

    // Create the ADC task
    xTaskCreate(adc_task, "adc_task", 2048, NULL, 4, NULL);

    // Create the HTTP request task
    xTaskCreate(http_request_task, "http_request_task", 8192, NULL, 3, NULL);
}