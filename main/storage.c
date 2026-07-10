#include "storage.h"

#include <string.h>

#include "esp_log.h"
#include "nvs.h"

#define HISTORY_MAGIC 0x4b4e4954u
#define HISTORY_VERSION 1u
#define SETTINGS_MAGIC 0x4b535447u
#define SETTINGS_VERSION 1u
#define SESSION_MAGIC 0x4b534553u
#define SESSION_VERSION 1u

typedef struct {
    uint32_t magic;
    uint32_t version;
    bool active;
    int rows;
    int64_t accumulated_active_ms;
    int state;
} session_store_t;

static const char *TAG = "storage";

static int clamp_int(int value, int min_value, int max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

void storage_history_reset(history_store_t *history)
{
    memset(history, 0, sizeof(*history));
    history->magic = HISTORY_MAGIC;
    history->version = HISTORY_VERSION;
}

void storage_settings_reset(settings_store_t *settings)
{
    memset(settings, 0, sizeof(*settings));
    settings->magic = SETTINGS_MAGIC;
    settings->version = SETTINGS_VERSION;
    settings->boot_beep_enabled = true;
    settings->brightness_pct = 50;
    settings->sleep_enabled = false;
    settings->sleep_timeout_s = 60;
}

void storage_save_history(const history_store_t *history)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("knit", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS open for history save failed: %s", esp_err_to_name(err));
        return;
    }

    err = nvs_set_blob(handle, "history", history, sizeof(*history));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS history save failed: %s", esp_err_to_name(err));
    }
    nvs_close(handle);
}

void storage_save_settings(const settings_store_t *settings)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("knit", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS open for settings save failed: %s", esp_err_to_name(err));
        return;
    }

    err = nvs_set_blob(handle, "settings", settings, sizeof(*settings));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS settings save failed: %s", esp_err_to_name(err));
    }
    nvs_close(handle);
}

void storage_save_session(int rows, int64_t accumulated_active_ms, session_state_t state)
{
    session_store_t session = {
        .magic = SESSION_MAGIC,
        .version = SESSION_VERSION,
        .active = state != SESSION_NONE,
        .rows = rows,
        .accumulated_active_ms = accumulated_active_ms,
        .state = state,
    };

    nvs_handle_t handle;
    esp_err_t err = nvs_open("knit", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS open for session save failed: %s", esp_err_to_name(err));
        return;
    }
    err = nvs_set_blob(handle, "session", &session, sizeof(session));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS session save failed: %s", esp_err_to_name(err));
    }
    nvs_close(handle);
}

void storage_clear_session(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("knit", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS open for session clear failed: %s", esp_err_to_name(err));
        return;
    }
    err = nvs_erase_key(handle, "session");
    if (err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND) {
        err = nvs_commit(handle);
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS session clear failed: %s", esp_err_to_name(err));
    }
    nvs_close(handle);
}

void storage_load(history_store_t *history, settings_store_t *settings, session_restore_t *session)
{
    storage_history_reset(history);
    storage_settings_reset(settings);
    memset(session, 0, sizeof(*session));

    nvs_handle_t handle;
    esp_err_t err = nvs_open("knit", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS open for load failed: %s", esp_err_to_name(err));
        return;
    }

    size_t size = sizeof(*history);
    err = nvs_get_blob(handle, "history", history, &size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        storage_history_reset(history);
    } else if (err != ESP_OK || size != sizeof(*history) || history->magic != HISTORY_MAGIC || history->version != HISTORY_VERSION) {
        ESP_LOGW(TAG, "Invalid NVS history, starting empty");
        storage_history_reset(history);
    }

    size = sizeof(*settings);
    err = nvs_get_blob(handle, "settings", settings, &size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        storage_settings_reset(settings);
    } else if (err != ESP_OK || size != sizeof(*settings) || settings->magic != SETTINGS_MAGIC || settings->version != SETTINGS_VERSION) {
        ESP_LOGW(TAG, "Invalid NVS settings, using defaults");
        storage_settings_reset(settings);
    }
    settings->brightness_pct = clamp_int(settings->brightness_pct, 0, 100);
    settings->sleep_timeout_s = clamp_int(settings->sleep_timeout_s, 5, 3600);

    session_store_t stored_session;
    size = sizeof(stored_session);
    err = nvs_get_blob(handle, "session", &stored_session, &size);
    if (err == ESP_OK && size == sizeof(stored_session) && stored_session.magic == SESSION_MAGIC &&
        stored_session.version == SESSION_VERSION && stored_session.active) {
        session->active = true;
        session->rows = stored_session.rows < 0 ? 0 : stored_session.rows;
        session->accumulated_active_ms = stored_session.accumulated_active_ms < 0 ? 0 : stored_session.accumulated_active_ms;
        session->state = stored_session.state == SESSION_PAUSED ? SESSION_PAUSED : SESSION_ACTIVE;
    }

    nvs_close(handle);
}
