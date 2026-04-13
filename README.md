# ESP32 Hub

A fully local smart home hub built on **ESPHome** that integrates voice control, environmental sensors, BLE proxy for SwitchBot devices, and WiFi-based presence detection — entirely without cloud services.

---

## Architecture

```
[ESP32-C6]  ──UART──  [ESP32-S3 Main Hub]  ──WiFi──  Home Assistant
 Thread node            Voice · BLE · Sensors
 SHT31

[ESP32-C3]  ──WiFi──  Home Assistant
 ESPectre (presence detection via WiFi CSI)
```

### Configuration files

| File | Chip | Role |
|---|---|---|
| `hub_main.yaml` | ESP32-S3 N16R8 | Main hub: wake word, voice assistant, BLE proxy, BME280, BH1750, SSD1306, MAX98357A, INMP441 |
| `hub_thread_node.yaml` | ESP32-C6 | Thread node: SHT31 over UART → S3 |
| `hub-espectre-c3.yaml` | ESP32-C3 | Presence detection via WiFi CSI (ESPectre) |
| `switchbot.cpp` | Any ESP32 | SwitchBot BLE → MQTT bridge (Arduino/PlatformIO, optional) |

---

## Pin mapping

```
┌─────────────────────────── ESP32-S3 ──────────────────────────────────┐
│  GPIO8/9      → I2C bus: BME280(0x76) + BH1750(0x23) + SSD1306(0x3C)  │
│  GPIO10/11/12 → INMP441 microphone (I2S)                              │
│  GPIO13/14/15 → MAX98357A speaker (I2S)                               │
│  GPIO16/17    → UART ↔ ESP32-C6 (TX/RX crossed)                       │
│  GPIO4        → DND button (toggle)                                   │
│  GPIO5        → Vol+ button                                           │
│  GPIO6        → Vol- button                                           │
│  GPIO7        → Assistant button (single press)                       │
│  GPIO48       → WS2812B status LED (on-board)                         │
└───────────────────────────────────────────────────────────────────────┘
                        UART (2 wires + GND)
┌─────────────────────────── ESP32-C6 ──────────────────────────────┐
│  GPIO2/3   → I2C: SHT31 or BME280                                 │
│  GPIO16/17 → UART ↔ ESP32-S3                                      │
│  GPIO9     → Local test button                                    │
└───────────────────────────────────────────────────────────────────┘
```

> **UART wiring (crossed):** S3 TX17 → C6 RX17, S3 RX16 ← C6 TX16, common GND required.

### WS2812B wiring (if external)

```
┌─────────────────────────────────────────────────────────────────┐
│ ESP32-S3          WS2812B LED                                   │
│ ─────────         ──────────                                    │
│ GPIO48 ──[300Ω]──► DIN  (Data In)                               │
│ 3.3V or 5V ──────► VCC  (5V preferred for brighter colors)      │
│ GND ─────────────► GND                                          │
└─────────────────────────────────────────────────────────────────┘
```

---

## LED states

| State | Color | Effect |
|---|---|---|
| Idle | Soft blue | Slow breathing |
| Listening | Bright violet | Solid |
| DND | Orange | Slow pulse |
| No WiFi / HA disconnected | Red | Fast pulse |
| Active / TTS playing | Green | Solid |

### OLED face expressions

| State | Face |
|---|---|
| Idle | Half-moon eyes (sleepy), neutral mouth |
| Listening | Wide eyes with pupils, open "O" mouth |
| DND | ✕ eyes, downturned mouth |
| No WiFi | Spiral eyes (dazed), zigzag mouth |
| Active | Star eyes, curved smile |

### LED state machine

| Event | LED script |
|---|---|
| Boot | `led_stato_no_wifi` (red pulse) |
| WiFi connected | `led_stato_riposo` (blue breathe) |
| HA connected | `led_stato_riposo` |
| Wake word detected | `led_stato_ascolto` (violet) |
| STT complete | `led_stato_attivo` (green) |
| TTS start | `led_stato_attivo` |
| Session end | `led_stato_riposo` (if DND off) |
| DND on | `led_stato_dnd` (orange) |
| DND off | `led_stato_riposo` |

---

## Bill of Materials

### ESP32-S3 (Main Hub)

| Component | Recommended model | ~Price |
|---|---|---|
| MCU | ESP32-S3-DevKitC-1 N16R8 | €8–12 |
| Temp/Humidity/Pressure | BME280 breakout I2C | €3–5 |
| Light sensor | BH1750 breakout I2C | €2–3 |
| Display | SSD1306 OLED 128×64 I2C | €3–5 |
| Microphone | INMP441 I2S module | €3–4 |
| Amplifier | MAX98357A I2S DAC/Amp | €2–3 |
| Speaker | 4Ω 3W, 40–50mm | €2–4 |
| RGB LED | WS2812B single or 1-LED strip | €1–2 |
| Buttons | 6×6mm PCB momentary ×4 | €1 |
| Resistors | 300Ω 1/4W ×2 (LED data + protection) | <€1 |
| Power supply | USB 5V 2A (minimum) | €5–8 |
| **S3 subtotal** | | **~€30–47** |

### Power budget (ESP32-S3 hub)

The ESP32-S3-DevKitC-1 uses an on-board **LDO** (AMS1117-3.3) to step down USB 5 V to 3.3 V.
A linear regulator passes roughly the **same current** on both sides (efficiency ≈ V_out / V_in = 66%).
The MAX98357A and WS2812B are wired directly to the 5 V USB rail.

#### Per-component estimate

| Component | Rail | Typical (mA) | Peak (mA) | Notes |
|---|---|---|---|---|
| ESP32-S3 (WiFi + BLE) | 3.3 V | 250 | 500 | RF bursts |
| SSD1306 OLED 128×64 | 3.3 V | 20 | 30 | display fully on |
| BME280 | 3.3 V | 1 | 1 | |
| BH1750 | 3.3 V | 0.2 | 0.2 | |
| INMP441 | 3.3 V | 1 | 1.4 | |
| **3.3 V subtotal** | 3.3 V | **~272** | **~533** | |
| → drawn from 5 V via LDO | 5 V | **~272** | **~533** | same current, more heat in LDO |
| MAX98357A | 5 V | 2 | 680 | peak = 3 W into 4 Ω at ~88 % eff. |
| WS2812B (single) | 5 V | 20 | 60 | full white (R+G+B all on) |
| **Total from USB 5 V** | 5 V | **~294** | **~1 273** | |
| **Total power** | | **~1.5 W** | **~6.4 W** | |

> Peak = all components maxed simultaneously (worst case, rarely happens in practice).

#### Realistic operating scenarios

| Scenario | 5 V current | Power | Notes |
|---|---|---|---|
| Idle (WiFi connected, no audio, display on) | ~320 mA | ~1.6 W | typical background load |
| Wake word detected + voice reply | ~450 mA | ~2.3 W | S3 at ~300 mA + 0.5 W audio |
| Loud TTS playback (full volume) | ~800 mA | ~4.0 W | S3 + amp at ~1.5 W |
| Absolute worst case (all peaks) | ~1 300 mA | ~6.5 W | unrealistic continuous load |

#### PSU recommendation

| Supply | Max current | Suitable for |
|---|---|---|
| USB 5 V / 2 A (10 W) | 2 000 mA | Normal use — idle + voice assistant at moderate volume |
| USB 5 V / 3 A (15 W) | 3 000 mA | **Recommended** — comfortable headroom, loud audio without voltage sag |
| USB 5 V / 5 A (25 W) | 5 000 mA | Over-spec; useful only if adding more LEDs or peripherals |

> **Note:** Cheap USB chargers often cannot sustain their rated current; choose a quality brand (Anker, MEAN WELL, etc.) to avoid brownouts during WiFi transmission or audio playback.

### ESP32-C6 (Thread Node)

| Component | Recommended model | ~Price |
|---|---|---|
| MCU | ESP32-C6-DevKitC-1 | €5–8 |
| Temp/Humidity | SHT31 breakout I2C (or BME280) | €4–6 |
| RGB LED | WS2812B single | €1 |
| Resistor | 300Ω ×1 (LED data) | <€1 |
| Cables | Dupont M-M 4-pin (UART + GND) | €1 |
| **C6 subtotal** | | **~€11–16** |

### ESP32-C3 (Presence, optional)

| Component | Model | ~Price |
|---|---|---|
| MCU | ESP32-C3 SuperMini | €3–5 |
| Power supply | USB-A/C 5V 500mA | €3–5 |
| USB cable | Micro-USB or USB-C | <€1 |
| **C3 subtotal** | | **~€6–11** |

**Estimated total: ~€47–74**

---

## Software requirements

- Python **3.10+**
- ESPHome **2024.x+**
- Home Assistant with a configured **Assist** pipeline (for voice assistant)

---

## Environment setup

### 1. Create the virtual environment

It is strongly recommended to isolate ESPHome in a virtual environment to avoid conflicts with other system-level Python packages.

```powershell
# From the project folder
python -m venv .venv

# Activate (Windows PowerShell)
.venv\Scripts\Activate.ps1

# Activate (Windows CMD)
.venv\Scripts\activate.bat

# Activate (Linux / macOS)
source .venv/bin/activate
```

Once activated, the prompt will show `(.venv)` at the beginning.

### 2. Install ESPHome

```powershell
pip install esphome
```

To upgrade ESPHome in the future:

```powershell
pip install --upgrade esphome
```

---

## Secrets configuration

Copy the template and fill in your credentials:

```powershell
copy secrets.yaml.example secrets.yaml   # Windows
cp secrets.yaml.example secrets.yaml     # Linux/macOS
```

Open `secrets.yaml` and fill in all fields:

```yaml
wifi_ssid: "YourWiFiSSID"
wifi_password: "YourWiFiPassword"
ap_password: "FallbackAPPassword"       # emergency access point
api_key: ""                             # generate with: esphome generate-api-key
ota_password: "YourOTAPassword"
```

> `secrets.yaml` is already in `.gitignore` — it will never be committed.

---

## Build and install

### First flash (via USB)

With the ESP32 connected via USB:

```powershell
# Main hub (S3)
.venv\Scripts\python -m esphome run hub_main.yaml

# Thread node (C6)
.venv\Scripts\python -m esphome run hub_thread_node.yaml

# Presence node (C3)
.venv\Scripts\python -m esphome run hub-espectre-c3.yaml
```

`run` compiles, flashes, and opens the serial monitor in one command.

#### New / brand-new device (first-time flashing)

A factory-fresh ESP32-S3 will not enter download mode automatically. If `esptool` reports `Failed to connect to ESP32-S3: No serial data received`, follow these steps:

1. Hold the **BOOT** button on the board.
2. While holding BOOT, press and release the **RESET** (RST) button.
3. Release **BOOT** — the chip is now in download mode.
4. Run the `esphome run` command again and select the COM port when prompted.

After `esptool` finishes writing the flash it tries to reset the chip via the RTS pin and the log may stall at:

```
Hard resetting via RTS pin...
INFO Successfully uploaded program.
INFO Starting log output from COMx with baud rate 115200
```

If the serial output stays silent, **press the RESET button once** to reboot the device. ESPHome will then start streaming the device logs normally.

> Subsequent OTA updates do not require physical access to the board.

### Compile only (without flashing)

```powershell
.venv\Scripts\python -m esphome compile hub_main.yaml
```

### OTA updates (Over The Air)

After the first USB flash, no cable is needed. ESPHome discovers the device on the network via mDNS:

```powershell
.venv\Scripts\python -m esphome run hub_main.yaml
```

### Monitor logs in real time

```powershell
.venv\Scripts\python -m esphome logs hub_main.yaml
```

---

## Troubleshooting build cache

ESPHome keeps a compilation cache under `.esphome/`. After significant configuration changes (framework switch, adding C++ components, modifying `sdkconfig_options`, or editing included `.h` files), the cache can become stale and produce hard-to-diagnose build errors.

### Clean a single device's cache

```powershell
.venv\Scripts\python -m esphome clean hub_main.yaml
```

Removes only the build directory for the specified device. Downloaded toolchains and libraries are preserved, so the next build is faster than a full clean.

### Full clean (clean-all)

```powershell
.venv\Scripts\python -m esphome clean-all hub_main.yaml
```

Removes **everything** under `.esphome/build/` for that device, including all generated sources and compiled objects. The next compilation starts from scratch and will be slower, but guarantees a consistent state.

**When to use it:**
- Unexplained `undefined reference` or `multiple definition` linker errors
- After modifying `sdkconfig_options`
- After adding or removing `external_components`
- After modifying `.h` files referenced via `esphome.includes`

---

## Project structure

```
esp32_hub/
├── hub_main.yaml           # S3 hub: voice, BLE proxy, sensors, display
├── hub_thread_node.yaml    # C6 node: UART → S3 bridge
├── hub-espectre-c3.yaml    # C3 node: WiFi CSI presence detection
├── switchbot.cpp           # SwitchBot BLE→MQTT bridge (PlatformIO)
├── hub_includes.h          # C++ headers for ESPHome lambdas
├── esp32_buttons.yaml      # Button definitions (reference/include)
├── esp32_display.yaml      # Display logic (reference/include)
├── esp32_uart.yaml         # UART definitions (reference/include)
├── BOM                     # Bill of Materials with pricing
├── schema                  # Pin map, LED states, wiring diagrams
├── secrets.yaml            # Local credentials (git-ignored)
└── secrets.yaml.example    # Credentials template (safe to commit)
```

