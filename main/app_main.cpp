#include "app_main.hpp"

system_state_t current_state = STATE_INITIALIZATION;

TaskHandle_t mqtt_task_handle;
TaskHandle_t gps_task_handle;
TaskHandle_t speed_task_handle;
TaskHandle_t display_task_handle;

QueueHandle_t speed_queue = xQueueCreate(1, sizeof(double));
QueueHandle_t latitude_queue = xQueueCreate(1, sizeof(double));
QueueHandle_t longitude_queue = xQueueCreate(1, sizeof(double));

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

    ESP_ERROR_CHECK(example_connect());

    while (1) {
        switch (current_state) {
            case STATE_INITIALIZATION:
                xTaskCreatePinnedToCore(update_display_loop, "update_display_loop", 8192, &current_state, 1, &display_task_handle, APP_CPU_NUM);
                xTaskCreatePinnedToCore(update_speed_loop, "update_speed_loop", 8192, NULL, 1, &speed_task_handle, APP_CPU_NUM);
                xTaskCreatePinnedToCore(update_gps_loop, "update_gps_loop", 8192, NULL, 1, &gps_task_handle, APP_CPU_NUM);
                current_state = STATE_STANDBY;
                break;
                
            case STATE_STANDBY:
                if (false) {
                    current_state = STATE_RACING;
                }
                break;
                
            case STATE_RACING:
                xTaskCreatePinnedToCore(update_mqtt_loop, "update_mqtt_loop", 8192, NULL, 1, &mqtt_task_handle, APP_CPU_NUM);
                if (true) {
                    vTaskSuspend(mqtt_task_handle);
                    current_state = STATE_STANDBY;
                }
                break;
                
            default:
                break;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}