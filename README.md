# embedded.cpp

Native ESP-IDF firmware port for https://github.com/polymech-info/polymech-firmware. 

Applications live in `applications/`, each one a CMake project built with `idf.py`. Two Guition boards are in `vendor/`: the JC-ESP32P4-M3-DEV module, and the 10.1-inch JC8012P4A1 panel.

`applications/hello-p4` is a serial playground for the M3-DEV. The panel’s preloaded desktop is the factory `esp_brookesia_phone` project, written up in [docs/JC8012P4A1.md](docs/JC8012P4A1.md). Research notes on other panels, DIN-rail I/O, and what might move from `audio.cpp` are in [docs/boards.md](docs/boards.md). How to build, flash, and talk to the M3-DEV is in [docs/guide.md](docs/guide.md).

```text
npm install
npm run test:hello-p4
```

That builds hello-p4, flashes the board, and checks ping, a Wi-Fi scan, and the ES8311 codec over serial.

| Path | Role |
|---|---|
| `applications/<name>` | Firmware we build and flash |
| `scripts/` | Node wrappers around `idf.py` and the serial tests |
| `vendor/JC-ESP32P4-M3-DEV` | Factory demos and pin maps for the module |
| `vendor/JC8012P4A1` | Factory phone desktop, schematic, and pins for the 10.1-inch panel |
| `docs/guide.md` | Toolchain, commands, serial protocol |
| `docs/boards.md` | Hardware survey. Not a build spec |
