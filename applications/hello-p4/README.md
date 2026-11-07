# hello-p4

Playground for the connected JC-ESP32P4-M3-DEV. Serial commands, not a product app.

From `embedded.cpp`:

```text
npm run test:hello-p4
```

That builds with ESP-IDF, flashes the board, then checks `ping` → `@pong` and `roundtrip` (Wi-Fi scan through the ESP32-C6, then ES8311 codec open).

| Send | Device |
|---|---|
| `ping` | `@pong` |
| `wifi` | `@wifi {"ssid","rssi"}` lines, then `@wifi {"done":true,"count":N,"status":"ok"}` |
| `audio` | `@audio {"status":"ok","codec":"es8311",...}` |
| `roundtrip` | `@pong`, then wifi, then audio, then `@roundtrip {"wifi":"ok","audio":"ok"}` |

Wi-Fi follows the vendor `wifi_scan` demo (ESP-Hosted SDIO to the C6). Audio follows the vendor `mp3_player` codec init: ES8311 on I2C GPIO7/8 and I2S 12/13/10/9/48. The flash port is the only serial device, or an Espressif USB port when one is present. Set `EMBEDDED_PORT` when that guess is wrong.
