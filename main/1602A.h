#pragma once
#include "driver/i2c.h"

esp_err_t i2c_init(i2c_port_t i2cPort, int sdaPin, int sclPin, uint32_t clkSpeedHz);

esp_err_t _1602A_init(i2c_port_t i2c_port, uint8_t sens_addr, char* data, TickType_t time_out);

esp_err_t _1602A_send_command(i2c_port_t i2c_port, uint8_t command);

esp_err_t _1602A_send_data(i2c_port_t i2c_port, uint8_t data);

void _1602A_display_string(const char *str);
