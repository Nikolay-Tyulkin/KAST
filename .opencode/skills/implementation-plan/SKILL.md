---
name: implementation-plan
description: Use to convert agreed device requirements into an incremental embedded implementation plan with dependencies, risks, and verification steps.
license: MIT
compatibility: opencode
metadata:
  project: knitting-assistant
  purpose: planning
---

## What to Do

Convert agreed requirements into an implementation plan before code changes
begin.

## Required Output

Include sections for:

- implementation stages;
- files or modules likely to change;
- dependencies between tasks;
- verification steps for each stage;
- risks and fallback options;
- decisions that still require user confirmation.

## Rules

- Do not create application code during planning unless the user explicitly asks
  to move into implementation.
- Make the plan incremental.
- Prefer the smallest correct architecture.
- Track work by modules: buttons, row count, screen, battery, state storage, and
  tests.
