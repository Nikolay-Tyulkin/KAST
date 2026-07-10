#pragma once

#include <stdbool.h>

#include "esp_err.h"

#include "app_types.h"

#define SETTINGS_AP_SSID "KAST Settings"
#define SETTINGS_AP_IP "192.168.4.1"

typedef void (*wifi_portal_settings_saved_cb_t)(const settings_store_t *settings, void *ctx);

esp_err_t wifi_portal_start(settings_store_t *settings, wifi_portal_settings_saved_cb_t on_saved, void *ctx);
void wifi_portal_stop(void);
bool wifi_portal_is_running(void);
