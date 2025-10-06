#include "ui_manager.hpp"
#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "../app_main.hpp"
#include "io_board/io_board.hpp"
#include "lcd/lcd.hpp"

#define I2C_MASTER_SCL_IO           10                          /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           9                           /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_FREQ_HZ          100000                      /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */

const i2c_port_t IO_BOARDS_I2C_PORT = I2C_NUM_1;

lcd display;

static esp_err_t i2c_master_init(void) {
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;

    i2c_param_config(IO_BOARDS_I2C_PORT, &conf);

    return i2c_driver_install(IO_BOARDS_I2C_PORT, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

static void set_spd_data() {
    double spd_queue_buff = 0;
    bool spd_pulse_queue_buff;
    xQueuePeek(speed_queue, &spd_queue_buff, 0);
    xQueuePeek(speed_pulse_queue, &spd_pulse_queue_buff, 0);
    display.set_speed(spd_queue_buff);
    // Update indicator led
    // ESP_LOGI(TAG, "speed_pulse_queue_buff:%lf", spd_queue_buff);
    if (spd_pulse_queue_buff) {
        display.set_indicator(1, true);
    } else {
        display.set_indicator(1, false);
    }
}

static void set_gps_data() {
    double lati_queue_buff;
    double long_queue_buff;
    xQueuePeek(latitude_queue, &lati_queue_buff, 0);
    xQueuePeek(longitude_queue, &long_queue_buff, 0);
    // ESP_LOGI(TAG, "(lati, long):(%lf, %lf)", lati_queue_buff, long_queue_buff);
    if (lati_queue_buff > 0.f && long_queue_buff > 0.f) {
        display.set_indicator(2, true);
        display.set_gps_lati(lati_queue_buff);
        display.set_gps_long(long_queue_buff);
    } else {
        display.set_indicator(2, false);
    }
}

static void set_outline_boarder() {
    display.set_outline_border(display.VEGA_GRN, 10);
}

void ui_manager_loop(void *pvParameters) {
    display.initialize();
    // one time process
    i2c_master_init();
    vTaskDelay(500 / portTICK_PERIOD_MS);
    io_board io_board_l(IO_BOARDS_I2C_PORT, 0x20);
    io_board io_board_r(IO_BOARDS_I2C_PORT, 0x21);
    // loop
    uint8_t hbt_led_timer = 0;
    bool hbt_led_status = false;
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    while(1) {
        ESP_ERROR_CHECK(io_board_l.fetch_input_port_register());
        ESP_ERROR_CHECK(io_board_r.fetch_input_port_register());
        if (io_board_l.get_input_port_register_single_bit(5)) {
            ESP_ERROR_CHECK(io_board_l.set_bot_red(true));
        } else {
            ESP_ERROR_CHECK(io_board_l.set_bot_red(false));
            bool tx_buff = true;
            xQueueOverwrite(ctrl_sw_queue, &tx_buff); 
        }
        ESP_ERROR_CHECK(
            io_board_r.set_bot_red(io_board_r.get_input_port_register_single_bit(5))
        );
        
        set_outline_boarder();
        set_spd_data();
        set_gps_data();

        if (hbt_led_timer == 50) {
            hbt_led_timer = 0;
            hbt_led_status = !hbt_led_status;
            display.set_indicator(0, hbt_led_status);
            ESP_ERROR_CHECK(io_board_l.set_top_grn(hbt_led_status));
        }
        hbt_led_timer++;
        vTaskDelayUntil(&xLastWakeTime, 10 / portTICK_PERIOD_MS);
    }
}