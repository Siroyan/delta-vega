set(CMAKE_CXX_STANDARD 11)

idf_component_register(SRCS "app_main.cpp" "nmea_parser.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi protocol_examples_common mqtt LovyanGFX esp_driver_pcnt json
                    )