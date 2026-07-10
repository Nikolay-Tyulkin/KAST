# Acceptance Criteria: Device Settings Captive Portal

## Acceptance Criteria

- Holding `+` for `3 s` from the main screen enables Wi-Fi AP.
- Holding `+` for `3 s` again disables Wi-Fi AP.
- Short `+` continues to add one row and does not enable AP.
- After AP starts, the device screen uses the pause color scheme.
- After AP starts, the device screen shows `SETTINGS`.
- After AP starts, the device screen shows a QR code for joining `KAST Settings`.
- After AP starts, the device screen no longer shows manual SSID and URL text in place of the QR code.
- Battery percentage remains visible and is not covered by missing glyph boxes or settings text.
- After joining the device Wi-Fi AP, the OS may open the captive portal automatically.
- If captive portal does not open automatically, the settings page is available at `http://192.168.4.1`.
- The web settings page is in English.
- The web settings page has a boot beep toggle.
- Boot beep controls only the startup melody.
- The web settings page has brightness control from `0` to `100`.
- Default brightness is `50` when settings are empty.
- The web settings page has a screen sleep toggle.
- The web settings page has a screen sleep timeout in seconds.
- If screen sleep is enabled, the display dims after the configured idle timeout.
- If screen sleep is disabled, the display does not dim because of the sleep timeout.
- Settings apply only after pressing `Save`.
- After `Save`, settings persist and restore after reboot.
- Current rows are saved after every row change.
- After power off/on, the device restores the last saved row session.
- Firmware builds with `idf.py build` after activating the ESP-IDF profile.

## Main Verification Scenarios

- Start with empty settings and verify defaults: boot beep enabled, brightness `50`, screen sleep disabled, timeout `60 s`.
- Short press `+` from the main screen and verify rows increase by `1` and AP stays off.
- Hold `+` for `3 s` from the main screen and verify AP starts.
- Verify the device screen shows `SETTINGS` and a scannable Wi-Fi QR code without overlap.
- Join `KAST Settings` from a phone and verify captive portal or manual URL.
- Change brightness, press `Save`, and verify brightness applies and persists.
- Change screen sleep and timeout, press `Save`, and verify dimming behavior.
- Change boot beep, press `Save`, reboot, and verify startup melody behavior.
- Hold `+` for `3 s` again and verify AP stops.
- Change rows, power off, power on, and verify rows are restored.

## Negative Scenarios

- Open an unknown URL while connected to AP; the device should serve the settings page or redirect without crashing.
- Submit brightness below `0` or above `100`; the device clamps the value to the valid range.
- Submit malformed `Save` query; the device must not crash.
- Disconnect while editing without `Save`; settings must not change.
- Hold `+` outside supported states; AP must not toggle where it would conflict with other flows.

## Edge Cases

- Very fast `+` or `-` presses should persist the latest row value.
- Reboot immediately after a row change should restore the latest committed value.
- Reboot immediately after `Save` should restore saved settings.
- Captive portal may not open automatically on some OS versions; `http://192.168.4.1` must still work.
- AP enabled and screen sleep enabled must follow the confirmed priority behavior.
- Missing or invalid NVS settings/session blobs must fall back safely.

## Test Requirements

- If unit tests are added later, cover settings parsing and validation.
- If unit tests are added later, cover settings load/save defaults.
- If unit tests are added later, cover screen sleep timeout validation.
- If unit tests are added later, cover current rows restore from storage.
- Current required automated check: `idf.py build`.

## Manual Verification

- Activate ESP-IDF PowerShell profile: `. "C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1"`.
- Run `idf.py build`.
- Before flashing, increment `PROJECT_VER` in root `CMakeLists.txt`.
- Flash the current board: `idf.py -p COM3 build flash` or the active board port.
- Verify the new firmware version at the bottom of the screen.
- Verify `+`, `-`, long `+`, settings screen, captive portal, manual URL, `Save`, screen sleep, and reboot restore for settings and rows.

## Definition Of Done

- The settings web page is English and saves settings through `Save`.
- The AP starts and stops through long `+` from the main/settings state.
- Captive portal works where the OS allows it, and `http://192.168.4.1` works as fallback.
- The settings screen is English and does not overlap the battery percentage.
- Current row session restores after power off/on.
- Firmware builds through `idf.py build`.
- Manual device verification passes for the main scenarios.

## Not Verified In This Feature

- Captive portal consistency across all iOS, Android, Windows, and macOS versions.
- Long-term flash wear from months of saving every row change.
- Security of an open AP against hostile users; authentication is out of scope.
- OTA update through the web page.
- Station-mode Wi-Fi through an external router.
