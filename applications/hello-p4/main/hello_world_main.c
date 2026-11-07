#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "hello_audio.h"
#include "hello_wifi.h"

static void trim(char *line)
{
    size_t length = strlen(line);
    while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r' || line[length - 1] == ' ')) {
        line[--length] = 0;
    }
    char *start = line;
    while (*start == ' ') {
        start++;
    }
    if (start != line) {
        memmove(line, start, strlen(start) + 1);
    }
}

static void cmd_roundtrip(void)
{
    printf("@pong\n");
    fflush(stdout);
    bool wifi_ok = hello_wifi_scan();
    bool audio_ok = hello_audio_check();
    printf("@roundtrip {\"wifi\":\"%s\",\"audio\":\"%s\"}\n", wifi_ok ? "ok" : "fail", audio_ok ? "ok" : "fail");
    fflush(stdout);
}

static void dispatch(char *line)
{
    trim(line);
    if (line[0] == 0) {
        return;
    }
    if (strcmp(line, "ping") == 0) {
        printf("@pong\n");
    } else if (strcmp(line, "wifi") == 0) {
        hello_wifi_scan();
    } else if (strcmp(line, "audio") == 0) {
        hello_audio_check();
    } else if (strcmp(line, "roundtrip") == 0) {
        cmd_roundtrip();
        return;
    } else {
        printf("@error {\"reason\":\"unknown\"}\n");
    }
    fflush(stdout);
}

void app_main(void)
{
    printf("@ready\n");
    fflush(stdout);

    char line[160];
    while (true) {
        if (fgets(line, sizeof(line), stdin) == NULL) {
            vTaskDelay(pdMS_TO_TICKS(50));
            clearerr(stdin);
            continue;
        }
        dispatch(line);
    }
}
