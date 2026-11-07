# embedded.cpp

Native ESP-IDF firmware port for https://github.com/polymech-info/polymech-firmware. 

Applications live in `applications/`, each one a CMake project built with `idf.py`. The connected board is a GUITION JC-ESP32P4-M3-DEV: ESP32-P4 plus an ESP32-C6 for Wi-Fi.

`applications/hello-p4` is a serial playground, not a product. Vendor demos under `vendor/` are reference code for that board. Research notes on other panels, DIN-rail I/O, and what might move from `audio.cpp` are in [docs/boards.md](docs/boards.md). How to build, flash, and talk to the board is in [docs/guide.md](docs/guide.md).

```text
npm install
npm run test:hello-p4
```

That builds hello-p4, flashes the board, and checks ping, a Wi-Fi scan, and the ES8311 codec over serial.

| Path | Role |
|---|---|
| `applications/<name>` | Firmware we build and flash |
| `scripts/` | Node wrappers around `idf.py` and the serial tests |
| `vendor/JC-ESP32P4-M3-DEV` | Factory demos and pin maps for the connected board |
| `docs/guide.md` | Toolchain, commands, serial protocol |
| `docs/boards.md` | Hardware survey. Not a build spec |
