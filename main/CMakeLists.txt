idf_component_register(SRCS "main.c" "storage.c" "wifi_portal.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_lcd driver esp_driver_ledc esp_timer lvgl esp_lvgl_port esp_adc nvs_flash esp_wifi esp_http_server esp_netif)

target_compile_definitions(${COMPONENT_LIB} PRIVATE APP_VERSION="${PROJECT_VER}")
