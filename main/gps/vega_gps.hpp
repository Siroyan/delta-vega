#pragma once

#include "../app_main.hpp"

#define TIME_ZONE (+9)
#define YEAR_BASE (2000) //date in GPS starts from 2000

void init_gps();
void deinit_gps();
void update_gps_loop(void *pvParameters);