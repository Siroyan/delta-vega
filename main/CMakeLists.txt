set(CMAKE_CXX_STANDARD 11)

idf_component_register(SRCS "app_main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi protocol_examples_common mqtt LovyanGFX 
                    )
