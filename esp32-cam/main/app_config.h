/*
 * ============================================================
 * WILLARY ROBOTICS — Wildlife Detector (ESP-IDF)
 * User configuration — EDIT THIS FILE before building.
 * ============================================================
 */
#pragma once

#include "driver/gpio.h"
#include "esp_camera.h"

/* ── WiFi ─────────────────────────────────────────────────── */
#define WIFI_SSID       "YOUR_WIFI_NAME"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"
#define WIFI_MAX_RETRY  10

/* ── Backend server ───────────────────────────────────────── */
/* Your laptop's LAN IP while running backend/server.py        */
#define BACKEND_URL     "http://192.168.1.100:5000/detect"
#define HTTP_TIMEOUT_MS 20000       /* covers the GPT-4o round trip */

/* ── GPIO (AI Thinker ESP32-CAM) ──────────────────────────── */
#define PIR_PIN         GPIO_NUM_13 /* PIR motion sensor input        */
#define FLASH_LED_PIN   GPIO_NUM_4  /* Onboard white flash LED        */
#define STATUS_LED_PIN  GPIO_NUM_33 /* Onboard red LED — ACTIVE LOW   */
#define TRIGGER_BTN_PIN GPIO_NUM_14 /* Manual trigger btn, active LOW */

/* ── Timing ───────────────────────────────────────────────── */
#define CAPTURE_COOLDOWN_MS 5000    /* min gap between captures       */
#define FLASH_DURATION_MS   200     /* flash-on before capture        */
#define POLL_INTERVAL_MS    100     /* trigger poll period            */

/* ── Camera ───────────────────────────────────────────────── */
#define CAM_FRAMESIZE    FRAMESIZE_VGA  /* 640x480 — good for AI detection */
#define CAM_JPEG_QUALITY 10             /* 0-63, lower = better quality    */
#define CAM_FB_COUNT     2              /* needs PSRAM                     */
