set(CMAKE_CXX_STANDARD 11)

idf_component_register(SRCS "app_main.cpp" "gps/nmea_parser.c" "gps/vega_gps.cpp" "lcd/vega_lcd.cpp" "spd/spd_mes.cpp" "ioexp/ui_manager.cpp" "mqtt/vega_mqtt.cpp" "ioexp/io_board.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi protocol_examples_common mqtt LovyanGFX json
                    )