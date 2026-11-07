#include "hello_audio.h"

#include <stdio.h>

#include "driver/i2c_master.h"
#include "driver/i2s_std.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "esp_log.h"

/*
 * Same ES8311 wiring as the vendor NoDisplay mp3_player BSP
 * (espressif__esp32_p4_function_ev_board): I2C GPIO7/8, I2S 12/13/10/9/48, PA GPIO11.
 */
#define AUDIO_I2C_PORT I2C_NUM_1
#define AUDIO_I2S_PORT I2S_NUM_1
#define AUDIO_I2C_SDA GPIO_NUM_7
#define AUDIO_I2C_SCL GPIO_NUM_8
#define AUDIO_I2S_MCLK GPIO_NUM_13
#define AUDIO_I2S_BCLK GPIO_NUM_12
#define AUDIO_I2S_WS GPIO_NUM_10
#define AUDIO_I2S_DOUT GPIO_NUM_9
#define AUDIO_I2S_DIN GPIO_NUM_48
#define AUDIO_PA_PIN GPIO_NUM_11

static const char *TAG = "hello_audio";
static bool s_up;

static void report(const char *status, const char *stage, esp_err_t err)
{
    if (err == ESP_OK) {
        printf("@audio {\"status\":\"%s\",\"codec\":\"es8311\",\"stage\":\"%s\"}\n", status, stage);
    } else {
        printf("@audio {\"status\":\"%s\",\"codec\":\"es8311\",\"stage\":\"%s\",\"reason\":\"%s\"}\n",
               status, stage, esp_err_to_name(err));
    }
    fflush(stdout);
}

bool hello_audio_check(void)
{
    if (s_up) {
        report("ok", "cached", ESP_OK);
        return true;
    }

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = AUDIO_I2C_PORT,
        .sda_io_num = AUDIO_I2C_SDA,
        .scl_io_num = AUDIO_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t i2c = NULL;
    esp_err_t err = i2c_new_master_bus(&bus_cfg, &i2c);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c bus: %s", esp_err_to_name(err));
        report("fail", "i2c", err);
        return false;
    }

    /* ES8311_CODEC_DEFAULT_ADDR is the 8-bit address (0x30). Probe wants 7-bit. */
    err = i2c_master_probe(i2c, ES8311_CODEC_DEFAULT_ADDR >> 1, 50);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "es8311 probe: %s", esp_err_to_name(err));
        report("fail", "probe", err);
        i2c_del_master_bus(i2c);
        return false;
    }

    i2s_chan_handle_t tx = NULL;
    i2s_chan_handle_t rx = NULL;
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(AUDIO_I2S_PORT, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    err = i2s_new_channel(&chan_cfg, &tx, &rx);
    if (err != ESP_OK) {
        report("fail", "i2s", err);
        i2c_del_master_bus(i2c);
        return false;
    }

    const i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = AUDIO_I2S_MCLK,
            .bclk = AUDIO_I2S_BCLK,
            .ws = AUDIO_I2S_WS,
            .dout = AUDIO_I2S_DOUT,
            .din = AUDIO_I2S_DIN,
        },
    };
    err = i2s_channel_init_std_mode(tx, &std_cfg);
    if (err == ESP_OK) {
        err = i2s_channel_init_std_mode(rx, &std_cfg);
    }
    if (err == ESP_OK) {
        err = i2s_channel_enable(tx);
    }
    if (err == ESP_OK) {
        err = i2s_channel_enable(rx);
    }
    if (err != ESP_OK) {
        report("fail", "i2s", err);
        return false;
    }

    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = AUDIO_I2S_PORT,
        .tx_handle = tx,
        .rx_handle = rx,
    };
    const audio_codec_data_if_t *data_if = audio_codec_new_i2s_data(&i2s_cfg);
    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();
    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = AUDIO_I2C_PORT,
        .addr = ES8311_CODEC_DEFAULT_ADDR,
        .bus_handle = i2c,
    };
    const audio_codec_ctrl_if_t *ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    if (data_if == NULL || gpio_if == NULL || ctrl_if == NULL) {
        report("fail", "codec-if", ESP_ERR_NO_MEM);
        return false;
    }

    es8311_codec_cfg_t es8311_cfg = {
        .ctrl_if = ctrl_if,
        .gpio_if = gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH,
        .pa_pin = AUDIO_PA_PIN,
        .pa_reverted = false,
        .master_mode = false,
        .use_mclk = true,
        .digital_mic = false,
        .invert_mclk = false,
        .invert_sclk = false,
        .hw_gain = {
            .pa_voltage = 5.0,
            .codec_dac_voltage = 3.3,
        },
    };
    const audio_codec_if_t *codec_if = es8311_codec_new(&es8311_cfg);
    if (codec_if == NULL) {
        report("fail", "es8311", ESP_FAIL);
        return false;
    }

    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_IN_OUT,
        .codec_if = codec_if,
        .data_if = data_if,
    };
    esp_codec_dev_handle_t dev = esp_codec_dev_new(&dev_cfg);
    if (dev == NULL) {
        report("fail", "codec-dev", ESP_FAIL);
        return false;
    }

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = 16000,
        .channel = 1,
        .bits_per_sample = 16,
    };
    err = esp_codec_dev_open(dev, &fs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "codec open: %s", esp_err_to_name(err));
        report("fail", "open", err);
        return false;
    }
    esp_codec_dev_set_out_vol(dev, 40);
    esp_codec_dev_set_in_gain(dev, 24.0);

    s_up = true;
    report("ok", "open", ESP_OK);
    return true;
}
