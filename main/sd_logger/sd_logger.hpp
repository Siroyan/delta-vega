#pragma once
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// SDカード設定
#define MOUNT_POINT "/sdcard"
#define SPI_DMA_CHAN SPI_DMA_CH_AUTO
#define PIN_NUM_MISO GPIO_NUM_4
#define PIN_NUM_MOSI GPIO_NUM_6
#define PIN_NUM_CLK  GPIO_NUM_5
#define PIN_NUM_CS   GPIO_NUM_40

// ログデータ構造体
typedef struct {
    double speed_kmh;           // 現在速度 (km/h)
    double average_speed_kmh;   // 平均速度 (km/h) 
    uint8_t lap_number;         // ラップ数
    uint32_t total_time_ms;     // 合計時間 (ms)
    uint32_t lap_time_ms;       // ラップ時間 (ms)
    double latitude;            // 緯度
    double longitude;           // 経度
    uint32_t timestamp_ms;      // タイムスタンプ (ms)
} sd_log_data_t;

class SDLogger {
private:
    FILE* log_file;
    bool is_initialized;
    bool is_logging;
    char filename[64];

public:
    SDLogger();
    ~SDLogger();
    
    // SDカード初期化
    esp_err_t initialize();
    
    // ログファイル作成とヘッダー書き込み
    esp_err_t create_log_file();
    
    // データログ
    esp_err_t log_data(const sd_log_data_t* data);
    
    // ログ開始・停止
    esp_err_t start_logging();
    esp_err_t stop_logging();
    
    // ファイルフラッシュ
    esp_err_t flush();
    
    // 状態取得
    bool is_ready() const { return is_initialized; }
    bool is_logging_active() const { return is_logging; }
    
    // SDカード終了処理
    esp_err_t deinitialize();
};

// SDカードロガータスク
void sd_logger_task(void *pvParameters);