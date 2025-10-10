#pragma once
#include <stdint.h>

// MQTT送信用のデータ構造体
typedef struct {
    double speed_kmh;           // 現在速度 (km/h)
    double average_speed_kmh;   // 平均速度 (km/h)
    uint8_t lap_number;         // ラップ数
    uint32_t total_time_ms;     // 合計時間 (ms)
    uint32_t lap_time_ms;       // ラップ時間 (ms)
    double latitude;            // 緯度
    double longitude;           // 経度
    uint32_t timestamp_ms;      // タイムスタンプ (ms)
} mqtt_telemetry_data_t;

void update_mqtt_loop(void *pvParameters);
void mqtt_cleanup();