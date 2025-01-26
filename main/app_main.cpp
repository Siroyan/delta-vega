#include "app_main.hpp"

QueueHandle_t speed_queue = xQueueCreate(1, sizeof(double));

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

    xTaskCreatePinnedToCore(update_display_loop, "update_display_loop", 8192, NULL, 1, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(update_speed_loop, "update_speed_loop", 8192, NULL, 1, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(update_mqtt_loop, "update_mqtt_loop", 8192, NULL, 1, NULL, APP_CPU_NUM);

    ESP_ERROR_CHECK(example_connect());
}