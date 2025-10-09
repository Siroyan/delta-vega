#pragma once
#include <stdio.h>

const uint8_t UI_MANAGER_LOOP_TIME_MS = 50;

// ログデータ構造体（ui_managerからのデータ取得用）
typedef struct {
    double average_speed_kmh;
    uint8_t lap_number;
    uint32_t total_time_ms;
    uint32_t lap_time_ms;
    bool timer_started;
} ui_manager_log_data_t;

void ui_manager_loop(void *pvParameters);

// ログデータ取得関数
void get_ui_manager_log_data(ui_manager_log_data_t* data);