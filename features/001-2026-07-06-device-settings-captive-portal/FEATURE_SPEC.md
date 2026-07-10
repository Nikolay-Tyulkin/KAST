# Feature Spec: Device Settings Captive Portal

## Goal

Add a local settings web page to KAST, served by the device through its own Wi-Fi access point, and keep the current row session after power off and power on.

## Context And Problem

KAST needs basic user-adjustable settings without reflashing firmware. The settings must be reachable without an external router or internet connection.

The current knitting session must also survive power loss. Losing the current row count is a critical failure during knitting, so the current rows must be persisted as the user works.

## Target Users

- A knitter who uses KAST during a project and wants to adjust basic device behavior.
- A developer or tester who needs quick access to settings without rebuilding firmware.

## User Scenarios

- From the main screen, the user holds `+` for `3 s`; the device enables its Wi-Fi access point and opens the settings screen.
- The settings screen uses the pause color scheme, shows `SETTINGS`, and shows a Wi-Fi QR code.
- The user joins the `KAST Settings` Wi-Fi AP. The OS may open the captive portal automatically.
- If the captive portal does not open, the user opens `http://192.168.4.1` manually from documentation or known fallback behavior.
- The user changes settings on the web page and presses `Save`; the device applies and persists the settings.
- The user holds `+` for `3 s` again from the main/settings state; the device disables the Wi-Fi AP.
- The user changes rows with `+` or `-`, powers the device off, powers it on again, and the row session continues from the last saved value.

## Functional Requirements

- The device must toggle Wi-Fi AP mode with a `3 s` long press on `+` from the main screen.
- The AP toggle must be reversible: long `+` turns AP on when it is off and turns AP off when it is on.
- The AP SSID is `KAST Settings`.
- The settings page must be available through captive portal behavior after joining the AP.
- If the captive portal does not open automatically, the settings page must be available at `http://192.168.4.1`.
- The settings screen on the device must use the same color scheme as pause.
- The settings screen on the device must show the English title `SETTINGS`.
- The settings screen must show a QR code for joining the `KAST Settings` Wi-Fi AP.
- The settings screen must not show manual Wi-Fi connection text in place of the QR code.
- The settings screen content must not overlap the battery percentage.
- The web settings page must be in English.
- The web settings page must contain a boot beep on/off setting.
- Boot beep controls only the startup melody, not all possible device sounds.
- The web settings page must contain screen brightness from `0` to `100`.
- The default brightness must be `50`.
- The web settings page must contain screen sleep on/off.
- Screen sleep means dimming the display to save battery, not shutting down the device or ending the row session.
- The web settings page must contain a screen sleep timeout in seconds.
- Settings must apply and persist only after pressing `Save`.
- Saved settings must survive reboot and power off/on.
- The current row session must be saved after every row change.
- After power off/on, the device must restore the last saved row count and session state.
- Existing UI and logic terminology must use `rows`.

## Non-Functional Requirements

- The web page must be simple enough for ESP32-S3 resources.
- Settings must work without internet and without an external router.
- Enabling AP must not break the row counter.
- Row persistence after every change must be reliable while considering NVS flash wear.
- The settings page must be readable on a mobile phone.
- Firmware must use ESP-IDF, not Arduino or PlatformIO.

## Out Of Scope

- Connecting KAST to a home Wi-Fi router in station mode.
- User accounts, admin passwords, or authentication.
- Firmware update through the web page.
- Configuring all future sounds; only boot beep is in scope.
- Changing hardware pins or adding new sound hardware.
- Syncing history with external services.

## Dependencies And Constraints

- Project target: ESP-IDF on `esp32s3`.
- Board: `Waveshare ESP32-S3-LCD-1.69`.
- `+` button: `GPIO2`, active level `0`.
- Current firmware already uses NVS for history and now also uses it for settings and current session state.
- Captive portal behavior requires DNS interception and an HTTP server on the device.
- Wi-Fi AP increases power consumption and can conflict with screen sleep UX.
- Current LVGL fonts do not include Cyrillic glyphs, so the device settings screen uses English text.

## Open Questions

- Should the AP be open forever, or should it get an optional password later?
- Should AP automatically stop after an inactivity timeout?
- Should screen sleep be disabled while AP is enabled so the QR code remains visible?
- Should the first button press after screen dimming only wake the screen, or also perform the button action?
- Should row persistence use a wear-aware log format if the device is used heavily for long sessions?

## Risks And Assumptions

- Assumption: the web page is served directly by the device in AP mode.
- Assumption: the manual settings URL remains `http://192.168.4.1`.
- Risk: captive portal behavior is OS-specific and automatic opening cannot be guaranteed on every phone or desktop OS.
- Risk: saving rows after every change increases flash writes; future wear-aware storage may be needed.
- Risk: enabled AP increases battery drain.
- Risk: if screen sleep dims while AP is on, the user may not see the QR code until waking the screen.
