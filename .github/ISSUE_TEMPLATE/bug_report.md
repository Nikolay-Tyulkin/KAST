---
name: Bug report
about: Report incorrect KAST firmware, hardware, or documentation behavior
title: "Bug: "
labels: bug
assignees: ""
---

## Summary

Describe the problem briefly.

## Steps to Reproduce

1. 
2. 
3. 

## Expected Behavior

What should happen?

## Actual Behavior

What happened instead?

## Affected Area

Check all that apply:

- [ ] Row counter
- [ ] `+` / `-` buttons
- [ ] Universal button
- [ ] Reset flow
- [ ] Screen/UI
- [ ] Battery percentage
- [ ] Wi-Fi settings portal
- [ ] NVS storage/history
- [ ] Hardware/enclosure
- [ ] Documentation

## Environment

- Firmware version shown on screen:
- Board: `Waveshare ESP32-S3-LCD-1.69`
- Hardware revision or wiring notes:
- Port used for logs, if applicable:

## Logs

If relevant, attach output from:

```powershell
idf.py -p COM14 monitor
```

For battery issues, include lines like:

```text
Battery ADC raw=... adc_mv=... bat_mv=... pct=...
```

## Media

Attach photos or video if they help explain screen, buttons, wiring, or enclosure
behavior.
