#pragma once
#include "esp_err.h"

/* Initialise the OV2640 in JPEG mode. Call once at startup. */
esp_err_t camera_init(void);
