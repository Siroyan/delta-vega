#include "vega_mqtt.hpp"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_system.h"
#include <cJSON.h>
#include <string.h>
#include <inttypes.h>

// Define TAG for logging
static const char *TAG = "VEGA-MQTT";

// Module variables
static esp_mqtt_client_handle_t client;

void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        // Use printf instead of ESP_LOGE to avoid configuration issues
        printf("Last error %s: 0x%x\n", message, error_code);
    }
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    // Removed ESP_LOGD to avoid configuration issues
    esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);
    esp_mqtt_client_handle_t client = event->client;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        // Use printf instead of ESP_LOGI to avoid configuration issues
        printf("MQTT_EVENT_CONNECTED\n");
        esp_mqtt_client_subscribe(client, "/hoge/bar", 1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        printf("MQTT_EVENT_DISCONNECTED\n");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        printf("MQTT_EVENT_SUBSCRIBED, msg_id=%d\n", event->msg_id);
        esp_mqtt_client_publish(client, "/hoge/fuga", "data", 0, 0, 0);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        printf("MQTT_EVENT_UNSUBSCRIBED, msg_id=%d\n", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        printf("MQTT_EVENT_PUBLISHED, msg_id=%d\n", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        printf("MQTT_EVENT_DATA\n");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        printf("MQTT_EVENT_ERROR\n");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            printf("Last errno string (%s)\n", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        printf("Other event id:%d\n", event->event_id);
        break;
    }
}

void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = "mqtts://a1pv0kof3jbrbo-ats.iot.ap-northeast-1.amazonaws.com:8883";
    mqtt_cfg.broker.verification.certificate = (const char *)server_cert_pem_start;
    mqtt_cfg.credentials.client_id = "delta-machine-alpha";
    mqtt_cfg.credentials.authentication.certificate =  (const char *)client_cert_pem_start;
    mqtt_cfg.credentials.authentication.key =  (const char *)client_key_pem_start;

    // Use printf instead of ESP_LOGI to avoid configuration issues
    printf("[APP] Free memory: %" PRIu32 " bytes\n", esp_get_free_heap_size());
    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID), mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void update_mqtt_loop(void *pvParameters) {
    mqtt_app_start();           // MQTT Start.
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    while(1) {
        double speed_queue_buff;
        double latitude_queue_buff;
        double longitude_queue_buff;
        xQueuePeek(speed_queue, &speed_queue_buff, 0);
        xQueuePeek(latitude_queue, &latitude_queue_buff, 0);
        xQueuePeek(longitude_queue, &longitude_queue_buff, 0);

        cJSON *mqtt_packet_json = cJSON_CreateObject();
        cJSON_AddNumberToObject(mqtt_packet_json, "speed", speed_queue_buff);
        cJSON_AddNumberToObject(mqtt_packet_json, "latitude", latitude_queue_buff);
        cJSON_AddNumberToObject(mqtt_packet_json, "longitude", longitude_queue_buff);
        cJSON_AddNumberToObject(mqtt_packet_json, "machine_ts", 999999);

        char *json_str = cJSON_PrintUnformatted(mqtt_packet_json);
        esp_mqtt_client_publish(client, "v0/delta_machine_alpha/test0130/machine_data", json_str, 0, 0, 0);
    
        // Use printf instead of ESP_LOGI to avoid configuration issues
        printf("---------------------\n");
        printf("Publish v0/delta_machine_alpha/test0130/machine_data\n");
        printf("%s\n", json_str);
        printf("---------------------\n");

        free(json_str);
        cJSON_Delete(mqtt_packet_json);
        vTaskDelayUntil(&xLastWakeTime, 1000);  // 1000 ticks delay
    }
}
