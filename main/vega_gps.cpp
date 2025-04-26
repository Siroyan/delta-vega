#include "vega_gps.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_event.h"
#include "driver/uart.h"  // Added for UART definitions

static const char *TAG = "VEGA-GPS";

// Module variables
static nmea_parser_handle_t nmea_hdl;
static double latitude_raw;
static double longitude_raw;

static void gps_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    gps_t *gps = NULL;
    switch (event_id) {
    case GPS_UPDATE:
        gps = (gps_t *)event_data;
        latitude_raw = gps->latitude;
        longitude_raw = gps->longitude;
        ESP_LOGI(TAG, "%d/%d/%d %d:%d:%d => \r\n"
            "\t\t\t\t\t\tlatitude   = %.05f°N\r\n"
            "\t\t\t\t\t\tlongitude = %.05f°E\r\n"
            "\t\t\t\t\t\taltitude   = %.02fm\r\n"
            "\t\t\t\t\t\tspeed      = %fm/s",
            gps->date.year + YEAR_BASE, gps->date.month, gps->date.day,
            gps->tim.hour + TIME_ZONE, gps->tim.minute, gps->tim.second,
            gps->latitude, gps->longitude, gps->altitude, gps->speed);
        break;
    case GPS_UNKNOWN:
        ESP_LOGW(TAG, "Unknown statement:%s", (char *)event_data);
        break;
    default:
        break;
    }
}

void init_gps() {
    // Use a direct initialization to avoid CONFIG_NMEA_PARSER_UART_RXD errors
    nmea_parser_config_t config = {
        .uart = {
            .uart_port = UART_NUM_1,
            .rx_pin = 5,  // Default value, adjust if needed
            .baud_rate = 9600,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .event_queue_size = 16
        }
    };
    
    nmea_hdl = nmea_parser_init(&config);
    nmea_parser_add_handler(nmea_hdl, gps_event_handler, NULL);
}

void deinit_gps() {
    nmea_parser_remove_handler(nmea_hdl, gps_event_handler);
    nmea_parser_deinit(nmea_hdl);
}

void update_gps_loop(void *pvParameters) {
    init_gps();
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    while(1) {
        xQueueOverwrite(latitude_queue, &latitude_raw);
        xQueueOverwrite(longitude_queue, &longitude_raw);
        vTaskDelayUntil(&xLastWakeTime, 200);  // 200 ticks delay
    }
}
