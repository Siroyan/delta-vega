#pragma once

#include "nmea_parser.h"
#include "esp_log.h"

// Constants
#define TIME_ZONE (+9)
#define YEAR_BASE (2000) // date in GPS starts from 2000

// Forward declarations
extern QueueHandle_t latitude_queue;
extern QueueHandle_t longitude_queue;

// Function declarations
void init_gps();
void deinit_gps();
void update_gps_loop(void *pvParameters);
