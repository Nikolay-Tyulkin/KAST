# Implementation Plan: Device Settings Captive Portal

## Prerequisites

- Confirm the board target remains ESP-IDF `esp32s3`.
- Use existing external button mappings: `+ = GPIO2`, `- = GPIO16`, universal = `GPIO17`, active low.
- Use current buzzer hardware on `GPIO42` for the startup melody.
- Use `GPIO15` backlight PWM for brightness and screen dimming.
- Keep `rows` terminology in UI and logic.

## Implementation Stages

1. Extend the persistent device settings model.
2. Add NVS load/save for settings with safe defaults.
3. Add current session persistence and restore.
4. Save current session state after every row change and key session transition.
5. Add per-button long-press durations so `+` can use a `3 s` AP toggle while other long presses keep existing behavior.
6. Add Wi-Fi AP lifecycle: start, stop, and status.
7. Add an HTTP settings page with a `Save` handler.
8. Add captive portal DNS interception with fallback access at `http://192.168.4.1`.
9. Apply settings to boot beep, backlight brightness, and screen sleep dimming.
10. Add the device settings screen using the pause color scheme, title `SETTINGS`, SSID, and manual URL.
11. Increase the app partition preset to `single app large`, because Wi-Fi and HTTP support exceed the default `1 MB` app partition.
12. Build and flash firmware for device verification.

## Expected Files And Modules

- `main/main.c` - current single-file firmware implementation.
- `main/CMakeLists.txt` - add ESP-IDF Wi-Fi, HTTP server, and networking dependencies.
- `sdkconfig` - generated ESP-IDF configuration updated for the larger app partition.
- `sdkconfig.defaults` - default partition preset for reproducible builds.
- `README.md` - user-facing behavior and settings portal instructions.
- Root `CMakeLists.txt` - increment `PROJECT_VER` before flashing.

## Task Dependencies

- NVS settings must exist before the web `Save` handler can persist real values.
- Current session persistence is independent from the settings portal and must work even when AP is never enabled.
- The `+` long-press handler must not turn a long press into a row increment.
- Wi-Fi AP must start before DNS captive portal and HTTP serving are useful.
- Screen sleep depends on brightness PWM support.
- The app partition must be large enough before flash can succeed.

## Stage Checks

- After settings persistence: build with `idf.py build`.
- After session persistence: change rows, power off/on, and confirm the row count is restored.
- After button handling: short `+` increments rows, long `+` toggles AP.
- After HTTP server: open `http://192.168.4.1`, change settings, press `Save`, and confirm settings apply.
- After captive portal: join `KAST Settings` from a phone and verify automatic portal opening or manual fallback.
- After settings screen: verify pause color, `SETTINGS`, SSID, URL, and battery percentage do not overlap.
- After screen sleep: verify the display dims after the configured timeout and wakes on button activity.

## Migrations And Data

- Add a versioned settings blob in NVS.
- Settings defaults: boot beep enabled, brightness `50`, screen sleep disabled, sleep timeout `60 s`.
- Add a versioned current-session blob in NVS.
- Session persistence stores at least active flag, rows, session state, and accumulated active time.
- Missing or invalid NVS blobs fall back to safe defaults without crashing.
- Existing history storage remains separate.

## Documentation

- Update README with AP toggle, SSID, manual URL, and settings descriptions.
- Document that captive portal automatic opening depends on the OS.
- Document that current rows are saved after every row change.
- Keep feature artifacts in English and aligned with implemented firmware behavior.

## Implementation Risks

- Wi-Fi and HTTP significantly increase firmware size.
- DNS captive portal handling may differ across Android, iOS, Windows, and macOS.
- Frequent row writes may increase NVS flash wear.
- AP mode increases battery drain.
- Screen sleep may hide the manual URL while AP is enabled.
- LVGL Cyrillic text requires a font with Cyrillic glyphs; the current device screen uses English to avoid missing glyph boxes.

## Decisions Requiring Confirmation

- Whether to add AP password protection later.
- Whether AP should stop automatically after inactivity.
- Whether AP should temporarily disable screen sleep.
- Whether first button press after dimming should only wake the screen.
- Whether to implement wear-aware row storage.
