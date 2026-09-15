# **Contributing to ytnova**
> Thank you for your interest in contributing to ytnova! This document provides guidelines for setting up your development environment, running tests, and submitting your changes.
>
> See **[ARCHITECTURE.md](ARCHITECTURE.md)** for technical design principles (how the code is built).
> See **[SPECIFICATION.md](SPECIFICATION.md)** for behavioral requirements (how the UI behaves).
> See **[CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md)** for community expectations.

## Pull Request Scope

> **Submit small, focused pull requests. Each pull request should address a single fix or feature to keep reviews straightforward.**
> **Do not submit monolithic PRs. Prefer incremental (stacked/sequenced) PRs that preserve logical checkpoints.**
>
> See **[Submitting Changes](#submitting-changes)** for the full PR workflow.

## Pull Request Guidelines

- Ensure your PR addresses an existing issue or a well-defined new feature.
- Keep PRs focused on a single issue or feature.
- Provide a clear PR description and link any relevant issues.
- PR title/summary should describe the concrete behavior/problem fixed; do not rely on a volatile task or bug number as the primary description.
- Ensure your code passes all quality checks.
- Include or update tests for your changes. New features require tests; bug fixes should include a regression test when possible.
- Be responsive to review feedback and follow-up comments.

### Commit Message Guidelines

Use **Conventional Commits** so history stays searchable and consistent:

- Format: `<type>(<scope>): <description>` (scope optional).
- Allowed `type` values: `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `build`, `ci`, `chore`, `revert`.
- Write the description as the durable outcome of the change (not workflow notes).
- Do **not** use workflow labels like `task` or `step`, and do **not** include numeric workflow IDs.
- Keep subjects short and specific (prefer ~72 chars or less).

**Note for AI-Assisted Development:**
For the AI-assisted development workflow, script usage, System Persona prompts, and semantic tool integration (**Serena**, **jCodeMunch**), please see **[ai/WORKFLOW.md](ai/WORKFLOW.md)**.
Persona routing, shorthand persona commands, and persona-skill auto-load controls are documented in that same workflow doc.
For the continuous audit workflow (during development and PRs) plus final release PASS/FAIL criteria, see **[AUDIT.md](AUDIT.md)**.
For maintainer copy/paste prompts, see **[ai/TASK_PROMPT_TEMPLATE.md](ai/TASK_PROMPT_TEMPLATE.md)** for implementation/remediation work and **[ai/AUDIT_PROMPT_TEMPLATE.md](ai/AUDIT_PROMPT_TEMPLATE.md)** for scope-locked post-implementation audits.
For pull-request governance and conflict triage policy, see **[PR_GATE.md](PR_GATE.md)**.

## Planning and History Docs Policy

- `docs/ROADMAP.md` and `docs/BUGS.md` are forward-looking (`planned`/`in-progress`).
- Completed items may be removed from those active planning docs after landing/fix.
- `docs/CHANGELOG.md` is milestone-level only; do not add every trivial fix.
- Git history is the complete detailed archive of all changes.

## Development Setup

This project uses a combination of C for the application and Python for the test suite and AI assistance tools.

### 1. Prerequisites

- A C compiler (`gcc` or `clang`) and `make`.
- `libncurses-dev`, `libtinfo-dev`, `libreadline-dev`, `libarchive-dev`.
- `python3` and `python3-venv`.
- `cmark` (for generating the usage doc and man page).
- `lcov` (for baseline CI coverage reports).
- `llvm` tools (`llvm-symbolizer`) for sanitizer/fuzz diagnostics.

On a Debian/Ubuntu-based system, you can install these with:
```bash
sudo apt-get update
sudo apt-get install build-essential clang llvm libncurses-dev libtinfo-dev libreadline-dev libarchive-dev python3-venv cmark lcov
```

### 2. Initial Python Environment Setup

A helper script is provided to create an isolated Python environment for testing and AI tools. From the root of the project, run:

```bash
scripts/setup_dev.sh
```

This script will create a `.venv` directory, activate it, and install the pinned Python libraries from `scripts/requirements.txt`.

### 2.1 Updating Python Dependencies

Top-level dependencies are declared in `scripts/requirements.in`.
The pinned lock file is `scripts/requirements.txt`.

Refresh both with:

```bash
make py-requirements
```

This runs `scripts/update_python_requirements.sh`, which uses `pip-compile --upgrade` and writes a deterministic lock file.

After the first run, you must reactivate this environment for any new shell session:
```bash
source .venv/bin/activate
```

### 3. Enable Repository Git Hooks (Recommended)

This repository ships tracked hooks under `.githooks/` so the pre-push checks are maintained in one place.
Enable them once per clone:

```bash
make hooks-install
```

This installs a tracked pre-push gate:
- Pushes with codebase changes (`src/`, `include/`, `tests/`, `scripts/`, `.githooks/`, `Makefile`): run `make qa-code-quality` (which now includes `make qa-clean-code`).
- Pushes that also update `main` run `make ci-baseline` after `qa-code-quality`.
- Pushes without codebase changes (for example docs-only updates): skip the local pre-push quality gate.
- Set `YTNOVA_PRE_PUSH_FORCE=1` to force `make qa-code-quality`.

`make hooks-install` also installs repo-local git aliases so fast push is available as native git subcommands in this clone:
- `git push-fast-up` -> fast push with `-u origin <current-branch>` for first push of a new branch.
- `git push-fast` -> fast push for an already-tracked branch.

For amended or rebased commits, pass force-with-lease through the alias:

```bash
git push-fast --force-with-lease
```

To inspect current hook configuration:

```bash
make hooks-status
```

To inspect alias installation state:

```bash
git config --local --get-regexp '^alias\\.push-fast'
```

### 4. Branch and Merge Gate Policy (Required)

Use branch-first development and keep `main` protected:

1. Create a feature branch from `main` (`feature/...`, `fix/...`, `docs/...`).
2. Push to the feature branch.
3. Open a PR into `main`.
4. Merge only when required checks are green.

Configure GitHub branch protection on `main`:

1. Settings -> Branches -> Add branch protection rule for `main`.
2. Enable `Require status checks to pass before merging`.
3. Select required checks from GitHub Actions:
   - `.github/workflows/ci.yml`: `Docs gate`.
   - `.github/workflows/full-qa.yml`: `Guard and code-quality gate`.
   - `.github/workflows/ci.yml`: `Guard fuzz harness sync`.
   - `.github/workflows/ci.yml`: `File mutation integrity gate`.
   - `.github/workflows/full-qa.yml`: `Static analyzer gate`.
   - `.github/workflows/full-qa.yml`: `Runtime and security gate`.
   - `.github/workflows/full-qa.yml`: `Full pytest gate`.
   - `.github/workflows/full-qa.yml`: `Sanitizer gate`.
   - `.github/workflows/ci.yml`: `Full coverage baseline gate`.
   - `.github/workflows/ci.yml`: `Fuzz baseline gate`.
   - `.github/workflows/pr-conflict-assistant.yml`: `Up To Date With Main`.
4. Enable `Require branches to be up to date before merging`.

### 5. Handling Red Checks on Branches

- A red check on an old commit cannot be turned green retroactively.
- Merge gating is based on the latest commit status on the PR branch.
- To clean PR history after fixes, squash/rebase your branch and force-push with lease:

```bash
git rebase -i origin/main
git push --force-with-lease
```

- If you use fast bypass during iteration, run a full gate before final PR update:

```bash
git push
```

## Building the Project

The `Makefile` supports two build modes: Release (default) and Debug.

### Release Build
For standard testing and usage, perform a release build. This applies optimizations (`-O2`) and produces a binary with no runtime debug dependencies.

```bash
make clean
make
```

### Debug Build (AddressSanitizer)
When developing new features or debugging crashes (especially segmentation faults), you should compile with debug mode enabled. This links against **AddressSanitizer (ASan)** and disables optimizations (`-O1`), providing detailed stack traces on memory errors.

```bash
make clean
make DEBUG=1
```

**Note:** The debug binary is significantly slower. Do not use this for general performance testing.

## Running the Test Suite

**Prerequisite:** Ensure your Python virtual environment is activated (`source .venv/bin/activate`).

```bash
# Standard run (quiet, shows progress dots) — auto-rebuilds the binary
make test

# Verbose run (shows individual test names and stdout)
make test-v
```

For direct `pytest` CLI usage, test naming conventions, infrastructure details, and harness rules, see **[ai/TESTING.md](ai/TESTING.md)**.

## Audit Workflow (Required)

Use **[AUDIT.md](AUDIT.md)** as the single source of truth.

- During implementation, run focused checks first (`make` + targeted tests for touched scope).
- Before merge to `main`, require green PR full-QA CI (`.github/workflows/full-qa.yml`, `make qa-all` equivalent). Local full audit loop is optional unless explicitly requested.
- Treat any PR that triggers `.github/workflows/full-qa.yml` (`src/**`, `include/**`, `tests/**`, `scripts/**`, `Makefile`, `.github/workflows/**`) as non-trivial: include `make qa-unsafe-apis` in the validation evidence, and include `make qa-fileops-integrity` as well when file/archive mutation flows change.
- Split-touching PRs must also pass the path-filtered `make qa-split-panel-gates` CI gate before merge.
- Use the canonical tool order from `AUDIT.md`: `clang-tidy`, `cppcheck`, `scan-build`, `valgrind`, then `pytest` for regression verification.

### Local QA Commands

- Normal development build: `make`
- Full local QA gate (optional unless maintainer-requested): `make qa-all` (includes `pytest`, unsafe C API guard, module-boundary guard, and fuzz smoke)
- Split panel regression gate (use when touching split-panel invariants, transition handoff, or split-authority code): `make qa-split-panel-gates`
- Full local QA gate with captured log (optional unless maintainer-requested): `make qa-all-log` (writes `qa-all.log` in repo root; override with `QA_LOG=/path/to/file`)
- Max-depth unattended QA sweep: `make qa-deep` (runs composite deep checks and writes structured logs to `${TMPDIR:-/tmp}/ytnova-qa-deep/<timestamp>/`; override with `QA_DEEP_LOG_ROOT=/path`)
- Optional strict mode: `make QA_ON_BUILD=1` (runs `qa-all` after build)
- GitHub baseline CI (`.github/workflows/ci.yml`) runs `make ci-baseline` on PRs to `main` and pushes to `main`.
- GitHub split panel regression CI (`.github/workflows/ci.yml`) runs `make qa-split-panel-gates` on PRs that touch split-panel paths.
- GitHub PR full QA CI (`.github/workflows/full-qa.yml`) runs `make qa-all` on PRs to `main` and is the required full pre-merge gate.
- GitHub PR sanitizer gate in `.github/workflows/full-qa.yml` runs on the same full-QA-triggering PRs as the other pre-merge full-QA jobs.
- GitHub nightly deep Valgrind CI (`.github/workflows/nightly-deep-valgrind.yml`) runs `make qa-valgrind-full` on schedule (and manual dispatch).

Individual gates:

- `make qa-clang`
- `make qa-cppcheck`
- `make qa-scan`
- `make qa-valgrind` (non-interactive Valgrind smoke on `ytnova --version`)
- `make qa-valgrind-interactive` (manual interactive Valgrind session)
- `make qa-valgrind-full` (scripted deep Valgrind session)
- `make qa-pytest`
- `make qa-pytest-coverage` (coverage build + pytest + gcov/lcov report)
- `make qa-unsafe-apis`
- `make qa-module-boundaries`
- `make qa-clean-code`
- `make qa-dead-history-comments`
- `make qa-ai-config`
- `make qa-code-quality` (runs `qa-unsafe-apis`, `qa-dead-history-comments`, `qa-clean-code`, `qa-appstate-contract`, `qa-ai-config`, and the generated catalog/template sync guards)
- `make qa-fuzz` (builds and runs all fuzz smoke targets)
- `make qa-deep` (max-depth composite audit with per-step timing and AI-handoff artifacts)
- `make fuzz` (builds all fuzz binaries under `build/fuzz/`)
- `make fuzz-smoke` (runs bounded libFuzzer smoke passes for all harnesses)

### Recurring code-quality burn-down cadence

When you are doing deliberate debt burn-down work rather than a one-off bugfix, run the recurring cadence described in `docs/AUDIT.md` and `docs/ai/CODE_QUALITY.md`.

The minimum evidence set is:

```bash
python3 scripts/report_code_quality_hotspots.py --format json > /tmp/ytnova-hotspots-before.json
python3 scripts/report_code_quality_hotspots.py --baseline /tmp/ytnova-hotspots-before.json --format markdown --top 5
make
source .venv/bin/activate && pytest -q tests/path/to/focused_test.py
make qa-module-boundaries
```

Also run the matching bundled QA gates for the surfaces you changed (`make qa-clean-code`, `make qa-code-quality`, `make qa-fileops-integrity`, `make qa-split-panel-gates`, and so on). Record which hotspots shrank, which stayed flat, and any explicitly deferred items.

---

## Architectural Decisions & Constraints

The project enforces strict architectural constraints (single-threaded event loop, hard ncurses dependency, context-passing with no global mutable state). For the full specification of these rules, permitted exceptions, and rationale, see **[ARCHITECTURE.md](ARCHITECTURE.md)**.

---

## Submitting Changes

> **Submit small, focused pull requests. Each pull request should address a single fix or feature to keep reviews straightforward.**
> **Do not submit monolithic PRs. Prefer incremental (stacked/sequenced) PRs that preserve logical checkpoints.**

1.  **Fork the repository** on GitHub.
2.  **Create a new branch** for your feature or bugfix (`git checkout -b feature/my-new-feature`).
3.  **Make your changes** and commit them using **Conventional Commits** (`type(scope): summary`).
    Keep the message specific to the change and use scopes that match the area touched.
    Examples:
    - `fix(ui): keep active panel selection after volume cycle`
    - `docs(ai): clarify planner/executor packet handoff rules`
    - `test(pytest): add regression for split-panel redraw ordering`
4.  **Push your branch** to your fork (`git push origin feature/my-new-feature`).
5.  **Open a Pull Request** against the `main` branch of the upstream ytnova repository.

### Bugfix Gate (Red -> Green)

For bug fixes, include strict red-green evidence in the PR:
- Add or adjust a regression test first.
- Demonstrate that the regression test fails on current code before implementation changes.
- After implementing the fix, re-run and show that the regression test is green.

## Coding Style

Please adhere to the existing coding style found throughout the project. The codebase follows C89/C99 standards with a focus on readability, consistency, and proper resource management.

## Security Practices

Contributions must preserve a secure-by-default codebase:

- Do not introduce known vulnerability classes (buffer/integer overflows, format-string misuse, use-after-free/double-free, path traversal, symlink TOCTOU races, command injection).
- Validate and bound-check untrusted input before memory, file, or process operations.
- Prefer standard/POSIX and well-maintained existing primitives over custom security-sensitive implementations.
- Default to fail-closed behavior on invalid or unexpected states.
- Use least-privilege file/process handling; avoid broad permissions or escalation unless explicitly required.

Security evidence is part of the required audit flow: include `make qa-unsafe-apis` results for every non-trivial PR per **[AUDIT.md](AUDIT.md)**.
If you change safety-sensitive behavior, update **[TRUST.md](TRUST.md)** so user-facing trust claims stay accurate.

## Source Comment Policy

Keep source code self-explanatory where possible. Use comments for durable, non-obvious context:

- Document invariants, ownership/lifetime assumptions, aliasing constraints, and rationale for unusual behavior.
- Do not restate obvious code mechanics line-by-line.
- Do not use source comments as temporary change history ("fixed yesterday", "changed in commit ..."). Run `make qa-dead-history-comments` when you are touching comment-heavy code or guard wiring.
- If a comment becomes stale due to a code change, update or remove it in the same change.

## Ncurses Guidelines

### Key Bindings

When mapping keys in a terminal environment, many Control combinations generate the same ASCII codes as physical keys. Binding these "Taboo" keys will break standard navigation or cause unexpected behavior.
Also note: `Ctrl+Shift+<letter>` is not distinct from `Ctrl+<letter>` in terminal byte streams, so it cannot be used as a separate binding.

**Taboo Bindings (Do Not Use)**

| Control Key | ASCII Value | Physical Key Collision | Impact |
| :--- | :--- | :--- | :--- |
| **`^I`** | `0x09` | **Tab** | Navigation failure. |
| **`^M`** | `0x0D` | **Enter** (CR) | Inability to confirm actions. |
| **`^J`** | `0x0A` | **Enter** (LF) | Inability to confirm actions. |
| **`^[`** | `0x1B` | **Escape** | Breaks cancel/exit logic. |

**Safe Control Keys**

Preferred bindings that generally avoid conflicts:
`^A` (Home), `^B`, `^E` (End), `^F`, `^G`, `^K`, `^L` (Redraw), `^N`, `^O`, `^P`, `^R`, `^T`, `^U`, `^V`, `^W`, `^X`, `^Y`.

### Rendering

When working with ncurses rendering, follow these rules:
1.  Set `WbkgdSet()` once per window refresh path, not inside per-line loops.
2.  After setting window background, call `werase()` before drawing lines.
3.  Use `wattron()`/`wattroff()` for line-level styling.

These three rules prevent flicker, color bleed, and inconsistent redraw behavior.
