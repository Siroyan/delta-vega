#pragma once

#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"

#define I2C_MASTER_TIMEOUT_MS           1000

#define INPUT_PORT_REGISTER_ADDR        0x00
#define OUTPUT_PORT_REGISTER_ADDR       0x01
#define CONFIGURATION_REGISTER_ADDR     0x03

class io_board
{
private:
    uint8_t addr_;
    i2c_port_t i2c_port_;
    uint8_t input_port_register_bit_;
    uint8_t output_port_register_bits_ = 0b11111111;

    esp_err_t fetch_input_port_register(uint8_t *data);
    esp_err_t set_output_port_register(uint8_t reg_bits);
    esp_err_t set_configuration_register(uint8_t reg_bits);
public:
    io_board(i2c_port_t i2c_port, uint8_t addr);
    ~io_board();

    esp_err_t set_top_grn_on(void);
    esp_err_t set_top_red_on(void);
    esp_err_t set_bot_grn_on(void);
    esp_err_t set_bot_red_on(void);
    
    esp_err_t set_top_grn_of(void);
    esp_err_t set_top_red_of(void);
    esp_err_t set_bot_grn_of(void);
    esp_err_t set_bot_red_of(void);

    esp_err_t on_all_leds(void);

    bool get_ctrl_sw_state(void);
    uint8_t get_input_port_register();
};

io_board::io_board(i2c_port_t i2c_port, uint8_t addr) {
    i2c_port_ = i2c_port;
    addr_ = addr;
    set_configuration_register(0b11110000);
}

esp_err_t io_board::set_top_grn_on(void) {
    return set_output_port_register(output_port_register_bits_ | (1 << 3));
}

esp_err_t io_board::set_top_red_on(void) {
    return set_output_port_register(output_port_register_bits_ | (1 << 2));
}

esp_err_t io_board::set_bot_grn_on(void) {
    return set_output_port_register(output_port_register_bits_ | (1 << 1));
}

esp_err_t io_board::set_bot_red_on(void) {
    return set_output_port_register(output_port_register_bits_ | (1 << 0));
}

esp_err_t io_board::set_top_grn_of(void) {
    return set_output_port_register(output_port_register_bits_ & ~(1 << 3));
}

esp_err_t io_board::set_top_red_of(void) {
    return set_output_port_register(output_port_register_bits_ & ~(1 << 2));
}

esp_err_t io_board::set_bot_grn_of(void) {
    return set_output_port_register(output_port_register_bits_ & ~(1 << 1));
}

esp_err_t io_board::set_bot_red_of(void) {
    return set_output_port_register(output_port_register_bits_ & ~(1 << 0));
}

esp_err_t io_board::on_all_leds(void) {
    output_port_register_bits_ = output_port_register_bits_ & 0b11110000;
    return set_output_port_register(output_port_register_bits_);
}

bool io_board::get_ctrl_sw_state(void) {
    return !((input_port_register_bit_ & 0b00100000) >> 5);
}

uint8_t io_board::get_input_port_register(void) {
    ESP_ERROR_CHECK(fetch_input_port_register(&input_port_register_bit_));
    return input_port_register_bit_;
}

esp_err_t io_board::fetch_input_port_register(uint8_t *data) {
    uint8_t input_port_register_address = INPUT_PORT_REGISTER_ADDR;
    return i2c_master_write_read_device(i2c_port_, addr_, &input_port_register_address, 1, data, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

esp_err_t io_board::set_output_port_register(uint8_t reg_bits) {
    uint8_t write_buf[2] = {OUTPUT_PORT_REGISTER_ADDR, reg_bits};
    return i2c_master_write_to_device(i2c_port_, addr_, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

esp_err_t io_board::set_configuration_register(uint8_t reg_bits) {
    uint8_t write_buf[2] = {CONFIGURATION_REGISTER_ADDR, reg_bits};
    return i2c_master_write_to_device(i2c_port_, addr_, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

io_board::~io_board() {
}
