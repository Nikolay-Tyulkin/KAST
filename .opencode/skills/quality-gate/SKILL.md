---
name: quality-gate
description: Use before finishing a task to review changed files, run relevant checks, describe unchecked areas, and list remaining risks.
license: MIT
compatibility: opencode
metadata:
  project: knitting-assistant
  purpose: verification
---

## What to Do

Verify that the current task is complete before the final response.

## Checklist

- Review changed files.
- Confirm that changes match the task.
- Run relevant tests, build, lint, typecheck, or smoke checks when available.
- Update documentation if behavior changed.
- Confirm that secrets were not added.
- Note everything that could not be checked.
- For this project, specifically check row logic, reset behavior, and battery
  display when firmware changed.

## Final Response Format

Report:

- what changed;
- which files were touched;
- which checks were run;
- which checks were not run and why;
- remaining risks or follow-up tasks.
