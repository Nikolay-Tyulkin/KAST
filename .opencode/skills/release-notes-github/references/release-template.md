# Release Template

Use this as the starting point for new GitHub releases in this repository.

Replace `<from-tag>`, `<to-tag>`, and artifact names before returning the final
text.

```md
## Included In This Release

Ready-to-flash ESP-IDF firmware for `Waveshare ESP32-S3-LCD-1.69`.

Firmware:
- counts knitting rows with external `+` and `-` buttons
- uses the universal button for start, pause, resume, reset, and secondary flows
- shows rows, battery percentage, session state, and firmware version on screen
- stores session data, statistics, history, and supported settings in NVS
- is built for ESP32-S3 with ESP-IDF

## Release Files

- [kast_esp32s3_<to-tag>.bin](https://github.com/Nikolay-Tyulkin/KAST/releases/download/<to-tag>/kast_esp32s3_<to-tag>.bin) - firmware binary for the device

## What Changed

- list the key changes in the `<from-tag> -> <to-tag>` range

## Supported Hardware

- `Waveshare ESP32-S3-LCD-1.69` board
- external `+`, `-`, and universal buttons connected between GPIO and `GND`

Button wiring:
- `+` -> `GPIO2`
- `-` -> `GPIO16`
- universal -> `GPIO17`

Battery measurement:
- `BAT_ADC` / `GPIO1` -> ESP-IDF `ADC_CHANNEL_0`
- voltage divider multiplier: `3`

## How To Flash

Activate the ESP-IDF PowerShell profile first:

```powershell
. "C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1"
```

Flash the current board, replacing `COM14` if the board appears on a different
port:

```powershell
idf.py -p COM14 build flash
```

## Device Smoke Check

- screen turns on
- expected `<to-tag>` version is visible at the bottom of the screen
- `+` and `-` change rows
- battery is shown as a percentage
- universal button starts, pauses, resumes, and follows the documented reset flow

## Notes And Limitations

- Captive portal automatic opening, when included in the release, depends on the
  phone or desktop OS.
- Use `http://192.168.4.1` as the manual settings fallback when the settings AP
  is enabled.
- This firmware uses ESP-IDF, not Arduino or PlatformIO.
```

Before finalizing a real release package, ensure requested artifacts exist in
the workspace or were supplied by the user.
