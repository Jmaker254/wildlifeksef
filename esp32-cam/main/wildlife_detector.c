/*
 * ============================================================
 * WILLARY ROBOTICS — Wildlife Detector
 * Phase 1: ESP32-CAM image capture + backend upload (ESP-IDF)
 * ============================================================
 * Board:    AI Thinker ESP32-CAM
 * Flow:     PIR / button trigger -> flash -> capture JPEG ->
 *           HTTP POST to Python backend -> GPT-4o Vision.
 * ============================================================
 */

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "app_config.h"
#include "camera.h"
#include "wifi_sta.h"
#include "http_upload.h"

static const char *TAG = "willary";

/* ── Status LED (active LOW on AI Thinker) ─────────────────── */
#define STATUS_ON()  gpio_set_level(STATUS_LED_PIN, 0)
#define STATUS_OFF() gpio_set_level(STATUS_LED_PIN, 1)

static void blink_status(int times)
{
    for (int i = 0; i < times; i++) {
        STATUS_ON();  vTaskDelay(pdMS_TO_TICKS(150));
        STATUS_OFF(); vTaskDelay(pdMS_TO_TICKS(150));
    }
}

/* ─────────────────────────────────────────────────────────── */
static void init_gpio(void)
{
    gpio_config_t out = {
        .pin_bit_mask = (1ULL << FLASH_LED_PIN) | (1ULL << STATUS_LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&out));

    gpio_config_t pir = {
        .pin_bit_mask = (1ULL << PIR_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&pir));

    gpio_config_t btn = {
        .pin_bit_mask = (1ULL << TRIGGER_BTN_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&btn));

    gpio_set_level(FLASH_LED_PIN, 0);
    STATUS_OFF();
}

/* ─────────────────────────────────────────────────────────── */
static void capture_and_send(void)
{
    ESP_LOGI(TAG, "capturing...");

    gpio_set_level(FLASH_LED_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(FLASH_DURATION_MS));

    /* Discard one stale frame — the OV2640's first frame after idle is
     * often underexposed / green-tinted, which throws off the AI. */
    camera_fb_t *warm = esp_camera_fb_get();
    if (warm) {
        esp_camera_fb_return(warm);
    }

    camera_fb_t *fb = esp_camera_fb_get();
    gpio_set_level(FLASH_LED_PIN, 0);

    if (!fb) {
        ESP_LOGE(TAG, "camera capture failed");
        blink_status(5);
        return;
    }

    ESP_LOGI(TAG, "image captured: %u bytes", (unsigned)fb->len);
    STATUS_ON();

    char resp[1024];
    bool ok = http_upload_jpeg(fb->buf, fb->len, resp, sizeof(resp));

    esp_camera_fb_return(fb);
    STATUS_OFF();

    if (ok) {
        ESP_LOGI(TAG, "== DETECTION RESULT ==\n%s\n======================", resp);
        blink_status(2);
    } else {
        ESP_LOGE(TAG, "send FAILED");
        blink_status(5);
    }
}

/* ─────────────────────────────────────────────────────────── */
static void trigger_task(void *arg)
{
    int64_t last_us = -(int64_t)CAPTURE_COOLDOWN_MS * 1000;

    blink_status(3);
    ESP_LOGI(TAG, "ready — waiting for trigger...");

    for (;;) {
        bool pir = gpio_get_level(PIR_PIN) == 1;
        bool btn = gpio_get_level(TRIGGER_BTN_PIN) == 0;   /* active LOW */

        if (pir || btn) {
            int64_t now_us = esp_timer_get_time();
            if (now_us - last_us > (int64_t)CAPTURE_COOLDOWN_MS * 1000) {
                last_us = now_us;
                ESP_LOGI(TAG, "%s trigger", btn ? "manual" : "PIR");

                if (!wifi_is_connected()) {
                    ESP_LOGW(TAG, "WiFi down — skipping capture");
                    blink_status(5);
                } else {
                    capture_and_send();
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
    }
}

/* ─────────────────────────────────────────────────────────── */
void app_main(void)
{
    ESP_LOGI(TAG, "== Willary Wildlife Detector — Phase 1 (ESP-IDF) ==");

    esp_err_t nvs = nvs_flash_init();
    if (nvs == ESP_ERR_NVS_NO_FREE_PAGES || nvs == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs);

    init_gpio();
    ESP_ERROR_CHECK(camera_init());

    if (wifi_init_sta() != ESP_OK) {
        ESP_LOGW(TAG, "continuing without network — captures will be skipped");
    }

    xTaskCreate(trigger_task, "trigger_task", 12288, NULL, 5, NULL);
}
