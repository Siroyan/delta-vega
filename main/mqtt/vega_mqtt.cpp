#include "vega_mqtt.hpp"
#include "../app_main.hpp"
#include "../ui/ui_manager.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include <cJSON.h>

esp_mqtt_client_handle_t client = nullptr;
static bool mqtt_connected = false;
static uint32_t last_publish_time = 0;
static bool client_initialized = false;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);
    esp_mqtt_client_handle_t client = event->client;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        mqtt_connected = true;
        esp_mqtt_client_subscribe(client, "/hoge/bar", 1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        mqtt_connected = false;
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        esp_mqtt_client_publish(client, "/hoge/fuga", "data", 0, 0, 0);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGE(TAG, "Last error reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGE(TAG, "Last tls stack error: 0x%x", event->error_handle->esp_tls_stack_err);
            ESP_LOGE(TAG, "Last captured errno: %d (%s)", event->error_handle->esp_transport_sock_errno,
                     strerror(event->error_handle->esp_transport_sock_errno));
        } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
            ESP_LOGE(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
        }
        
        // 再接続を試行
        ESP_LOGI(TAG, "Attempting to reconnect MQTT...");
        esp_mqtt_client_reconnect(client);
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void) {
    // 既にクライアントが初期化されている場合はクリーンアップ
    if (client_initialized && client != nullptr) {
        ESP_LOGW(TAG, "MQTT client already initialized, cleaning up first");
        mqtt_cleanup();
    }
    
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = "mqtts://a1pv0kof3jbrbo-ats.iot.ap-northeast-1.amazonaws.com:8883";
    mqtt_cfg.broker.verification.certificate = (const char *)server_cert_pem_start;
    mqtt_cfg.credentials.client_id = "delta-machine-alpha";
    mqtt_cfg.credentials.authentication.certificate =  (const char *)client_cert_pem_start;
    mqtt_cfg.credentials.authentication.key =  (const char *)client_key_pem_start;
    
    // 接続安定性の設定を追加
    mqtt_cfg.session.keepalive = 60;                    // Keep-alive: 60秒
    mqtt_cfg.network.timeout_ms = 10000;                // 接続タイムアウト: 10秒
    mqtt_cfg.network.refresh_connection_after_ms = 0;   // 自動再接続無効化（手動制御）
    mqtt_cfg.network.disable_auto_reconnect = false;    // 自動再接続有効
    mqtt_cfg.session.disable_clean_session = false;     // Clean session有効
    
    // TLS設定の改善
    mqtt_cfg.broker.verification.skip_cert_common_name_check = true;  // Common Name検証スキップ

    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == nullptr) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return;
    }
    
    esp_mqtt_client_register_event(client, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID), mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    client_initialized = true;
    ESP_LOGI(TAG, "MQTT client initialized and started successfully");
}

static void collect_telemetry_data(mqtt_telemetry_data_t* data) {
    // キューからデータ取得
    xQueuePeek(speed_queue, &data->speed_kmh, 0);
    xQueuePeek(latitude_queue, &data->latitude, 0);
    xQueuePeek(longitude_queue, &data->longitude, 0);
    
    // ui_managerからデータ取得
    ui_manager_log_data_t ui_data;
    get_ui_manager_log_data(&ui_data);
    
    data->average_speed_kmh = ui_data.average_speed_kmh;
    data->lap_number = ui_data.lap_number;
    data->total_time_ms = ui_data.total_time_ms;
    data->lap_time_ms = ui_data.lap_time_ms;
    
    // タイムスタンプ設定
    data->timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
}

static char* create_telemetry_json(const mqtt_telemetry_data_t* data) {
    cJSON *mqtt_packet_json = cJSON_CreateObject();
    
    // 基本テレメトリデータ
    cJSON_AddNumberToObject(mqtt_packet_json, "speed", data->speed_kmh);
    cJSON_AddNumberToObject(mqtt_packet_json, "average_speed", data->average_speed_kmh);
    cJSON_AddNumberToObject(mqtt_packet_json, "lap_number", data->lap_number);
    cJSON_AddNumberToObject(mqtt_packet_json, "total_time_ms", data->total_time_ms);
    cJSON_AddNumberToObject(mqtt_packet_json, "lap_time_ms", data->lap_time_ms);
    cJSON_AddNumberToObject(mqtt_packet_json, "latitude", data->latitude);
    cJSON_AddNumberToObject(mqtt_packet_json, "longitude", data->longitude);
    cJSON_AddNumberToObject(mqtt_packet_json, "timestamp_ms", data->timestamp_ms);
    
    // 追加メタデータ
    cJSON_AddStringToObject(mqtt_packet_json, "machine_id", "pi");
    cJSON_AddStringToObject(mqtt_packet_json, "memo", "Honda Eco Mileage Challenge 2025 Day0");
    
    char *json_str = cJSON_PrintUnformatted(mqtt_packet_json);
    cJSON_Delete(mqtt_packet_json);
    
    return json_str;
}

void mqtt_cleanup() {
    if (client != nullptr) {
        ESP_LOGI(TAG, "Cleaning up MQTT client...");
        
        // 接続を停止
        if (mqtt_connected) {
            esp_mqtt_client_stop(client);
            mqtt_connected = false;
        }
        
        // クライアントを削除
        esp_mqtt_client_destroy(client);
        client = nullptr;
        client_initialized = false;
        
        ESP_LOGI(TAG, "MQTT client cleanup completed");
    }
}

void update_mqtt_loop(void *pvParameters) {
    mqtt_app_start();           // MQTT Start.
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    
    while(1) {
        // テレメトリデータ収集
        mqtt_telemetry_data_t telemetry_data = {
            .speed_kmh = 0.0,
            .average_speed_kmh = 0.0,
            .lap_number = 0,
            .total_time_ms = 0,
            .lap_time_ms = 0,
            .latitude = 0.0,
            .longitude = 0.0,
            .timestamp_ms = 0
        };

        collect_telemetry_data(&telemetry_data);
        
        // JSON作成
        char *json_str = create_telemetry_json(&telemetry_data);
        
        // MQTT送信（接続状態確認）
        if (mqtt_connected && client != nullptr) {
            int msg_id = esp_mqtt_client_publish(client, "v0/delta_machine_alpha/telemetry/racing_data", json_str, 0, 0, 0);
            
            if (msg_id >= 0) {
                last_publish_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
                ESP_LOGI(TAG, "---------------------");
                ESP_LOGI(TAG, "Publish v0/delta_machine_alpha/telemetry/racing_data (msg_id: %d)", msg_id);
                ESP_LOGI(TAG, "Speed: %.1f km/h, Avg: %.1f km/h, Lap: %d", 
                         telemetry_data.speed_kmh, telemetry_data.average_speed_kmh, telemetry_data.lap_number);
                ESP_LOGI(TAG, "Total: %ld ms, Lap: %ld ms", 
                         (unsigned long)telemetry_data.total_time_ms, (unsigned long)telemetry_data.lap_time_ms);
                ESP_LOGI(TAG, "GPS: %.6f, %.6f", telemetry_data.latitude, telemetry_data.longitude);
                ESP_LOGI(TAG, "JSON: %s", json_str);
                ESP_LOGI(TAG, "---------------------");
            } else {
                ESP_LOGE(TAG, "Failed to publish MQTT message, error: %d", msg_id);
            }
        } else {
            ESP_LOGW(TAG, "MQTT not connected, skipping publish");
        }

        free(json_str);
        vTaskDelayUntil(&xLastWakeTime, 500 / portTICK_PERIOD_MS);
    }
}