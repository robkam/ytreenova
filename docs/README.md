# Documentation Map

This directory is for `ytnova` product and project documentation. This page is the primary index for the complete documentation set.

## What belongs here

- **Canonical behavior and UX contract**: `SPECIFICATION.md`
- **Canonical implementation and ownership contract**: `ARCHITECTURE.md`
- **User and contributor reference and guidance**: `USAGE.md`, `FAQ.md`, `quickstart.md`, `CONTRIBUTING.md`
- **Project planning and tracking**: `ROADMAP.md`, `BUGS.md`
- **Recurring engineering obligations**: `MAINTENANCE.md`
- **Project QA/reference docs**: `AUDIT.md`, `PR_GATE.md`, `TRUST.md`

## What does not belong here

- AI-only workflow notes
- reusable internal prompt templates
- internal AI/developer orchestration procedures
- task-specific work-in-progress audit notes or relay files

Those belong under `docs/ai/` when they are reusable internal guidance, or under `.agent/handoffs/` when they are task-specific working state.

## `docs/` vs `docs/ai/`

- `docs/` explains how `ytnova` works, what the product contract is, and how contributors or users should understand the software itself.
- `docs/ai/` contains internal workflow, governance, audit, and prompt/procedure documentation for developer/AI-assisted work. It is not user documentation and does not define product behavior unless it explicitly points back to a canonical file in `docs/`.

## File guide

- [ARCHITECTURE.md](ARCHITECTURE.md) — canonical internal architecture, ownership, and module-boundary contract.
- [AUDIT.md](AUDIT.md) — project QA gates and when each audit layer is used.
- [AUTHORS.md](AUTHORS.md) — contributor credits.
- [BUGS.md](BUGS.md) — unresolved confirmed defects and architectural violations.
- [CHANGELOG.md](CHANGELOG.md) — significant shipped outcomes and release history.
- [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) — community conduct expectations.
- [COMPATIBILITY_SHIMS.md](COMPATIBILITY_SHIMS.md) — temporary compatibility-shim inventory contract and guard.
- [CONTRIBUTING.md](CONTRIBUTING.md) — contributor setup and contribution rules.
- [FAQ.md](FAQ.md) — short answers to recurring user/contributor questions.
- [MAINTENANCE.md](MAINTENANCE.md) — active recurring engineering obligations and their triggers.
- [PR_GATE.md](PR_GATE.md) — pull-request merge/readiness gate reference.
- `README.md` — documentation ownership map for this directory.
- [ROADMAP.md](ROADMAP.md) — unfinished finite planned improvements.
- [SPECIFICATION.md](SPECIFICATION.md) — canonical user-visible behavior and UX contract.
- [TRANSLATORS.md](TRANSLATORS.md) — translation workflow and translator guidance.
- [TRUST.md](TRUST.md) — trust/safety posture and operator expectations.
- [USAGE.md](USAGE.md) — generated user reference from [`etc/help/man.en.md`](../etc/help/man.en.md).
- [V1_RELEASE_LINE.md](V1_RELEASE_LINE.md) — bounded release criteria for the path from beta to stable v1.
- [quickstart.md](quickstart.md) — brief getting-started path for running ytnova.
- [clean_code_allowlist.json](clean_code_allowlist.json) — QA allowlist for approved clean-code exceptions.
- [ai/README.md](ai/README.md) — index of reusable internal developer/AI workflow, governance, audit, and prompt documentation.
- `screenshots/` — documentation image assets.
