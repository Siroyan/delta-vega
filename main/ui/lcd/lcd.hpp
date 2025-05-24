#pragma once
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include <lgfx_user/LGFX_ESP32S3_ILI9488.hpp>
#include "image_data.hpp"
class lcd {
private:
    LGFX display;
    LGFX_Sprite arrow_sprite(&display);

    static const uint16_t LCD_W = 480;
    static const uint16_t LCD_H = 320;

    uint8_t VEGA_BLK = display.color332(0x00, 0x00, 0x00);
    uint8_t VEGA_RED = display.color332(0xEE, 0x17, 0x1B);
    uint8_t VEGA_GRN = display.color332(0x22, 0xC8, 0x17);
    uint8_t VEGA_GRY = display.color332(0xA4, 0xAF, 0xA8);
    uint8_t VEGA_ORG = display.color332(0xFF, 0xA1, 0x23);
    uint8_t VEGA_WHT = display.color332(0xFF, 0xFF, 0xFF);
    
    bool hbt_led_status = false;
    
    uint32_t indicator_labal_1st_x = 20;
    uint32_t indicator_labal_1st_y = 250;
    
    uint8_t lap_num = 0;

    void draw_static_contents();
    void draw_outline_border(uint8_t color, uint16_t weight);
public:
    lcd();
    ~lcd();

    // Setter
    void set_speed(double speed_value);             // Main speed
    void set_ave_speed(double speed_value);         // Average speed
    void set_gps_lati(double lati_value);           // GPS latitude value
    void set_gps_long(double long_value);           // GPS longitude value
    void set_lap_num(uint8_t num);                  // Lap number
    void set_ttl_time_now(uint16_t time_value);     // Now time on total time row
    void set_ttl_time_tgt(uint16_t time_value);     // Target time on total time row
    void set_lap_time_now(uint16_t time_value);     // Now time on lap time row
    void set_lap_time_tgt(uint16_t time_value);     // Target time on lap time row
    void set_arrow(uint8_t arrow_type);             // Pace arrow
    void set_boarder(uint8_t color);                // Boarder

    // TODO (Remove)
    void update_display_loop(void *pvParameters);
};
