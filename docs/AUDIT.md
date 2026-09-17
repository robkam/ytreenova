# AUDIT.md

## 1. Purpose
This document defines the mandatory quality process for YtreeNova. Auditing is an ongoing process that starts during implementation and continues through merge and release. The release gate is the final checkpoint.
[MAINTENANCE.md](MAINTENANCE.md) is the canonical register of recurring maintenance families and their triggers; this document owns the commands and audit procedures used to execute them.

## 1.1 Cadence
- Use focused checks during feature-sized change or PR iteration.
- Before merge to `main`, require green PR full-QA CI (`make qa-all` equivalent). Local full audit loop is optional unless explicitly requested by the maintainer.
- For feature-sized changes, include explicit `make qa-module-boundaries` evidence (controller allowlists + growth budgets) in the verification notes.
- Always run the merge/release gate before merge/release.
- Do not run the full gate after every prompt-level micro-edit unless risk justifies it.

### 1.1.1 Recurring Code-Quality Burn-Down Cadence

Recurring code-quality burn-down passes are mandatory on this cadence:

1. After every **five merged structural PRs** that touch `src/`, `include/`, or code-quality guard scripts under `scripts/`.
2. Before any milestone or release tag.
3. When a PR intentionally lowers a controller/file/function budget or retires a documented legacy boundary exception.

Each burn-down pass must attach measurable before/after evidence for the same hotspot rows:

```bash
python3 scripts/report_code_quality_hotspots.py --format json > /tmp/ytnova-hotspots-before.json
python3 scripts/report_code_quality_hotspots.py --baseline /tmp/ytnova-hotspots-before.json --format markdown --top 5
```

Required local evidence for the pass:

- `make`
- focused `pytest` for the touched behavior
- `make qa-module-boundaries`

Add the other bundled gates that match the changed risk surface (`make qa-code-quality`, `make qa-fileops-integrity`, `make qa-split-panel-gates`, and so on). The detailed smell taxonomy, simplicity contract, and burn-down checklist live in `docs/ai/CODE_QUALITY.md`.

## 1.2 QA Layers

The project uses seven QA layers with increasing depth and cost:

| Layer | Command | What it checks | When to run |
|---|---|---|---|
| CI Gate | `git push` (automatic) | Draft-PR baseline confidence checks (`qa-code-quality` + `qa-fileops-integrity`, path-filtered `qa-split-panel-gates` for split-touching changes, docs gate for docs-touching changes, coverage pytest gate, fuzz gate) | Every push to `main` and every PR update targeting `main` (automatic) |
| PR Full QA CI (required) | `.github/workflows/full-qa.yml` (`make qa-all`) | clang-tidy, cppcheck, scan-build, Valgrind smoke (`--version`), full `pytest`, test-contract resilience guard, tracker-identifier leak guard, unsafe API guard, dead-history comment guard, compatibility-shim guard, gitleaks, module-boundary guard, ai-config guard, fuzz guard | Must be green before merge to `main` |
| Fileops Integrity Gate | `make qa-fileops-integrity` | Deterministic file/archive mutation integrity + security regression checks (copy/move/delete/rename/archive rewrite, cancel/failure safeguards, shell/tempfile hardening contracts) | Before merge and when touching file/archive mutation flows |
| Sanitizer QA | `make qa-sanitize` | Main ytnova build + `pytest` under AddressSanitizer/UndefinedBehaviorSanitizer | Before release, after memory/UB-sensitive changes, or when triaging suspicious crashes |
| Deep Audit | `make qa-valgrind-full` | Automated interactive Valgrind Memcheck session (leak, uninit, FD, use-after-free checks) | Before release, after major refactoring, or periodically |
| Max-Depth Composite Audit | `make qa-deep` | Runs `qa-all`, `qa-pytest-coverage`, `qa-sanitize`, and `qa-valgrind-full`; captures per-step timing, pytest duration artifacts, and AI-handoff triage files into timestamped temp logs | Periodic deep health checks, pre-release confidence sweeps, and unattended overnight runs |
| Manual Feature Audit | `make qa-valgrind-interactive` | You manually drive ytnova under Valgrind to exercise new feature code paths | After adding a major new feature |

- **CI Gate** runs automatically on push via GitHub Actions. No developer action needed.
- **PR Full QA CI** (`.github/workflows/full-qa.yml`, `make qa-all` equivalent) is the standard pre-merge gate.
- **Local QA** (`make qa-all`) is optional for faster local confidence and maintainer-requested deep preflight; avoid running it on every iteration by default.
- **Fileops Integrity Gate** (`make qa-fileops-integrity`) is the dedicated regression wall for mutation integrity/security contracts; run it before merge and whenever file/archive mutation code or prompts change.
- **Split Panel Regression Gate** (`make qa-split-panel-gates`) is the path-filtered regression wall for split-panel invariants, transition handoff, and split-authority source guards; split-touching PRs must satisfy it before merge.
- **Non-trivial PRs** are the PRs that trigger `.github/workflows/full-qa.yml` (`src/**`, `include/**`, `tests/**`, `scripts/**`, `Makefile`, `.github/workflows/**`). They must carry explicit security evidence in PR validation notes (`make qa-unsafe-apis`, plus `make qa-fileops-integrity` when file/archive mutation flows change) and must not merge until the required security/full-QA checks are green.
- **Deep Audit** (`make qa-valgrind-full`) is on-demand. It drives a scripted interactive ytnova session under Valgrind and takes ~2-3 minutes. Run it:
  - Before tagging a release
  - After changes to memory management, allocation, or cleanup paths
  - After major refactoring sessions
  - Periodically as a health check
  - After reviewing the results, remove `valgrind.log` from the repository root when it is no longer needed
- **Sanitizer QA** (`make qa-sanitize`) is on-demand and complementary to Valgrind. It builds the main binary with ASan/UBSan and runs `pytest` with fail-fast sanitizer settings. Run it:
  - After touching pointer arithmetic, allocation/free paths, or integer-heavy logic
  - When triaging intermittent or environment-sensitive crashes
  - Before alpha/release candidates as an extra memory/UB gate
- **Max-Depth Composite Audit** (`make qa-deep`) is on-demand and unattended. It composes static, dynamic, sanitizer, coverage, and deep Valgrind checks in one run and writes structured triage output (summary, failures, timing, handoff prompt) under `${TMPDIR:-/tmp}/ytnova-qa-deep/<timestamp>/` by default.
- **Manual Feature Audit** (`make qa-valgrind-interactive`) launches ytnova under Valgrind for you to drive manually. Use it after adding a major feature to exercise the new code paths specifically. Exit cleanly, then inspect `valgrind.txt`.

## 1.3 Gate Organization & Efficiency

This section defines how to keep QA gates efficient during iteration without reducing safety.
Scope is strict: QA/check/test organization and efficiency only. Non-QA workflow-policy edits (for example PR title/body wording rules) are out of scope here and must be tracked in a separate task/PR.

### Branch + PR Workflow (Mandatory)

- Never push directly to `main`.
- Always use a non-`main` branch and PR workflow.
- If work was committed locally on `main`, create a branch from that current `HEAD` before first push.

### Hybrid PR Cadence (Mandatory)

- Before first push, run a quick local gate (`make`, plus targeted smoke/tests for touched scope).
- Open a regular PR early; PR checks may be red while iterating.
- Do not request review while checks are red unless the maintainer explicitly asks.
- Before merge to `main`, require green PR full-QA CI (`make qa-all` equivalent) plus required loop evidence from this document; local full audit reruns are optional unless the maintainer explicitly requests them.
- Before merge, require green PR checks and reviewer signoff.

### Required Gate Tiers

| Tier | Owner | Trigger | Required checks | Non-overlap default intent |
|---|---|---|---|---|
| Tier A (local fast iteration) | Developer | During implementation before first push and between risky edits | `make`; targeted pytest for touched scope; targeted guards (`qa-unsafe-apis`, `qa-fileops-integrity`) when relevant | Keep iteration fast; avoid full-suite duplication unless local risk demands it |
| Tier B (PR baseline CI) | CI + PR author | Every push while the PR is active | `ci-baseline` (`qa-code-quality` + `qa-fileops-integrity` + `qa-pytest-coverage` + `qa-fuzz`) plus path-filtered `Docs gate` on docs-touching PRs and `qa-split-panel-gates` on split-touching PRs | Provide baseline branch-protection signal (includes coverage by design); do not duplicate Tier C full local gate content |
| Tier C (pre-merge full gate) | CI + PR author | Before merge to `main` (or earlier only when explicitly requested) | Green PR full-QA CI (`.github/workflows/full-qa.yml`, `make qa-all` equivalent); explicit `make qa-unsafe-apis` evidence for full-QA-triggering PRs; optional local `make qa-all-log` evidence when maintainer-requested; plus explicit `qa-fileops-integrity` evidence when mutation workflows changed | Use CI as canonical full-gate signal; avoid duplicate local full-gate reruns during routine iteration |
| Tier D (merge/release gate) | Maintainer + reviewer | Before merge and before release/tag cut | Branch-protection checks green; reviewer signoff; `qa-sanitize`; `qa-valgrind-full` for release-risk changes; `qa-valgrind-interactive` after major feature flows | Reserve deepest runtime checks for merge/release assurance to avoid slowing every draft iteration |

### branch-protection and PR State Criteria (Mandatory)

- **Open PR:** Branch exists and the PR is open. Red checks are acceptable while iterating. No review requests while checks are red unless the maintainer explicitly asks.
- **Merge:** All required branch-protection checks are green, reviewer signoff is present, and Tier D evidence is attached for the current diff/risk.
- Required branch-protection checks (sync with current workflows):
  - `.github/workflows/ci.yml`: `Docs gate`
  - `.github/workflows/full-qa.yml`: `Guard and code-quality gate`
  - `.github/workflows/ci.yml`: `Guard fuzz harness sync`
  - `.github/workflows/ci.yml`: `File mutation integrity gate`
  - `.github/workflows/full-qa.yml`: `Static analyzer gate`
  - `.github/workflows/full-qa.yml`: `Runtime and security gate`
  - `.github/workflows/full-qa.yml`: `Full pytest gate`
  - `.github/workflows/full-qa.yml`: `Sanitizer gate`
  - `.github/workflows/ci.yml`: `Full coverage baseline gate`
  - `.github/workflows/ci.yml`: `Fuzz baseline gate`
  - `.github/workflows/pr-conflict-assistant.yml`: `Up To Date With Main`

### Duplication Control Policy

- Keep all beneficial checks, but avoid running overlapping suites in the same gate unless explicitly justified.
- Prefer targeted suites for iteration cadence and reserve full-suite runs for review/merge gates.
- Any gate optimization must preserve equivalent or stronger defect-detection coverage.

### Efficiency Rules for Tests

- Prefer parametrized tests and shared helpers over near-duplicate test bodies.
- Replace fixed sleeps with deterministic condition/poll-based waits where possible.
- Treat flaky behavior as a root-cause issue; do not weaken gates with silent skips/suppressions.

### Fail-Fast Ordering Guidance

Within a gate, prefer this order:

1. Cheap high-signal policy checks (guards/scripts).
2. Targeted pytest suites for touched risk areas.
3. Full pytest (when required by the gate).
4. Static analyzers and deep runtime analyzers.

### Runtime Budget + Trend Reporting (Mandatory)

Track these metrics per PR and as rolling team trends:

- **median PR feedback time:** median elapsed time from PR push to first CI result.
- **time-to-green:** elapsed time from first PR push to all required branch-protection checks passing.
- **full-gate runtime:** runtime for full local/CI gates (`make qa-all-log`, plus sanitizer/deep gates when invoked).
- **flake rate:** reruns caused by non-deterministic failures ÷ total gate runs.

Report metrics in PR notes or release prep notes with before/after deltas when gate organization changes.

Provisional targets (make completion auditable; tighten over time as data improves):

- **Tier B p50 runtime target:** <= 30 minutes per PR push.
- **Full-gate p50 runtime target:** <= 60 minutes for `make qa-all-log` evidence runs.
- **Max acceptable flake rate:** <= 5% of gate runs requiring rerun due to non-determinism.
- **Target time-to-green:** <= 120 minutes from first PR push to required checks all green.

### Rollout, Rollback, and Risk Register (Mandatory)

Rollout criteria:

- Every check listed in Tier B/C/D remains present or has documented equivalent/stronger replacement.
- At least one full Ready-for-review cycle demonstrates complete evidence flow with no coverage regressions.
- Trend metrics are captured for comparison against prior baseline.

Rollback criteria:

- Any safety-critical check is removed or bypassed without equivalent replacement.
- Flake rate or time-to-green regresses for consecutive cycles with no matching quality gain.
- A missed regression shows reduced signal quality caused by gate reorganization.

Risk register (coverage/signal preservation):

| Risk | Signal-quality guard | Rollback trigger |
|---|---|---|
| Fast-tier drift removes critical checks | Tier B/C/D matrix requires explicit named checks and owners | Any missing required check in CI/PR evidence |
| Runtime optimizations hide flaky behavior | Track flake rate and require deterministic repro/root-cause handling | Flake rate trend worsens without root-cause fixes |
| Deep runtime checks deferred too aggressively | Tier D requires sanitizer + deep Valgrind at merge/release cadence | Release/merge evidence lacks required deep-runtime gates |

## 1.4 Security Baseline Audit Evidence (2026-08-15)

This dated table records the runtime-launch security baseline until a later audit replaces it; it is not an active backlog. Unresolved confirmed defects and planned improvements must also be recorded in `docs/BUGS.md` or `docs/ROADMAP.md`.
Its scope is limited to the four Phase 5 security families already named in `docs/ROADMAP.md`:
shell-command construction/escaping, archive path trust, tempfile lifecycle, and unsafe API usage.

| Risk family | Current evidence | Baseline finding | Severity | Owner | Disposition | Residual risk |
|---|---|---|---|---|---|---|
| Shell-command construction and escaping | `tests/test_security_shell_paths.py` proves compare/view/execute placeholder paths preserve literal filenames containing shell metacharacters, locks the `QuerySystemCall()` restore ordering, and asserts the remaining runtime-launch debt surfaces now route through the shared launcher. | Runtime command paths now converge on `src/cmd/runtime_launch.c` for `fork()`/`execvp()`/`waitpid()` handling across synchronous commands, print/tagged pipes, captured helper output, detached application launches, and the final `stty sane` fallback. Some explicit command-template surfaces still execute shell command lines via `sh -c`, so quoting discipline remains relevant, but the mixed `system()`/`popen()` launch debt is gone. | Medium | Runtime command launch + file-command flows | Mitigated by the runtime-launch hardening; keep placeholder-quoting regression coverage and shared-launcher assertions mandatory for future command-surface edits. | User-configured command templates can still invoke shell syntax intentionally, so future placeholder or quoting drift could re-open injection risk even though the launch backend is now standardized. |
| Archive path trust policy | `src/fs/archive_read.c` centralizes `Archive_ValidateInternalPath()`, and inspected call paths in `src/cmd/execute.c`, `src/ui/view_preview.c`, and `src/ui/tagged_view.c` reject unsafe archive member paths before use. | No blocker/high defect was confirmed in the inspected call paths, but the trust policy is still enforced mostly by code inspection rather than a dedicated regression suite that proves traversal rejection across extract/preview/tagged-view surfaces. | Medium | Archive/fileops runtime + regression tests | Planned under **Multi-Round Adversarial Security Review**; add explicit regression coverage before expanding archive extraction behavior further. | A future call path that bypasses `Archive_ValidateInternalPath()` could reopen archive traversal risk without an audit catching it quickly. |
| Tempfile lifecycle | `src/util/path_utils.c` provides `Path_BuildTempTemplate()` and `Path_CreateTempFile()`, and `tests/test_security_tempfiles.py` plus `tests/test_commands_exhaustive.py` lock the execute/view/hex/preview callers onto that shared helper. | `src/cmd/copy.c` still hardcodes `/tmp/ytnova_copy_XXXXXX` and calls `mkstemp()` directly when it extracts archive content for copy workflows, so the tempfile policy is not yet fully centralized. | Medium | File/archive mutation flows | Tracked in `docs/BUGS.md` as **Archive Copy Tempfiles Bypass Shared TMPDIR Policy**. | TMPDIR-aware policy, cleanup invariants, and future guard expansion can still drift because one production path bypasses the shared helper. |
| Unsafe API usage and QA detection | `make qa-unsafe-apis` runs `scripts/check_c_unsafe_apis.py`, which rejects the legacy string APIs plus disallowed runtime-launch APIs/patterns (`system`, `popen`, `execl*`, `execv*`, `posix_spawn*`) with no remaining runtime-launch grandfathering; `tests/test_c_unsafe_apis_guard.py` locks both the denylist and the now-empty allowlist baseline, and `make qa-fileops-integrity` still covers the command/tempfile regression tests. | No known production runtime path still uses `system()`/`popen()` or non-`execvp()` launch variants, and the guard now fails on every reintroduction attempt rather than carrying audited debt exceptions forward. | Medium | Build/QA guard scripts | The runtime-launch hardening completed the migration; keep `make qa-unsafe-apis` mandatory in runtime-launch change PR evidence and extend the guard only if a new runtime-launch surface appears. | The guard depends on repository coverage of runtime-launch source files, so new launch helpers or moved code must stay within the scanned `src/**/*.c` surface. |

Inspected surfaces for this baseline:

- `docs/ROADMAP.md`
- `scripts/check_c_unsafe_apis.py`
- `Makefile` (`qa-unsafe-apis`, `qa-fileops-integrity`, `qa-all`)
- `src/cmd/runtime_launch.c`
- `src/cmd/system.c`
- `src/cmd/pipe.c`
- `src/cmd/print_ops.c`
- `src/ui/ctrl_file_ops.c`
- `src/ui/fileinfo_git.c`
- `src/ui/render_file.c`
- `src/core/quit.c`
- `src/cmd/system.c`
- `src/cmd/print_ops.c`
- `src/ui/ctrl_file_ops.c`
- `src/core/quit.c`
- `src/ui/fileinfo_git.c`
- `src/ui/render_file.c`
- `src/fs/archive_read.c`
- `src/util/path_utils.c`
- `src/cmd/copy.c`
- `tests/test_c_unsafe_apis_guard.py`
- `tests/test_security_shell_paths.py`
- `tests/test_security_tempfiles.py`
- `tests/test_commands_exhaustive.py`

## 2. Role Mapping
The workflow relies on four distinct roles. When using AI agents, these must be treated as adversarial personas to ensure objectivity.

- **Architect (Human):** Defines scope, constraints, and acceptance criteria.
- **Developer (Agent):** Implements logic. Primary tool: `clangd` (LSP) via `compile_commands.json`.
- **Code Auditor (Agent/Human):** adversarial review. Static tools: `clang-tidy`, `cppcheck`, `scan-build`.
- **Tester (Human/Agent):** Validates regression safety. Dynamic tools: `pytest`, `Valgrind`.

### 2.1 AI Persona Execution Mapping
When an AI agent performs this workflow, it must execute the loop explicitly using these personas in order:
1. Architect (scope/invariants/acceptance criteria)
2. Developer (implementation/fixes)
3. Code Auditor (findings/gate decision)
4. Tester (verification/regression)
5. Code Auditor (final pass/fail)

## 3. Mandatory Toolchain Commands
Run these commands in this order to generate evidence-based findings.

0. **Compile Database Preflight:** `make clean && bear -- make`
1. **Linting & Modernization:** `clang-tidy $(rg --files src -g '*.c') -p .`
2. **Static Analysis:** `cppcheck --enable=all --inconclusive --force --std=c99 -I include --error-exitcode=1 --suppressions-list=.cppcheck-suppressions.txt src include`
3. **Logic Path Analysis:** `make clean && scan-build --status-bugs make`
4. **Memory/Runtime Analysis:** `valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 --log-file=valgrind.txt ./build/ytnova .` (then exit ytnova cleanly). For automated interactive runs, use `make qa-valgrind-full` which drives a scripted pexpect session.
5. **Regression Tests:** `source .venv/bin/activate && pytest`
6. **Secret Scanning:** `gitleaks detect --source . --redact --exit-code 1` (or `gitleaks dir --redact --exit-code 1 .` on newer CLI variants)

For step 4 in automated runs, drive a deterministic start/exit path (for example with `pexpect`) so Valgrind can finish and return an actionable exit code.

Local shortcut targets are available in the `Makefile`:
- `make qa-clang`
- `make qa-cppcheck`
- `make qa-scan`
- `make qa-valgrind`
- `make qa-valgrind-full`
- `make qa-valgrind-interactive`
- `make qa-sanitize`
- `make qa-pytest`
- `make qa-fileops-integrity`
- `make qa-split-panel-gates`
- `make qa-unsafe-apis`
- `make qa-gitleaks`
- `make qa-module-boundaries`
- `make qa-clean-code`
- `make qa-dead-history-comments`
- `make qa-compatibility-shims`
- `make qa-ai-config`
- `make qa-code-quality` (runs `qa-unsafe-apis`, `qa-dead-history-comments`, `qa-compatibility-shims`, `qa-clean-code`, `qa-appstate-contract`, `qa-ai-config`, `qa-tracker-id-leaks`, and the generated catalog/template sync guards)
- `make qa-split-panel-gates` (runs the split-panel invariants, transition-handoff, and split-authority regression subset)
- `make qa-fuzz`
- `make qa-all` (runs `qa-clang`, `qa-cppcheck`, `qa-scan`, `qa-valgrind`, `qa-pytest`, `qa-code-quality`, `qa-gitleaks`, `qa-fuzz` in order; run `qa-fileops-integrity` separately when touching mutation flows)
- `make qa-all-log` (same as `qa-all`, with full output captured to `qa-all.log` in repo root; override with `QA_LOG=/path/to/file`)
- `make qa-deep` (max-depth unattended composite run; default logs in `${TMPDIR:-/tmp}/ytnova-qa-deep`; override root with `QA_DEEP_LOG_ROOT=/path`)

For feature-sized/PR-scope changes, audit evidence must include a successful `make qa-module-boundaries` run so controller-slimming checks are explicitly validated.
Audit evidence must also include `make qa-unsafe-apis` results as explicit enforcement evidence for the shared Security gate policy in `.ai/shared.md` Core Engineering Rules.
Split-touching PRs must also include `make qa-split-panel-gates` evidence so panel isolation and split-authority contracts remain enforced in CI and audit notes.

GitHub CI is a baseline gate (including `qa-fileops-integrity`, path-filtered `qa-split-panel-gates` for split-touching changes, coverage pytest, and `qa-fuzz`) and does not replace the full local audit loop.

## 4. Continuous Audit Loop (Default)
Run this loop for every non-trivial change and every PR.

### Phase A: Architecture Check (Architect)
- Confirm scope, invariants, and acceptance criteria.
- Identify modules/symbols affected and known risk areas (e.g., UI redraw, pointer arithmetic).

### Phase B: Fix (Developer)
- **Real-time Guard:** `clangd` must be active. The Developer must resolve all LSP diagnostics before proceeding.
- **Task:** Implement fixes or features using atomic prompts. Iterate on the prompt (not the code) until functional requirements are met.

### Phase C: Audit (Code Auditor)
- **Lint Gate:** Run `clang-tidy $(rg --files src -g '*.c') -p .`. Any unresolved correctness/safety finding is an automatic REJECT.
- **Static Gate:** Run `cppcheck --enable=all --inconclusive --force --std=c99 -I include --error-exitcode=1 --suppressions-list=.cppcheck-suppressions.txt src include`. Any unresolved finding is an automatic REJECT.
- **Logic Gate:** Run `make clean && scan-build --status-bugs make`. Any reported analyzer bug is an automatic REJECT.
- **Rules:** Provide file:line and evidence for every finding. Prefer root-cause fixes over "if(ptr)" symptom patches.
- **Tracking Rule:** Medium/Low findings must be either fixed in the PR or explicitly tracked with rationale.

### Phase D: Verify (Tester)
- **Memory Gate:** Run `valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 --log-file=valgrind.txt ./build/ytnova .` and exit cleanly.
- **Dynamic Gate:** Run relevant `pytest` suites for the touched scope (or full `pytest` when scope is broad).
- **Criteria:** Failure occurs if any test fails or if Valgrind returns a non-zero exit code.

### Phase E: Merge Gate (Code Auditor)
- Re-audit the final PR diff before merge.
- **Exit Criteria:** Blocker = 0, High = 0, scan-build = Green for scoped analysis, and verification evidence attached.

## 5. Release Gate (Final Checkpoint)
Before a release tag/cut, run a full audit pass across the release scope:
- Fresh `clang-tidy` run against all `src/**/*.c` files.
- Fresh `cppcheck --enable=all --inconclusive --force --std=c99 -I include --error-exitcode=1 --suppressions-list=.cppcheck-suppressions.txt src include` run.
- Fresh `make qa-sanitize` run (main binary + pytest under ASan/UBSan).
- Full `pytest` suite.
- Fresh `scan-build --status-bugs` run.
- Valgrind verification for release-critical paths.
- Re-audit of final release diff.
- **Exit Criteria:** Blocker = 0, High = 0, scan-build = Green, Valgrind = 0 bytes definitely lost.

## 6. Audit Notes (Lightweight)
Audience: project maintainer, future contributors, and AI agents continuing work on the same area.

Before merge and before release, keep short notes:
- **Summary:** Total findings by severity.
- **Checks Run:** Which tools were run and outcomes (`clang-tidy`, `cppcheck`, `scan-build`, `pytest`, `valgrind` as applicable).
- **Residual Risk:** List any remaining Low/Medium items.
- **Status:** PASS or FAIL.

## 7. YtreeNova-Specific Priorities
- **Stability Over Novelty:** Features must not compromise the database-logger tree integrity.
- **MVC Integrity:** Maintain context-passing; avoid global variables.
- **ncurses Hygiene:** Every `newwin` or `derwin` must have a matching `delwin`.
- **Deterministic State:** No uninitialized variables. Every allocation must have a clear ownership and cleanup path.

---

**Technical Note:** Keep `compile_commands.json` current for `clangd` and `clang-tidy`. Recommended command: `make clean && bear -- make`.
