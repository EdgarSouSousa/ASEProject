#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_ota_ops.h"
#include "OTA.h"

static const char *TAG = "OTA";

static bool isUpdateAvailable(esp_http_client_handle_t client, esp_http_client_config_t *config)
{
    int content_length = esp_http_client_fetch_headers(client);
    if (content_length > 0)
    {
        esp_https_ota_handle_t ota_handle = NULL;
        esp_https_ota_config_t ota_config = {
            .http_config = config,
        };

        esp_err_t err = esp_https_ota_begin(&ota_config, &ota_handle);
        if (err == ESP_OK)
        {
            esp_app_desc_t new_app_info;
            esp_https_ota_get_img_desc(ota_handle, &new_app_info);

            // Compare current version with the new version
            esp_app_desc_t running_app_info;
            esp_ota_get_partition_description(esp_ota_get_running_partition(), &running_app_info);

            esp_https_ota_finish(ota_handle);

            if (strcmp(new_app_info.version, running_app_info.version) != 0)
            {
                return true;
            }
        }
    }

    return false;
}

static void performOTAUpdate(esp_http_client_handle_t client, esp_http_client_config_t *config)
{
    esp_https_ota_handle_t ota_handle = NULL;
    esp_https_ota_config_t ota_config = {
        .http_config = config,
    };

    esp_err_t err = esp_https_ota_begin(&ota_config, &ota_handle);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Updating firmware...");

        while (1)
        {
            err = esp_https_ota_perform(ota_handle);
            if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS)
            {
                break;
            }
        }
    }

    if (isUpdateAvailable(client, config))
    {
        performOTAUpdate(client, config);
    }
    else
    {
        ESP_LOGI(TAG, "No new firmware available");
    }
}

void checkForUpdates(void)
{
    // URL of the firmware binary hosted on your HTTP server
    const char *firmwareUrl = "http://example.com/firmware.bin";

    esp_http_client_config_t config = {
        .url = firmwareUrl,
        .event_handler = NULL,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_open(client, 0);

    if (err == ESP_OK)
    {
        if (isUpdateAvailable(client, &config))
        {
            performOTAUpdate(client, &config);
        }
        else
        {
            ESP_LOGI(TAG, "No new firmware available");
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to check for updates, HTTP error: %d", err);
    }

    esp_http_client_cleanup(client);
}
