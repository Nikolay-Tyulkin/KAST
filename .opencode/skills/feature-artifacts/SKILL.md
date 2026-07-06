---
name: feature-artifacts
description: Use when preparing a new product feature before implementation: create features/NNN-YYYY-MM-DD-feature-slug/ with FEATURE_SPEC.md, IMPLEMENTATION_PLAN.md, and ACCEPTANCE_CRITERIA.md.
license: MIT
compatibility: opencode
metadata:
  project: knitting_assistant
  purpose: feature-artifacts
---

## When To Use

Use this skill when the user asks to prepare a new product feature, feature
artifacts, a feature specification, an implementation plan, or acceptance
criteria before changing application code.

## Required Output

Create a feature folder with this structure:

```text
features/
  NNN-YYYY-MM-DD-feature-slug/
    FEATURE_SPEC.md
    IMPLEMENTATION_PLAN.md
    ACCEPTANCE_CRITERIA.md
```

## Folder Naming Rule

The folder name must use the format `NNN-YYYY-MM-DD-feature-slug`:

- `NNN` is a three-digit feature number, for example `001`, `002`, `003`.
- `YYYY-MM-DD` is the artifact creation date.
- `feature-slug` is a short feature description in English kebab-case.
- For a new number, inspect existing folders in `features/` and use the next
  available number.
- If `features/` does not exist yet, create it and start with `001`.

## FEATURE_SPEC.md

Required sections:

- `# Feature Spec: <feature name>`
- `## Goal`
- `## Context And Problem`
- `## Target Users`
- `## User Scenarios`
- `## Functional Requirements`
- `## Non-Functional Requirements`
- `## Out Of Scope`
- `## Dependencies And Constraints`
- `## Open Questions`
- `## Risks And Assumptions`

## IMPLEMENTATION_PLAN.md

Required sections:

- `# Implementation Plan: <feature name>`
- `## Prerequisites`
- `## Implementation Stages`
- `## Expected Files And Modules`
- `## Task Dependencies`
- `## Stage Checks`
- `## Migrations And Data`
- `## Documentation`
- `## Implementation Risks`
- `## Decisions Requiring Confirmation`

## ACCEPTANCE_CRITERIA.md

Required sections:

- `# Acceptance Criteria: <feature name>`
- `## Acceptance Criteria`
- `## Main Verification Scenarios`
- `## Negative Scenarios`
- `## Edge Cases`
- `## Test Requirements`
- `## Manual Verification`
- `## Definition Of Done`
- `## Not Verified In This Feature`

## Rules

- Do not change application code while creating feature artifacts.
- Do not create frontend, backend, database, AI integration, Docker artifacts, or
  application tests as part of this skill.
- Only create or update documentation artifacts under
  `features/NNN-YYYY-MM-DD-feature-slug/`.
- Write feature artifacts in English.
- Use the term `rows` for device UI and logic unless the user explicitly asks
  for different terminology.
- If the feature boundary, goal, user scenario, or scope is unclear, ask a
  clarification question before creating files.
- Ask clarification questions through the `question` tool.
- Do not record unverified assumptions as requirements. Put them explicitly in
  `Open Questions`, `Risks And Assumptions`, or `Decisions Requiring Confirmation`.
