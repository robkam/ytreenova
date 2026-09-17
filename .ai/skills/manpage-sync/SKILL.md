---
name: manpage-sync
description: Keep ytnova manpage, usage, and runtime help synchronized from their authored help sources.
---

# Manpage Sync

Use this skill when commands, options, keybindings, or user-facing behavior docs change.

## Source of Truth

- Edit `etc/help/man.en.md` for manpage and usage-reference prose.
- Edit `etc/help/f1.en.md` for runtime contextual-help prose.
- Treat `etc/ytnova.1.md`, `docs/USAGE.md`, `src/core/generated_help_topics.h`, and the build manpage as generated outputs.

## Sync Workflow

1. Apply documentation edits in the applicable authored source under `etc/help/`.
2. Regenerate the help assets:
   - `make help-assets`
3. Verify generated assets are in sync:
   - `make qa-help-assets`
4. Verify generated docs and runtime help reflect current behavior.

## Checks

- Do not edit generated help outputs independently of their authored sources.
- Options/commands in docs match implemented behavior.
- Prompt/menu labels are consistent with current UI text.
- Documentation placement MUST be audience-relevant and non-duplicative; you MUST NOT scatter the same guidance across unrelated sections.
