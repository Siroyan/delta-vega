#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#define LGFX_M5STACK_CORES3
#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>
static LGFX lcd;

void update_display_loop(void *pvParameters)
{
    lcd.init();
    lcd.setRotation(1);
    lcd.setBrightness(128);
    lcd.fillScreen(0xFFFFFFu);

    uint16_t unko = 0;
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    while(1)
    {
        vTaskDelayUntil(&xLastWakeTime, 1000 / portTICK_PERIOD_MS);
        lcd.drawNumber(unko, 10, 10);
        unko++;
    }
}