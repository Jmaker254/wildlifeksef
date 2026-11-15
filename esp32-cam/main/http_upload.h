#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* POST a JPEG to BACKEND_URL as image/jpeg.
 * Up to (resp_cap - 1) bytes of the response body are copied into `resp`
 * (NUL-terminated). Returns true on HTTP 200. */
bool http_upload_jpeg(const uint8_t *data, size_t len,
                      char *resp, size_t resp_cap);
