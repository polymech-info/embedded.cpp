idf_component_register(
    SRCS "hello_world_main.c" "hello_wifi.c" "hello_audio.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_event esp_wifi esp_driver_i2c esp_driver_i2s esp_codec_dev
)
