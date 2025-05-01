#pragma once

#include "app_main.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include <lgfx_user/LGFX_ESP32S3_ILI9488.hpp>
static LGFX lcd;
static LGFX_Sprite speed_sprite(&lcd);
static LGFX_Sprite gps_sprite(&lcd);
static LGFX_Sprite arrow_sprite(&lcd);

static const uint16_t LCD_W = 480;
static const uint16_t LCD_H = 320;

static uint8_t VEGA_BLK = lcd.color332(0x00, 0x00, 0x00);
static uint8_t VEGA_RED = lcd.color332(0xEE, 0x17, 0x1B);
static uint8_t VEGA_GRN = lcd.color332(0x22, 0xC8, 0x17);
static uint8_t VEGA_GRY = lcd.color332(0xA4, 0xAF, 0xA8);
static uint8_t VEGA_ORG = lcd.color332(0xFF, 0xA1, 0x23);
static uint8_t VEGA_WHT = lcd.color332(0xFF, 0xFF, 0xFF);

#include "image_data.hpp"
extern const uint16_t arrow_w;
extern const uint16_t arrow_h;
extern const unsigned short arrow_up[];
extern const unsigned short arrow_down[];
extern const unsigned short arrow_stay[];

static bool hbt_led_status = false;

uint32_t indicator_labal_1st_x = 20;
uint32_t indicator_labal_1st_y = 250;

uint8_t lap_num = 0;

void draw_static_contents() {
    lcd.init();
    lcd.setRotation(1);
    lcd.setBrightness(255);
    lcd.fillScreen(0xFFFFFFu);

    // Current speed
    lcd.setTextColor(0x000000u);
    lcd.setFont(&fonts::Font4);
    lcd.drawString("km/h", 218, 75);

    // Lap and Average speed
    lcd.setFont(&fonts::Font4);
    lcd.drawString("Lap", 285, 25);
    lcd.drawString("Ave", 285, 75);
    lcd.setFont(&fonts::Font6);
    lcd.drawString("0   7", 335, 15);
    lcd.drawLine(370, 55, 390, 15, VEGA_BLK);
    lcd.drawLine(369, 55, 389, 15, VEGA_BLK);
    lcd.drawFloat(10.0, 1, 335, 65);
    lcd.setFont(&fonts::Font2);
    lcd.drawString("km/h", 435, 88);

    // Total time area
    lcd.setFont(&fonts::Font2);
    lcd.drawString("TTL Time", 20, 100);
    lcd.setFont(&fonts::Font6);
    lcd.drawString("01:23", 20, 120);
    lcd.drawString("39:20", 166, 120);
    lcd.setColor(VEGA_GRY);
    lcd.fillTriangle(150, 120, 150, 156, 160, 138);
    
    // Lap time area
    lcd.setFont(&fonts::Font2);
    lcd.drawString("LAP Time", 20, 170);
    lcd.setFont(&fonts::Font6);
    lcd.drawString("01:23", 20, 190);
    lcd.drawString("04:56", 166, 190);
    lcd.setColor(VEGA_GRY);
    lcd.fillTriangle(150, 190, 150, 226, 160, 208);
    lcd.fillRect(20, 240, 440, 2, VEGA_GRY);
    
    // Indicator labels
    lcd.setFont(&fonts::Font2);
    lcd.drawString("HBT", indicator_labal_1st_x, indicator_labal_1st_y +  0);
    lcd.drawString("SEN", indicator_labal_1st_x, indicator_labal_1st_y + 18);
    lcd.drawString("GPS", indicator_labal_1st_x, indicator_labal_1st_y + 36);
    lcd.drawString("ENG", indicator_labal_1st_x + 50, indicator_labal_1st_y +  0);
    lcd.drawString("AAA", indicator_labal_1st_x + 50, indicator_labal_1st_y + 18);
    lcd.drawString("BBB", indicator_labal_1st_x + 50, indicator_labal_1st_y + 36);

    // Indicator LEDs
    lcd.fillCircle(indicator_labal_1st_x + 35, indicator_labal_1st_y +  7, 5, VEGA_GRY);             // HBT
    lcd.fillCircle(indicator_labal_1st_x + 35, indicator_labal_1st_y + 25, 5, VEGA_GRY);             // SEN
    lcd.fillCircle(indicator_labal_1st_x + 35, indicator_labal_1st_y + 43, 5, VEGA_GRY);             // GPS
    lcd.fillCircle(indicator_labal_1st_x + 85, indicator_labal_1st_y +  7, 5, VEGA_GRY);             // ENG
    lcd.fillCircle(indicator_labal_1st_x + 85, indicator_labal_1st_y + 25, 5, VEGA_GRY);
    lcd.fillCircle(indicator_labal_1st_x + 85, indicator_labal_1st_y + 43, 5, VEGA_GRY);
}

void draw_outline_border(uint8_t color, uint16_t weight = 10) {
    lcd.fillRect(0, 0, LCD_W, weight, color);        // Upper border
    lcd.fillRect(0, 0, weight, LCD_H, color);        // Left border
    lcd.fillRect(0, LCD_H - weight, LCD_W, weight, color);        // Bottom border
    lcd.fillRect(LCD_W - weight, 0, weight, LCD_H, color);        // Right border
}

void update_display_loop(void *pvParameters) {
    draw_static_contents();
    
    speed_sprite.setColorDepth(2);
    speed_sprite.createSprite(200, 85);

    gps_sprite.setColorDepth(2);
    gps_sprite.createSprite(320, 50);

    arrow_sprite.setBuffer((void*)arrow_down, arrow_w, arrow_h, 16);
    // arrow_sprite.pushSprite(320, 105, TFT_BLACK);
    arrow_sprite.pushRotateZoom(380.f, 170.f, 0.f, 0.8, 0.8, TFT_BLACK);

    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    while(1) {
        double speed_queue_buff;
        double latitude_queue_buff;
        double longitude_queue_buff;
        bool ctrl_sw_queue_buff;
        bool main_sw_queue_buff;
        // Borders
        switch (*((system_state_t *) pvParameters)) {           
            case STATE_STANDBY:
                draw_outline_border(VEGA_GRN);
                break;
            case STATE_RACING:
                draw_outline_border(VEGA_RED);
                break;
            default:
                draw_outline_border(VEGA_GRY);
                break;
        }
        // Speed
        if (xQueuePeek(speed_queue, &speed_queue_buff, 0)) {
            // // Update indicator led
            lcd.fillCircle(indicator_labal_1st_x + 35, indicator_labal_1st_y + 25, 5, VEGA_GRN);
            // Update speed
            lcd.setTextColor(VEGA_BLK, VEGA_WHT);
            lcd.setFont(&fonts::Font8);
            lcd.setTextDatum(textdatum_t::top_right);
            lcd.drawFloat(speed_queue_buff, 1, 210, 20);
            lcd.setTextDatum(textdatum_t::top_left);
            ESP_LOGI(TAG, "disp_speed:%lf", speed_queue_buff);
        } else {
            lcd.fillCircle(indicator_labal_1st_x + 35, indicator_labal_1st_y + 25, 5, VEGA_GRY);
        }
        // GPS
        xQueuePeek(latitude_queue, &latitude_queue_buff, 0);
        xQueuePeek(longitude_queue, &longitude_queue_buff, 0);
        lcd.setTextColor(VEGA_BLK, VEGA_WHT);
        lcd.setFont(&fonts::Font4);
        lcd.drawString("LATI", 125, 250);
        lcd.drawFloat(latitude_queue_buff, 6, 210, 250);
        lcd.drawString("LONG", 125, 282);
        lcd.drawFloat(longitude_queue_buff, 6, 210, 282);

        if (latitude_queue_buff > 0.f && longitude_queue_buff > 0.f) {
            lcd.fillCircle(indicator_labal_1st_x + 35, indicator_labal_1st_y + 43, 5, VEGA_GRN);
        } else {
            lcd.fillCircle(indicator_labal_1st_x + 35, indicator_labal_1st_y + 43, 5, VEGA_GRY);
        }
        // Lap
        if (xQueueReceive(ctrl_sw_queue, &ctrl_sw_queue_buff, 0)) {
            if (lap_num == 7) {
                lap_num = 0;
            } else {
                lap_num++;
            }
            lcd.setFont(&fonts::Font6);
            lcd.setTextColor(VEGA_BLK, VEGA_WHT);
            lcd.setCursor(335, 15);
            lcd.printf("%d", lap_num);
            ESP_LOGI(TAG, "ctrl_sw:%d", ctrl_sw_queue_buff);
        }
        // Heart beat
        if (hbt_led_status) {
            lcd.fillCircle(indicator_labal_1st_x + 35, indicator_labal_1st_y +  7, 5, VEGA_GRY);
        } else {
            lcd.fillCircle(indicator_labal_1st_x + 35, indicator_labal_1st_y +  7, 5, VEGA_GRN);
        }
        // ENG

        hbt_led_status = !hbt_led_status;
        vTaskDelayUntil(&xLastWakeTime, 250 / portTICK_PERIOD_MS);
    }
}