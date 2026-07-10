# Contributing to KAST

Thank you for considering a contribution to KAST.

KAST is a portable knitting assistant based on the
`Waveshare ESP32-S3-LCD-1.69` board. The firmware is built with ESP-IDF for the
`esp32s3` target. Arduino and PlatformIO are not used in this repository.

## Before You Start

- Read [`README.md`](README.md) for the current device behavior and build steps.
- Read [`PROJECT_SPEC.md`](PROJECT_SPEC.md) for product requirements.
- Use the term `rows` in UI text, code, issues, and documentation.
- Keep changes focused and small enough to review.

## Development Environment

Use PowerShell 5.1 from the repository root and activate the ESP-IDF profile:

```powershell
. "C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1"
```

Build the firmware:

```powershell
idf.py build
```

Flash the current development board, when hardware verification is needed:

```powershell
idf.py -p COM14 build flash
```

Open logs:

```powershell
idf.py -p COM14 monitor
```

Exit the monitor with `Ctrl+]`.

## Firmware Version

The firmware version is set in the root [`CMakeLists.txt`](CMakeLists.txt) using
`PROJECT_VER`, for example `0.0.1-dev.37`.

Before flashing a working board, increment the dev counter by `1` so the version
shown at the bottom of the screen identifies the running build.

## Hardware Notes

Confirmed external buttons are active low and connected between GPIO and `GND`:

| Button | GPIO | Purpose |
| --- | --- | --- |
| `+` | `GPIO2` | Add one row |
| `-` | `GPIO16` | Remove one row |
| Universal | `GPIO17` | Start, pause, resume, reset, and secondary actions |

Battery measurement uses `GPIO1`, which maps to `ADC_CHANNEL_0` on ESP32-S3.
The board divider is `R3=200k` and `R7=100k`, with voltage multiplier `3`.

## Verification

At minimum, run:

```powershell
idf.py build
```

After flashing hardware, verify:

- the screen turns on;
- the expected `0.0.1-dev.N` version is visible at the bottom;
- `+` and `-` change the row counter;
- the universal button controls the expected session/reset behavior;
- the battery is displayed as a percentage.

For battery diagnostics, check monitor logs for:

```text
Battery ADC raw=... adc_mv=... bat_mv=... pct=...
```

## Pull Requests

Pull requests should include:

- a short summary of the change;
- affected areas, such as firmware, UI, storage, battery, Wi-Fi settings, or
  hardware documentation;
- verification performed;
- any risks or unchecked hardware scenarios.

Update documentation when behavior, commands, hardware wiring, or verification
steps change.
