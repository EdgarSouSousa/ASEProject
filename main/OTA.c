#include "esp_http_client.h"
#include "esp_ota_ops.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_app_desc.h"
#include "OTA.h"

#define BUFFSIZE 1024
uint8_t ota_write_data[BUFFSIZE + 1] = { 0 };


static const char *TAG = "OTA";

#ifndef ESP_APP_DESC_SHA256_LEN
#define ESP_APP_DESC_SHA256_LEN 32
#endif


void uint8_to_char(char *dest, uint8_t *src, size_t src_len)
{
    for (size_t i = 0; i < src_len; i++)
    {
        sprintf(dest + (i * 2), "%02x", src[i]);
    }
}

void checkForUpdates(void)
{
    // URL of the firmware binary hosted on your HTTP server
    const char *firmwareUrl = "http://192.168.1.77:6000/final-project.bin";

    esp_http_client_config_t config = {
        .url = firmwareUrl,
        .event_handler = NULL,
        .timeout_ms = 50000,
    };

    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = NULL;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_open(client, 0);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Connection established...");
        int content_length = esp_http_client_fetch_headers(client);
        ESP_LOGI(TAG, "Content length: %d bytes", content_length);

       
        if (content_length > 0)
        {

            update_partition = esp_ota_get_next_update_partition(NULL);
            err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "esp_ota_begin failed, error=%d", err);
                return;
            }
            ESP_LOGI(TAG, "esp_ota_begin succeeded");

            // Perform OTA Update
            while (1)
            {
                int data_read = esp_http_client_read(client, (char *)ota_write_data, BUFFSIZE);
                if (data_read < 0)
                {
                    ESP_LOGE(TAG, "Error: SSL data read error");
                    return;
                }
                else if (data_read > 0)
                {
                    err = esp_ota_write( update_handle, (const void *)ota_write_data, data_read);
                    if (err != ESP_OK)
                    {
                        return;
                    }
                }
                else if (data_read == 0)
                {
                    ESP_LOGI(TAG, "Connection closed, all data received no update found");
                    break;
                }
            }

            err = esp_ota_end(update_handle);
            if (err != ESP_OK) 
            {
                ESP_LOGE(TAG, "esp_ota_end failed!");
                return;
            }

            err = esp_ota_set_boot_partition(update_partition);
            if (err != ESP_OK) 
            {
                ESP_LOGE(TAG, "esp_ota_set_boot_partition failed!");
                return;
            }

            ESP_LOGI(TAG, "Prepare to restart system!");
            esp_restart();
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to check for updates, HTTP error: %d", err);
    }

    esp_http_client_cleanup(client);
}