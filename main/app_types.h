#pragma once

#include <stdbool.h>
#include <stdint.h>

#define HISTORY_MAX 20

typedef enum {
    SESSION_NONE,
    SESSION_ACTIVE,
    SESSION_PAUSED,
} session_state_t;

typedef enum {
    SCREEN_MAIN,
    SCREEN_RESET_CONFIRM,
    SCREEN_SETTINGS,
} screen_mode_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    bool boot_beep_enabled;
    uint8_t brightness_pct;
    bool sleep_enabled;
    uint16_t sleep_timeout_s;
} settings_store_t;

typedef struct {
    int rows;
    int duration_s;
    int reason;
} history_entry_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t count;
    uint32_t next;
    uint32_t total_rows;
    uint32_t total_seconds;
    history_entry_t last;
    history_entry_t entries[HISTORY_MAX];
} history_store_t;

typedef struct {
    bool active;
    int rows;
    int64_t accumulated_active_ms;
    session_state_t state;
} session_restore_t;
