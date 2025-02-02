#pragma once

#include "app_main.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#define LGFX_M5STACK_CORES3
#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>
static LGFX lcd;
static LGFX_Sprite sprite(&lcd);

static uint8_t VEGA_RED = lcd.color332(0xEE, 0x17, 0x1B);
static uint8_t VEGA_GRN = lcd.color332(0x22, 0xC8, 0x17);
static uint8_t VEGA_GRY = lcd.color332(0xA4, 0xAF, 0xA8);
static uint8_t VEGA_ORG = lcd.color332(0xFF, 0xA1, 0x23);
static uint8_t VEGA_WHT = lcd.color332(0xFF, 0xFF, 0xFF);

static bool hbt_led_status = false;

void draw_static_contents() {
    lcd.init();
    lcd.setRotation(1);
    lcd.setBrightness(128);
    lcd.fillScreen(0xFFFFFFu);

    // Current speed
    lcd.setFont(&fonts::Font7);
    lcd.setTextColor(0x000000u);
    lcd.drawFloat(25.2, 1, 20, 20);
    lcd.setFont(&fonts::Font2);
    lcd.drawString("km/h", 130, 55);

    // Lap and Average speed
    lcd.setFont(&fonts::Font2);
    lcd.drawString("Lap", 170, 30);
    lcd.drawString("Average", 220, 30);
    lcd.setFont(&fonts::Font4);
    lcd.drawString("2/7", 170, 50);
    lcd.drawFloat(29.9, 1, 220, 50);
    lcd.setFont(&fonts::Font2);
    lcd.drawString("km/h", 275, 55);

    // Current time area
    lcd.setFont(&fonts::Font4);
    lcd.drawString("01:23", 20, 82);
    lcd.setColor(VEGA_RED);
    lcd.fillTriangle(25, 112, 75, 112, 50, 122);
    lcd.drawString("04:56", 20, 132);

    lcd.fillRect( 90,  82,   2,  70, VEGA_GRY);
    
    // Time log area
    lcd.setFont(&fonts::Font2);
    lcd.drawString("1 01:23-04:56", 100,  82);
    lcd.drawString("2 01:23-04:56", 205,  82);
    lcd.drawString("3 01:23-04:56", 100,  99);
    lcd.drawString("4 01:23-04:56", 205,  99);
    lcd.drawString("5 01:23-04:56", 100, 118);
    lcd.drawString("6 01:23-04:56", 205, 118);
    lcd.drawString("F 01:23-04:56", 100, 137);
    
    // Communication area
    lcd.fillRect(  20, 168, 180,  52, VEGA_GRY);        // Background
    lcd.setFont(&fonts::Font4);
    lcd.drawString("Dummy", 60, 180);

    // Indicator labels
    lcd.setFont(&fonts::Font2);
    lcd.drawString("HBT", 210, 168);
    lcd.drawString("SEN", 210, 186);
    lcd.drawString("GPS", 210, 204);
    lcd.drawString("ENG", 260, 168);
    lcd.drawString("AAA", 260, 186);
    lcd.drawString("BBB", 260, 204);

    // Indicator LEDs
    lcd.fillCircle( 245, 175, 5, VEGA_GRY);             // HBT
    lcd.fillCircle( 245, 193, 5, VEGA_GRY);             // SEN
    lcd.fillCircle( 245, 211, 5, VEGA_GRY);             // GPS
    lcd.fillCircle( 295, 175, 5, VEGA_GRY);             // ENG
    lcd.fillCircle( 295, 193, 5, VEGA_GRY);
    lcd.fillCircle( 295, 211, 5, VEGA_GRY);
}

void update_display_loop(void *pvParameters)
{
    draw_static_contents();
    
    sprite.setColorDepth(2);
    sprite.createSprite(115, 60);
    
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    while(1) {
        double speed_queue_buff;
        double latitude_queue_buff;
        double longitude_queue_buff;
        // Borders
        switch (*((system_state_t *) pvParameters)) {           
            case STATE_STANDBY:
                lcd.fillRect(   0,   0, 320,  10, VEGA_GRN);        // Upper border
                lcd.fillRect(   0,   0,  10, 240, VEGA_GRN);        // Left border
                lcd.fillRect(   0, 230, 320,  10, VEGA_GRN);        // Bottom border
                lcd.fillRect( 310,   0,  10, 240, VEGA_GRN);        // Right border
                break;

            case STATE_RACING:
                lcd.fillRect(   0,   0, 320,  10, VEGA_RED);        // Upper border
                lcd.fillRect(   0,   0,  10, 240, VEGA_RED);        // Left border
                lcd.fillRect(   0, 230, 320,  10, VEGA_RED);        // Bottom border
                lcd.fillRect( 310,   0,  10, 240, VEGA_RED);        // Right border
                break;

            default:
                lcd.fillRect(   0,   0, 320,  10, VEGA_GRY);        // Upper border
                lcd.fillRect(   0,   0,  10, 240, VEGA_GRY);        // Left border
                lcd.fillRect(   0, 230, 320,  10, VEGA_GRY);        // Bottom border
                lcd.fillRect( 310,   0,  10, 240, VEGA_GRY);        // Right border
                break;
        }
        // Speed
        if (xQueuePeek(speed_queue, &speed_queue_buff, 0)) {
            // Update indicator led
            lcd.fillCircle( 245, 193, 5, VEGA_GRN);
            // Update speed
            sprite.fillScreen(VEGA_WHT);
            sprite.setTextColor(0);
            sprite.setFont(&fonts::Font7);
            sprite.setTextDatum(textdatum_t::top_right);
            sprite.drawFloat(speed_queue_buff, 1, 115, 10);
            sprite.setTextDatum(textdatum_t::top_left);
            sprite.pushSprite(&lcd, 10, 10);
            sprite.clear();
            ESP_LOGI(TAG, "disp_speed:%lf", speed_queue_buff);
        } else {
            lcd.fillCircle( 245, 193, 5, VEGA_GRY);
        }
        // GPS
        xQueuePeek(speed_queue, &latitude_queue_buff, 0);
        xQueuePeek(speed_queue, &longitude_queue_buff, 0);
        if (latitude_queue_buff > 0.f && longitude_queue_buff > 0.f) {
            lcd.fillCircle( 245, 211, 5, VEGA_GRN);             // GPS
        }
        // Heart beat
        if (hbt_led_status) {
            lcd.fillCircle( 245, 175, 5, VEGA_GRY);
        } else {
            lcd.fillCircle( 245, 175, 5, VEGA_GRN);
        }
        hbt_led_status = !hbt_led_status;
        vTaskDelayUntil(&xLastWakeTime, 250 / portTICK_PERIOD_MS);
    }
}