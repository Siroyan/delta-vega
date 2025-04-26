#include "spd_mes.hpp"
#include "esp_log.h"

static const char *TAG = "SPD-MES";

double calc_speed_raw(int16_t diff_count, TickType_t diff_tick) {
    double distance_meter = (double)diff_count * WHEEL_CIRCUMFERENCE_METER;
    uint16_t diff_time_ms = diff_tick * portTICK_PERIOD_MS;
    return 3.6f * distance_meter / ((double)diff_time_ms / 1000.f);
}

double calc_speed(count_info* count_info_buff) {
    double speed_km_h_1 = calc_speed_raw(
        count_info_buff[0].count_num - count_info_buff[1].count_num,
        count_info_buff[0].tick_num - count_info_buff[1].tick_num
    );
    double speed_km_h_2 = calc_speed_raw(
        count_info_buff[1].count_num - count_info_buff[2].count_num,
        count_info_buff[1].tick_num - count_info_buff[2].tick_num
    );
    double speed_km_h_3 = calc_speed_raw(
        count_info_buff[2].count_num - count_info_buff[3].count_num,
        count_info_buff[2].tick_num - count_info_buff[3].tick_num
    );
    double speed_km_h_4 = calc_speed_raw(
        count_info_buff[3].count_num - count_info_buff[4].count_num,
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
    pcnt_counter_pause(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_0);
    pcnt_counter_resume(PCNT_UNIT_0);
}

void measure_speed() {
    static count_info count_info_buff[5];
    int16_t temp_count;
    pcnt_get_counter_value(PCNT_UNIT_0, &temp_count);
    // Shift data in buffer.
    count_info_buff[4].count_num = count_info_buff[3].count_num;
    count_info_buff[4].tick_num = count_info_buff[3].tick_num;
    count_info_buff[3].count_num = count_info_buff[2].count_num;
    count_info_buff[3].tick_num = count_info_buff[2].tick_num;
    count_info_buff[2].count_num = count_info_buff[1].count_num;
    count_info_buff[2].tick_num = count_info_buff[1].tick_num;
    count_info_buff[1].count_num = count_info_buff[0].count_num;
    count_info_buff[1].tick_num = count_info_buff[0].tick_num;
    // Update count and time.
    count_info_buff[0].count_num = temp_count;
    count_info_buff[0].tick_num = xTaskGetTickCount();
    
    double tx_buff = calc_speed(count_info_buff);
    xQueueOverwrite(speed_queue, &tx_buff);
}

void update_speed_loop(void *pvParameters) {
    init_pcnt();
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    while(1) {
        measure_speed();
        vTaskDelayUntil(&xLastWakeTime, 1000);  // Use a direct value of 1000 ticks (1 second)
    }
}
