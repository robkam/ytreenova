# AI Governance

## Scope and Intent

This page is the canonical navigation map for AI-governance documentation in `ytnova`: what each file governs, where to edit, and how to avoid policy drift across duplicate copies.

## Root Discovery Stubs (Repository Root)

Root stubs are entry points only. They should stay short and contain:
- Pointer to the provider canonical file under `.ai/`.
- Pointer to shared policy in `.ai/shared.md`.
- Small, provider-relevant quick reference (for example, common test invocation notes).
- No duplicated long-form policy text that belongs in canonical files.

Files:
- `AGENTS.md` (Codex discovery stub)

## Canonical Provider and Shared Policy Files

Primary governance sources:
- Shared cross-provider policy: `.ai/shared.md`
- Codex provider policy: `.ai/codex.md`

Ownership model:
- Shared policy changes are centralized in `.ai/shared.md`.
- Provider-specific execution details live only in that provider's `.ai/<provider>.md`.
- Root stubs mirror discoverability only; they do not become policy sources.

## Persona Rules vs Skill Procedures

Keep role and procedure guidance separated:
- Persona role rules and boundaries: `.agent/rules/*`
- Skill procedures and checklists: `.ai/skills/*/SKILL.md`

Use persona files for role posture and constraints; use skill files for step-by-step workflows.

## Edit Here, Not There

For common governance edits, use these canonical targets:

| Change Type | Edit Here (Canonical) | Do Not Treat As Canonical |
| --- | --- | --- |
| Rule shared across all providers | `.ai/shared.md` | `AGENTS.md` |
| Codex-only behavior | `.ai/codex.md` | `AGENTS.md` |
| Persona role boundary or posture | `.agent/rules/<persona>.md` | `.ai/skills/*` |
| Repeatable operational checklist/procedure | `.ai/skills/<skill>/SKILL.md` | `.agent/rules/*` |
| Workflow policy/process narrative | `docs/ai/WORKFLOW.md` | Root stubs |

## Prompt Template Map

Prompt templates live in `docs/ai/` and are maintained as copy/paste entrypoints:
- `PROMPT_TEMPLATE.md`: architect-supervised implementation workflow for one tracked task or bugfix. The maintainer edits only the applicable `Work item:` selector line; the AI derives scope from that selector and auto-consumes matching failed-audit relay files if present.
- `AUDIT_PROMPT_TEMPLATE.md`: scope-locked adversarial audit workflow for one completed task or bugfix surface. The maintainer edits only the applicable `Audit target:` selector line; the AI derives scope from that selector, writes failed-audit relay files only when follow-up work is needed, and leaves `.agent/handoffs/` empty again after a clean PASS closeout.
- Both prompt templates enforce facts-first visibility updates (completed evidence before planned next action).

When template behavior or routing rules change, update both the template file and `docs/ai/WORKFLOW.md`.
