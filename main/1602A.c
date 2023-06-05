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
    i2c_master_write_byte(cmd, high_nibble | 0x0C, true);
    i2c_master_write_byte(cmd, high_nibble | 0x80, true); // Send enable pulse (EN=1)
    
    i2c_master_write_byte(cmd, low_nibble | 0x0C,true);
    i2c_master_write_byte(cmd, low_nibble | 0x80, true); // Send enable pulse (EN=1)
    
    i2c_master_stop(cmd);
    err = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return err;
}

esp_err_t _1602A_send_data(i2c_port_t i2c_port, uint8_t command) {
    esp_err_t err;
    
    uint8_t high_nibble = command & 0xF0;
    uint8_t low_nibble = command << 4;
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DISPLAY_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, high_nibble | 0x09, true);
    i2c_master_write_byte(cmd, high_nibble | 0x0D, true); // Send enable pulse (EN=1)

    
    
    i2c_master_write_byte(cmd, low_nibble | 0x09,true);
    i2c_master_write_byte(cmd, low_nibble | 0x0D, true); // Send enable pulse (EN=1)
    
    i2c_master_stop(cmd);
    err = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
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

   
    // Configure the display
    uint8_t config_commands[] = {
        0x30, // 2-line, 5x8 matrix
        0x30, // Turn cursor off
        0x30,
        0x20, // Shift cursor right
        0x28,  // Clear screen
		0x08,
        0x01, // Display on, cursor off, blink off
        0x06, // Increment cursor
        0x0C, // Turn on display

    };

    // Send configuration commands
    for (int i = 0; i < sizeof(config_commands); i++) {
        err = _1602A_send_command(i2c_port, config_commands[i]);
        //wait 100 ms 
        vTaskDelay(100 / portTICK_PERIOD_MS);
		
        if (err != ESP_OK) {
            return err;
        }
    }

    //wait 100 ms
    vTaskDelay(100 / portTICK_PERIOD_MS);
    return ESP_OK;
}