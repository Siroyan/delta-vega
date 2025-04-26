#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/pcnt.h"
#include "driver/gpio.h"

extern QueueHandle_t speed_queue;

#define TIRE_PULSE_PIN GPIO_NUM_8

const double WHEEL_CIRCUMFERENCE_METER = 0.65;

typedef struct count {
    int16_t count_num;
    TickType_t tick_num;
} count_info;

double calc_speed_raw(int16_t diff_count, TickType_t diff_tick);
double calc_speed(count_info* count_info_buff);
void init_pcnt();
void measure_speed();
void update_speed_loop(void *pvParameters);