#include "vega_gps.hpp"
#include "nmea_parser.h"

nmea_parser_handle_t nmea_hdl;

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
        break;
    case GPS_UNKNOWN:
        /* print unknown statements */
        ESP_LOGW(TAG, "Unknown statement:%s", (char *)event_data);
        break;
    default:
        break;
    }
}

void init_gps() {
    nmea_parser_config_t config = NMEA_PARSER_CONFIG_DEFAULT();
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
        vTaskDelayUntil(&xLastWakeTime, 1000 / portTICK_PERIOD_MS);
    }
}
