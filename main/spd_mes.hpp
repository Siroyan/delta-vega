#pragma once

#include "app_main.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "driver/pcnt.h"

#define TIRE_PULSE_PIN GPIO_NUM_18

const double WHEEL_CIRCUMFERENCE_METER = 0.65;
uint32_t true_pulse_num = 0;
typedef struct count {
    uint32_t true_pulse_num;
    TickType_t tick_num;
} count_info;

double calc_speed_raw(int16_t diff_count, TickType_t diff_tick) {
    double distance_meter = (double)diff_count * WHEEL_CIRCUMFERENCE_METER;
    uint16_t diff_time_ms = diff_tick * portTICK_PERIOD_MS;     // Covert to real time.
    return 3.6f * distance_meter / ((double)diff_time_ms / 1000.f);
}

double calc_speed(count_info* count_info_buff) {
    double speed_km_h_1 = calc_speed_raw(
        count_info_buff[0].true_pulse_num - count_info_buff[1].true_pulse_num,
        count_info_buff[0].tick_num - count_info_buff[1].tick_num
    );
    double speed_km_h_2 = calc_speed_raw(
        count_info_buff[1].true_pulse_num - count_info_buff[2].true_pulse_num,
        count_info_buff[1].tick_num - count_info_buff[2].tick_num
    );
    double speed_km_h_3 = calc_speed_raw(
        count_info_buff[2].true_pulse_num - count_info_buff[3].true_pulse_num,
        count_info_buff[2].tick_num - count_info_buff[3].tick_num
    );
    double speed_km_h_4 = calc_speed_raw(
        count_info_buff[3].true_pulse_num - count_info_buff[4].true_pulse_num,
        count_info_buff[3].tick_num - count_info_buff[4].tick_num
    );
    return (speed_km_h_1 + speed_km_h_2 + speed_km_h_3 + speed_km_h_4 ) / 4.f;
}

void init_pcnt() {
    pcnt_config_t pcnt_config = {};
    pcnt_config.pulse_gpio_num  = TIRE_PULSE_PIN;
    pcnt_config.ctrl_gpio_num   = PCNT_PIN_NOT_USED;
    pcnt_config.lctrl_mode      = PCNT_MODE_KEEP;
    pcnt_config.hctrl_mode      = PCNT_MODE_REVERSE;
    pcnt_config.pos_mode        = PCNT_COUNT_DIS;
    pcnt_config.neg_mode        = PCNT_COUNT_INC;
    pcnt_config.counter_h_lim   = 32767;
    pcnt_config.counter_l_lim   = -32768;
    pcnt_config.unit            = PCNT_UNIT_0;
    pcnt_config.channel         = PCNT_CHANNEL_0;

    pcnt_unit_config(&pcnt_config);
    
    pcnt_set_filter_value(PCNT_UNIT_0, 1023);
    pcnt_filter_enable(PCNT_UNIT_0);
    
    pcnt_counter_pause(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_0);
    pcnt_counter_resume(PCNT_UNIT_0);
}

void count_true_pulse() {
    static int16_t prev_raw_pulse_count = 0;        // Raw pulse count at previous loop.
    static int16_t curr_raw_pulse_count = 0;        // Raw pulse count at current loop.
    pcnt_get_counter_value(PCNT_UNIT_0, &curr_raw_pulse_count);
    if (curr_raw_pulse_count - prev_raw_pulse_count) {
        true_pulse_num++;
    }
    // ESP_LOGI(TAG, "\t\t\t %lu \t %d \t %d", true_pulse_num, prev_raw_pulse_count, curr_raw_pulse_count);
    prev_raw_pulse_count = curr_raw_pulse_count;
}

void measure_speed() {
    static count_info count_info_buff[5];
    // Shift data in buffer.
    count_info_buff[4].true_pulse_num = count_info_buff[3].true_pulse_num;
    count_info_buff[4].tick_num = count_info_buff[3].tick_num;
    count_info_buff[3].true_pulse_num = count_info_buff[2].true_pulse_num;
    count_info_buff[3].tick_num = count_info_buff[2].tick_num;
    count_info_buff[2].true_pulse_num = count_info_buff[1].true_pulse_num;
    count_info_buff[2].tick_num = count_info_buff[1].tick_num;
    count_info_buff[1].true_pulse_num = count_info_buff[0].true_pulse_num;
    count_info_buff[1].tick_num = count_info_buff[0].tick_num;
    // Update count and time.
    count_info_buff[0].true_pulse_num = true_pulse_num;
    count_info_buff[0].tick_num = xTaskGetTickCount();
    
    double tx_buff = calc_speed(count_info_buff);
    xQueueOverwrite(speed_queue, &tx_buff);
}

void send_pulse_input() {
    static uint32_t prev_true_pulse_num = 0;
    if(true_pulse_num > prev_true_pulse_num) {
        bool tx_buff = true;
        xQueueOverwrite(speed_pulse_queue, &tx_buff); 
    } else {
        bool tx_buff = false;
        xQueueOverwrite(speed_pulse_queue, &tx_buff);
    }
    prev_true_pulse_num = true_pulse_num;
}

void update_speed_loop(void *pvParameters) {
    init_pcnt();
    uint8_t loop_cnt = 0;
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    while(1) {
        count_true_pulse();
        if (loop_cnt % 10 == 0) {
            send_pulse_input();
        }
        if (loop_cnt % 50 == 0) {
            measure_speed();
        }
        if (loop_cnt < 99) {
            loop_cnt++;
        } else {
            loop_cnt = 0;
        }
        vTaskDelayUntil(&xLastWakeTime, 20 / portTICK_PERIOD_MS);
    }
}