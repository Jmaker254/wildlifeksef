idf_component_register(
    SRCS
        "wildlife_detector.c"
        "camera.c"
        "wifi_sta.c"
        "http_upload.c"
    INCLUDE_DIRS "."
    REQUIRES
        driver
        esp_timer
        esp_wifi
        esp_netif
        esp_event
        esp_http_client
        nvs_flash
)
# The espressif/esp32-camera dependency in idf_component.yml is added to the
# build automatically as a requirement of this component.
