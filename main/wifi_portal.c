#include "wifi_portal.h"

#include <stdlib.h>
#include <string.h>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"

#define SETTINGS_DNS_PORT 53

static const char *TAG = "wifi_portal";

static settings_store_t *s_settings = NULL;
static wifi_portal_settings_saved_cb_t s_on_saved = NULL;
static void *s_on_saved_ctx = NULL;
static bool s_wifi_started = false;
static bool s_portal_running = false;
static bool s_dns_running = false;
static httpd_handle_t s_http_server = NULL;
static TaskHandle_t s_dns_task = NULL;

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

static const char *checked_attr(bool value)
{
    return value ? " checked" : "";
}

static bool query_has_key(const char *query, const char *key)
{
    char value[8];
    return httpd_query_key_value(query, key, value, sizeof(value)) == ESP_OK;
}

static int query_int(const char *query, const char *key, int fallback)
{
    char value[16];
    if (httpd_query_key_value(query, key, value, sizeof(value)) != ESP_OK) {
        return fallback;
    }
    return atoi(value);
}

static esp_err_t settings_page_send(httpd_req_t *req)
{
    char page[1800];
    int len = snprintf(page, sizeof(page),
        "<!doctype html><html><head><meta charset='utf-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>KAST Settings</title>"
        "<style>body{font-family:system-ui,sans-serif;margin:0;background:#bfe3ff;color:#454449;}"
        "main{max-width:460px;margin:0 auto;padding:24px;}h1{font-size:34px;margin:0 0 8px;}"
        ".card{background:rgba(255,255,255,.68);border-radius:22px;padding:18px;margin:16px 0;}"
        "label{display:block;font-size:18px;margin:12px 0;}input[type=number],input[type=range]{width:100%%;}"
        "button{width:100%%;border:0;border-radius:18px;background:#454449;color:white;font-size:22px;padding:14px;}"
        ".hint{font-size:15px;opacity:.8}</style></head><body><main>"
        "<h1>Settings</h1>"
        "<form action='http://%s/save' method='get'>"
        "<div class='card'><label><input type='checkbox' name='boot_beep' value='1'%s> Boot beep</label></div>"
        "<div class='card'><label>Brightness: <span id='bv'>%u</span>%%</label>"
        "<input type='range' min='0' max='100' name='brightness' value='%u' oninput='bv.textContent=this.value'></div>"
        "<div class='card'><label><input type='checkbox' name='sleep' value='1'%s> Screen sleep</label>"
        "<label>Dim after, seconds<input type='number' min='5' max='3600' name='sleep_timeout' value='%u'></label></div>"
        "<button type='submit'>Save</button></form></main></body></html>",
        SETTINGS_AP_IP,
        checked_attr(s_settings->boot_beep_enabled),
        (unsigned)s_settings->brightness_pct,
        (unsigned)s_settings->brightness_pct,
        checked_attr(s_settings->sleep_enabled),
        (unsigned)s_settings->sleep_timeout_s);

    if (len < 0) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "render failed");
    }
    if (len >= (int)sizeof(page)) {
        len = sizeof(page) - 1;
    }
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, page, len);
}

static esp_err_t settings_save_handler(httpd_req_t *req)
{
    char query[160] = {0};
    ESP_LOGI(TAG, "Settings save request uri=%s", req->uri);
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        ESP_LOGI(TAG, "Settings save query=%s", query);
        s_settings->boot_beep_enabled = query_has_key(query, "boot_beep");
        s_settings->brightness_pct = clamp_int(query_int(query, "brightness", s_settings->brightness_pct), 0, 100);
        s_settings->sleep_enabled = query_has_key(query, "sleep");
        s_settings->sleep_timeout_s = clamp_int(query_int(query, "sleep_timeout", s_settings->sleep_timeout_s), 5, 3600);
        if (s_on_saved != NULL) {
            s_on_saved(s_settings, s_on_saved_ctx);
        }
        ESP_LOGI(TAG, "Settings saved boot_beep=%d brightness=%u sleep=%d sleep_timeout=%u",
                 s_settings->boot_beep_enabled, (unsigned)s_settings->brightness_pct,
                 s_settings->sleep_enabled, (unsigned)s_settings->sleep_timeout_s);
    }
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "http://" SETTINGS_AP_IP "/?saved=1");
    return httpd_resp_send(req, "Saved", HTTPD_RESP_USE_STRLEN);
}

static esp_err_t captive_handler(httpd_req_t *req)
{
    if (strstr(req->uri, "/save") != NULL) {
        return settings_save_handler(req);
    }
    return settings_page_send(req);
}

static void dns_task(void *arg)
{
    (void)arg;
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGW(TAG, "DNS socket create failed");
        s_dns_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    struct timeval timeout = { .tv_sec = 1, .tv_usec = 0 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(SETTINGS_DNS_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGW(TAG, "DNS bind failed");
        close(sock);
        s_dns_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    uint8_t buf[512];
    while (s_dns_running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int len = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *)&client_addr, &client_len);
        if (len <= 12) {
            continue;
        }

        int q_end = 12;
        while (q_end < len && buf[q_end] != 0) {
            q_end += buf[q_end] + 1;
        }
        if (q_end + 5 > len) {
            continue;
        }

        uint8_t resp[576];
        if (len + 16 > (int)sizeof(resp)) {
            continue;
        }
        memcpy(resp, buf, len);
        resp[2] = 0x81;
        resp[3] = 0x80;
        resp[6] = 0x00;
        resp[7] = 0x01;
        resp[8] = 0x00;
        resp[9] = 0x00;
        resp[10] = 0x00;
        resp[11] = 0x00;

        int pos = len;
        resp[pos++] = 0xC0;
        resp[pos++] = 0x0C;
        resp[pos++] = 0x00;
        resp[pos++] = 0x01;
        resp[pos++] = 0x00;
        resp[pos++] = 0x01;
        resp[pos++] = 0x00;
        resp[pos++] = 0x00;
        resp[pos++] = 0x00;
        resp[pos++] = 0x3C;
        resp[pos++] = 0x00;
        resp[pos++] = 0x04;
        resp[pos++] = 192;
        resp[pos++] = 168;
        resp[pos++] = 4;
        resp[pos++] = 1;
        sendto(sock, resp, pos, 0, (struct sockaddr *)&client_addr, client_len);
    }

    close(sock);
    s_dns_task = NULL;
    vTaskDelete(NULL);
}

esp_err_t wifi_portal_start(settings_store_t *settings, wifi_portal_settings_saved_cb_t on_saved, void *ctx)
{
    if (s_portal_running) {
        return ESP_OK;
    }
    s_settings = settings;
    s_on_saved = on_saved;
    s_on_saved_ctx = ctx;

    if (!s_wifi_started) {
        ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "esp_netif_init failed");
        ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "event loop failed");
        esp_netif_create_default_wifi_ap();
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "wifi init failed");
        s_wifi_started = true;
    }

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.ap.ssid, SETTINGS_AP_SSID, sizeof(wifi_config.ap.ssid));
    wifi_config.ap.ssid_len = strlen(SETTINGS_AP_SSID);
    wifi_config.ap.channel = 1;
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_AP), TAG, "wifi set mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &wifi_config), TAG, "wifi set config failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "wifi start failed");

    httpd_config_t server_config = HTTPD_DEFAULT_CONFIG();
    server_config.uri_match_fn = httpd_uri_match_wildcard;
    ESP_RETURN_ON_ERROR(httpd_start(&s_http_server, &server_config), TAG, "httpd start failed");
    httpd_uri_t save_handler = {
        .uri = "/save",
        .method = HTTP_GET,
        .handler = settings_save_handler,
        .user_ctx = NULL,
    };
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_http_server, &save_handler), TAG, "httpd save handler failed");
    httpd_uri_t handler = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = captive_handler,
        .user_ctx = NULL,
    };
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_http_server, &handler), TAG, "httpd handler failed");

    s_dns_running = true;
    xTaskCreate(dns_task, "dns_task", 4096, NULL, 4, &s_dns_task);
    s_portal_running = true;
    ESP_LOGI(TAG, "Settings AP started: %s http://%s", SETTINGS_AP_SSID, SETTINGS_AP_IP);
    return ESP_OK;
}

void wifi_portal_stop(void)
{
    if (!s_portal_running) {
        return;
    }
    s_dns_running = false;
    if (s_http_server != NULL) {
        httpd_stop(s_http_server);
        s_http_server = NULL;
    }
    esp_wifi_stop();
    s_portal_running = false;
    ESP_LOGI(TAG, "Settings AP stopped");
}

bool wifi_portal_is_running(void)
{
    return s_portal_running;
}
