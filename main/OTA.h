#ifndef OTA_H
#define OTA_H

#include "esp_http_client.h"

// bool isUpdateAvailable(esp_http_client_handle_t client, esp_http_client_config_t *config);
// void performOTAUpdate(esp_http_client_handle_t client, esp_http_client_config_t *config);
void checkForUpdates(void);

#endif // OTA_H
