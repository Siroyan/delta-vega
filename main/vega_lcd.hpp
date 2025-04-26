#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#define LGFX_M5STACK_CORES3
#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>

typedef enum {
    STATE_INITIALIZATION,  // 初期化状態
    STATE_STANDBY,         // スタンバイ状態
    STATE_RACING           // レース中状態
} system_state_t;

extern QueueHandle_t speed_queue;
extern QueueHandle_t latitude_queue;
extern QueueHandle_t longitude_queue;

// Function declarations
void draw_static_contents();
void update_display_loop(void *pvParameters);