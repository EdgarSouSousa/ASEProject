#include "http_request.h"
#include "esp_http_client.h"
#include "esp_log.h"
#define TAG "final-project"

void send_http_request(int adc_value1, int adc_value2, float temperature, float humidity)
{
    esp_http_client_config_t config = {
        .url = "http://192.168.1.77:5000",
        .method = HTTP_METHOD_POST,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    char post_data[64];
    snprintf(post_data, sizeof(post_data), "value1=%d&value2=%d&temperature=%.1f&humidity=%.1f", adc_value1, adc_value2, temperature, humidity);

    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP request sent successfully, status = %d", esp_http_client_get_status_code(client));
    }
    else
    {
        ESP_LOGE(TAG, "Error sending HTTP request: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}
