# IMPLEMENTATION_PLAN: KAST

## 1. Implementation Goal

Implement the KAST MVP firmware for the `Waveshare ESP32-S3-LCD-1.69` board
using ESP-IDF.

The MVP must provide:

- row counting with `+` and `-`;
- one current session controlled by the universal button;
- a no-active-session screen;
- a clean session timer that excludes pause time;
- local history for the latest `20` completed sessions;
- statistics: last session, total rows, total clean time;
- battery display in the top-right area;
- low-battery warning below `15%`;
- firmware version at the bottom of the screen;
- build and flash through ESP-IDF CLI.

This document defines implementation order. It does not require code to be
created while planning.

## 2. Target Project Structure

The current project has most MVP logic in `main/main.c`. A later cleanup can
split it into smaller modules under `main/`:

```text
knitting_assistant/
  CMakeLists.txt
  sdkconfig.defaults
  README.md
  PROJECT_SPEC.md
  IMPLEMENTATION_PLAN.md
  ACCEPTANCE_CRITERIA.md
  main/
    CMakeLists.txt
    main.c
    app_config.h
    board_pins.h
    board_power.c
    board_power.h
    battery.c
    battery.h
    buttons.c
    buttons.h
    display.c
    display.h
    session.c
    session.h
    storage.c
    storage.h
    time_utils.c
    time_utils.h
    idf_component.yml
```

Module responsibilities:

- `main.c` - initialization, application task, top-level event loop;
- `app_config.h` - timeouts, thresholds, history size, behavior constants;
- `board_pins.h` - GPIO, ADC channels, display pins, battery divider values;
- `board_power.*` - power latch, power button, shutdown flow;
- `battery.*` - ADC reading and conversion to `adc_mv`, `battery_mv`, percent,
  and low-battery state;
- `buttons.*` - debounce and short/long/sequence/combination detection;
- `display.*` - LCD/LVGL initialization, screens, UI updates;
- `session.*` - session state, row count, clean timer, state transitions;
- `storage.*` - NVS history and aggregate statistics;
- `time_utils.*` - `hh:mm:ss` formatting and time helpers.

Minimal alternative: if a full split is too large for the first pass, extract
`board_pins.h`, `buttons.*`, `session.*`, `display.*`, and `battery.*` first,
then move storage later.

## 3. GPIO, Schematics, and Wiring

### 3.1 Confirmed Board Pins

LCD ST7789:

- `LCD_SCLK = GPIO6`;
- `LCD_MOSI = GPIO7`;
- `LCD_RST = GPIO8`;
- `LCD_DC = GPIO4`;
- `LCD_CS = GPIO5`;
- `LCD_BL = GPIO15`.

Power latch:

- `SYS_OUT = GPIO40`;
- `SYS_EN = GPIO41`.

Battery:

- `BAT_ADC = GPIO1`;
- ESP-IDF on ESP32-S3 uses `ADC_UNIT_1`, `ADC_CHANNEL_0`;
- Waveshare divider: `R3=200k`, `R7=100k`;
- multiplier: `3`;
- formula: `battery_mv = adc_mv * 3`.

I2C on the board:

- `ESP32_SCL = GPIO10`;
- `ESP32_SDA = GPIO11`;
- used by RTC/IMU; do not reuse for buttons without an explicit decision.

Built-in power button:

- powers the board through hardware;
- after boot, `SYS_EN` must be held at `1`;
- `SYS_OUT` is used as input to read the power-button action;
- in MVP, the power button is for power on/off, not row counting.

### 3.2 Previous Temporary Button Pins

Earlier firmware used:

- `PIN_BTN_PLUS = GPIO0`;
- `PIN_BTN_MINUS = GPIO38`.

Risks:

- `GPIO0` is boot/strap related and can affect download mode;
- `GPIO38` is connected to `QMI_INT2` on the schematic and may conflict with
  the IMU;
- the universal button was not assigned to its final GPIO.

Conclusion: these pins are acceptable for early tests only. MVP uses the
confirmed external GPIOs.

### 3.3 Confirmed External Button Wiring

Each button is wired as:

```text
ESP32 GPIO ---- button ---- GND
```

GPIO settings:

- mode: input;
- pull: internal pull-up;
- active level: `0` when pressed;
- debounce: at least `30-50 ms`.

Confirmed assignments:

- `BTN_PLUS = GPIO2`;
- `BTN_MINUS = GPIO16`;
- `BTN_UNIVERSAL = GPIO17`.

Do not use these pins for external buttons without a separate decision:

- `GPIO0` - boot/strap;
- `GPIO1` - battery;
- `GPIO4/5/6/7/8/15` - LCD;
- `GPIO10/11` - I2C;
- `GPIO38` - `QMI_INT2`;
- `GPIO39` - `RTC_INT` on V2;
- `GPIO40/41` - power;
- `GPIO42` - buzzer on V2;
- `GPIO43/44` - UART/console;
- `GPIO45/46` - ESP32-S3 service/strap pins.

### 3.4 MVP Button Logic

`+` button:

- short press in active session: `rows_count += 1`;
- paused: row changes disabled;
- no active session: ignored or shows a start hint.

`-` button:

- short press in active session: `rows_count -= 1`, not below `0`;
- paused: row changes disabled;
- no active session: ignored except as part of the statistics combination.

Universal button:

- short press with no active session: start new session;
- short press in active session: pause;
- short press in paused session: resume;
- long press (`1.5 s`) in active or paused session: finish and save session;
- reset sequence: `3` short presses, then `1` long press within `2 s`;
- reset confirmation: one short press within `5 s`;
- statistics: hold `-` and universal together.

Built-in power button:

- powers the board on through hardware;
- during shutdown, active or paused session must be closed and saved;
- after saving, set `SYS_EN=0`.

## 4. Software Implementation Order

### Stage 0. Baseline Build and Versioning

Goal:

- ensure reproducible build, flash, and visible firmware version.

Files:

- `CMakeLists.txt`;
- `main/CMakeLists.txt`;
- `main/main.c`;
- `sdkconfig.defaults`;
- `README.md` if needed.

Tasks:

- ensure `PROJECT_VER` is set in the root `CMakeLists.txt`;
- pass `PROJECT_VER` into firmware as `APP_VERSION`;
- show version at the bottom of the screen;
- increment `0.0.1-dev.N` before every flash;
- keep `idf.py build` as the base check.

Minimal checks:

- activate ESP-IDF profile;
- run `idf.py build`;
- flash on request with `idf.py -p COM14 build flash`;
- verify `App version: 0.0.1-dev.N` in monitor or on screen.

Dependencies: none.

### Stage 1. Board and Config Layer

Goal:

- move GPIO and hardware constants out of business logic.

Files:

- `main/board_pins.h`;
- `main/app_config.h`;
- `main/main.c`;
- `main/CMakeLists.txt`.

Tasks:

- define LCD, power, battery, and button pins;
- define timeouts and thresholds:
  - debounce `30-50 ms`;
  - long press `1500 ms`;
  - reset sequence window `2000 ms`;
  - reset confirmation timeout `5000 ms`;
  - low battery threshold `15%`;
  - history size `20`;
- migrate button pins to `GPIO2`, `GPIO16`, `GPIO17`.

Checks:

- `idf.py build`;
- each button generates one clear event in monitor.

Dependencies: Stage 0.

### Stage 2. Battery Module

Goal:

- read battery voltage reliably and expose percentage and diagnostics.

Files:

- `main/battery.c`;
- `main/battery.h`;
- `main/board_pins.h`;
- `main/main.c`;
- `main/CMakeLists.txt`.

Tasks:

- configure ADC oneshot for `ADC_CHANNEL_0`;
- read raw ADC value;
- convert to `adc_mv`;
- calculate `battery_mv = adc_mv * 3`;
- map voltage to approximate percentage;
- set low-battery flag below `15%`;
- log `Battery ADC raw=... adc_mv=... bat_mv=... pct=...`.

Checks:

- `idf.py build`;
- monitor shows realistic `bat_mv` and percentage;
- UI shows battery at top-right.

Dependencies: Stage 1.

### Stage 3. Buttons and Input Events

Goal:

- provide stable input events independent from UI and session logic.

Files:

- `main/buttons.c`;
- `main/buttons.h`;
- `main/app_config.h`;
- `main/board_pins.h`;
- `main/main.c`.

Tasks:

- configure input GPIO with pull-ups;
- implement debounce;
- detect short and long press;
- detect reset sequence;
- detect `-` plus universal statistics combination;
- log events during debugging.

Checks:

- one physical short press creates one event;
- long press is detected after `1.5 s`;
- reset is not triggered by one long press alone.

Dependencies: Stage 1.

### Stage 4. Session Model and Row Counter

Goal:

- isolate session state transitions from UI.

Files:

- `main/session.c`;
- `main/session.h`;
- `main/time_utils.c`;
- `main/time_utils.h`;
- `main/main.c`.

Tasks:

- implement states: no session, active, paused, reset confirmation, statistics;
- implement start, pause, resume, finish;
- implement row increment/decrement with lower bound `0`;
- implement clean timer excluding pause time;
- implement reset confirmation behavior;
- create a completed-session record.

Checks:

- scenario transitions match acceptance criteria;
- `+/-` are disabled while paused;
- no active session starts automatically after boot.

Dependencies: Stage 3.

### Stage 5. UI and Screens

Goal:

- make the device readable and useful on the built-in screen.

Files:

- `main/display.c`;
- `main/display.h`;
- `main/main.c`;

Tasks:

- build main screen with large row count;
- show battery percentage at top-right;
- show timer and status;
- show firmware version at the bottom;
- show reset confirmation screen;
- show statistics screen;
- show low-battery state;
- keep text readable on `240x280`.

Checks:

- screen is readable on the real device;
- rows are the most prominent element;
- version is visible at the bottom.

Dependencies: Stages 2 and 4.

### Stage 6. Local History Storage

Goal:

- persist completed sessions and aggregate statistics in NVS.

Files:

- `main/storage.c`;
- `main/storage.h`;
- `main/session.c`;
- `main/main.c`;

Tasks:

- initialize NVS;
- store completed sessions on finish or normal shutdown;
- keep the latest `20` sessions;
- compute last session, total rows, total clean time;
- tolerate empty or unreadable history.

Checks:

- history survives reboot;
- more than `20` sessions keeps the latest `20`;
- NVS read errors do not block startup.

Dependencies: Stage 4.

### Stage 7. Normal Shutdown

Goal:

- save the current session before power is dropped.

Files:

- `main/board_power.c`;
- `main/board_power.h`;
- `main/session.c`;
- `main/storage.c`;
- `main/main.c`.

Tasks:

- read power-button action through `SYS_OUT`;
- close active or paused session with reason `power_off`;
- save session to NVS;
- set `SYS_EN=0` after saving.

Checks:

- active or paused session appears in history after power-cycle;
- closed session does not resume automatically.

Dependencies: Stage 6.

### Stage 8. Documentation and Final MVP Acceptance

Goal:

- align docs with actual firmware and hardware behavior.

Files:

- `README.md`;
- `PROJECT_SPEC.md`;
- `IMPLEMENTATION_PLAN.md`;
- `ACCEPTANCE_CRITERIA.md`;
- `hardware/`.

Tasks:

- document build, flash, monitor, and ESP-IDF profile activation;
- document button wiring and GPIOs;
- document reset sequence;
- document battery ADC mapping;
- document versioning workflow;
- run the acceptance smoke scenario on device when requested.

Checks:

- `idf.py build`;
- `idf.py -p COM14 build flash` when flashing is requested;
- `idf.py -p COM14 monitor` for logs;
- verify start, `+`, `-`, pause, reset confirmation, finish, statistics, and
  shutdown with saving.

Dependencies: all previous stages.

## 5. Stage Dependencies

Main order:

```text
Stage 0 -> Stage 1 -> Stage 2
                    -> Stage 3 -> Stage 4 -> Stage 5 -> Stage 6 -> Stage 7 -> Stage 8
```

Notes:

- battery can be implemented before complete button logic;
- UI can evolve in parallel with session state if a stable view model exists;
- history depends on the completed-session model;
- normal shutdown depends on storage;
- final docs depend on actual button behavior.

Critical blockers:

- final `board_pins.h` depends on confirmed `BTN_PLUS`, `BTN_MINUS`, and
  `BTN_UNIVERSAL` wiring;
- reset, statistics, and finish require stable button event handling;
- shutdown saving requires storage.

## 6. Minimum Checks by Stage

General checks:

- activate ESP-IDF profile:
  `. "C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1"`;
- run `idf.py build`;
- before flashing, increment `PROJECT_VER` to the next `0.0.1-dev.N`;
- flash with `idf.py -p COM14 build flash`;
- monitor with `idf.py -p COM14 monitor`;
- exit monitor with `Ctrl+]`.

Stage smoke checks:

- Stage 0: version is visible in ESP-IDF `Application information` and at the
  bottom of the screen.
- Stage 1: build works after pin/config extraction.
- Stage 2: battery shows realistic `bat_mv` and percentage; monitor logs raw,
  ADC, battery voltage, and percentage.
- Stage 3: each press creates exactly one event; long press is detected after
  `1.5 s`.
- Stage 4: session state follows scenarios; `+/-` are disabled while paused.
- Stage 5: all screens are readable; rows are large and centered.
- Stage 6: history survives reboot and keeps the latest `20` sessions.
- Stage 7: shutdown closes and saves session before `SYS_EN=0`.
- Stage 8: full MVP acceptance passes on the device.

## 7. Risks and Mitigations

### R-001. Wrong GPIO Choice for External Buttons

Risk: chosen GPIO conflicts with boot, LCD, I2C, IMU, RTC, power, or UART.

Mitigation:

- keep all GPIOs in `board_pins.h`;
- do not use known reserved pins without explicit reason;
- verify selected GPIOs through pinout and monitor test before soldering;
- keep alternative candidates documented.

### R-002. Universal Button Complexity

Risk: short, long, sequence, and combination gestures become confusing.

Mitigation:

- log all button events during debugging;
- show clear confirmation prompts;
- make reset require the rare `3 short + long` sequence plus confirmation;
- keep statistics on a separate `- + universal` combination.

### R-003. Accidental Session Finish

Risk: the user holds the universal button too long and finishes a session.

Mitigation:

- save immediately after finish;
- show a saved status;
- consider undo later, but keep it out of MVP unless separately specified.

### R-004. Flash Wear From Frequent Writes

Risk: writing every row quickly wears NVS/flash.

Mitigation:

- save history only on finish or normal shutdown in MVP;
- keep intermediate state in RAM;
- add periodic checkpoints later only if needed.

### R-005. Active Session Loss on Sudden Power Loss

Risk: battery disconnect or power loss occurs before saving.

Mitigation:

- normal shutdown always follows `SYS_OUT -> save -> SYS_EN=0`;
- warn on low battery at `15%`;
- consider active-session checkpoints post-MVP if the risk becomes critical.

### R-006. Inaccurate Battery

Risk: ADC without calibration gives an approximate percentage.

Mitigation:

- use the correct `ADC_CHANNEL_0` for `GPIO1`;
- use divider multiplier `3`;
- log `raw`, `adc_mv`, `bat_mv`, `pct`;
- add ADC calibration or a discharge table later if needed.

### R-007. Screen Readability Issues

Risk: small text, color artifacts, or wrong RGB/BGR settings hurt UX.

Mitigation:

- use large fonts for rows;
- keep the main screen minimal;
- verify on the real display after UI changes;
- if artifacts remain, check `LV_COLOR_16_SWAP`, data endian, RGB/BGR, and
  ST7789 settings.

### R-008. Firmware Size Growth From LVGL Fonts

Risk: large fonts increase binary size.

Mitigation:

- include only used font sizes;
- do not include LVGL demos/examples unless needed;
- watch the app partition size after builds.

## 8. Decisions Still Requiring Confirmation

Hardware:

- confirm the final physical wiring for `+`, `-`, and universal buttons on each
  built device;
- confirm whether any temporary `GPIO0/GPIO38` references remain in code;
- document where button wires are routed in the current physical unit.

UI:

- confirm display language for firmware labels: English (`ROWS`, `PAUSE`) or
  another target language;
- decide whether the no-active-session screen shows last-session summary or
  only a start hint;
- decide whether low-battery warning is overlayed on all screens or shown as a
  status.

Storage:

- confirm that `20` sessions are enough without real calendar dates if RTC is
  not configured;
- decide whether history clearing is post-MVP.

## 9. Plan Readiness Criteria

Implementation can proceed after confirming:

- GPIOs for the three user buttons;
- UI language;
- no-active-session screen behavior;
- statistics screen format;
- time model: uptime seconds only or RTC-based timestamps.

If those decisions are not confirmed, implementation can still start with safe
stages:

- Stage 0;
- Stage 1;
- Stage 2;
- partial Stage 4 with C-level state logic independent of final GPIO.
