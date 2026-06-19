idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_lcd driver esp_driver_ledc esp_timer lvgl esp_lvgl_port esp_adc nvs_flash)

target_compile_definitions(${COMPONENT_LIB} PRIVATE APP_VERSION="${PROJECT_VER}")
