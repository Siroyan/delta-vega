#include "sd_logger.hpp"
#include "../app_main.hpp"
#include "../ui/ui_manager.hpp"
#include <time.h>

SDLogger::SDLogger() : log_file(nullptr), is_initialized(false), is_logging(false) {
    memset(filename, 0, sizeof(filename));
}

SDLogger::~SDLogger() {
    stop_logging();
    deinitialize();
}

esp_err_t SDLogger::initialize() {
    esp_err_t ret;
    
    // SDカードのSPIバス設定
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");
    
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;
    
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    
    ret = spi_bus_initialize((spi_host_device_t)host.slot, &bus_cfg, SPI_DMA_CHAN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return ret;
    }
    
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = (spi_host_device_t)host.slot;
    
    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);
    
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                          "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                          "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return ret;
    }
    ESP_LOGI(TAG, "Filesystem mounted");
    
    // カード情報表示
    sdmmc_card_print_info(stdout, card);
    
    is_initialized = true;
    return ESP_OK;
}

esp_err_t SDLogger::create_log_file() {
    if (!is_initialized) {
        ESP_LOGE(TAG, "SD card not initialized");
        return ESP_FAIL;
    }
    
    // タイムスタンプベースのファイル名生成
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    snprintf(filename, sizeof(filename), MOUNT_POINT"/racing_log_%02d%02d_%02d%02d%02d.csv",
             timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    
    log_file = fopen(filename, "w");
    if (log_file == nullptr) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", filename);
        return ESP_FAIL;
    }
    
    // CSVヘッダー書き込み
    fprintf(log_file, "timestamp_ms,speed_kmh,average_speed_kmh,lap_number,total_time_ms,lap_time_ms,latitude,longitude\n");
    fflush(log_file);
    
    ESP_LOGI(TAG, "Log file created: %s", filename);
    return ESP_OK;
}

esp_err_t SDLogger::start_logging() {
    if (!is_initialized) {
        ESP_LOGE(TAG, "SD card not initialized");
        return ESP_FAIL;
    }
    
    if (log_file == nullptr) {
        esp_err_t ret = create_log_file();
        if (ret != ESP_OK) {
            return ret;
        }
    }
    
    is_logging = true;
    ESP_LOGI(TAG, "Logging started");
    return ESP_OK;
}

esp_err_t SDLogger::stop_logging() {
    if (is_logging && log_file != nullptr) {
        fflush(log_file);
        fclose(log_file);
        log_file = nullptr;
        is_logging = false;
        ESP_LOGI(TAG, "Logging stopped, file saved: %s", filename);
    }
    return ESP_OK;
}

esp_err_t SDLogger::log_data(const sd_log_data_t* data) {
    if (!is_logging || log_file == nullptr) {
        return ESP_FAIL;
    }
    
    // CSVフォーマットでデータ書き込み
    fprintf(log_file, "%lu,%.1f,%.1f,%d,%lu,%lu,%.6f,%.6f\n",
            (unsigned long)data->timestamp_ms,
            data->speed_kmh,
            data->average_speed_kmh,
            data->lap_number,
            (unsigned long)data->total_time_ms,
            (unsigned long)data->lap_time_ms,
            data->latitude,
            data->longitude);
    
    return ESP_OK;
}

esp_err_t SDLogger::flush() {
    if (log_file != nullptr) {
        fflush(log_file);
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t SDLogger::deinitialize() {
    if (is_initialized) {
        stop_logging();
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, nullptr);
        ESP_LOGI(TAG, "SD card unmounted");
        is_initialized = false;
    }
    return ESP_OK;
}

// グローバルSDロガーインスタンス
static SDLogger sd_logger;

void sd_logger_task(void *pvParameters) {
    ESP_LOGI(TAG, "SD Logger task started");
    
    // SDカード初期化
    esp_err_t ret = sd_logger.initialize();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD card initialization failed: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }
    
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    
    bool logging_started = false;
    
    while (1) {
        // RACING状態でタイマーが開始されているときのみログ
        if (current_state == STATE_RACING) {
            if (!logging_started) {
                ret = sd_logger.start_logging();
                if (ret == ESP_OK) {
                    logging_started = true;
                } else {
                    ESP_LOGE(TAG, "Failed to start logging");
                }
            }
            
            if (logging_started) {
                // データ収集
                sd_log_data_t log_data = {0};
                
                // 現在時刻取得
                log_data.timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
                
                // 各キューからデータ取得
                xQueuePeek(speed_queue, &log_data.speed_kmh, 0);
                xQueuePeek(latitude_queue, &log_data.latitude, 0);
                xQueuePeek(longitude_queue, &log_data.longitude, 0);
                
                // ui_managerからデータ取得
                ui_manager_log_data_t ui_data;
                get_ui_manager_log_data(&ui_data);
                
                log_data.average_speed_kmh = ui_data.average_speed_kmh;
                log_data.lap_number = ui_data.lap_number;
                log_data.total_time_ms = ui_data.total_time_ms;
                log_data.lap_time_ms = ui_data.lap_time_ms;
                
                // ログ書き込み
                ret = sd_logger.log_data(&log_data);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to log data");
                }
                
                // 定期的にフラッシュ（1秒ごと）
                static int flush_counter = 0;
                if (++flush_counter >= 10) {  // 100ms * 10 = 1秒
                    sd_logger.flush();
                    flush_counter = 0;
                }
            }
        } else {
            // RACING状態以外ではログ停止
            if (logging_started) {
                sd_logger.stop_logging();
                logging_started = false;
            }
        }
        
        // 100ms間隔で実行
        vTaskDelayUntil(&xLastWakeTime, 100 / portTICK_PERIOD_MS);
    }
}