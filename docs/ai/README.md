# Internal Documentation Map

This directory is for reusable internal workflow and governance documentation.

## What belongs here

- AI/developer workflow rules and prompt templates
- reusable internal audit procedures and checklists
- governance docs describing where policy lives
- internal testing/debugging guidance that is about contributor process rather than product behavior

## What does not belong here

- canonical product behavior rules (`docs/SPECIFICATION.md`)
- canonical architecture and ownership rules (`docs/ARCHITECTURE.md`)
- user-facing help/reference docs
- task-specific work-in-progress notes

Task-specific WIP, inventories, and live relay state belong in `.agent/handoffs/`, not here.

## Current boundaries

- `docs/SPECIFICATION.md` is the canonical behavior/UX contract for `ytnova`.
- `docs/ARCHITECTURE.md` is the canonical internal architecture/ownership contract.
- `docs/ai/` is for reusable internal process and audit guidance only.

## File guide

- [ADVERSARIAL_AUDIT.md](ADVERSARIAL_AUDIT.md) — internal guidance for hostile/failure-seeking audit posture.
- [AGENTIC_ENGINEERING_CARGO_CULT.md](AGENTIC_ENGINEERING_CARGO_CULT.md) — internal essay on failure modes in superficial agentic-engineering practice.
- [AUDIT_PROMPT_TEMPLATE.md](AUDIT_PROMPT_TEMPLATE.md) — reusable prompt template for scope-locked audit runs.
- [CODE_QUALITY.md](CODE_QUALITY.md) — internal code-quality smell taxonomy and remediation blueprint.
- [DEBUGGING.md](DEBUGGING.md) — internal debugging workflow and investigation notes.
- [GOVERNANCE.md](GOVERNANCE.md) — canonical map of where AI/developer governance rules live.
- [PRIMARY_ACTION_AUDIT.md](PRIMARY_ACTION_AUDIT.md) — reusable checklist for auditing primary-action depth and prompt-surface correctness.
- [PROMPT_BUREAUCRACY_AUDIT.md](PROMPT_BUREAUCRACY_AUDIT.md) — reusable checklist for auditing prompt necessity and redundant follow-up prompts.
- `README.md` — ownership map for this internal-doc directory.
- [PROMPT_TEMPLATE.md](PROMPT_TEMPLATE.md) — reusable prompt template for tracked implementation runs.
- [TESTING.md](TESTING.md) — internal testing standards and test-writing workflow.
- [WORKFLOW.md](WORKFLOW.md) — end-to-end AI/developer workflow contract for the repo.
