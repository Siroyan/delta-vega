set(CMAKE_CXX_STANDARD 11)

idf_component_register(
    SRCS
        app_main.cpp
        gps/nmea_parser.c
        gps/vega_gps.cpp
        ioexp/io_board.cpp
        ioexp/ui_manager.cpp
        lcd/vega_lcd.cpp
        mqtt/vega_mqtt.cpp
        spd/spd_mes.cpp
    INCLUDE_DIRS "."
    REQUIRES esp_wifi protocol_examples_common mqtt LovyanGFX json
)