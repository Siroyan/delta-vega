#include "lcd.hpp"

extern const uint16_t arrow_w;
extern const uint16_t arrow_h;
extern const unsigned short arrow_up[];
extern const unsigned short arrow_down[];
extern const unsigned short arrow_stay[];

lcd::lcd() {
    
}

lcd::~lcd() {
}

void lcd::draw_static_contents() {
    display.init();
    display.setRotation(1);
    display.setBrightness(255);
    display.fillScreen(0xFFFFFFu);

    // Current speed
    display.setTextColor(0x000000u);
    display.setFont(&fonts::Font4);
    display.drawString("km/h", 218, 75);

    // Lap and Average speed
    display.setFont(&fonts::Font4);
    display.drawString("Lap", 285, 25);
    display.drawString("Ave", 285, 75);
    display.setFont(&fonts::Font6);
    display.drawString("0   7", 335, 15);
    display.drawLine(370, 55, 390, 15, VEGA_BLK);
    display.drawLine(369, 55, 389, 15, VEGA_BLK);
    display.drawFloat(10.0, 1, 335, 65);
    display.setFont(&fonts::Font2);
    display.drawString("km/h", 435, 88);

    // Total time area
    display.setFont(&fonts::Font2);
    display.drawString("TTL Time", 20, 100);
    display.setFont(&fonts::Font6);
    display.drawString("01:23", 20, 120);
    display.drawString("39:20", 166, 120);
    display.setColor(VEGA_GRY);
    display.fillTriangle(150, 120, 150, 156, 160, 138);
    
    // Lap time area
    display.setFont(&fonts::Font2);
    display.drawString("LAP Time", 20, 170);
    display.setFont(&fonts::Font6);
    display.drawString("01:23", 20, 190);
    display.drawString("04:56", 166, 190);
    display.setColor(VEGA_GRY);
    display.fillTriangle(150, 190, 150, 226, 160, 208);
    display.fillRect(20, 240, 440, 2, VEGA_GRY);
    
    // Indicator labels
    display.setFont(&fonts::Font2);
    display.drawString("HBT", indicator_labal_1st_x, indicator_labal_1st_y +  0);
    display.drawString("SEN", indicator_labal_1st_x, indicator_labal_1st_y + 18);
    display.drawString("GPS", indicator_labal_1st_x, indicator_labal_1st_y + 36);
    display.drawString("ENG", indicator_labal_1st_x + 50, indicator_labal_1st_y +  0);
    display.drawString("AAA", indicator_labal_1st_x + 50, indicator_labal_1st_y + 18);
    display.drawString("BBB", indicator_labal_1st_x + 50, indicator_labal_1st_y + 36);

    // Indicator LEDs
    display.fillCircle(indicator_labal_1st_x + 35, indicator_labal_1st_y +  7, 5, VEGA_GRY);             // HBT
    display.fillCircle(indicator_labal_1st_x + 35, indicator_labal_1st_y + 25, 5, VEGA_GRY);             // SEN
    display.fillCircle(indicator_labal_1st_x + 35, indicator_labal_1st_y + 43, 5, VEGA_GRY);             // GPS
    display.fillCircle(indicator_labal_1st_x + 85, indicator_labal_1st_y +  7, 5, VEGA_GRY);             // ENG
    display.fillCircle(indicator_labal_1st_x + 85, indicator_labal_1st_y + 25, 5, VEGA_GRY);
    display.fillCircle(indicator_labal_1st_x + 85, indicator_labal_1st_y + 43, 5, VEGA_GRY);
}

void lcd::draw_outline_border(uint8_t color, uint16_t weight) {
    display.fillRect(0, 0, LCD_W, weight, color);        // Upper border
    display.fillRect(0, 0, weight, LCD_H, color);        // Left border
    display.fillRect(0, LCD_H - weight, LCD_W, weight, color);        // Bottom border
    display.fillRect(LCD_W - weight, 0, weight, LCD_H, color);        // Right border
}

void lcd::set_speed(double speed_value) {
    display.setTextColor(VEGA_BLK, VEGA_WHT);
    display.setFont(&fonts::Font8);
    display.setCursor(20, 20);
    display.printf("%04.1f", speed_value);
}

void lcd::update_display_loop(void *pvParameters) {
    draw_static_contents();
    arrow_sprite.setBuffer((void*)arrow_down, arrow_w, arrow_h, 16);
    arrow_sprite.pushRotateZoom(380.f, 170.f, 0.f, 0.8, 0.8, TFT_BLACK);
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    while(1) {
        double speed_queue_buff;
        bool speed_pulse_queue_buff;
        double latitude_queue_buff;
        double longitude_queue_buff;
        bool ctrl_sw_queue_buff;
        bool main_sw_queue_buff;
        // Borders
        switch (*((system_state_t *) pvParameters)) {           
            case STATE_STANDBY:
                draw_outline_border(VEGA_GRN, 10);
                break;
            case STATE_RACING:
                draw_outline_border(VEGA_RED, 10);
                break;
            default:
                draw_outline_border(VEGA_GRY, 10);
                break;
        }
        // Speed
        // Update speed
        xQueueReceive(speed_queue, &speed_queue_buff, 0);
        display.setTextColor(VEGA_BLK, VEGA_WHT);
        display.setFont(&fonts::Font8);
        display.setCursor(20, 20);
        display.printf("%04.1f", speed_queue_buff);
        // Update indicator led
        xQueueReceive(speed_pulse_queue, &speed_pulse_queue_buff, 0);
        ESP_LOGI(TAG, "speed_pulse_queue_buff:%d", speed_pulse_queue_buff);
        if (speed_pulse_queue_buff) {
            display.fillCircle(indicator_labal_1st_x + 35, indicator_labal_1st_y + 25, 5, VEGA_GRN);
        } else {
            display.fillCircle(indicator_labal_1st_x + 35, indicator_labal_1st_y + 25, 5, VEGA_GRY);
        }
        // GPS
        xQueuePeek(latitude_queue, &latitude_queue_buff, 0);
        xQueuePeek(longitude_queue, &longitude_queue_buff, 0);
        display.setTextColor(VEGA_BLK, VEGA_WHT);
        display.setFont(&fonts::Font4);
        display.drawString("LATI", 125, 250);
        display.drawFloat(latitude_queue_buff, 6, 210, 250);
        display.drawString("LONG", 125, 282);
        display.drawFloat(longitude_queue_buff, 6, 210, 282);

        if (latitude_queue_buff > 0.f && longitude_queue_buff > 0.f) {
            display.fillCircle(indicator_labal_1st_x + 35, indicator_labal_1st_y + 43, 5, VEGA_GRN);
        } else {
            display.fillCircle(indicator_labal_1st_x + 35, indicator_labal_1st_y + 43, 5, VEGA_GRY);
        }
        // Lap
        if (xQueueReceive(ctrl_sw_queue, &ctrl_sw_queue_buff, 0)) {
            if (lap_num == 7) {
                lap_num = 0;
            } else {
                lap_num++;
            }
            display.setFont(&fonts::Font6);
            display.setTextColor(VEGA_BLK, VEGA_WHT);
            display.setCursor(335, 15);
            display.printf("%d", lap_num);
            ESP_LOGI(TAG, "ctrl_sw:%d", ctrl_sw_queue_buff);
        }
        // Heart beat
        if (hbt_led_status) {
            display.fillCircle(indicator_labal_1st_x + 35, indicator_labal_1st_y +  7, 5, VEGA_GRY);
        } else {
            display.fillCircle(indicator_labal_1st_x + 35, indicator_labal_1st_y +  7, 5, VEGA_GRN);
        }
        // ENG
        hbt_led_status = !hbt_led_status;
        vTaskDelayUntil(&xLastWakeTime, 100 / portTICK_PERIOD_MS);
    }
}