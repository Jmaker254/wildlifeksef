#include "http_upload.h"
#include "app_config.h"

#include <string.h>

#include "esp_http_client.h"
#include "esp_log.h"

static const char *TAG = "http";

typedef struct {
    char  *buf;
    size_t cap;
    size_t len;
} resp_ctx_t;

static esp_err_t on_http_event(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->user_data) {
        resp_ctx_t *ctx = (resp_ctx_t *)evt->user_data;
        if (ctx->cap == 0) {
            return ESP_OK;
        }
        size_t room = ctx->cap - 1 - ctx->len;
        size_t n = (evt->data_len > 0 && (size_t)evt->data_len < room)
                       ? (size_t)evt->data_len
                       : room;
        if (n > 0) {
            memcpy(ctx->buf + ctx->len, evt->data, n);
            ctx->len += n;
            ctx->buf[ctx->len] = '\0';
        }
    }
    return ESP_OK;
}

bool http_upload_jpeg(const uint8_t *data, size_t len,
                      char *resp, size_t resp_cap)
{
    resp_ctx_t ctx = { .buf = resp, .cap = resp_cap, .len = 0 };
    if (resp && resp_cap) {
        resp[0] = '\0';
    }

    esp_http_client_config_t config = {
        .url           = BACKEND_URL,
        .method        = HTTP_METHOD_POST,
        .timeout_ms    = HTTP_TIMEOUT_MS,
        .event_handler = on_http_event,
        .user_data     = &ctx,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "esp_http_client_init failed");
        return false;
    }

    esp_http_client_set_header(client, "Content-Type", "image/jpeg");
    esp_http_client_set_post_field(client, (const char *)data, len);

    ESP_LOGI(TAG, "POST %u bytes -> %s", (unsigned)len, BACKEND_URL);
    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "perform failed: %s", esp_err_to_name(err));
        return false;
    }
    ESP_LOGI(TAG, "HTTP status %d", status);
    return status == 200;
}
