#pragma once

#include "app_main.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

esp_mqtt_client_handle_t client;

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
        esp_mqtt_client_subscribe(client, "/hoge/bar", 1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
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
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = "mqtts://a1pv0kof3jbrbo-ats.iot.ap-northeast-1.amazonaws.com:8883";
    mqtt_cfg.broker.verification.certificate = (const char *)server_cert_pem_start;
    mqtt_cfg.credentials.client_id = "delta-machine-alpha";
    mqtt_cfg.credentials.authentication.certificate =  (const char *)client_cert_pem_start;
    mqtt_cfg.credentials.authentication.key =  (const char *)client_key_pem_start;

    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
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
        char json_buffer[256];
        sprintf(
            json_buffer,
            "{\"speed\":%lf, \"latitude\":%lf, \"longitude\":%lf, \"machine_ts\":%d}",
            speed_queue_buff,
            latitude_queue_buff,
            longitude_queue_buff,
            999999
        );
        esp_mqtt_client_publish(client, "v0/delta_machine_alpha/test0130/machine_data", json_buffer, 0, 0, 0);
    
        ESP_LOGI(TAG, "---------------------");
        ESP_LOGI(TAG, "Publish v0/delta_machine_alpha/test0130/machine_data");
        ESP_LOGI(TAG, "%s", json_buffer);
        ESP_LOGI(TAG, "---------------------");

        vTaskDelayUntil(&xLastWakeTime, 1000 / portTICK_PERIOD_MS);
    }
}