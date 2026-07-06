---
name: release-notes-github
description: Use when drafting or updating GitHub release notes for KAST releases in this repository. Trigger when the user asks for release notes, GitHub release text, tag-to-tag changelog text, release asset links, or expected versioned artifact names. Do not use this skill to build or flash firmware.
license: MIT
compatibility: opencode
metadata:
  project: knitting_assistant
  purpose: release-notes-github
---

# Release Notes GitHub

Create GitHub release notes by reusing this repository's established KAST
release structure instead of inventing a new format. The final release text must
be in English.

## Workflow

1. Start from `references/release-template.md`.
2. Require both tags in the user request or working prompt:
   - source tag: the previous release used as the comparison base
   - target tag: the new release for which the notes and links are produced
3. Confirm current firmware behavior against repository sources before copying
   technical claims:
   - `README.md`
   - `CHANGELOG.md`
   - `main/main.c`
   - `features/*/FEATURE_SPEC.md` when the release includes a documented feature
4. If the user gives artifact names, tags, or links, update them everywhere
   consistently.
5. Write the final release description in English.

## Required Output Shape

Keep this section order unless the user asks otherwise:

1. `## Included In This Release`
2. `## Release Files`
3. `## What Changed`
4. `## Supported Hardware`
5. `## How To Flash`
6. `## Device Smoke Check`
7. `## Notes And Limitations`

Use short bullet lists and fenced code blocks where the template already uses
them.

## Tag And Artifact Rules

- Always state both tags in the working context: `from <old-tag> to <new-tag>`.
- Replace the old tag in all release asset links with the target tag.
- Keep firmware links in this form:
  `https://github.com/Nikolay-Tyulkin/KAST/releases/download/<tag>/<filename>`
- Include the target version in release artifact filenames.
- Prefer this firmware artifact filename form:
  `kast_esp32s3_<to-tag>.bin`
- If a merged flashing binary is explicitly produced, prefer:
  `kast_esp32s3_<to-tag>_merged.bin`
- If the user gives only filenames and no URLs, infer URLs from the target tag
  and the GitHub release pattern above.
- For `## What Changed`, describe the delta from source tag to target tag rather
  than writing a generic project summary.

## Artifact References

- Use these expected filenames when artifacts are part of the release:
  - `kast_esp32s3_<to-tag>.bin`
  - `kast_esp32s3_<to-tag>_merged.bin` only when a merged image exists
- Do not claim an artifact exists unless the user provided it or it can be
  verified in the workspace.
- If the user asks to actually build or flash firmware, do not handle it inside
  this release-note task. Use the normal firmware workflow instead.

## Technical Accuracy Rules

- Do not blindly copy behavior from older release notes.
- Preserve the target board exactly when unchanged:
  `Waveshare ESP32-S3-LCD-1.69`.
- Preserve confirmed external button mapping exactly when unchanged:
  `+ = GPIO2`, `- = GPIO16`, universal = `GPIO17`, active low.
- Preserve battery mapping exactly when unchanged:
  `GPIO1` maps to `ADC_CHANNEL_0` on ESP32-S3, voltage divider multiplier `3`.
- Use `rows`, not stitches, rounds, or row count variants unless the user
  explicitly asks otherwise.
- If the release includes settings portal behavior, mention that captive portal
  automatic opening is OS-dependent and `http://192.168.4.1` is the manual
  fallback.
- If the release includes flashing instructions, use ESP-IDF commands rather
  than Arduino or PlatformIO.

## Resources

- For the reusable release-note skeleton with placeholders, read
  `references/release-template.md`.

## Default Behavior

If the user asks to "write release notes" without more detail:

1. Start from the template in `references/release-template.md`.
2. Ask for the missing tag range if either source tag or target tag is absent.
3. Ask for missing artifact names only if they cannot be inferred safely.
4. Return release notes only; do not build, flash, tag, push, or create a GitHub
   release unless the user explicitly asks for that work.
