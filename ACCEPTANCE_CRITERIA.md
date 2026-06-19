# ACCEPTANCE_CRITERIA: KAST

## 1. MVP Definition of Done

The MVP is complete when all conditions below are met:

- firmware builds with ESP-IDF using `idf.py build` after activating the
  ESP-IDF PowerShell profile;
- firmware flashes successfully to `Waveshare ESP32-S3-LCD-1.69` with
  `idf.py -p COM14 build flash`, unless another port is specified;
- firmware version uses `0.0.1-dev.N`, is visible at the bottom of the screen,
  and appears in ESP-IDF `Application information`;
- after power-on, the device shows a no-active-session state and does not start
  a session automatically;
- the user can start one new session with a short universal-button press;
- in an active session, `+` increases row count by `1`;
- in an active session, `-` decreases row count by `1`, but not below `0`;
- row changes are visible on screen no later than `200 ms` after a valid press;
- pause stops the clean session timer, and another short universal-button press
  resumes the session;
- while paused, `+` and `-` do not change row count;
- a `1.5 s` long universal-button press finishes an active or paused session
  and saves it to history;
- row reset requires `3` short universal-button presses and `1` long press
  within `2 s`, then confirmation with one short press within `5 s`;
- unconfirmed reset is canceled after `5 s` without changing row count;
- statistics open by holding `-` and universal together;
- history stores the latest `20` completed sessions locally;
- statistics show last session, total rows, and total clean time;
- normal shutdown closes and saves an active or paused session before power is
  dropped;
- after power-on, a closed session does not resume automatically;
- battery is shown as a percentage in the top-right area;
- when charge is below `15%`, the user sees a low-battery warning;
- monitor logs allow checking firmware version and battery diagnostics in the
  format `Battery ADC raw=... adc_mv=... bat_mv=... pct=...`;
- UI uses the term `rows` and keeps the row count as the dominant central
  element;
- the device works without BLE, Wi-Fi, phone, cloud, Arduino, or PlatformIO;
- documentation describes buttons, screens, build, flashing, and basic
  diagnostics.

## 2. User Acceptance Scenarios

### AC-U01. Power On

Preconditions:

- board is flashed with the current MVP firmware;
- battery is connected or power is supplied normally.

Steps:

1. Press the board power button.
2. Wait for the screen to turn on.

Expected result:

- the device holds power;
- the no-active-session screen is shown;
- a new session does not start automatically;
- battery percentage is visible in the top-right area;
- firmware version `0.0.1-dev.N` is visible at the bottom.

### AC-U02. Start New Session

Precondition:

- the device is on the no-active-session screen.

Steps:

1. Short-press the universal button.

Expected result:

- a new session is created;
- row count is `0`;
- timer shows `00:00:00` and starts counting;
- status shows active session.

### AC-U03. Add Rows

Precondition:

- active session is running.

Steps:

1. Press `+` once.
2. Press `+` two more times.

Expected result:

- after the first press, `1` row is displayed;
- after three presses, `3` rows are displayed;
- each valid press creates exactly one increment;
- screen updates within `200 ms` after the press.

### AC-U04. Remove Rows and Lower Bound

Preconditions:

- active session is running;
- row count is `1`.

Steps:

1. Press `-` once.
2. Press `-` again.

Expected result:

- after the first press, `0` rows are displayed;
- after the second press, the value remains `0`;
- negative values never appear.

### AC-U05. Pause and Resume

Preconditions:

- active session is running;
- timer is greater than `00:00:00`.

Steps:

1. Short-press the universal button.
2. Wait a few seconds.
3. Press `+` and `-` while paused.
4. Short-press the universal button again.

Expected result:

- status changes to paused after the first press;
- clean timer does not increase while paused;
- `+` and `-` do not change row count while paused;
- second short press resumes the session;
- timer counts active time again after resume.

### AC-U06. Reset Rows With Confirmation

Preconditions:

- active session is running;
- row count is greater than `0`.

Steps:

1. Perform `3` short universal-button presses.
2. Perform `1` long universal-button press within the `2 s` window.
3. Wait for the reset confirmation screen.
4. Short-press the universal button within `5 s`.

Expected result:

- the device shows reset confirmation;
- after confirmation, row count becomes `0`;
- the session remains current unless it is separately finished.

### AC-U07. Cancel Reset by Timeout

Preconditions:

- active session is running;
- row count is greater than `0`;
- reset confirmation screen is open.

Steps:

1. Do not press the universal button for `5 s`.

Expected result:

- reset confirmation closes;
- row count remains unchanged;
- the device returns to the previous session screen.

### AC-U08. Finish Session

Preconditions:

- active or paused session is running;
- row count is greater than `0`.

Steps:

1. Perform one `1.5 s` long universal-button press.

Expected result:

- session finishes immediately;
- session is saved to history;
- final rows and clean duration are saved;
- screen returns to no-active-session state or shows a short saved status.

### AC-U09. View Statistics

Precondition:

- history contains at least one completed session.

Steps:

1. Hold `-` and universal together.

Expected result:

- statistics screen opens;
- last-session data is visible;
- total rows are visible;
- total clean time is visible;
- the user can return to the main screen without external devices.

### AC-U10. Normal Shutdown With Save

Preconditions:

- active or paused session is running;
- row count is greater than `0`.

Steps:

1. Turn off the device with the board power button.
2. Turn the device on again.
3. Open statistics.

Expected result:

- before power-off, the current session is closed with reason `power_off`;
- after power-on, the session does not resume automatically;
- the closed session is available in history or statistics.

### AC-U11. Battery and Low Charge

Preconditions:

- device is powered on;
- battery ADC is connected to `GPIO1`, using `ADC_CHANNEL_0` and divider
  multiplier `3`.

Steps:

1. Check battery percentage on screen.
2. Check battery monitor log.
3. Check `pct < 15` with a real low battery, temporary threshold, or mock mode.

Expected result:

- battery is shown as a percentage in the top-right area;
- monitor contains `raw`, `adc_mv`, `bat_mv`, and `pct`;
- when `pct < 15`, the screen shows a low-battery warning.

## 3. Software Criteria

### AC-S01. Build

Check commands:

```powershell
. "C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1"
idf.py build
```

Expected result:

- build completes without errors;
- target remains `esp32s3`;
- build does not require Arduino or PlatformIO.

### AC-S02. Flash

Check commands:

```powershell
idf.py -p COM14 build flash
```

Before flashing, increment `PROJECT_VER` to the next `0.0.1-dev.N`.

Expected result:

- flashing succeeds;
- new version is visible on the screen;
- the same `App version` is visible in monitor.

### AC-S03. Diagnostic Logs

Check command:

```powershell
idf.py -p COM14 monitor
```

Expected result:

- monitor shows application start;
- monitor shows firmware version;
- monitor shows battery diagnostics;
- during button debugging, monitor can show events such as `PLUS_SHORT`,
  `MINUS_SHORT`, `UNIVERSAL_SHORT`, `UNIVERSAL_LONG`, `RESET_REQUEST`,
  `STATS_OPEN`, `POWER_LONG`, or equivalent messages.

### AC-S04. Buttons and Debounce

Expected result:

- one physical short press creates one user event;
- long press is detected after `1.5 s`;
- bounce does not increment or decrement rows multiple times;
- one long press alone does not count as reset sequence without the preceding
  three short presses.

### AC-S05. History Storage

Expected result:

- history is written on session finish or normal shutdown;
- history is not written on every row;
- history is limited to the latest `20` completed sessions;
- history read errors do not break startup: history is treated as empty and the
  error is logged.

### AC-S06. UI and Readability

Expected result:

- row count is the most prominent screen element;
- battery is top-right;
- firmware version is at the bottom;
- timer uses `hh:mm:ss`;
- statuses for no session, active, paused, reset confirmation, and low battery
  are distinguishable on the `240x280` screen.

## 4. Error and Empty-State Criteria

### AC-E01. No Active Session

Expected result:

- screen clearly shows that there is no active session;
- `+` does not change rows;
- `-` does not change rows except as part of the statistics combination;
- short universal-button press starts a new session.

### AC-E02. Rows Are Zero

Expected result:

- pressing `-` keeps the value at `0`;
- negative values never appear in UI, logs, or session model.

### AC-E03. Empty History

Expected result:

- statistics screen does not crash and does not show garbage data;
- the user sees a clear empty state;
- aggregate values are `0`.

### AC-E04. History Overflow

Expected result:

- after more than `20` completed sessions, the latest `20` are retained;
- statistics remain correct;
- device does not hang or corrupt NVS because history grows.

### AC-E05. Unconfirmed Reset

Expected result:

- if confirmation is not completed within `5 s`, rows are not reset;
- the device returns to the previous session state.

### AC-E06. Battery Error or Invalid ADC Reading

Expected result:

- application keeps running;
- monitor contains a diagnostic message;
- UI does not show invalid negative percentages or values above the allowed
  range without clamping;
- row counting and session control remain available.

### AC-E07. NVS Read Error

Expected result:

- device starts with empty history;
- error is logged;
- a new completed session can be saved if NVS writes are available.

### AC-E08. Fast or Accidental Presses

Expected result:

- accidental single universal-button press does not reset rows;
- accidental single long press finishes the session according to MVP rules and
  saves it to history;
- reset requires the full sequence plus confirmation.

## 5. Minimum Checks Before Final Agent Response

For firmware changes, the agent must perform or explicitly state why it did not
perform:

1. Review changed files and ensure unrelated user changes were not overwritten.
2. Activate the ESP-IDF PowerShell profile:
   `. "C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1"`.
3. Run `idf.py build`.
4. If the user requested flashing, increment `PROJECT_VER` to the next
   `0.0.1-dev.N`.
5. If the user requested flashing, run `idf.py -p COM14 build flash`, unless
   another port is specified.
6. If device verification is needed, run `idf.py -p COM14 monitor` and exit with
   `Ctrl+]`.
7. Verify that the expected firmware version is visible on screen.
8. Verify the main smoke scenario: start session, `+`, `-`, pause, resume, reset
   with confirmation, finish, statistics.
9. Verify battery on screen and monitor log:
   `Battery ADC raw=... adc_mv=... bat_mv=... pct=...`.
10. In the final answer, list which checks were performed and which were not.

For documentation-only tasks:

1. Review created or changed documents.
2. Ensure terminology uses `rows`.
3. Do not build or flash unless firmware code changed or the user requested it.
4. State in the final answer that hardware checks were not run because only
   documentation changed.
