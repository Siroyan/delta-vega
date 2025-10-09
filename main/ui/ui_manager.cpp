#include "ui_manager.hpp"
#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "../app_main.hpp"
#include "io_board/io_board.hpp"
#include "lcd/lcd.hpp"

#define I2C_MASTER_SCL_IO           10                          /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           9                           /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_FREQ_HZ          100000                      /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */

const i2c_port_t IO_BOARDS_I2C_PORT = I2C_NUM_1;

lcd display;

static esp_err_t i2c_master_init(void) {
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;

    i2c_param_config(IO_BOARDS_I2C_PORT, &conf);

    return i2c_driver_install(IO_BOARDS_I2C_PORT, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

static void set_spd_data() {
    double spd_queue_buff = 0;
    bool spd_pulse_queue_buff;
    xQueuePeek(speed_queue, &spd_queue_buff, 0);
    xQueuePeek(speed_pulse_queue, &spd_pulse_queue_buff, 0);
    display.set_speed(spd_queue_buff);
    // Update indicator led
    // ESP_LOGI(TAG, "speed_pulse_queue_buff:%lf", spd_queue_buff);
    if (spd_pulse_queue_buff) {
        display.set_indicator(1, true);
    } else {
        display.set_indicator(1, false);
    }
}

static void set_gps_data() {
    double lati_queue_buff;
    double long_queue_buff;
    xQueuePeek(latitude_queue, &lati_queue_buff, 0);
    xQueuePeek(longitude_queue, &long_queue_buff, 0);
    // ESP_LOGI(TAG, "(lati, long):(%lf, %lf)", lati_queue_buff, long_queue_buff);
    if (lati_queue_buff > 0.f && long_queue_buff > 0.f) {
        display.set_indicator(2, true);
        display.set_gps_lati(lati_queue_buff);
        display.set_gps_long(long_queue_buff);
    } else {
        display.set_indicator(2, false);
    }
}

static void set_outline_border_by_state() {
    // 現在のシステム状態に応じて外枠色を変更
    switch (current_state) {
        case STATE_RACING:
            display.set_outline_border(display.VEGA_ORG, 10);  // RACING状態はオレンジ
            break;
        case STATE_STANDBY:
            display.set_outline_border(display.VEGA_GRN, 10);  // STANDBY状態は緑
            break;
        case STATE_INITIALIZATION:
        default:
            display.set_outline_border(display.VEGA_GRY, 10);  // 初期化状態はグレー
            break;
    }
}

static void update_stopwatch_timers(uint32_t *total_time_ms, uint32_t *lap_time_ms, uint8_t elapsed_ms) {
    // RACING状態の判定とタイマーの更新
    if (current_state == STATE_RACING) {
        // 実際の経過時間でタイマーを更新
        *total_time_ms += elapsed_ms;
        *lap_time_ms += elapsed_ms;
    } else {
        // RACING状態以外では必ずタイマーをクリア
        *total_time_ms = 0;
        *lap_time_ms = 0;
    }
}

static void update_time_display(uint32_t total_time_ms, uint32_t lap_time_ms) {
    // 時間を秒単位に変換して表示
    uint16_t total_time_sec = total_time_ms / 1000;
    uint16_t lap_time_sec = lap_time_ms / 1000;
    
    display.set_ttl_time_now(total_time_sec);  // 総経過時間を表示
    display.set_lap_time_now(lap_time_sec);    // ラップ時間を表示
}

void ui_manager_loop(void *pvParameters) {
    display.initialize();
    // one time process
    i2c_master_init();
    vTaskDelay(500 / portTICK_PERIOD_MS);
    io_board io_board_l(IO_BOARDS_I2C_PORT, 0x20);
    io_board io_board_r(IO_BOARDS_I2C_PORT, 0x21);
    
    // ボタン状態管理用のローカル変数
    uint32_t button_press_count = 0;         // ボタン押下回数カウンター
    uint32_t button_long_press_timer = 0;    // 長押し判定用タイマー
    bool button_pressed_flag = false;        // ボタン押下状態フラグ
    bool prev_button_state = false;          // 前回のボタン状態
    
    // ストップウォッチ機能用の変数
    uint32_t total_time_ms = 0;              // 総経過時間（ms）
    uint32_t lap_time_ms = 0;                // 現在のラップ時間（ms）
    
    // loop
    uint16_t hbt_led_timer = 0;
    bool hbt_led_status = false;
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    while(1) {
        // ストップウォッチ機能の更新
        update_stopwatch_timers(&total_time_ms, &lap_time_ms, UI_MANAGER_LOOP_TIME_MS);
        // 時間表示の更新（常に行い、状態に応じて適切な値を表示）
        update_time_display(total_time_ms, lap_time_ms);

        ESP_ERROR_CHECK(io_board_l.fetch_input_port_register());
        ESP_ERROR_CHECK(io_board_r.fetch_input_port_register());
        
        // ボタン状態の取得（Port5）
        bool current_button_state = !io_board_l.get_input_port_register_single_bit(5);
                
        // ボタン押下状態のLED表示
        ESP_ERROR_CHECK(io_board_l.set_bot_red(!current_button_state));
        
        // ボタン状態変化検出とカウンター更新
        if (current_button_state && !prev_button_state) {
            // ボタンが押された瞬間（エッジ検出）
            button_press_count++;
            button_pressed_flag = true;
            
            // RACING状態中はラップ時間をリセット
            if (current_state == STATE_RACING) {
                lap_time_ms = 0;
                ESP_LOGI(TAG, "Lap time reset! Button press count: %ld", button_press_count);
            } else {
                ESP_LOGI(TAG, "Button pressed! Count: %ld, setting pressed_flag=true", button_press_count);
            }
        } else if (!current_button_state && prev_button_state) {
            // ボタンが離された瞬間
            button_pressed_flag = false;
            button_long_press_timer = 0;  // 長押しタイマーをリセット
            ESP_LOGI(TAG, "Button released! setting pressed_flag=false, timer reset");
        }
        
        // 長押し判定（ボタンが押され続けている時間をカウント）
        if (button_pressed_flag && current_button_state) {
            button_long_press_timer += UI_MANAGER_LOOP_TIME_MS;
            
            // 状態に応じて長押し処理を分岐
            if (button_long_press_timer >= 1000) {
                if (current_state == STATE_STANDBY) {
                    // STANDBY状態: RACING状態への遷移を通知
                    state_transition_msg_t msg = STATE_TRANSITION_TO_RACING;
                    if (xQueueSend(state_transition_queue, &msg, 0) == pdTRUE) {
                        ESP_LOGI(TAG, "Long press in STANDBY - requesting transition to RACING (timer=%ldms)", button_long_press_timer);
                    }
                } else if (current_state == STATE_RACING) {
                    // RACING状態: 全データをクリアしてSTANDBY状態への遷移を通知
                    total_time_ms = 0;
                    lap_time_ms = 0;
                    state_transition_msg_t msg = STATE_TRANSITION_TO_STANDBY;
                    if (xQueueSend(state_transition_queue, &msg, 0) == pdTRUE) {
                        ESP_LOGI(TAG, "Long press in RACING - clearing all timer data and requesting transition to STANDBY (timer=%ldms)", button_long_press_timer);
                    }
                }
                button_press_count = 0;
                button_long_press_timer = 0;  // 重複送信を防ぐ
                button_pressed_flag = false;
                ESP_LOGI(TAG, "Reset: press_count=0, pressed_flag=false, timer=0 after long press");
            }
        } else if (button_pressed_flag && !current_button_state) {
            // pressed_flagが立っているのにボタンが押されていない異常状態
            ESP_LOGW(TAG, "WARNING: pressed_flag=true but button not pressed! Resetting flags.");
            button_pressed_flag = false;
            button_long_press_timer = 0;
        }
        
        // 8回押下でSTANDBY状態への遷移を通知
        if (button_press_count >= 8) {
            state_transition_msg_t msg = STATE_TRANSITION_TO_STANDBY;
            if (xQueueSend(state_transition_queue, &msg, 0) == pdTRUE) {
                ESP_LOGI(TAG, "8 button presses detected - requesting transition to STANDBY");
            }
            button_press_count = 0;  // リセット
        }
        
        // ボタン状態をキューに送信
        bool tx_buff = current_button_state;
        xQueueOverwrite(ctrl_sw_queue, &tx_buff);
        
        // 前回のボタン状態を保存
        prev_button_state = current_button_state;
        ESP_ERROR_CHECK(
            io_board_r.set_bot_red(io_board_r.get_input_port_register_single_bit(5))
        );
        
        set_outline_border_by_state();  // 状態に応じた外枠色設定
        
        set_spd_data();
        set_gps_data();

        if (hbt_led_timer >= 500) {
            hbt_led_timer = 0;
            hbt_led_status = !hbt_led_status;
            display.set_indicator(0, hbt_led_status);
            ESP_ERROR_CHECK(io_board_l.set_top_grn(hbt_led_status));
        }
        hbt_led_timer += UI_MANAGER_LOOP_TIME_MS;
        
        // 現在の時刻を保存し、次のループまで待機
        vTaskDelayUntil(&xLastWakeTime, UI_MANAGER_LOOP_TIME_MS / portTICK_PERIOD_MS);
    }
}