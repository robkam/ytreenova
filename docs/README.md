# Documentation Map

This directory is for `ytnova` product and project documentation.

## What belongs here

- **Canonical behavior and UX contract**: `SPECIFICATION.md`
- **Canonical implementation and ownership contract**: `ARCHITECTURE.md`
- **User-facing reference and guidance**: `USAGE.md`, `FAQ.md`, `quickstart.md`
- **Project planning and tracking**: `ROADMAP.md`, `BUGS.md`
- **Recurring engineering obligations**: `MAINTENANCE.md`
- **Project QA/reference docs**: `AUDIT.md`, `PR_GATE.md`, `TRUST.md`

## What does not belong here

- AI-only workflow notes
- reusable internal prompt templates
- internal contributor process docs
- task-specific work-in-progress audit notes or relay files

Those belong under `docs/ai/` when they are reusable internal guidance, or under `.agent/handoffs/` when they are task-specific working state.

## `docs/` vs `docs/ai/`

- `docs/` explains how `ytnova` works, what the product contract is, and how contributors or users should understand the software itself.
- `docs/ai/` contains internal workflow, governance, audit, and prompt/procedure documentation for developer/AI-assisted work. It is not user documentation and does not define product behavior unless it explicitly points back to a canonical file in `docs/`.

## File guide

- `ARCHITECTURE.md` — canonical internal architecture, ownership, and module-boundary contract.
- `AUDIT.md` — project QA gates and when each audit layer is used.
- `AUTHORS.md` — contributor credits.
- `BUGS.md` — tracked defect backlog and bug-family notes.
- `CHANGELOG.md` — durable release/history highlights.
- `CODE_OF_CONDUCT.md` — community conduct expectations.
- `CONTRIBUTING.md` — contributor setup and contribution rules.
- `FAQ.md` — short answers to recurring user/contributor questions.
- `MAINTENANCE.md` — active recurring engineering obligations and their triggers.
- `PR_GATE.md` — pull-request merge/readiness gate reference.
- `README.md` — documentation ownership map for this directory.
- `ROADMAP.md` — planned feature and remediation backlog.
- `SPECIFICATION.md` — canonical user-visible behavior and UX contract.
- `TRANSLATORS.md` — translation workflow and translator guidance.
- `TRUST.md` — trust/safety posture and operator expectations.
- `USAGE.md` — generated user reference from authored help sources.
- `quickstart.md` — brief getting-started path for running ytnova.
- `clean_code_allowlist.json` — QA allowlist for approved clean-code exceptions.
- `ai/` — reusable internal developer/AI workflow and audit docs.
- `screenshots/` — documentation image assets.
