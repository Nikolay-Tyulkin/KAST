#pragma once

#include "app_types.h"

void storage_history_reset(history_store_t *history);
void storage_settings_reset(settings_store_t *settings);
void storage_load(history_store_t *history, settings_store_t *settings, session_restore_t *session);
void storage_save_history(const history_store_t *history);
void storage_save_settings(const settings_store_t *settings);
void storage_save_session(int rows, int64_t accumulated_active_ms, session_state_t state);
void storage_clear_session(void);
