# SKILLS

Project skill index for `knitting_assistant`.

Important: opencode loads skills from `.opencode/skills/<name>/SKILL.md`, not
from this file.

## Active Skills

- `product-spec` (`.opencode/skills/product-spec/SKILL.md`) - use when device
  requirements must be captured: buttons, rows, screen, battery, and board
  constraints.
- `implementation-plan` (`.opencode/skills/implementation-plan/SKILL.md`) - use
  when agreed requirements must be broken down into embedded implementation
  stages.
- `quality-gate` (`.opencode/skills/quality-gate/SKILL.md`) - use before a
  final answer for implementation tasks: what was checked, what was not checked,
  and what risks remain.

## Mini Rules

- Do not write code during `product-spec` or `implementation-plan` work unless
  the user explicitly asks to move into implementation.
- Use the term `rows` for UI and logic.
- Before the final answer for implementation work, use `quality-gate`: review
  changes, run available checks, and explicitly state unchecked areas.
- Confirmed firmware check: activate the ESP-IDF profile with
  `. "C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1"`, then run
  `idf.py build`.
- Confirmed board flash command: `idf.py -p COM14 build flash`, unless the user
  specifies another port.
- Before assistant-performed flashing, increment `PROJECT_VER` in the root
  `CMakeLists.txt` to the next `0.0.1-dev.N`; the version must be shown at the
  bottom of the screen after flashing.
- Battery mapping: Waveshare `GPIO1/BAT_ADC` maps to ESP-IDF `ADC_CHANNEL_0` on
  ESP32-S3; voltage divider is `200k/100k`, multiplier `3`.
- For logs and smoke checks, use `idf.py -p COM14 monitor`; exit with `Ctrl+]`.
- There are no separate tests or linters currently; do not invent commands
  until matching configs exist.
