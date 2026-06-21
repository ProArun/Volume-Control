# PotVolume — Hardware Volume Knob Daemon for Raspberry Pi 5 (AOSP 15)

An AOSP vendor daemon that reads a physical potentiometer via SPI and controls Android media volume in real time. Built entirely inside `vendor/myoem/` with no modifications to `frameworks/` or `device/`.

---

## Hardware Setup

```
Potentiometer (wiper) → MCP3008 CH0 → /dev/spidev0.0 → potvolumed
                                                              ↓
                                                       /dev/uinput
                                                              ↓
                                                  Android InputReader
                                                              ↓
                                                  PhoneWindowManager
                                                              ↓
                                                  AudioManager (STREAM_MUSIC)
```

**Components:**
- Potentiometer (10kΩ linear) wired to MCP3008 analog channel 0
- MCP3008 10-bit SPI ADC connected to RPi5 GPIO header (SPI0, CE0)
- `/dev/spidev0.0` — SPI character device (enabled via `dtparam=spi=on` in `/boot/config.txt`)

---

## How It Works

The daemon runs a 20 Hz poll loop:

1. **SpiReader** — reads a 10-bit ADC sample (0–1023) from the MCP3008 via a 3-byte SPI transaction
2. **AdcFilter** — applies a two-stage filter to eliminate noise:
   - Exponential Moving Average (α = 0.2) smooths rapid fluctuations
   - Dead zone (±8 counts) suppresses jitter when the knob is stationary
3. **VolumeMapper** — linearly maps the filtered ADC value to an `AUDIO_STREAM_MUSIC` index (0–15)
4. **VolumeController** — injects `KEY_VOLUMEUP` / `KEY_VOLUMEDOWN` events via a `uinput` virtual input device to reach the target index

---

## Why uinput Instead of libaudioclient

`libaudioclient` has no `image:vendor` variant in AOSP 15 — it is a system-only library that vendor binaries cannot link against. The `uinput` approach requires only kernel headers and `liblog`, avoids VNDK/LLNDK issues entirely, and routes through Android's standard input pipeline exactly as physical volume buttons do.

---

## Source Layout

```
potvolumed/
├── Android.bp                          — Soong build (cc_binary, vendor: true)
├── potvolumed.rc                       — init.rc service definition + spidev/uinput permissions
├── sepolicy/private/
│   ├── potvolumed.te                   — SELinux domain + spidev_device type
│   ├── file_contexts                   — labels /vendor/bin/potvolumed and /dev/spidev*
│   └── service_contexts                — empty (no Binder service registered)
└── src/
    ├── main.cpp                        — entry point, poll loop, signal handling
    ├── SpiReader.h / SpiReader.cpp     — spidev ioctl wrapper for MCP3008
    ├── AdcFilter.h / AdcFilter.cpp     — EMA + dead zone noise filter
    ├── VolumeMapper.h / VolumeMapper.cpp — ADC-to-volume-index linear mapping
    └── VolumeController.h / VolumeController.cpp — uinput virtual device + key injection
```

---

## Build & Install

```bash
lunch myoem_rpi5-trunk_staging-userdebug
m potvolumed
```

Add to `myoem_base.mk`:
```makefile
PRODUCT_PACKAGES               += potvolumed
PRODUCT_SOONG_NAMESPACES       += vendor/myoem/services/potvolumed
PRODUCT_PRIVATE_SEPOLICY_DIRS  += vendor/myoem/services/potvolumed/sepolicy/private
```

---

## Key Technical Details

| Topic | Detail |
|---|---|
| SPI config | Mode 0 (CPOL=0, CPHA=0), 1 MHz, 8 bits/word — within MCP3008's 3.6 MHz max |
| MCP3008 protocol | 3-byte transaction: `0x01`, `0x80\|(ch<<4)`, `0x00`; result in `rx[1][1:0]` + `rx[2]` |
| ADC range | 0–1023 (10-bit, 0V–3.3V) |
| Volume steps | 16 steps (AUDIO_STREAM_MUSIC index 0–15); each step ≈ 64 ADC counts |
| uinput device name | `PotVolume Knob` (visible in `/proc/bus/input/devices`) |
| Boot behaviour | First reading sets the reference index silently — no volume jump at startup |
| SELinux | `/dev/uinput` uses `uhid_device` label in AOSP 15 (not `uinput_device`); `/dev/spidev*` uses a custom `spidev_device` type declared in vendor policy |
| spidev permissions | Kernel creates as `root:root 0600`; RC file `chown root system` + `chmod 0660` at `on boot` |

---

## Environment

| Item | Value |
|---|---|
| Device | Raspberry Pi 5 |
| AOSP branch | `android-15.0.0_r14` |
| Build system | Soong (Android.bp) |
| Language | C++17 |
| Dependencies | `liblog`, Linux kernel headers only |
