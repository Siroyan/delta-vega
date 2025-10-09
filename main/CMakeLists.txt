set(CMAKE_CXX_STANDARD 11)

idf_component_register(
    SRCS
        app_main.cpp
        gps/nmea_parser.c
        gps/vega_gps.cpp
        ui/ui_manager.cpp
        ui/io_board/io_board.cpp
        ui/lcd/lcd.cpp
        mqtt/vega_mqtt.cpp
        spd/spd_mes.cpp
        sd_logger/sd_logger.cpp
    INCLUDE_DIRS "."
    REQUIRES esp_wifi protocol_examples_common mqtt LovyanGFX json fatfs vfs sdmmc
)