#pragma once

#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
class io_board
{
private:
    uint8_t addr_;
    i2c_port_t i2c_port_;
    uint8_t input_port_register_bit_;
    uint8_t output_port_register_bits_ = 0b11111111;

    esp_err_t set_output_port_register(uint8_t reg_bits);
    esp_err_t set_configuration_register(uint8_t reg_bits);
public:
    io_board(i2c_port_t i2c_port, uint8_t addr);
    ~io_board();
    
    esp_err_t fetch_input_port_register(void);
    
    esp_err_t set_top_grn(bool toggle);
    esp_err_t set_top_red(bool toggle);
    esp_err_t set_bot_grn(bool toggle);
    esp_err_t set_bot_red(bool toggle);

    esp_err_t set_all_leds_on(void);

    bool get_input_port_register_single_bit(uint8_t bit_pos);
    uint8_t get_input_port_register();
};
