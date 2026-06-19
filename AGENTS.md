# AGENTS

## Project Goal

- Product: KAST, a knitting assistant based on the
  `Waveshare ESP32-S3-LCD-1.69` board.
- Controls: 3 buttons: add row, remove row, and a universal reset/secondary
  function button based on press type.
- Interface: the screen shows the row count and battery charge as a percentage.

## Current Repository State

- The project contains an ESP-IDF MVP firmware in `main/main.c`: session state,
  row count, pause, finish, reset confirmation, statistics, and NVS history.
- Target stack: ESP-IDF CLI, target `esp32s3`; do not use Arduino or PlatformIO
  unless the user explicitly decides to.
- Main local check: activate the ESP-IDF PowerShell profile and run
  `idf.py build`.
- Main board flashing command: `idf.py -p COM14 build flash`.
- Device logs: use `idf.py -p COM14 monitor`; exit the monitor with `Ctrl+]`.
- The current firmware version is set in the root `CMakeLists.txt` through
  `PROJECT_VER` in SemVer dev format, for example `0.0.1-dev.2`. It is passed to
  the code as `APP_VERSION`, shown at the bottom of the screen, and included in
  ESP-IDF `Application information`.
- The Waveshare board battery is measured on `GPIO1`, but in ESP-IDF for
  ESP32-S3 this is `ADC_CHANNEL_0`, not `ADC_CHANNEL_1`. The schematic divider
  is `R3=200k` and `R7=100k`, with voltage multiplier `3`.
- Confirmed external buttons on the available connector: `+ = GPIO2`,
  `- = GPIO16`, universal = `GPIO17`. Each button is connected between GPIO and
  `GND`; active level is `0`.

## Firmware Workflow

- Before ESP-IDF commands in PowerShell 5.1, activate the profile through
  dot-sourcing:
  `. "C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1"`.
- Do not use `source ...` in PowerShell 5.1. It does not activate the profile in
  the current environment.
- If the user asks to flash the current board, use port `COM14` unless another
  port is specified.
- Before every flash performed by the assistant, increment the dev counter in
  `PROJECT_VER` by `1` (`0.0.1-dev.N`) and flash that version. After flashing,
  the user should see it at the bottom of the screen.
- If the user asks to run tests or linting, first check that matching configs
  exist. Currently only `idf.py build` is confirmed.
- Keep terminology consistent: use `rows` in UI and logic unless the user
  explicitly asks otherwise.
- Row reset specification: `3` short universal-button presses followed by
  `1` long press within a `2 s` window, then confirmation with one short press
  within `5 s`.
- When designing UI, prioritize readability on the device screen: large row
  digits and a visible battery percentage.
- Update this file as soon as new tools are confirmed. It should contain only
  verifiable, repository-specific instructions.

## Checks After Changes

- Build: `idf.py build` after activating the ESP-IDF profile.
- Flash on request: `idf.py -p COM14 build flash`.
- Device smoke check: screen turns on, expected `0.0.1-dev.N` is visible at the
  bottom, `+/-` changes the row counter, and battery is shown as a percentage.
- For battery diagnostics, inspect monitor logs:
  `Battery ADC raw=... adc_mv=... bat_mv=... pct=...`.
