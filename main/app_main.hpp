#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "driver/gpio.h"

static const char *TAG = "DELTA-VEGA";

typedef enum {
    STATE_INITIALIZATION,  // 初期化状態
    STATE_STANDBY,         // スタンバイ状態
    STATE_RACING           // レース中状態
} system_state_t;

// 状態遷移通知用のメッセージタイプ
typedef enum {
    STATE_TRANSITION_TO_RACING,   // RACING状態への遷移要求
    STATE_TRANSITION_TO_STANDBY   // STANDBY状態への遷移要求
} state_transition_msg_t;

extern const uint8_t client_cert_pem_start[] asm("_binary_client_crt_start");
extern const uint8_t client_cert_pem_end[] asm("_binary_client_crt_end");
extern const uint8_t client_key_pem_start[] asm("_binary_client_key_start");
extern const uint8_t client_key_pem_end[] asm("_binary_client_key_end");
extern const uint8_t server_cert_pem_start[] asm("_binary_AmazonRootCA1_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_AmazonRootCA1_pem_end");

extern QueueHandle_t speed_queue;
extern QueueHandle_t speed_pulse_queue;
extern QueueHandle_t latitude_queue;
extern QueueHandle_t longitude_queue;
extern QueueHandle_t ctrl_sw_queue;
extern QueueHandle_t main_sw_queue;
extern QueueHandle_t state_transition_queue;  // 状態遷移通知用キュー

extern system_state_t current_state;
