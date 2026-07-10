#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "esp_lvgl_port.h"
#include "lvgl.h"

#include "app_types.h"
#include "storage.h"
#include "wifi_portal.h"

#define LCD_H_RES 250
#define LCD_V_RES 280

#define LCD_HOST SPI3_HOST
#define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)
#define LCD_CMD_BITS 8
#define LCD_PARAM_BITS 8
#define LCD_BITS_PER_PIXEL 16
#define LCD_DRAW_BUF_HEIGHT 40

#define PIN_LCD_SCLK GPIO_NUM_6
#define PIN_LCD_MOSI GPIO_NUM_7
#define PIN_LCD_RST GPIO_NUM_8
#define PIN_LCD_DC GPIO_NUM_4
#define PIN_LCD_CS GPIO_NUM_5
#define PIN_LCD_BL GPIO_NUM_15

#define PIN_PWR_SYS_OUT GPIO_NUM_40
#define PIN_PWR_SYS_EN GPIO_NUM_41
#define PIN_BUZZER GPIO_NUM_42

#define BUZZER_LEDC_TIMER LEDC_TIMER_0
#define BUZZER_LEDC_MODE LEDC_LOW_SPEED_MODE
#define BUZZER_LEDC_CHANNEL LEDC_CHANNEL_0
#define BUZZER_DUTY_RES LEDC_TIMER_10_BIT
#define BUZZER_DUTY 26

#define BACKLIGHT_LEDC_TIMER LEDC_TIMER_1
#define BACKLIGHT_LEDC_MODE LEDC_LOW_SPEED_MODE
#define BACKLIGHT_LEDC_CHANNEL LEDC_CHANNEL_1
#define BACKLIGHT_DUTY_RES LEDC_TIMER_10_BIT
#define BACKLIGHT_DUTY_MAX 1023

#define PIN_BTN_PLUS GPIO_NUM_2
#define PIN_BTN_MINUS GPIO_NUM_16
#define PIN_BTN_UNIVERSAL GPIO_NUM_17

#define BAT_ADC_CHANNEL ADC_CHANNEL_0
#define BAT_ADC_ATTEN ADC_ATTEN_DB_12
#define BAT_MEASURE_MIN_MV 3300
#define BAT_MEASURE_MAX_MV 3840
#define BAT_VOLTAGE_DIVIDER_NUM 3
#define BAT_VOLTAGE_DIVIDER_DEN 1
#define BAT_LOW_PERCENT 15

#define UI_TEXT_COLOR lv_color_hex(0x454449)
#define UI_MAIN_BG_COLOR lv_color_hex(0xFCA5A5)
#define UI_PAUSE_BG_COLOR lv_color_hex(0xBFE3FF)
#define UI_BG_PALETTE_COUNT 30

#define APP_TICK_MS 20
#define BUTTON_DEBOUNCE_MS 20
#define BUTTON_LONG_MS 1500
#define PLUS_AP_LONG_MS 3000
#define BOOT_ANIMATION_MS 1500
#define MELODY_NOTE_MS 90
#define IDLE_BLINK_MS 100
#define IDLE_BLINK_MIN_MS 2000
#define IDLE_BLINK_MAX_MS 7000
#define IDLE_HAPPY_MS 4000
#define IDLE_HAPPY_MIN_MS 25000
#define IDLE_HAPPY_MAX_MS 35000
#define RESET_MULTI_CLICK_WINDOW_MS 2000
#define RESET_CONFIRM_TIMEOUT_MS 5000

typedef struct {
    gpio_num_t pin;
    const char *name;
    int stable_level;
    int last_raw_level;
    int64_t last_change_ms;
    int64_t pressed_at_ms;
    int64_t long_ms;
    bool long_reported;
} button_t;

static const char *TAG = "knitting_assistant";

static esp_lcd_panel_io_handle_t s_lcd_io = NULL;
static esp_lcd_panel_handle_t s_lcd_panel = NULL;
static lv_display_t *s_lv_display = NULL;
static adc_oneshot_unit_handle_t s_adc_handle = NULL;

static lv_obj_t *s_bat_label = NULL;
static lv_obj_t *s_rows_label = NULL;
static lv_obj_t *s_rows_label_bold_x = NULL;
static lv_obj_t *s_rows_label_bold_y = NULL;
static lv_obj_t *s_time_label = NULL;
static lv_obj_t *s_anim_label = NULL;
static lv_obj_t *s_color_label = NULL;
static lv_obj_t *s_version_label = NULL;
static lv_obj_t *s_settings_title_label = NULL;
static lv_obj_t *s_settings_qr = NULL;
static lv_obj_t *s_settings_url_label = NULL;

static const uint32_t s_bg_palette[UI_BG_PALETTE_COUNT] = {
    0xD8F1C7, 0xF4EBD0, 0xFFE0E0, 0xFFD1A8, 0xFFF3A3,
    0xD9F99D, 0xA7F3D0, 0xBFEFE7, 0xBFE3FF, 0xC7D2FE,
    0xE9D5FF, 0xFBCFE8, 0xFDE68A, 0xFDBA74, 0xFCA5A5,
    0x86EFAC, 0x5EEAD4, 0x7DD3FC, 0xA5B4FC, 0xD8B4FE,
    0x14532D, 0x164E63, 0x1E3A8A, 0x312E81, 0x581C87,
    0x7F1D1D, 0x78350F, 0x365314, 0x374151, 0x111827,
};
static int s_bg_palette_index = 14;
static bool s_palette_mode = false;

static button_t s_btn_plus = { .pin = PIN_BTN_PLUS, .name = "PLUS", .stable_level = 1, .last_raw_level = 1, .long_ms = PLUS_AP_LONG_MS };
static button_t s_btn_minus = { .pin = PIN_BTN_MINUS, .name = "MINUS", .stable_level = 1, .last_raw_level = 1, .long_ms = BUTTON_LONG_MS };
static button_t s_btn_universal = { .pin = PIN_BTN_UNIVERSAL, .name = "UNIVERSAL", .stable_level = 1, .last_raw_level = 1, .long_ms = BUTTON_LONG_MS };

static session_state_t s_session_state = SESSION_NONE;
static screen_mode_t s_screen_mode = SCREEN_MAIN;
static history_store_t s_history;
static settings_store_t s_settings;
static int s_rows = 0;
static int s_bat_percent = 0;
static int64_t s_session_started_ms = 0;
static int64_t s_active_started_ms = 0;
static int64_t s_accumulated_active_ms = 0;
static int64_t s_boot_until_ms = 0;
static int64_t s_boot_started_ms = 0;
static int64_t s_reset_confirm_started_ms = 0;
static int64_t s_last_universal_short_ms = 0;
static int s_universal_short_count = 0;
static int s_melody_note = -1;
static int64_t s_idle_face_until_ms = 0;
static int64_t s_idle_next_blink_ms = 0;
static int64_t s_idle_happy_until_ms = 0;
static int64_t s_idle_next_happy_ms = 0;
static int64_t s_last_activity_ms = 0;
static bool s_ui_dirty = true;
static bool s_display_dimmed = false;

static const int s_startup_melody_hz[] = {
    523, 659, 784, 988, 1319, 988, 784, 1047,
};

static int64_t now_ms(void)
{
    return esp_timer_get_time() / 1000;
}

static int64_t random_delay_ms(int min_ms, int max_ms)
{
    if (max_ms <= min_ms) {
        return min_ms;
    }
    return min_ms + (esp_random() % (uint32_t)(max_ms - min_ms + 1));
}

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

static void activity_mark(int64_t now)
{
    s_last_activity_ms = now;
    if (s_display_dimmed) {
        s_display_dimmed = false;
        s_ui_dirty = true;
    }
}

static const char *idle_face_text(int64_t now)
{
    if (now < s_idle_happy_until_ms) {
        return "(^_^)";
    }
    if (now < s_idle_face_until_ms) {
        return "(-_-)";
    }
    return "(o_o)";
}

static void idle_face_schedule_reset(int64_t now)
{
    s_idle_face_until_ms = 0;
    s_idle_happy_until_ms = 0;
    s_idle_next_blink_ms = now + random_delay_ms(IDLE_BLINK_MIN_MS, IDLE_BLINK_MAX_MS);
    s_idle_next_happy_ms = now + random_delay_ms(IDLE_HAPPY_MIN_MS, IDLE_HAPPY_MAX_MS);
}

static void idle_face_update(int64_t now)
{
    static bool idle_was_visible = false;
    bool idle_visible = (s_session_state == SESSION_NONE && s_screen_mode == SCREEN_MAIN);

    if (!idle_visible) {
        idle_was_visible = false;
        s_idle_face_until_ms = 0;
        s_idle_happy_until_ms = 0;
        return;
    }

    if (!idle_was_visible || s_idle_next_blink_ms == 0 || s_idle_next_happy_ms == 0) {
        idle_face_schedule_reset(now);
        idle_was_visible = true;
        s_ui_dirty = true;
        return;
    }

    if (s_idle_happy_until_ms != 0 && now >= s_idle_happy_until_ms) {
        s_idle_happy_until_ms = 0;
        s_idle_next_happy_ms = now + random_delay_ms(IDLE_HAPPY_MIN_MS, IDLE_HAPPY_MAX_MS);
        s_ui_dirty = true;
    }

    if (s_idle_face_until_ms != 0 && now >= s_idle_face_until_ms) {
        s_idle_face_until_ms = 0;
        s_idle_next_blink_ms = now + random_delay_ms(IDLE_BLINK_MIN_MS, IDLE_BLINK_MAX_MS);
        s_ui_dirty = true;
    }

    if (s_idle_happy_until_ms == 0 && now >= s_idle_next_happy_ms) {
        s_idle_happy_until_ms = now + IDLE_HAPPY_MS;
        s_idle_face_until_ms = 0;
        s_ui_dirty = true;
        return;
    }

    if (s_idle_happy_until_ms == 0 && s_idle_face_until_ms == 0 && now >= s_idle_next_blink_ms) {
        s_idle_face_until_ms = now + IDLE_BLINK_MS;
        s_ui_dirty = true;
    }
}

static int active_duration_s(void)
{
    int64_t active_ms = s_accumulated_active_ms;
    if (s_session_state == SESSION_ACTIVE) {
        active_ms += now_ms() - s_active_started_ms;
    }
    if (active_ms < 0) {
        active_ms = 0;
    }
    return (int)(active_ms / 1000);
}

static void format_duration(char *buf, size_t size, int duration_s)
{
    int hours = duration_s / 3600;
    int minutes = (duration_s / 60) % 60;
    int seconds = duration_s % 60;
    snprintf(buf, size, "%02d:%02d:%02d", hours, minutes, seconds);
}

static void session_store_save(void)
{
    int64_t accumulated_active_ms = s_accumulated_active_ms;
    if (s_session_state == SESSION_ACTIVE) {
        accumulated_active_ms += now_ms() - s_active_started_ms;
    }
    storage_save_session(s_rows, accumulated_active_ms, s_session_state);
}

static void history_add(int rows, int duration_s, int reason)
{
    history_entry_t entry = {
        .rows = rows,
        .duration_s = duration_s,
        .reason = reason,
    };

    s_history.entries[s_history.next] = entry;
    s_history.last = entry;
    s_history.next = (s_history.next + 1) % HISTORY_MAX;
    if (s_history.count < HISTORY_MAX) {
        s_history.count++;
    }
    s_history.total_rows += rows;
    s_history.total_seconds += duration_s;
    storage_save_history(&s_history);
}

static esp_err_t lcd_init(void)
{
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_LCD_SCLK,
        .mosi_io_num = PIN_LCD_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = LCD_H_RES * LCD_DRAW_BUF_HEIGHT * sizeof(uint16_t),
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI init failed");

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_LCD_DC,
        .cs_gpio_num = PIN_LCD_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &s_lcd_io), TAG, "new panel IO failed");

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .data_endian = LCD_RGB_DATA_ENDIAN_BIG,
        .bits_per_pixel = LCD_BITS_PER_PIXEL,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7789(s_lcd_io, &panel_config, &s_lcd_panel), TAG, "new st7789 panel failed");

    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_lcd_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_lcd_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_lcd_panel, true, true));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(s_lcd_panel, -10, 20));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_lcd_panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_lcd_panel, true));
    return ESP_OK;
}

static void backlight_set_percent(int percent)
{
    percent = clamp_int(percent, 0, 100);
    uint32_t duty = (uint32_t)((percent * BACKLIGHT_DUTY_MAX) / 100);
    ledc_set_duty(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CHANNEL, duty);
    ledc_update_duty(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CHANNEL);
}

static void backlight_apply(void)
{
    backlight_set_percent(s_display_dimmed ? 0 : s_settings.brightness_pct);
}

static esp_err_t lvgl_init_display(void)
{
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 4096,
        .task_affinity = -1,
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5,
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "lvgl_port_init failed");

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = s_lcd_io,
        .panel_handle = s_lcd_panel,
        .buffer_size = LCD_H_RES * LCD_DRAW_BUF_HEIGHT * sizeof(uint16_t),
        .double_buffer = true,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
            .sw_rotate = true,
        },
    };

    s_lv_display = lvgl_port_add_disp(&disp_cfg);
    if (s_lv_display != NULL) {
        lv_disp_set_rotation(s_lv_display, LV_DISP_ROT_270);
    }
    return (s_lv_display == NULL) ? ESP_FAIL : ESP_OK;
}

static lv_obj_t *ui_create_rows_label(lv_obj_t *parent, int x_offset, int y_offset)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_color(label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_size(label, 280, 70);
    lv_label_set_text(label, "0");
    lv_obj_align(label, LV_ALIGN_CENTER, x_offset, y_offset);
    return label;
}

static void ui_create(void)
{
    lvgl_port_lock(0);

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, UI_MAIN_BG_COLOR, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    s_bat_label = lv_label_create(scr);
    lv_obj_set_style_text_color(s_bat_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(s_bat_label, &lv_font_montserrat_20, 0);
    lv_label_set_text(s_bat_label, "--%");
    lv_obj_align(s_bat_label, LV_ALIGN_TOP_MID, 0, 14);

    s_rows_label_bold_x = ui_create_rows_label(scr, 1, 0);
    s_rows_label_bold_y = ui_create_rows_label(scr, 0, 1);
    s_rows_label = ui_create_rows_label(scr, 0, 0);

    s_time_label = lv_label_create(scr);
    lv_obj_set_style_text_color(s_time_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(s_time_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(s_time_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(s_time_label, "00:00");
    lv_obj_align(s_time_label, LV_ALIGN_CENTER, 0, 58);

    s_anim_label = lv_label_create(scr);
    lv_obj_set_style_text_color(s_anim_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(s_anim_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(s_anim_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_line_space(s_anim_label, -2, 0);
    lv_obj_set_width(s_anim_label, 220);
    lv_label_set_long_mode(s_anim_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(s_anim_label, "");
    lv_obj_align(s_anim_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(s_anim_label, LV_OBJ_FLAG_HIDDEN);

    s_color_label = lv_label_create(scr);
    lv_obj_set_style_text_color(s_color_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(s_color_label, &lv_font_montserrat_20, 0);
    lv_label_set_text(s_color_label, "#FCA5A5");
    lv_obj_align(s_color_label, LV_ALIGN_CENTER, 0, 54);
    lv_obj_add_flag(s_color_label, LV_OBJ_FLAG_HIDDEN);

    s_settings_title_label = lv_label_create(scr);
    lv_obj_set_style_text_color(s_settings_title_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(s_settings_title_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(s_settings_title_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(s_settings_title_label, "SETTINGS");
    lv_obj_align(s_settings_title_label, LV_ALIGN_TOP_MID, 0, 46);
    lv_obj_add_flag(s_settings_title_label, LV_OBJ_FLAG_HIDDEN);

    s_settings_qr = lv_qrcode_create(scr, 145, UI_TEXT_COLOR, UI_PAUSE_BG_COLOR);
    const char wifi_qr_payload[] = "WIFI:T:nopass;S:" SETTINGS_AP_SSID ";;";
    lv_qrcode_update(s_settings_qr, wifi_qr_payload, strlen(wifi_qr_payload));
    lv_obj_align(s_settings_qr, LV_ALIGN_CENTER, 0, -12);
    lv_obj_add_flag(s_settings_qr, LV_OBJ_FLAG_HIDDEN);

    s_settings_url_label = lv_label_create(scr);
    lv_obj_set_style_text_color(s_settings_url_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(s_settings_url_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(s_settings_url_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(s_settings_url_label, "http://" SETTINGS_AP_IP);
    lv_obj_align(s_settings_url_label, LV_ALIGN_BOTTOM_MID, 0, -18);
    lv_obj_add_flag(s_settings_url_label, LV_OBJ_FLAG_HIDDEN);

    s_version_label = lv_label_create(scr);
    lv_obj_set_style_text_color(s_version_label, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(s_version_label, &lv_font_montserrat_14, 0);
    lv_label_set_text(s_version_label, APP_VERSION);
    lv_obj_align(s_version_label, LV_ALIGN_BOTTOM_MID, 0, -14);

    lvgl_port_unlock();
}

static void power_latch_init(void)
{
    gpio_config_t cfg_out = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_PWR_SYS_EN,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg_out));
    ESP_ERROR_CHECK(gpio_set_level(PIN_PWR_SYS_EN, 1));

    gpio_config_t cfg_in = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << PIN_PWR_SYS_OUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg_in));
}

static void low_power_peripherals_init(void)
{
    ledc_timer_config_t timer_cfg = {
        .speed_mode = BUZZER_LEDC_MODE,
        .duty_resolution = BUZZER_DUTY_RES,
        .timer_num = BUZZER_LEDC_TIMER,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t channel_cfg = {
        .gpio_num = PIN_BUZZER,
        .speed_mode = BUZZER_LEDC_MODE,
        .channel = BUZZER_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = BUZZER_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_cfg));
    ESP_ERROR_CHECK(gpio_set_level(PIN_BUZZER, 0));

    ledc_timer_config_t backlight_timer_cfg = {
        .speed_mode = BACKLIGHT_LEDC_MODE,
        .duty_resolution = BACKLIGHT_DUTY_RES,
        .timer_num = BACKLIGHT_LEDC_TIMER,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&backlight_timer_cfg));

    ledc_channel_config_t backlight_channel_cfg = {
        .gpio_num = PIN_LCD_BL,
        .speed_mode = BACKLIGHT_LEDC_MODE,
        .channel = BACKLIGHT_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = BACKLIGHT_LEDC_TIMER,
        .duty = (50 * BACKLIGHT_DUTY_MAX) / 100,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&backlight_channel_cfg));
}

static void buzzer_tone(int freq_hz)
{
    if (freq_hz <= 0) {
        ledc_set_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, 0);
        ledc_update_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL);
        gpio_set_level(PIN_BUZZER, 0);
        return;
    }
    ledc_set_freq(BUZZER_LEDC_MODE, BUZZER_LEDC_TIMER, freq_hz);
    ledc_set_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, BUZZER_DUTY);
    ledc_update_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL);
}

static void startup_melody_update(int64_t now)
{
    if (!s_settings.boot_beep_enabled) {
        if (s_melody_note != -1) {
            s_melody_note = -1;
            buzzer_tone(0);
        }
        return;
    }
    if (now >= s_boot_until_ms) {
        if (s_melody_note != -1) {
            s_melody_note = -1;
            buzzer_tone(0);
        }
        return;
    }

    int note = (int)((now - s_boot_started_ms) / MELODY_NOTE_MS);
    int note_count = (int)(sizeof(s_startup_melody_hz) / sizeof(s_startup_melody_hz[0]));
    if (note >= note_count) {
        note = note_count - 1;
    }
    if (note != s_melody_note) {
        s_melody_note = note;
        buzzer_tone(s_startup_melody_hz[note]);
    }
}

static void settings_saved(const settings_store_t *settings, void *ctx)
{
    (void)ctx;
    storage_save_settings(settings);
    s_display_dimmed = false;
    activity_mark(now_ms());
    backlight_apply();
}

static void portal_toggle(void)
{
    if (s_screen_mode != SCREEN_MAIN && s_screen_mode != SCREEN_SETTINGS) {
        return;
    }
    if (wifi_portal_is_running()) {
        wifi_portal_stop();
        if (s_screen_mode == SCREEN_SETTINGS) {
            s_screen_mode = SCREEN_MAIN;
        }
        s_ui_dirty = true;
    } else if (wifi_portal_start(&s_settings, settings_saved, NULL) == ESP_OK) {
        s_screen_mode = SCREEN_SETTINGS;
        s_ui_dirty = true;
    } else {
        ESP_LOGW(TAG, "Settings AP start failed");
    }
}

static void buttons_init(void)
{
    gpio_config_t cfg = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << PIN_BTN_PLUS) | (1ULL << PIN_BTN_MINUS) | (1ULL << PIN_BTN_UNIVERSAL),
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
}

static void adc_init_battery(void)
{
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &s_adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = BAT_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_handle, BAT_ADC_CHANNEL, &chan_cfg));
}

static int battery_percent_read(void)
{
    int raw = 0;
    if (adc_oneshot_read(s_adc_handle, BAT_ADC_CHANNEL, &raw) != ESP_OK) {
        return s_bat_percent;
    }

    int adc_mv = (raw * 3300) / 4095;
    int bat_mv = (adc_mv * BAT_VOLTAGE_DIVIDER_NUM) / BAT_VOLTAGE_DIVIDER_DEN;
    int pct = (bat_mv - BAT_MEASURE_MIN_MV) * 100 / (BAT_MEASURE_MAX_MV - BAT_MEASURE_MIN_MV);

    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    ESP_LOGI(TAG, "Battery ADC raw=%d adc_mv=%d bat_mv=%d pct=%d", raw, adc_mv, bat_mv, pct);
    return pct;
}

static void ui_update(void)
{
    char bat_buf[32];
    char rows_buf[16];
    char time_buf[16];
    char color_buf[16];
    int64_t now = now_ms();
    bool boot_active = now < s_boot_until_ms;
    bool show_counter = !boot_active && s_session_state == SESSION_ACTIVE && s_screen_mode == SCREEN_MAIN;
    bool show_idle_prompt = s_session_state == SESSION_NONE && s_screen_mode == SCREEN_MAIN;
    bool show_pause = !boot_active && s_session_state == SESSION_PAUSED && s_screen_mode == SCREEN_MAIN;
    bool show_reset = !boot_active && s_screen_mode == SCREEN_RESET_CONFIRM;
    bool show_settings = !boot_active && s_screen_mode == SCREEN_SETTINGS;
    lv_color_t bg_color = UI_MAIN_BG_COLOR;

    snprintf(bat_buf, sizeof(bat_buf), "%d%%", s_bat_percent);
    snprintf(rows_buf, sizeof(rows_buf), "%d", s_rows);
    format_duration(time_buf, sizeof(time_buf), active_duration_s());
    snprintf(color_buf, sizeof(color_buf), "#%06lX", (unsigned long)s_bg_palette[s_bg_palette_index]);

    if (show_pause || show_settings) {
        bg_color = UI_PAUSE_BG_COLOR;
    }

    lvgl_port_lock(0);
    lv_obj_set_style_bg_color(lv_scr_act(), bg_color, 0);
    lv_label_set_text(s_bat_label, bat_buf);
    lv_obj_align(s_bat_label, LV_ALIGN_TOP_MID, 0, 14);
    if (show_settings) {
        lv_obj_add_flag(s_bat_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(s_bat_label, LV_OBJ_FLAG_HIDDEN);
    }

    lv_label_set_text(s_rows_label_bold_x, rows_buf);
    lv_obj_align(s_rows_label_bold_x, LV_ALIGN_CENTER, 1, 0);
    lv_label_set_text(s_rows_label_bold_y, rows_buf);
    lv_obj_align(s_rows_label_bold_y, LV_ALIGN_CENTER, 0, 1);
    lv_label_set_text(s_rows_label, rows_buf);
    lv_obj_align(s_rows_label, LV_ALIGN_CENTER, 0, 0);
    if (show_counter) {
        lv_obj_clear_flag(s_rows_label_bold_x, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_rows_label_bold_y, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_rows_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_rows_label_bold_x, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_rows_label_bold_y, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_rows_label, LV_OBJ_FLAG_HIDDEN);
    }

    lv_label_set_text(s_time_label, time_buf);
    lv_obj_align(s_time_label, LV_ALIGN_CENTER, 0, 58);
    if (show_counter) {
        lv_obj_clear_flag(s_time_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_time_label, LV_OBJ_FLAG_HIDDEN);
    }

    if (show_pause) {
        lv_label_set_text(s_anim_label, "PAUSE");
    } else if (show_idle_prompt) {
        lv_label_set_text_fmt(s_anim_label, "%s\n\nTAP (O) TO START", idle_face_text(now));
    } else {
        lv_label_set_text(s_anim_label, "RESET?");
    }
    lv_obj_align(s_anim_label, LV_ALIGN_CENTER, 0, 0);
    if (show_pause) {
        lv_obj_set_style_text_font(s_anim_label, &lv_font_montserrat_48, 0);
    } else {
        lv_obj_set_style_text_font(s_anim_label, &lv_font_montserrat_20, 0);
    }
    if (show_pause || show_reset || show_idle_prompt) {
        lv_obj_clear_flag(s_anim_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_anim_label, LV_OBJ_FLAG_HIDDEN);
    }

    lv_label_set_text(s_color_label, color_buf);
    lv_obj_align(s_color_label, LV_ALIGN_CENTER, 0, 54);
    if (s_palette_mode && !show_settings) {
        lv_obj_clear_flag(s_color_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_color_label, LV_OBJ_FLAG_HIDDEN);
    }

    lv_label_set_text(s_settings_title_label, "SETTINGS");
    lv_obj_align(s_settings_title_label, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_align(s_settings_qr, LV_ALIGN_CENTER, 0, -12);
    lv_label_set_text(s_settings_url_label, "http://" SETTINGS_AP_IP);
    lv_obj_align(s_settings_url_label, LV_ALIGN_BOTTOM_MID, 0, -18);
    if (show_settings) {
        lv_obj_clear_flag(s_settings_title_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_settings_qr, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_settings_url_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_settings_title_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_settings_qr, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_settings_url_label, LV_OBJ_FLAG_HIDDEN);
    }

    lv_label_set_text(s_version_label, APP_VERSION);
    lv_obj_align(s_version_label, LV_ALIGN_BOTTOM_MID, 0, -14);
    if (show_settings) {
        lv_obj_add_flag(s_version_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(s_version_label, LV_OBJ_FLAG_HIDDEN);
    }
    lvgl_port_unlock();

    backlight_apply();

    s_ui_dirty = false;
}

static void session_start(void)
{
    s_rows = 0;
    s_accumulated_active_ms = 0;
    s_session_started_ms = now_ms();
    s_active_started_ms = s_session_started_ms;
    s_session_state = SESSION_ACTIVE;
    s_screen_mode = SCREEN_MAIN;
    s_universal_short_count = 0;
    ESP_LOGI(TAG, "Session started");
    session_store_save();
    s_ui_dirty = true;
}

static void session_pause(void)
{
    if (s_session_state != SESSION_ACTIVE) {
        return;
    }
    s_accumulated_active_ms += now_ms() - s_active_started_ms;
    s_session_state = SESSION_PAUSED;
    ESP_LOGI(TAG, "Session paused");
    session_store_save();
    s_ui_dirty = true;
}

static void session_resume(void)
{
    if (s_session_state != SESSION_PAUSED) {
        return;
    }
    s_active_started_ms = now_ms();
    s_session_state = SESSION_ACTIVE;
    ESP_LOGI(TAG, "Session resumed");
    session_store_save();
    s_ui_dirty = true;
}

static void session_finish(int reason)
{
    if (s_session_state == SESSION_NONE) {
        return;
    }
    int duration_s = active_duration_s();
    history_add(s_rows, duration_s, reason);
    ESP_LOGI(TAG, "Session saved rows=%d duration_s=%d reason=%d", s_rows, duration_s, reason);
    s_session_state = SESSION_NONE;
    s_screen_mode = SCREEN_MAIN;
    s_rows = 0;
    s_accumulated_active_ms = 0;
    s_universal_short_count = 0;
    storage_clear_session();
    s_ui_dirty = true;
}

static void reset_request(void)
{
    if (s_session_state == SESSION_NONE) {
        return;
    }
    s_screen_mode = SCREEN_RESET_CONFIRM;
    s_reset_confirm_started_ms = now_ms();
    s_universal_short_count = 0;
    ESP_LOGI(TAG, "RESET_REQUEST");
    s_ui_dirty = true;
}

static void reset_confirm(void)
{
    if (s_screen_mode != SCREEN_RESET_CONFIRM) {
        return;
    }
    s_rows = 0;
    s_screen_mode = SCREEN_MAIN;
    session_store_save();
    ESP_LOGI(TAG, "Rows reset confirmed");
    s_ui_dirty = true;
}

static void palette_next(void)
{
    s_bg_palette_index = (s_bg_palette_index + 1) % UI_BG_PALETTE_COUNT;
    ESP_LOGI(TAG, "BG_COLOR #%06lX", (unsigned long)s_bg_palette[s_bg_palette_index]);
    s_ui_dirty = true;
}

static void palette_mode_toggle(void)
{
    s_palette_mode = !s_palette_mode;
    s_universal_short_count = 0;
    ESP_LOGI(TAG, "PALETTE_MODE_%s", s_palette_mode ? "ON" : "OFF");
    s_ui_dirty = true;
}

static void universal_short(void)
{
    if (s_screen_mode == SCREEN_SETTINGS) {
        return;
    }
    if (s_palette_mode) {
        palette_next();
        return;
    }

    int64_t now = now_ms();
    if (now - s_last_universal_short_ms > RESET_MULTI_CLICK_WINDOW_MS) {
        s_universal_short_count = 0;
    }
    s_last_universal_short_ms = now;
    s_universal_short_count++;

    if (s_screen_mode == SCREEN_RESET_CONFIRM) {
        reset_confirm();
        return;
    }
    if (s_session_state == SESSION_NONE) {
        session_start();
    } else if (s_session_state == SESSION_ACTIVE) {
        session_pause();
    } else {
        session_resume();
    }
}

static void universal_long(void)
{
    if (s_screen_mode == SCREEN_SETTINGS) {
        return;
    }
    int64_t now = now_ms();
    if (s_universal_short_count >= 3 && now - s_last_universal_short_ms <= RESET_MULTI_CLICK_WINDOW_MS) {
        reset_request();
        return;
    }
    session_finish(1);
}

static void plus_short(void)
{
    if (s_screen_mode != SCREEN_MAIN || s_session_state != SESSION_ACTIVE) {
        return;
    }
    s_rows++;
    ESP_LOGI(TAG, "PLUS_SHORT rows=%d", s_rows);
    session_store_save();
    s_ui_dirty = true;
}

static void minus_short(void)
{
    if (s_screen_mode != SCREEN_MAIN || s_session_state != SESSION_ACTIVE) {
        return;
    }
    if (s_rows > 0) {
        s_rows--;
    }
    ESP_LOGI(TAG, "MINUS_SHORT rows=%d", s_rows);
    session_store_save();
    s_ui_dirty = true;
}

static bool button_update(button_t *button, int64_t now, bool *short_event, bool *long_event)
{
    *short_event = false;
    *long_event = false;

    int raw = gpio_get_level(button->pin);
    if (raw != button->last_raw_level) {
        button->last_raw_level = raw;
        button->last_change_ms = now;
    }

    if (raw != button->stable_level && now - button->last_change_ms >= BUTTON_DEBOUNCE_MS) {
        button->stable_level = raw;
        if (raw == 0) {
            button->pressed_at_ms = now;
            button->long_reported = false;
            ESP_LOGI(TAG, "%s_DOWN", button->name);
        } else {
            int64_t held_ms = now - button->pressed_at_ms;
            ESP_LOGI(TAG, "%s_UP held_ms=%lld", button->name, (long long)held_ms);
            if (!button->long_reported && held_ms < button->long_ms) {
                *short_event = true;
            }
        }
        return true;
    }

    if (button->stable_level == 0 && !button->long_reported && now - button->pressed_at_ms >= button->long_ms) {
        button->long_reported = true;
        *long_event = true;
        ESP_LOGI(TAG, "%s_LONG", button->name);
        return true;
    }

    return false;
}

static void handle_power_button(int64_t now)
{
    static int prev_pwr = 1;
    static int64_t pwr_pressed_at_ms = 0;
    int pwr = gpio_get_level(PIN_PWR_SYS_OUT);

    if (prev_pwr == 1 && pwr == 0) {
        pwr_pressed_at_ms = now;
    }
    if (prev_pwr == 0 && pwr == 1) {
        int64_t held_ms = now - pwr_pressed_at_ms;
        if (held_ms >= BUTTON_LONG_MS) {
            ESP_LOGI(TAG, "POWER_LONG");
            session_store_save();
            storage_save_history(&s_history);
            wifi_portal_stop();
            gpio_set_level(PIN_PWR_SYS_EN, 0);
        }
    }
    prev_pwr = pwr;
}

static void app_task(void *arg)
{
    (void)arg;
    int battery_tick = 0;
    int ui_tick = 0;

    while (1) {
        int64_t now = now_ms();
        bool plus_short_event = false;
        bool plus_long_event = false;
        bool minus_short_event = false;
        bool minus_long_event = false;
        bool universal_short_event = false;
        bool universal_long_event = false;

        bool button_changed = false;
        button_changed |= button_update(&s_btn_plus, now, &plus_short_event, &plus_long_event);
        button_changed |= button_update(&s_btn_minus, now, &minus_short_event, &minus_long_event);
        button_changed |= button_update(&s_btn_universal, now, &universal_short_event, &universal_long_event);
        if (button_changed || plus_short_event || plus_long_event || minus_short_event || minus_long_event || universal_short_event || universal_long_event) {
            activity_mark(now);
        }

        bool plus_pressed = (s_btn_plus.stable_level == 0);
        bool minus_pressed = (s_btn_minus.stable_level == 0);
        bool universal_pressed = (s_btn_universal.stable_level == 0);
        static bool palette_combo_reported = false;
        static bool suppress_plus_release = false;
        static bool suppress_minus_release = false;
        static bool suppress_universal_release = false;

        if (plus_pressed && minus_pressed && universal_pressed && !palette_combo_reported) {
            palette_combo_reported = true;
            suppress_plus_release = true;
            suppress_minus_release = true;
            suppress_universal_release = true;
            palette_mode_toggle();
        }
        if (!plus_pressed || !minus_pressed || !universal_pressed) {
            palette_combo_reported = false;
        }

        if (plus_long_event && !minus_pressed && !universal_pressed && !s_palette_mode) {
            portal_toggle();
        }

        if (plus_short_event && suppress_plus_release) {
            suppress_plus_release = false;
        } else if (plus_short_event && !s_palette_mode) {
            plus_short();
        }

        if (minus_short_event && suppress_minus_release) {
            suppress_minus_release = false;
        } else if (minus_short_event && !s_palette_mode) {
            minus_short();
        }

        if (universal_short_event && suppress_universal_release) {
            suppress_universal_release = false;
        } else if (universal_short_event) {
            universal_short();
        }
        if (universal_long_event && !plus_pressed && !minus_pressed && !s_palette_mode) {
            universal_long();
        }

        handle_power_button(now);
        startup_melody_update(now);
        idle_face_update(now);

        if (s_settings.sleep_enabled && !s_display_dimmed && s_last_activity_ms > 0 &&
            now - s_last_activity_ms >= (int64_t)s_settings.sleep_timeout_s * 1000) {
            s_display_dimmed = true;
            s_ui_dirty = true;
            ESP_LOGI(TAG, "Display dimmed by sleep timeout");
        }

        if (s_screen_mode == SCREEN_RESET_CONFIRM && now - s_reset_confirm_started_ms >= RESET_CONFIRM_TIMEOUT_MS) {
            s_screen_mode = SCREEN_MAIN;
            ESP_LOGI(TAG, "Reset confirmation timed out");
            s_ui_dirty = true;
        }

        battery_tick += APP_TICK_MS;
        if (battery_tick >= 1000) {
            battery_tick = 0;
            s_bat_percent = battery_percent_read();
            s_ui_dirty = true;
        }

        ui_tick += APP_TICK_MS;
        if (ui_tick >= 250) {
            ui_tick = 0;
            if (s_session_state == SESSION_ACTIVE) {
                s_ui_dirty = true;
            }
        }
        if (s_ui_dirty) {
            ui_update();
        }

        vTaskDelay(pdMS_TO_TICKS(APP_TICK_MS));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting knitting assistant MVP firmware %s", APP_VERSION);

    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_err);
    session_restore_t restored_session;
    storage_load(&s_history, &s_settings, &restored_session);
    if (restored_session.active) {
        s_rows = restored_session.rows;
        s_accumulated_active_ms = restored_session.accumulated_active_ms;
        s_session_state = restored_session.state;
        s_session_started_ms = now_ms();
        s_active_started_ms = s_session_started_ms;
        ESP_LOGI(TAG, "Session restored rows=%d state=%d", s_rows, s_session_state);
    }

    power_latch_init();
    low_power_peripherals_init();
    buttons_init();
    adc_init_battery();

    ESP_ERROR_CHECK(lcd_init());
    ESP_ERROR_CHECK(lvgl_init_display());
    ui_create();
    s_boot_started_ms = now_ms();
    s_boot_until_ms = s_boot_started_ms + BOOT_ANIMATION_MS;
    s_last_activity_ms = s_boot_started_ms;
    backlight_apply();

    s_bat_percent = battery_percent_read();
    ui_update();

    xTaskCreate(app_task, "app_task", 6144, NULL, 5, NULL);
}
