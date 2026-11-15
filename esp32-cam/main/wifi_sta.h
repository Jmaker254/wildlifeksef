#pragma once
#include <stdbool.h>
#include "esp_err.h"

/* Bring up WiFi in station mode and block until connected (or give up
 * after WIFI_MAX_RETRY). Returns ESP_OK once an IP is obtained. */
esp_err_t wifi_init_sta(void);

/* True while the station currently holds an IP. */
bool wifi_is_connected(void);
