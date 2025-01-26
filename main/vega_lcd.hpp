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

static uint8_t VEGA_RED = lcd.color332(0xEE, 0x17, 0x1B);
static uint8_t VEGA_GRN = lcd.color332(0x22, 0xC8, 0x17);
static uint8_t VEGA_GRY = lcd.color332(0xA4, 0xAF, 0xA8);
static uint8_t VEGA_ORG = lcd.color332(0xFF, 0xA1, 0x23);
static uint8_t VEGA_WHT = lcd.color332(0xFF, 0xFF, 0xFF);

void draw_static_contents()
{
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
    
    // Borders
    lcd.fillRect(   0,   0, 320,  10, VEGA_RED);        // Upper border
    lcd.fillRect(   0,   0,  10, 240, VEGA_RED);        // Left border
    lcd.fillRect(   0, 230, 320,  10, VEGA_RED);        // Bottom border
    lcd.fillRect( 310,   0,  10, 240, VEGA_RED);        // Right border
    
    // Communication area
    lcd.fillRect(  20, 168, 180,  52, VEGA_GRY);        // Background

    // Indicator labels
    lcd.setFont(&fonts::Font2);
    lcd.drawString("HBT", 210, 168);
    lcd.drawString("SEN", 210, 186);
    lcd.drawString("ENG", 210, 204);
    lcd.drawString("LED", 260, 168);
    lcd.drawString("LED", 260, 186);
    lcd.drawString("LED", 260, 204);

    // Indicator LEDs
    lcd.fillCircle( 245, 175, 5, VEGA_GRN);             // HBT
    lcd.fillCircle( 245, 193, 5, VEGA_RED);             // SEN
    lcd.fillCircle( 245, 211, 5, VEGA_ORG);             // ENG
    lcd.fillCircle( 295, 175, 5, VEGA_GRN);
    lcd.fillCircle( 295, 193, 5, VEGA_RED);
    lcd.fillCircle( 295, 211, 5, VEGA_GRY);
}

void update_display_loop(void *pvParameters)
{
    draw_static_contents();
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    while(1) {
        double rx_buff;
        if (xQueueReceive(speed_queue, &rx_buff, 0)) {
            // Update indicator led
            lcd.fillCircle( 245, 193, 5, VEGA_GRN);
            // Update speed
            lcd.setFont(&fonts::Font7);
            lcd.setTextColor(0x000000u);
            lcd.fillRect( 10, 10, 115, 70, VEGA_WHT);
            lcd.setTextDatum(textdatum_t::top_right);
            lcd.drawFloat(rx_buff, 1, 125, 20);
            lcd.setTextDatum(textdatum_t::top_left);
            ESP_LOGI(TAG, "disp_speed:%lf", rx_buff);
        } else {
            lcd.fillCircle( 245, 193, 5, VEGA_GRY);
        }
        vTaskDelayUntil(&xLastWakeTime, 10 / portTICK_PERIOD_MS);
    }
}