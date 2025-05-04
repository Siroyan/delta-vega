#pragma once

#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"

#define I2C_MASTER_TIMEOUT_MS           1000

#define INPUT_PORT_REGISTER_ADDR        0x00
#define OUTPUT_PORT_REGISTER_ADDR       0x01
#define CONFIGURATION_REGISTER_ADDR     0x03

enum LedPort {
    kBotRed,
    kBotGrn,
    kTopRed,
    kTopGrn
};

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
    
    esp_err_t set_led(LedPort led, bool toggle);
    esp_err_t fetch_input_port_register(void);
    esp_err_t set_all_leds_on(void);

    bool get_input_port_register_single_bit(uint8_t bit_pos);
    uint8_t get_input_port_register();
};

io_board::io_board(i2c_port_t i2c_port, uint8_t addr) {
    i2c_port_ = i2c_port;
    addr_ = addr;
    set_configuration_register(0b11110000);
}

esp_err_t io_board::set_led(LedPort led_port, bool toggle) {
    if (toggle) {
        output_port_register_bits_ = output_port_register_bits_ | (1 << led_port);
    } else {
        output_port_register_bits_ = output_port_register_bits_ & ~(1 << led_port);
    }
    return set_output_port_register(output_port_register_bits_);
}

esp_err_t io_board::set_all_leds_on(void) {
    output_port_register_bits_ = output_port_register_bits_ & 0b11110000;
    return set_output_port_register(output_port_register_bits_);
}

bool io_board::get_input_port_register_single_bit(uint8_t bit_pos) {
    return (input_port_register_bit_ >> bit_pos) & 0x01;
}

uint8_t io_board::get_input_port_register(void) {
    return input_port_register_bit_;
}

esp_err_t io_board::fetch_input_port_register(void) {
    uint8_t input_port_register_address = INPUT_PORT_REGISTER_ADDR;
    return i2c_master_write_read_device(
        i2c_port_,
        addr_,
        &input_port_register_address,
        1,
        &input_port_register_bit_,
        1,
        I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS
    );
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
