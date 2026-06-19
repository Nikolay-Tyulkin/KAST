# TOOLS

Short tool reference for this repository.

## Default Tools

- For reading and editing files: built-in Codex/opencode file tools
  (`read`, `edit`, `apply_patch`).
- For commands: PowerShell. Run only commands that are actually supported by the
  repository.
- For searching: use `rg` before making assumptions about file structure.

## Current Project Constraints

- The project contains ESP-IDF firmware for `esp32s3`; use ESP-IDF CLI, not
  Arduino or PlatformIO.
- Confirmed check: `idf.py build`.
- There are no separate tests or linters currently; do not invent commands
  until matching configs exist.

## Confirmed Toolchain

- Working environment: `IDF PowerShell Environment`.
- `IDF_PATH`: `C:\esp\v6.0.1\esp-idf`
- `IDF_TOOLS_PATH`: `C:\Espressif\tools`
- `IDF_PYTHON_ENV_PATH`: `C:\Espressif\tools\python\v6.0.1\venv`
- ESP-IDF version: `v6.0.1`.

## Firmware Commands

- Main command: `idf.py`, for example `idf.py build`.
- Additional utilities: `esptool.py`, `espefuse.py`, `espsecure.py`,
  `otatool.py`, `parttool.py`.
- Before running commands in PowerShell 5.1, activate the ESP-IDF profile through
  dot-sourcing:
  `. "C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1"`
- Do not use `source ...` in PowerShell 5.1. The command is not found and the
  profile is not activated.
- Before flashing, increment the `PROJECT_VER` dev counter in the root
  `CMakeLists.txt` by `1` in `0.0.1-dev.N` format so the new firmware is visible
  at the bottom of the screen and in ESP-IDF `Application information`.
- Build: `idf.py build`.
- Flash current board: `idf.py -p COM14 build flash`.
- Log monitor: `idf.py -p COM14 monitor`; exit with `Ctrl+]`.
- If the profile is not activated, do not run firmware commands blindly. Report
  that prerequisite clearly.

## Hardware and Domain Notes

- Target device: `Waveshare ESP32-S3-LCD-1.69` + battery + 3 buttons.
- Use the term `rows` in text, UI, and logic.
- Battery: Waveshare connects `BAT_ADC` to `GPIO1`; in ESP-IDF on ESP32-S3 use
  `ADC_CHANNEL_0`, voltage-divider multiplier `3` (`200k/100k`).
- Verification priorities: visible firmware version at the bottom of the
  screen, correct row counting, correct reset behavior, and correct battery
  percentage display.

## When to Update This File

- Immediately after changing the ESP-IDF version or tool paths.
- Add only verified commands and constraints that can already be confirmed from
  repository files or the local environment.
