# PROJECT_SPEC: KAST

## 1. Product Goal

KAST is a standalone knitting assistant based on the
`Waveshare ESP32-S3-LCD-1.69` board. It helps the user count rows reliably
during knitting, control the current session, and see battery state.

The MVP goal is to replace a manual row counter with a simple gift-ready device
that requires no training, runs from battery power, and keeps local history for
completed sessions.

Core value:

- the user sees a large current row count;
- the user can quickly add or remove one row with physical buttons;
- the user can start, pause, finish, and inspect basic statistics;
- the device warns about low battery;
- on shutdown, the current session is closed and saved to history.

## 2. MVP Scope

Included in MVP:

- display current row count;
- manual row counter control with `+` and `-` buttons;
- universal button for session control, reset confirmation, and statistics;
- built-in board power button for power on/off;
- confirmation before row reset;
- battery percentage display;
- low-battery warning;
- current session timer in `hh:mm:ss`;
- current session status display;
- firmware version shown at the bottom of the screen;
- current session closure on shutdown or finish;
- completed session storage in local NVS history;
- firmware for `Waveshare ESP32-S3-LCD-1.69` built with ESP-IDF.

The MVP is designed around one current active session. After power-on, a new
session does not start automatically; the user must explicitly start it with the
universal button.

Out of scope for MVP:

- BLE, Wi-Fi, mobile app, web UI, cloud sync, OTA updates;
- project lists, user profiles, pattern templates, complex analytics;
- automatic row detection without a button press;
- multiple independent row counters;
- Arduino and PlatformIO builds.

## 3. Target Users

Primary user:

- a person who knits by hand and wants to avoid losing row count;
- a non-technical user;
- a user who may receive the device as a gift and expects understandable
  behavior without setup.

Secondary users:

- the gift giver, who wants a ready-to-use device;
- the developer or assembler, who flashes and checks the device before handoff.

UX requirements:

- minimal modes;
- large digits;
- clear physical actions;
- protection from accidental reset;
- no phone or computer required for normal use.

## 4. User Scenarios

### 4.1 Power On

1. The user presses the board power button.
2. The device turns on and holds power through the latch.
3. The screen shows the no-active-session state.
4. Battery percentage is visible in the top-right area.
5. Firmware version is visible at the bottom.

Expected result: the user understands that the device is ready, but no session
has started yet.

### 4.2 Start New Session

1. The user short-presses the universal button.
2. The device creates a new session.
3. Row count is set to `0`.
4. Session timer starts from `00:00:00`.
5. Status changes to active.

Expected result: the user can start knitting and count rows.

### 4.3 Add Row

1. The user completes a knitting row.
2. The user presses `+`.
3. Row count increases by `1`.
4. The updated number is shown immediately in the center of the screen.

Expected result: each completed row is recorded with one press.

### 4.4 Correct Counting Error

1. The user notices that one extra row was counted.
2. The user presses `-`.
3. If row count is greater than `0`, it decreases by `1`.
4. If row count is `0`, it remains `0`.

Expected result: the user can correct mistakes without creating negative values.

### 4.5 Pause and Resume

1. During an active session, the user short-presses the universal button.
2. The device pauses the session.
3. The clean session timer stops.
4. The screen shows paused status.
5. Another short universal-button press resumes the session.

Expected result: break time is not included in active knitting duration.

### 4.6 Reset Rows With Confirmation

1. The user initiates reset with `3` short universal-button presses followed by
   `1` long universal-button press within a `2 s` window.
2. The device shows a reset confirmation prompt.
3. The device waits up to `5 s` for confirmation.
4. The user confirms with one short universal-button press.
5. Row count becomes `0`.
6. If confirmation does not happen within `5 s`, reset is canceled.

Expected result: accidental presses do not erase the current row count.

### 4.7 Finish Session

1. The user finishes the current knitting work.
2. The user performs one long universal-button press.
3. The device saves the session to history.
4. The saved record includes start time, end time, duration, and final row count
   where available.
5. The screen returns to no-active-session state.

Expected result: current work is saved to history and the next session starts
separately.

### 4.8 Power Off

1. The user turns off the device with the board power button.
2. If a session is active or paused, the device closes and saves it.
3. The device drops the power latch.

Expected result: the closed session does not resume automatically on next
power-on.

### 4.9 View Statistics

1. The user holds `-` and the universal button together.
2. The device shows basic data from the last session and/or history.
3. The user can return to the main screen.

Expected result: the user can inspect previous work without external devices.

## 5. Functional Requirements

### 5.1 Display

- FR-001. The display shows the current row count as the most prominent element.
- FR-002. Battery percentage is visible in the top-right area.
- FR-003. The session timer is shown in `hh:mm:ss`.
- FR-004. The current status is visible: no session, active, paused, reset
  confirmation, low battery, or saved.
- FR-005. Firmware version is visible at the bottom of the screen.
- FR-006. The UI must remain readable on the `240x280` display.

### 5.2 Row Counter

- FR-010. In an active session, `+` increments row count by `1`.
- FR-011. In an active session, `-` decrements row count by `1`.
- FR-012. Row count never goes below `0`.
- FR-013. Row count changes are shown within `200 ms` after a valid press.
- FR-014. `+` and `-` do not change rows while the session is paused.
- FR-015. Without an active session, `+` and `-` do not change rows, except for
  valid statistics combinations.

### 5.3 Session Management

- FR-020. A short universal-button press starts a session when no session is
  active.
- FR-021. A short universal-button press pauses an active session.
- FR-022. A short universal-button press resumes a paused session.
- FR-023. A long universal-button press (`1.5 s`) finishes an active or paused
  session and saves it.
- FR-024. A session has a clean active timer that excludes pause time.
- FR-025. Only one session can be active at a time.

### 5.4 Row Reset

- FR-030. Reset requires `3` short universal-button presses followed by `1` long
  press within a `2 s` window.
- FR-031. Reset opens a confirmation prompt.
- FR-032. Confirmation requires one short universal-button press within `5 s`.
- FR-033. Unconfirmed reset is canceled and row count remains unchanged.

### 5.5 Battery and Power

- FR-040. Battery is measured through `GPIO1` / `ADC_CHANNEL_0`.
- FR-041. Battery voltage is calculated from ADC voltage using multiplier `3`.
- FR-042. Battery percentage is displayed.
- FR-043. `pct < 15` shows a low-battery warning.
- FR-044. On normal power-off, an active or paused session is saved before
  `SYS_EN` is set to `0`.

### 5.6 History and Statistics

- FR-050. Completed sessions are stored locally in NVS.
- FR-051. MVP history stores the latest `20` completed sessions.
- FR-052. Statistics show last session, total rows, and total clean time.
- FR-053. History write happens on finish or normal shutdown, not on every row.
- FR-054. Empty or unreadable history does not block device startup.

### 5.7 Firmware Version

- FR-060. Firmware version is set with `PROJECT_VER` in the root
  `CMakeLists.txt`.
- FR-061. Version format is `0.0.1-dev.N`.
- FR-062. `PROJECT_VER` is passed to the firmware as `APP_VERSION`.
- FR-063. The version is visible on screen and in ESP-IDF application metadata.

## 6. Non-Functional Requirements

- NFR-001. Firmware builds with ESP-IDF CLI using `idf.py build`.
- NFR-002. Target is `esp32s3`.
- NFR-003. Normal flashing uses `idf.py -p COM14 build flash`.
- NFR-004. The device works without network connectivity.
- NFR-005. The UI is readable on the built-in round-corner LCD.
- NFR-006. Button handling includes debounce.
- NFR-007. Storage writes are limited to avoid unnecessary flash wear.
- NFR-008. Battery readings are diagnostic-friendly through monitor logs.
- NFR-009. Documentation covers buttons, build, flashing, and basic diagnostics.

## 7. Data Model

### 7.1 Session

Fields:

- `id` - session identifier;
- `state` - none, active, paused, finished;
- `rows_count` - current or final row count;
- `started_at` - session start timestamp or uptime;
- `ended_at` - finish timestamp or uptime;
- `active_duration_sec` - clean active duration excluding pause time;
- `pause_started_at` - current pause start time, if paused;
- `close_reason` - `finished`, `power_off`, or equivalent.

### 7.2 SessionHistory

Fields:

- `sessions` - saved session list;
- `max_sessions` - maximum saved sessions in MVP (`20`);
- `last_session_id` - reference to the latest completed session.

### 7.3 BatteryState

Fields:

- `raw_adc` - raw ADC value;
- `adc_mv` - voltage at the ADC pin;
- `battery_mv` - calculated battery voltage;
- `percent` - calculated charge percentage;
- `is_low` - low-battery flag;
- `updated_at` - last measurement time.

### 7.4 DeviceState

Fields:

- `firmware_version` - firmware version;
- `power_state` - on or shutting down;
- `screen_state` - main, reset confirmation, statistics, warning;
- `active_session_id` - current session if present.

### 7.5 InputEvent

Fields:

- `button` - `plus`, `minus`, `universal`, `power`;
- `event_type` - short press, long press, sequence, combination;
- `timestamp` - event time;
- `handled` - handled or rejected.

## 8. Technical Stack

Platform:

- `Waveshare ESP32-S3-LCD-1.69`;
- `ESP32-S3` microcontroller;
- `240x280` ST7789 LCD;
- physical `+`, `-`, universal, and board power buttons;
- battery measurement through `BAT_ADC` on `GPIO1`.

Firmware:

- ESP-IDF CLI;
- target `esp32s3`;
- C;
- FreeRTOS;
- LVGL through `esp_lvgl_port`;
- `esp_lcd` for display;
- `adc_oneshot` for battery;
- NVS for local history.

Confirmed local environment:

- ESP-IDF `v6.0.1`;
- `IDF_PATH`: `C:\esp\v6.0.1\esp-idf`;
- `IDF_TOOLS_PATH`: `C:\Espressif\tools`;
- PowerShell profile:
  `. "C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1"`.

Commands:

- build: `idf.py build`;
- flash current board: `idf.py -p COM14 build flash`;
- monitor: `idf.py -p COM14 monitor`;
- exit monitor: `Ctrl+]`.

Battery:

- `BAT_ADC` is connected to `GPIO1`;
- ESP-IDF channel for ESP32-S3 is `ADC_CHANNEL_0`;
- divider is `R3=200k`, `R7=100k`;
- battery voltage multiplier is `3`.

## 9. Risks and Open Questions

### 9.1 Risks

- R-001. The universal button can become overloaded if too many gestures are
  added.
- R-002. Users may accidentally finish a session if long-press behavior is not
  obvious.
- R-003. Battery percentage may be approximate without calibration.
- R-004. Frequent flash writes can wear NVS if row changes are persisted too
  often.
- R-005. Small text or poor color settings can hurt readability.
- R-006. Sudden power loss can lose an active session if no checkpoint exists.
- R-007. Gift-device use requires very simple behavior; complex modes reduce
  clarity.

### 9.2 Open Questions

- Q-001. Should long-press finish require an additional confirmation?
- Q-002. Is `1.5 s` the final long-press threshold?
- Q-003. Is `15%` the final low-battery threshold?
- Q-004. Should session duration exclude all pause time? MVP says yes.
- Q-005. Is `20` completed sessions enough for MVP history?
- Q-006. Should statistics show only the last session or aggregated totals too?
- Q-007. Should row changes be allowed while paused? MVP says no.
- Q-008. What exact text should appear on the no-active-session screen?

## 10. MVP Acceptance Criteria

- AC-001. Firmware version is displayed in `0.0.1-dev.N` format.
- AC-002. After power-on, no session starts automatically.
- AC-003. The user can start a session with one short universal-button press.
- AC-004. In an active session, `+` increments rows by `1`.
- AC-005. In an active session, `-` decrements rows by `1`, but not below `0`.
- AC-006. Row count is displayed large in the center of the screen.
- AC-007. Battery is displayed as a percentage in the top-right area.
- AC-008. Session timer is displayed in `hh:mm:ss`.
- AC-009. Reset requires the specified sequence and confirmation.
- AC-010. Finishing a session saves it to history.
- AC-011. Statistics can be opened with `-` plus universal button.
- AC-012. Normal shutdown saves an active or paused session.
- AC-013. A closed session does not resume automatically after power-on.
- AC-014. Low battery shows a warning.
- AC-015. Firmware builds with `idf.py build`.
- AC-016. Firmware flashes with `idf.py -p COM14 build flash`.
- AC-017. Documentation explains buttons, build, flashing, and diagnostics.
