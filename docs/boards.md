# ESP32-P4, industrial HMI, remote I/O and audio/agent exploration

**Handover / research notes — 7 November 2026**  
**Status:** Exploratory. **No specific target architecture or implementation commitment.**  
**Context:** Polymech/Cassandra firmware, Tanit agent runtime and `audio.cpp`.

## 1. Where the discussion landed

We explored ESP32-P4-class hardware for a touchscreen-enabled device, initially around an extrusion controller, then industrial packaging, scalable remote I/O and finally audio/voice-agent capabilities. The current interest is **whether the hardware already obtained can run parts of `audio.cpp` and a lightweight agent stack**, but this handover is intended to preserve the findings, **not** to prescribe that as the goal.

Main takeaways:

- **ESP32-P4** is a viable native ESP-IDF C/C++ platform for a large touchscreen, Modbus, audio capture/playback and network forwarding.
- **ESP32-C6** is commonly paired with the P4 for wireless connectivity; it is not a replacement for the P4's main CPU.
- **LVGL 9** is a lightweight embedded graphical runtime, not an enclosure, industrial controller or desktop GUI dependency. **EEZ Studio** is a separate desktop designer; its **EEZ Flow runtime is optional**.
- An **ESP-IDF-only build** avoids the Arduino dependency chain that complicated previous firmware work, although specific peripherals and library source files still require checks.
- For **large DIN-rail I/O counts**, modular remote I/O (Modbus RTU or Modbus TCP) is more attractive than stacking small development-board relay modules.
- A subset of **`polymech-info/audio.cpp`** should be portable to P4; the complete desktop library and Tanit server/LLM inference stack should **not** be assumed to compile or run unchanged.
- A Linux panel is a credible alternative if more of Tanit's existing C++/audio dependencies must run directly on the device. It adds operating-system and deployment overhead.

## 2. Hardware already mentioned as obtained

### A. GUITION `JC-ESP32P4-M3-DEV` — ESP32-P4 + ESP32-C6 development board

AliExpress listing supplied by user:
https://es.aliexpress.com/item/1005009465140816.html

Product photo identified a board marked **`JC-ESP32P4-M3`**; the discussion identified the development-board designation as **`JC-ESP32P4-M3-DEV`**. Treat the printed marking and seller's exact SKU as more authoritative than generic listing text until verified against the PCB and pinout.

Reported/discussed characteristics:

- ESP32-P4 main processor; ESP32-C6 companion for Wi-Fi/BLE.
- Ethernet, USB connections, display connectors and expansion headers.
- A **4-pin RS485 connector was reported** in third-party documentation, but the exact pin assignment, transceiver, isolation and termination must be checked for the **specific board revision**.
- Development-board packaging: **not** an industrial 24 V DIN-rail controller, and not a sealed HMI.
- Onboard mic/speaker facilities were discussed; verify the exact codec and connectors on the actual PCB.

**Not to conflate:** This development board and the 10.1-inch display product below may be separate purchases and potentially separate P4-based boards. Their compatibility/relationship has **not been established solely from their names**.

### B. GUITION `JC8012P4A1` / `JC8012P4A1C` — 10.1-inch 800 × 1280 touchscreen product

User-described purchase: **“ESP32-P4 10,1 pulgadas 800*1280 IPS HMI módulo LCD pantalla Panel táctil JC8012P4A1 cámara WIFI Bluetooth para Arduino LVGL.”**

- **Exact seller model string:** `JC8012P4A1`.
- Some community material refers to **`JC8012P4A1C`**; **do not assume the A1 and A1C revisions are identical**.
- 10.1-inch portrait-oriented **800 × 1280 IPS** touch panel (orientation configurable by software).
- ESP32-P4 + companion wireless hardware as described in listings.
- Community references indicate **ES8311 audio codec**, microphone and speaker integration on at least certain variants; **verify the actual board's PCB revision, codec, microphone routing, speaker output and pin map**.
- High resolution matters: one full **RGB565** framebuffer at 800 × 1280 requires **2,048,000 bytes (~1.95 MiB)**; two require ~3.91 MiB, before LVGL draw buffers, assets and audio workspaces.

Community starting points discussed (evaluate revision/maintenance before adopting):

- `https://github.com/profi-max/JC8012P4A1_BSP_ESP32P4`
- `https://github.com/chickymonkey/guition-10.1-inch-esp32-p4`
- `https://github.com/winitboonkliang/windows95-esp32p4`

**The sale description says “for Arduino/LVGL”; that does not imply Arduino is required.** A native ESP-IDF port is possible with an appropriate board support package and audio/display drivers.

## 3. Other ESP32-P4 screens / panels explored

| Exact model / family | Type | Why it came up | Main qualification |
|---|---|---|---|
| **Waveshare `ESP32-P4-WIFI6-Touch-LCD-7B`** | 7-inch ESP32-P4 touchscreen board | 1024 × 600 capacitive touch, ESP-IDF/LVGL examples; onboard RS485 interface | Development-style packaging; enclosure and industrial wiring need attention |
| **Chipsee `EPC-ESP32P4-070`** | 7-inch embedded HMI | 1024 × 600, industrial-facing interfaces, voltage input, RS485 and I/O | Embedded unit, not necessarily a fully sealed final enclosure; verify options and BSP |
| **Chipsee `AIO-ESP32P4-101`** | 10.1-inch integrated industrial P4 HMI | More finished panel form factor; Ethernet/serial interfaces and relay | Not a large remote-I/O controller; check precise variant and Wi-Fi option |
| **Waveshare `ESP32-P4-86-Panel-ETH-2RO`** | Compact wall/panel device | Small touchscreen, Ethernet, RS485 and two relay outputs | Too small for some dashboards; not a high-channel-count DIN PLC |

Chipsee product entry:
https://chipsee.com/product/epc-esp32p4-070/

Waveshare docs/product starting point:
https://docs.waveshare.com/ESP32-P4-WIFI6-Touch-LCD-7B

Chipsee's factory-demo **UI engine was not confirmed**. The device can be developed using native ESP-IDF and LVGL, but factory firmware and BSP completeness should be examined separately.

## 4. Industrial controllers, DIN-rail I/O and Modbus

The original preference was for a **finished enclosure, external Wi-Fi antenna, screw terminals, relays and industrial 24 V wiring**, similar to:

- **Waveshare `RP2350-ETH-8DI-8RO`** — enclosed 8 digital-input / 8 relay-output controller:
  https://www.waveshare.com/product/esp32-related/industrial-controller/rp2350-eth-8di-8ro.htm
- **Waveshare `ESP32-S3-ETH-8DI-8RO`** — ESP32-S3-based industrial controller discussed as closer to the existing firmware ecosystem.

For considerably **more than 8 channels**, remote DIN-rail I/O options explored:

| Vendor / exact family | Connection approach | Notes |
|---|---|---|
| **ODOT C-Series remote I/O** | Coupler + stacked DIN-rail I/O slices; Modbus TCP kits available | 16/32-channel slices and mixed signal types; check chosen coupler protocol and slice limits |
| **WAGO 750 I/O System** | Modular terminal system; compatible Modbus RTU/TCP couplers | Mature industrial ecosystem, comparatively expensive; model-specific compatibility required |
| **WAGO `750-315`** | Modbus RTU coupler family discussed | Verify current model specifications and supported modules |
| **WAGO `750-362`** | Modbus TCP coupler | Ethernet, not RS485 RTU |
| **ICP DAS M-7000 series** | Independent DIN-rail RS485 Modbus RTU modules | Distributed field I/O, one address per module |
| **Advantech `ADAM-4055`** | RS485 digital remote I/O | 8 DI + 8 **transistor** DO, not mechanical relay outputs |
| **ICP DAS `M-7018Z`** | RS485 thermocouple/analog acquisition | Candidate for multi-zone temperature measurement |
| **Industrial Shields `moduleX`** | Modular expansion system | ESP32/industrial-oriented ecosystem; price per I/O may be higher |

Other module categories discussed: 16/32 digital inputs, 16/32 digital outputs or relays, RTD/PT100, thermocouple, 0–10 V, 4–20 mA and VFD Modbus interfaces. Exact cheap marketplace module SKUs were **not preserved reliably**, so do not treat those listings as selected parts.

For machine use, important distinctions:

- **DO transistor vs dry-contact relay:** not interchangeable. Transistor outputs can drive suitable interface relays/contactors.
- **Modbus RTU/RS485 vs Modbus TCP/Ethernet:** both valid, but require different couplers and integration.
- One RS485 multidrop segment can host multiple addressed slaves; check serial format, bus loading, termination, biasing, isolation and grounding.
- Verify **safe output behavior on communication loss**; it varies by module.
- Safety-related functions (emergency stop, independent overtemperature cutoff, protective interlocks) must not rely on an HMI, Wi-Fi or ordinary Modbus I/O alone.

**No final remote-I/O brand or bus was selected.**

## 5. Other platforms compared against ESP32-P4

| Model/family | Platform | Why consider it | Tradeoff |
|---|---|---|---|
| **Chipsee `PPC-A35-070`** | STM32MP257 / Linux, 7-inch panel | More usable C++/audio ecosystem, industrial housing | Yocto/Linux deployment and maintenance |
| **Chipsee `PPC-A55-070`** | RK3568 / Linux, 7-inch panel | More memory/CPU for rich UI, audio and Tanit client | Higher cost, OS complexity |
| **Chipsee `PPC-A76-070`** | RK3588 / Linux, 7-inch panel | Substantial compute for broader local workloads | Overkill for thin voice/HMI terminal |
| **TechNexion `TEP-0700-iMX6UL`** | NXP i.MX6UL Linux panel | Industrial longevity and embedded Linux | Older/weaker application CPU |
| **STM32H7**, **NXP i.MX RT1170** | High-performance MCUs | Credible non-ESP MCU alternatives | Different toolchains; not an obvious win for the current P4 hardware |

Takeaway: A Linux ARM panel is worth revisiting if a **large fraction of the existing Tanit desktop stack** must run locally. For **audio capture + processing + network forwarding + LVGL**, the P4 remains reasonable.

Prices mentioned previously were indicative listings at the time, **not purchasing quotes or verified current prices**.

## 6. UI software: native meaning and dependencies

- **ESP-IDF**: native Espressif SDK; application code in C/C++ with FreeRTOS and ESP-IDF peripherals.
- **LVGL 9**: native embedded GUI runtime (widgets, touch, charts, fonts, animations). No browser/HTML/JavaScript required. Needs display/touch integration and RAM-aware buffers.
- **EEZ Studio**: desktop GUI authoring tool, **not firmware runtime by itself**. Plain LVGL code generation can be used **without EEZ Flow**. EEZ Flow adds optional generated/runtime code for its logic engine.
- **SquareLine Studio**: alternative visual LVGL design tool.
- **ESP-Brookesia**: higher-level LVGL UI framework; explored but not essential.
- **PlatformIO / PIOArduino**: an option for P4, but the discussion favored a small dedicated **native ESP-IDF** target to avoid importing Arduino core dependencies with libraries.

The exact board BSP, pinout, ESP-IDF version and LVGL version must be validated together. A generic P4 build profile is not a substitute for a **JC8012P4A1 board-specific configuration**.

## 7. Existing repositories and integration context

- **Cassandra RC2 firmware:**
  https://github.com/polymech-info/polymech-firmware/tree/main/cassandra-rc2
- **Polymech audio library:**
  https://github.com/polymech-info/audio.cpp

Earlier discussion characterized Cassandra as an ESP32-S3/Arduino/PlatformIO-based machine stack with Modbus, REST and components such as extruder control, temperature regulation and VFD integration. It also mentioned a partial native ESP-IDF/P4 build scaffold. **Do not assume those scripts or the full native build are complete without a fresh repository review.**

Earlier review of `audio.cpp` noted portable C++ processing code alongside desktop-oriented device/dependency layers. For an ESP-IDF target, choose sources deliberately rather than compiling the entire desktop target.

## 8. `audio.cpp` on ESP32-P4: realistic partition

**Likely portable, subject to compile and benchmark:**

- PCM sample/frame types, fixed-size buffers and rings.
- Level, RMS, envelope and simple DSP operations.
- Lightweight resampling and FFT-based trigger processing.
- Existing whistle pattern detection.
- Session state, event routing and a small network transport client.

**Needs an ESP-IDF-specific implementation/adapter:**

- Microphone capture/speaker playback (I2S, board codec, DMA, clocking).
- Wi-Fi/Ethernet transport and authentication.
- Scheduling, memory allocation and synchronization.
- Echo cancellation / noise suppression / VAD backend (possibly Espressif ESP-SR; check actual P4 support and model resources).

**Not reasonable to assume locally on P4:**

- Whole desktop miniaudio/curl/ONNX/Whisper dependency graph.
- General-purpose local Whisper transcription or GGUF LLM inference.
- Full Tanit server, arbitrary desktop tools, OS-centric automation or all MCP/provider functionality.

Example memory figures for **16 kHz mono PCM16**: 20 ms = 640 bytes; 1 second = 32,000 bytes; 5 seconds = 160,000 bytes. This is small compared with PSRAM, but **DMA-capable internal memory, DSP working sets, LVGL buffers and stack sizes** may be the limiting resources.

**No successful `audio.cpp` ESP-IDF compile, on-device benchmark, full-duplex audio test or simultaneous LVGL/audio stress test was established in the discussion.** Feasibility is not a verified port.

## 9. Agent stack: possible scope, not a settled architecture

A small on-device client could conceivably manage:

- Audio capture / local VAD / wake or whistle trigger / playback.
- Persistent secure link to an existing Tanit host (e.g. WebSocket + raw PCM initially).
- Session lifecycle, reconnection, bounded queues, timestamps and telemetry.
- Remote-tool invocation and presentation of results.
- A limited deterministic local tool catalog, such as querying machine status or changing HMI pages.

The server-side Tanit stack would continue handling general LLM inference, speech recognition, TTS, complex tools, memory and orchestration unless deliberately re-engineered.

Possible separation:

```text
ESP32-P4 HMI / audio endpoint
  ├─ LVGL (optional for this audio-first exploration)
  ├─ audio.cpp embedded subset
  ├─ ESP-IDF codec + network adapters
  └─ lightweight agent/session client
               ⇅ Ethernet / Wi-Fi
Tanit server: STT / LLM / TTS / tools / orchestration
               ⇅ protocol / validated machine commands
Cassandra controller + independent industrial I/O
```

This is **one plausible topology**, not a decision. The present discussion has **no committed end product or final deployment architecture**.

## 10. Unresolved facts and useful verification points

1. **Board identity:** inspect PCB silkscreen and revision for `JC-ESP32P4-M3-DEV`, `JC8012P4A1`, and/or `JC8012P4A1C`; determine whether the development board and display are one integrated product or separate compatible hardware.
2. **Audio:** confirm codec (`ES8311` on relevant variant), microphone input wiring, bias, audio output/amplifier, playback reference for AEC and ESP-IDF driver support.
3. **Connectivity:** confirm physical RS485 transceiver on the exact board, A/B connector pinout, isolation, control pin and termination; verify external Wi-Fi antenna options if needed.
4. **Firmware BSP:** select maintained native ESP-IDF display/touch/audio examples for **exact** PCB revision; check compatibility with intended ESP-IDF/LVGL versions.
5. **`audio.cpp` dependencies:** identify portable compilation units, hidden miniaudio/JSON/OS dependencies, allocation behavior and PFFFT compatibility.
6. **Agent protocol:** decide whether reuse is at the transport/session API level or via a small embedded agent client; distinguish remote tool invocation from local execution.
7. **Profiling:** measure real internal RAM/PSRAM, full-duplex audio latency, AEC/VAD CPU, network jitter and 800 × 1280 LVGL performance together.
8. **I/O architecture:** no selection yet between RS485 remote I/O, Ethernet coupler racks or the existing S3 controller as master.

## 11. Source links / pointers

- User's AliExpress development-board listing: https://es.aliexpress.com/item/1005009465140816.html
- Chipsee `EPC-ESP32P4-070`: https://chipsee.com/product/epc-esp32p4-070/
- Waveshare `ESP32-P4-WIFI6-Touch-LCD-7B`: https://docs.waveshare.com/ESP32-P4-WIFI6-Touch-LCD-7B
- Waveshare `RP2350-ETH-8DI-8RO`: https://www.waveshare.com/product/esp32-related/industrial-controller/rp2350-eth-8di-8ro.htm
- Chipsee `PPC-A35-070`: https://chipsee.com/product/ppc-a35-070/
- Chipsee `PPC-A55-070`: https://chipsee.com/product/ppc-a55-070/
- `audio.cpp`: https://github.com/polymech-info/audio.cpp
- Cassandra RC2: https://github.com/polymech-info/polymech-firmware/tree/main/cassandra-rc2
- JC8012P4A1 community BSP: https://github.com/profi-max/JC8012P4A1_BSP_ESP32P4
- JC8012P4A1 voice/communications example: https://github.com/chickymonkey/guition-10.1-inch-esp32-p4

---

**Bottom line:** We have identified capable ESP32-P4 + ESP32-C6 hardware, including the GUITION development board and 10.1-inch touchscreen family, with plausible native ESP-IDF/LVGL/audio paths. Industrial alternatives, DIN-rail remote I/O families and Linux-based panel upgrades have been catalogued. The portability of `audio.cpp` and the lightweight agent-client concept looks promising, but **nothing here represents a verified full port, a final board selection or a fixed product goal**.


## Hardware

## ESP32P4 + ESP32-C6 ( JC-ESP32P4-M3-DEV )

Found several useful resources for your GUITION JC-ESP32P4-M3-DEV, including the actual manufacturer specifications, full schematics, GPIO mapping, native ESP-IDF demos and — particularly interesting for Tanit — a working voice-agent firmware project targeting this exact board.

## 1. Manufacturer specifications

[Zaujímavá vývojová doska za 22 € s modulom GUITION ESP32-P4 + ESP32-C6](https://images.openai.com/static-rsc-4/jqY4Bg3sVaxvFxQIODwQRrcc7AzB0rmk5upiJL0a6LxzPsB570HCCqSUOPQJPCVeS4b6a0iuNImFo7WLnegg3E-JuTKWWGzHQvOhlho7Ca1ER1L91EcgxbJYqKKU1yPzIP0O1Et4utmsWknGfVGeBfsLZvzkO9k3m2UBMgZCITo?purpose=inline)

GUITION JC-ESP32P4-M3-DEV

[Official product page](https://www.guition.com/esp32p4-display-module/esp32p4-display-module?utm_source=chatgpt.com)

[Manufacturer specification PDF](https://www.guition.com/icms/upload/fb081940d6fc11f09850077a33e1404f/file/productmanager-productfile/1d8749eb4c444726b7b85189aee66af3/Directory/JC-ESP32P4-M3-DEV%20Specifications-EN_1776241973799.pdf?utm_source=chatgpt.com)

| Specification        | Value                                     |
| -------------------- | ----------------------------------------- |
| Main processor       | ESP32-P4, dual-core RISC-V, up to 400 MHz |
| Wi-Fi / Bluetooth    | ESP32-C6 coprocessor                      |
| PSRAM                | 32 MB                                     |
| Flash                | 16 MB NOR                                 |
| Internal memory      | 768 KB HP L2MEM                           |
| Ethernet             | 100 Mbps, IP101 PHY                       |
| Industrial interface | RS485 via dedicated connector             |
| Audio                | Onboard microphone, speaker output        |
| Display              | MIPI-DSI                                  |
| Camera               | MIPI-CSI                                  |
| Power                | 5 V                                       |
| Board dimensions     | 92 × 62 mm                                |

Manufacturer and ESPHome hardware documentation.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://www.guition.com\&sz=32)

GUITION+1



## 2. GitHub — board documentation and firmware

[JC-ESP32P4-M3-DEV — full hardware package](https://github.com/DRubioG/JC-ESP32P4-M3-DEV?utm_source=chatgpt.com)

Most useful starting point. Contains `1-Demo`, specifications, PCB diagrams, chip datasheets, schematics, user manuals and flashing instructions.

Community mirror of hardware documentation and demos.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://github.com\&sz=32)

GitHub



[Complete board schematic](https://github.com/DRubioG/JC-ESP32P4-M3-DEV/blob/main/5-Schematic/schematic.pdf?utm_source=chatgpt.com)

Useful for tracing ES8311 audio, RS485/UART transceivers, SDIO connection to the ESP32-C6, power rails and GPIO assignments.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://github.com\&sz=32)

GitHub



[Unofficial Guition ESP32-P4 board collection](https://github.com/p1ngb4ck/unofficial_guition_esp32p4_repo?utm_source=chatgpt.com)

Includes pin definitions, manufacturer-provided documentation and ESPHome configuration examples. It also covers your separate `JC8012P4A1C` touchscreen family.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://github.com\&sz=32)

GitHub



[ESP-IDF demo and camera example](https://github.com/maohuynh-embedded/JC-ESP32P4-M3-DEV-FK?utm_source=chatgpt.com)

Fork with an `IDF-DEMO` directory and additional OV5647 camera development files.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://github.com\&sz=32)

GitHub



## 3. Most interesting for `audio.cpp` and Tanit

[ESP32-P4 native voice assistant firmware](https://github.com/dvucinozd/Home-Assistant-MQTT-Voice-Assistant?utm_source=chatgpt.com)

This is particularly relevant because it already targets JC-ESP32P4-M3-DEV, using ESP-IDF 5.5.

The author reports a functioning pipeline involving ESP-SR WakeNet9, local wake detection, VAD, WebSocket audio streaming, Home Assistant speech processing, TTS playback, MQTT and OTA.

It's a strong reference for the exact work you were considering with `audio.cpp`.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://github.com\&sz=32)

GitHub



[ESP32-P4 display and board support hub](https://github.com/embenix/ESP32-P4-DSI-Support-Hub?utm_source=chatgpt.com)

Includes the Guition M3 development board among its supported ESP32-P4 boards. Useful if you want to add the display without relying on Arduino.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://github.com\&sz=32)

GitHub



[ESPHome board pinout and configuration](https://devices.esphome.io/devices/guition-esp32-p4-m3-dev/?utm_source=chatgpt.com)

GPIO mapping for the C6 Wi-Fi coprocessor, IP101 Ethernet PHY and board peripherals, plus a minimal ESP-IDF-based configuration.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://devices.esphome.io\&sz=32)

ESPHome Devices



## 4. Important caveat: early silicon

One community BSP for the related Guition `JC4880P443C_I_W` reports an ESP32-P4 engineering-sample revision v1.3, requiring a particular build configuration.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://github.com\&sz=32)

GitHub



This does not establish that your own M3-DEV has engineering-sample silicon. However, it is worth checking its chip revision before choosing a bootloader or updating ESP-IDF:

```
esptool.py --chip esp32p4 chip_id
```

Also inspect the boot log for the reported chip revision.

For your immediate investigation, I'd start with DRubioG's hardware/demos repository and dvucinozd's native voice-agent firmware. Together, they're the most useful references for getting the actual GUITION board running with audio, Ethernet/Wi-Fi and a Tanit-compatible streaming client — without making Arduino a prerequisite.


## ESP32-P4 10,1 

pulgadas 800*1280 IPS HMI módulo LCD pantalla Panel táctil JC8012P4A1 cámara WIFI Bluetooth para Arduino LVGL

