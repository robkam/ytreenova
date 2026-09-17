# YtreeNova Cline bridge

This repository's canonical AI policy lives in `AGENTS.md`, `.ai/shared.md`, and `docs/ai/*`.
In Cline, treat those files as authoritative even when they mention Codex-specific mechanisms.

## Source of truth

- Read `AGENTS.md` first.
- Then read `.ai/shared.md`.
- Load only the relevant parts of `docs/ai/*` for the current task.
- Treat `docs/ai/PROMPT_TEMPLATE.md` as the reference for manual implementation/remediation workflow prompts and `docs/ai/AUDIT_PROMPT_TEMPLATE.md` as the reference for scope-locked audit prompts.

## Codex-to-Cline translation

- When repo guidance names Codex-only tools or surfaces, preserve the intent and use the closest Cline equivalent.
- Use Cline MCP servers for `serena`, `jcodemunch`, and `github-mcp-server` when they are available.
- For repo discovery and search, use `serena` and `jcodemunch` first. Do not start with shell search or generic file crawling when the semantic MCP tools are available.
- If semantic MCP tools are unavailable, say so once and then fall back to Cline's built-in file/search tools.
- Do not ignore repo policy just because the wording mentions Codex.

## Persona routing

Choose the working role by user intent, then load the matching project skill through `.cline/skills.cline`:

- `architect` -> `architect-planning`
- `developer` -> `developer-implementation`
- `code_auditor` -> `code-auditor-gate`
- `tester` -> `tester-regression-design`
- `greybeard` -> `greybeard-meta-guidance`

Honor explicit persona switches like `:at architect`, `:at developer`, `:at code_auditor`, `:at tester`, and `:at greybeard`.

## Cross-cutting skills

Also load the matching project skill when the task calls for it:

- bugfix -> `bugfix-red-green-proof`
- feature-sized or major work -> `full-audit-gate-c`
- PR review or conflict triage -> `pr-gate-review`
- QA failure remediation -> `qa-root-cause-remediation`
- PTY or `pexpect` debugging -> `pty-pexpect-debug`
- ncurses rendering changes -> `ncurses-render-safety`
- keybinding or help/menu key changes -> `keybinding-collision-check`
- manpage or usage doc sync -> `manpage-sync`
- UI workflow or menu-depth design -> `ui-economy-navigation`
- UI prompt-chain or offender audit -> `ui-flow-offender-audit`
- code-quality audit or cleanup -> `code-quality`
- prose de-slop or AI-writing-tell work -> `ai-writing-tells`

## Multi-agent and file-safety constraints

- Never have two active agents edit the same file at the same time.
- Separate concurrent work by task, file set, or worktree.
- For this repo, prefer the documented architect -> developer -> code_auditor loop over ad hoc parallel editing.
- When the maintainer is using relay files, keep only the minimal temporary `.agent/handoffs/` state needed for the active work item, and delete leftovers before the next task or bug begins.

## Validation and execution

- Run commands from repo root: `~/ytreenova`.
- Activate the venv before pytest: `source .venv/bin/activate`.
- Run pytest and `make qa-*` with host permissions from the start when they are needed.
- During normal implementation, prefer focused checks first and reserve full `make qa-all` for explicit maintainer request or final confidence.

## Documentation discipline

- Keep repo guidance canonical in the existing files instead of duplicating policy into new Cline-only files.
- Use this rule file only as a bridge for Cline-specific interpretation.
