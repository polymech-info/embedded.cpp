#include "hello_wifi.h"

#include <stdio.h>
#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#define SCAN_LIST_SIZE 20

static const char *TAG = "hello_wifi";
static bool s_started;

static void json_escape(const uint8_t *in, char *out, size_t out_len)
{
    size_t j = 0;
    for (size_t i = 0; in[i] != 0 && j + 2 < out_len; i++) {
        unsigned char ch = in[i];
        if (ch < 0x20) {
            continue;
        }
        if (ch == '"' || ch == '\\') {
            if (j + 3 >= out_len) {
                break;
            }
            out[j++] = '\\';
        }
        out[j++] = (char)ch;
    }
    out[j] = 0;
}

static esp_err_t ensure_wifi(void)
{
    if (s_started) {
        return ESP_OK;
    }

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        err = nvs_flash_erase();
        if (err == ESP_OK) {
            err = nvs_flash_init();
        }
    }
    if (err != ESP_OK) {
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_wifi_start();
    if (err != ESP_OK) {
        return err;
    }

    s_started = true;
    return ESP_OK;
}

bool hello_wifi_scan(void)
{
    esp_err_t err = ensure_wifi();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wifi start failed: %s", esp_err_to_name(err));
        printf("@wifi {\"done\":true,\"count\":0,\"status\":\"fail\",\"reason\":\"%s\"}\n", esp_err_to_name(err));
        fflush(stdout);
        return false;
    }

    err = esp_wifi_scan_start(NULL, true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "scan failed: %s", esp_err_to_name(err));
        printf("@wifi {\"done\":true,\"count\":0,\"status\":\"fail\",\"reason\":\"%s\"}\n", esp_err_to_name(err));
        fflush(stdout);
        return false;
    }

    uint16_t ap_count = 0;
    err = esp_wifi_scan_get_ap_num(&ap_count);
    if (err != ESP_OK) {
        printf("@wifi {\"done\":true,\"count\":0,\"status\":\"fail\",\"reason\":\"%s\"}\n", esp_err_to_name(err));
        fflush(stdout);
        return false;
    }

    wifi_ap_record_t ap_info[SCAN_LIST_SIZE];
    uint16_t stored = SCAN_LIST_SIZE;
    memset(ap_info, 0, sizeof(ap_info));
    err = esp_wifi_scan_get_ap_records(&stored, ap_info);
    if (err != ESP_OK) {
        printf("@wifi {\"done\":true,\"count\":0,\"status\":\"fail\",\"reason\":\"%s\"}\n", esp_err_to_name(err));
        fflush(stdout);
        return false;
    }

    uint16_t shown = stored < ap_count ? stored : ap_count;
    for (uint16_t i = 0; i < shown; i++) {
        char ssid[96];
        json_escape(ap_info[i].ssid, ssid, sizeof(ssid));
        printf("@wifi {\"ssid\":\"%s\",\"rssi\":%d}\n", ssid, ap_info[i].rssi);
    }
    printf("@wifi {\"done\":true,\"count\":%u,\"status\":\"ok\"}\n", (unsigned)ap_count);
    fflush(stdout);
    return true;
}
