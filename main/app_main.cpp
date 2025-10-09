#include "app_main.hpp"
#include "spd/spd_mes.hpp"
#include "mqtt/vega_mqtt.hpp"
#include "gps/vega_gps.hpp"
#include "ui/ui_manager.hpp"
#include "ui/lcd/lcd.hpp"
#include "sd_logger/sd_logger.hpp"

system_state_t current_state = STATE_INITIALIZATION;

TaskHandle_t mqtt_task_handle;
TaskHandle_t gps_task_handle;
TaskHandle_t speed_task_handle;
TaskHandle_t display_task_handle;
TaskHandle_t ui_manager_task_handle;
TaskHandle_t sd_logger_task_handle;

QueueHandle_t speed_queue = xQueueCreate(1, sizeof(double));
QueueHandle_t speed_pulse_queue = xQueueCreate(1, sizeof(bool));
QueueHandle_t latitude_queue = xQueueCreate(1, sizeof(double));
QueueHandle_t longitude_queue = xQueueCreate(1, sizeof(double));
QueueHandle_t ctrl_sw_queue = xQueueCreate(1, sizeof(bool));
QueueHandle_t main_sw_queue = xQueueCreate(1, sizeof(bool));
QueueHandle_t state_transition_queue = xQueueCreate(1, sizeof(state_transition_msg_t));

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // ESP_ERROR_CHECK(example_connect());

    while (1) {
        switch (current_state) {
            case STATE_INITIALIZATION:
                xTaskCreatePinnedToCore(ui_manager_loop, "ui_manager_loop", 8192, NULL, 1, &ui_manager_task_handle, APP_CPU_NUM);
                // xTaskCreatePinnedToCore(update_display_loop, "update_display_loop", 8192, &current_state, 1, &display_task_handle, APP_CPU_NUM);
                xTaskCreatePinnedToCore(update_speed_loop, "update_speed_loop", 8192, NULL, 1, &speed_task_handle, APP_CPU_NUM);
                xTaskCreatePinnedToCore(update_gps_loop, "update_gps_loop", 8192, NULL, 1, &gps_task_handle, APP_CPU_NUM);
                xTaskCreatePinnedToCore(sd_logger_task, "sd_logger_task", 12288, NULL, 1, &sd_logger_task_handle, APP_CPU_NUM);
                current_state = STATE_STANDBY;
                break;
                
            case STATE_STANDBY:
                ESP_LOGI(TAG, "STATE_STANDBY");
                // 状態遷移通知をチェック
                break;
                
            case STATE_RACING:
                ESP_LOGI(TAG, "STATE_RACING");
                // MQTTタスクを初回のみ作成
                if (mqtt_task_handle == NULL) {
                    // xTaskCreatePinnedToCore(update_mqtt_loop, "update_mqtt_loop", 8192, NULL, 1, &mqtt_task_handle, APP_CPU_NUM);
                    ESP_LOGI(TAG, "MQTT task started");
                }
                break;
                
            default:
                break;
        }
        
        // 状態遷移メッセージの監視
        state_transition_msg_t transition_msg;
        if (xQueueReceive(state_transition_queue, &transition_msg, 100 / portTICK_PERIOD_MS) == pdTRUE) {
            switch (transition_msg) {
                case STATE_TRANSITION_TO_RACING:
                    if (current_state == STATE_STANDBY) {
                        current_state = STATE_RACING;
                        ESP_LOGI(TAG, "State transition: STANDBY -> RACING");
                    }
                    break;
                    
                case STATE_TRANSITION_TO_STANDBY:
                    if (current_state == STATE_RACING) {
                        if (mqtt_task_handle != NULL) {
                            vTaskDelete(mqtt_task_handle);
                            mqtt_task_handle = NULL;
                            ESP_LOGI(TAG, "MQTT task stopped");
                        }
                        current_state = STATE_STANDBY;
                        ESP_LOGI(TAG, "State transition: RACING -> STANDBY");
                    }
                    break;
                    
                default:
                    break;
            }
        }
    }
}