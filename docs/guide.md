# Guide

Working notes for this tree. The hardware survey stays in [boards.md](boards.md).

## Toolchain

ESP-IDF **v6.1** is installed at `D:\sdk\espressif`. Cursor and VS Code call `idf.py`. The Eclipse Espressif-IDE is not part of this setup.

| | |
|---|---|
| ESP-IDF | `D:\sdk\espressif\v6.1\esp-idf` |
| Tools | `D:\sdk\espressif\tools` |
| Compiler | `riscv32-esp-elf-gcc` 15.2.0 |
| Python | `D:\sdk\espressif\tools\python\v6.1\venv` (3.12) |
| Activate | `. D:\sdk\espressif\Microsoft.v6.1.PowerShell_profile.ps1` |

ESP-IDF extension settings, if you point the IDE at this install instead of letting it download another copy:

- `idf.espIdfPath` = `D:\sdk\espressif\v6.1\esp-idf`
- `idf.toolsPath` = `D:\sdk\espressif\tools`
- `idf.pythonBinPath` = `D:\sdk\espressif\tools\python\v6.1\venv\Scripts\python.exe`

A new shell does not have `idf.py` until that profile is sourced. `scripts/idf-env.mjs` sources it for every npm script.

The target is `esp32p4`. ESP-IDF v6.1 defaults a new project to chip revision **v3.01**. The unit we flashed enumerates as **revision v1.0**. A v3.01 image will not boot on it. Every application `sdkconfig.defaults` needs:

```text
CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y
CONFIG_ESP32P4_REV_MIN_1=y
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_360=y
```

360 MHz is the clock to use. 400 MHz is not guaranteed before revision 3. Flash on this board is 16 MB QIO. Wi-Fi is the onboard ESP32-C6 over SDIO (ESP-Hosted), the same transport as the vendor `wifi_scan` demo.

## Layout

```text
embedded.cpp/
  applications/hello-p4/    playground we build
  scripts/                  idf.py and serial tests
  vendor/JC-ESP32P4-M3-DEV/ factory pack, read-only reference
  docs/boards.md            survey
```

An application is a directory under `applications/` with its own `CMakeLists.txt`. `npm run build` builds every such directory. Generated `sdkconfig`, `build/`, and `managed_components/` stay out of git. `sdkconfig.defaults` is the config we keep.

From `embedded.cpp`, after `npm install`:

| Command | Effect |
|---|---|
| `npm run build` | Build every application |
| `npm run build:hello-p4` | Build the playground |
| `npm run flash:hello-p4` | Build and flash |
| `npm run monitor:hello-p4` | Serial monitor |
| `npm run test:hello-p4` | Build, flash, ping, Wi-Fi scan, codec open |

The port is the only serial device, or an Espressif USB port when one is present. The board we used showed up as **COM3**, a WCH USB-UART (`wch.cn`). Set `EMBEDDED_PORT` when that guess is wrong.

```text
$env:EMBEDDED_PORT = "COM5"
npm run test:hello-p4
```

## hello-p4

The app stays running and reads one command per line on the console UART. Replies start with `@` so the test can ignore boot logs.

| Send | Device |
|---|---|
| `ping` | `@pong` |
| `wifi` | one `@wifi {"ssid","rssi"}` per network, then `@wifi {"done":true,"count":N,"status":"ok"}` |
| `audio` | `@audio {"status":"ok","codec":"es8311","stage":"open"}` |
| `roundtrip` | `@pong`, then wifi, then audio, then `@roundtrip {"wifi":"ok","audio":"ok"}` |

`npm run test:hello-p4` sends `ping`, then `roundtrip`, and fails unless both wifi and audio are `ok`. A scan that returns zero networks is still `ok`. A codec that does not ACK is `fail`.

On 7 November 2026 that test passed on the connected board: `Livebox6-EBCD`, `MOVISTAR_E488`, `TP-Link_B793`, and ES8311 open.

Wi-Fi init follows `vendor/.../NoDisplay/wifi_scan`. Audio follows the codec bring-up in `vendor/.../NoDisplay/mp3_player`, using the ESP32-P4 function-EV pin map shipped in that pack:

| Signal | GPIO |
|---|---|
| I2C SDA / SCL | 7 / 8 |
| I2S MCLK, BCLK, WS, DOUT, DIN | 13, 12, 10, 9, 48 |
| Power amp | 11 |

`ES8311_CODEC_DEFAULT_ADDR` is the 8-bit address `0x30`. `i2c_master_probe` wants the 7-bit address, so the probe uses `0x18`.

## Vendor pack

`vendor/JC-ESP32P4-M3-DEV` is the factory tree for this board. Build our apps, and read that tree when a pin or a demo already exists.

| Demo | Use it for |
|---|---|
| `1-Demo/IDF-DEMO/NoDisplay/wifi_scan` | C6 SDIO Wi-Fi scan |
| `1-Demo/IDF-DEMO/NoDisplay/mp3_player` | ES8311, I2S, speaker |
| `1-Demo/IDF-DEMO/NoDisplay/uart_echo_rs485` | RS485 UART echo |
| `1-Demo/IDF-DEMO/withDisplay/` | LVGL on the panel variants in the pack |
| `5-Schematic/schematic.pdf` | Pin check when a demo and the PCB disagree |

Those demos were generated around ESP-IDF 5.5 and 6.0. They are reference, not projects this npm tree builds.

## Adding an application

1. Create `applications/<name>/` with a top-level `CMakeLists.txt` that calls `project(...)`.
2. Put sources in `main/`, with `main/CMakeLists.txt` and, when a component registry package is required, `main/idf_component.yml`.
3. Copy the three revision and clock lines above into `sdkconfig.defaults`, plus 16 MB flash if it targets this board.
4. `npm run build` picks it up. A named script is optional. `node scripts/idf.mjs build <name>` and `node scripts/idf.mjs flash <name>` already work.
5. A serial test belongs in `scripts/test-<name>.mjs` and a `test:<name>` entry in `package.json`.

The first `idf.py` run on a new app executes `set-target esp32p4` because `sdkconfig` does not exist yet. Later builds reuse `sdkconfig`.

## Not in this tree yet

Portable pieces of `audio.cpp` (PCM types, levels, simple DSP, whistle detection) are the next firmware candidate. Microphone capture, the codec clock, and the network path have to be written against ESP-IDF. The desktop miniaudio, curl, ONNX, and Whisper graph does not belong on this chip. A small agent client, if we add one, would keep capture, a trigger, and a link to a Tanit host. Inference stays on that host.

Nothing here is a product decision. [boards.md](boards.md) records the other panels and I/O options that are still open.
