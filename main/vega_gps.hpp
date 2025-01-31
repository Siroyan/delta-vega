#pragma once

#define TIME_ZONE (+9)
#define YEAR_BASE (2000) //date in GPS starts from 2000

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
        /* print information parsed from GPS statements */
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
        vTaskDelayUntil(&xLastWakeTime, 200 / portTICK_PERIOD_MS);
    }
}
