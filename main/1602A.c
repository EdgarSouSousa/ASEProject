#include "1602A.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"

#define I2C_PORT I2C_NUM_0
#define TAG "_1602A"

#define DISPLAY_ADDR 0x27

esp_err_t i2c_init(i2c_port_t i2cPort, int sdaPin, int sclPin, uint32_t clkSpeedHz)
{
    i2c_config_t conf={
		.mode = I2C_MODE_MASTER,
		.sda_io_num = sdaPin,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_io_num = sclPin,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = clkSpeedHz
	};
    i2c_param_config(I2C_NUM_0, &conf);

    return i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
}


esp_err_t _1602A_send_command(i2c_port_t i2c_port, uint8_t command) {
    esp_err_t err;
    
    uint8_t high_nibble = command & 0xF0;
    uint8_t low_nibble = command << 4;
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DISPLAY_ADDR << 1) | I2C_MASTER_WRITE, true);
    vTaskDelay(1 / portTICK_PERIOD_MS); // Add this delay
    i2c_master_write_byte(cmd, high_nibble | 0x0C, true);
    vTaskDelay(1 / portTICK_PERIOD_MS); // Add this delay
    i2c_master_write_byte(cmd, high_nibble | 0x80, true); // Send enable pulse (EN=1)
    vTaskDelay(1 / portTICK_PERIOD_MS); // Add this delay
    
    i2c_master_write_byte(cmd, low_nibble | 0x0C,true);
    vTaskDelay(1 / portTICK_PERIOD_MS); // Add this delay
    i2c_master_write_byte(cmd, low_nibble | 0x80, true); // Send enable pulse (EN=1)
    vTaskDelay(1 / portTICK_PERIOD_MS); // Add this delay
    
    i2c_master_stop(cmd);
    err = i2c_master_cmd_begin(I2C_PORT, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return err;
}

esp_err_t _1602A_send_data(i2c_port_t i2c_port, uint8_t data) {
    esp_err_t err;
    
    uint8_t high_nibble = data & 0xF0; // Get the high nibble
    uint8_t low_nibble = (data & 0x0F) << 4; // Get the low nibble and shift it to the left by 4 bits

    //declare data array to send
    uint8_t data_array[4];

    data_array[0] = high_nibble | 0x0D;
    data_array[1] = high_nibble | 0x09;
    data_array[2] = low_nibble | 0x0D;
    data_array[3] = low_nibble | 0x09;

    //create command link
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    //start command
    i2c_master_start(cmd);
    //send address
    i2c_master_write_byte(cmd, (DISPLAY_ADDR << 1) | I2C_MASTER_WRITE, true);
    //send data
    i2c_master_write(cmd, data_array, 4, true);
    //stop command
    i2c_master_stop(cmd);
    //execute command
    err = i2c_master_cmd_begin(I2C_PORT, cmd, 1000 / portTICK_PERIOD_MS);
    //delete command link
    i2c_cmd_link_delete(cmd);


    return err;
}


// Add the lcd_display_string function
void _1602A_display_string(const char *str) {
    _1602A_send_command(I2C_PORT, 0x01); // Clear display
    // Wait for the command to be processed
    vTaskDelay(200 / portTICK_PERIOD_MS);


    for (size_t i = 0; str[i] != '\0'; i++) {
        _1602A_send_data(I2C_PORT, str[i]);
        // Wait for the command to be processed
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

esp_err_t _1602A_init(i2c_port_t i2c_port, uint8_t sens_addr, char *data, TickType_t time_out) {
    esp_err_t err;

    // Wait for the display to power up
    vTaskDelay(50 / portTICK_PERIOD_MS);

    // Configure the display
    uint8_t config_commands[] = {
        0x33, 
        0x32, 
        0x28, 
        0x0C, 
        0x06, 
        0x01, 
    };

    // Send configuration commands
    for (int i = 0; i < sizeof(config_commands); i++) {
        err = _1602A_send_command(i2c_port, config_commands[i]);

        if (err != ESP_OK) {
            return err;
        }

        // Wait for the command to be processed
        if (config_commands[i] == 0x01) {
            vTaskDelay(2 / portTICK_PERIOD_MS); // Clear screen takes longer
        } else {
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }
    }

    return ESP_OK;
}