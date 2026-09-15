# AI-Assisted Development Workflow

This document defines the standards and processes for using AI agents to maintain and extend the `ytnova` codebase. These rules ensure architectural integrity and prevent "hallucination debt."
For canonical governance file ownership and edit targets, see [GOVERNANCE.md](GOVERNANCE.md).

---

## 1. Core Principles

### 1.1 The Golden Loop (Spec-First Integrity)
The development process is strictly hierarchical. The Spec is the "Contract of Truth."
1.  **Write/Verify Spec:** Does `SPECIFICATION.md` describe the desired behavior?
2.  **Write Test:** Create a test case that fails if the behavior is missing (Red).
3.  **Implement:** Write C code to satisfy the test (Green).
4.  **Refactor:** Improve code structure without breaking the test.

**CRITICAL RULE:** If the implementation behaves differently than the Spec, **the implementation is wrong.**
*   **Allowed:** Rewrite the C code to match the Spec.
*   **Allowed:** Fix the Spec if (and only if) the Spec was logically flawed.
*   **FORBIDDEN:** Changing the Test to match the "working" code if it violates the Spec.

### 1.2 General Rules
1.  **Architecture as Blueprint:** [ARCHITECTURE.md](../ARCHITECTURE.md) defines the technical structure.
2.  **Testing as Standard:** [TESTING.md](TESTING.md) defines test naming, structure, and harness rules. All test code must conform to it.
3.  **Human as Architect:** The AI acts as a partner with varying levels of specialization (see Section 2). The human maintainer provides final architectural direction.
4.  **Atomic Missions:** One work item (bug or task) = One session.
5.  **Build-First Verification:** No code is accepted until it compiles and passes tests.
6.  **The "Clean Slate" Rule:** If an AI generates fragile code or follows a wrong path, do not attempt to "patch" the mistake via further dialogue. Revert the changes (`git restore .`), rephrase the original prompt from a different perspective, and restart the mission. This is mandatory because:
    *   **Context Clearing:** It removes the "bad" code and the flawed logic that led to it.
    *   **New Perspective:** Rephrasing prevents the model from staying stuck in a loop of bad assumptions.
    *   **Clean Implementation:** It ensures the final code is the result of a single, clean logical flow rather than a series of ad-hoc patches.
    *   **Collateral-breakage policy:** When a first implementation attempt causes adjacent-contract failures, invariant breakage, or other self-caused blast radius outside the intended task surface, treat that attempt as invalid and restart from the last green state with a fresh agent/context. Do not normalize hours of fix-on-fix churn as the default path to correctness.
7.  **Minor Step Corrections Should Amend:** If the immediately preceding step only needs a small correction and should remain the same logical history unit, update it with `git commit --amend --no-edit` rather than adding a trivial follow-up fix commit. Create a new commit only when the correction is meaningfully distinct, delayed enough to matter historically, or worth preserving separately in the project history.
8.  **Intent Over Literal Wording:** Treat the human maintainer as authoritative on goals, but not automatically on exact UI wording, naming, key choices, menu structure, or workflow details. If a requested detail does not follow convention or best practices, or conflicts with existing YtreeNova patterns, the AI must say so explicitly and recommend the stronger conventional approach before implementing it.

### 1.3 Convention & Best-Practice Check

The maintainer may sometimes provide a very specific interaction detail while still intending "do what users would expect." Agents must not assume that specificity means correctness.

When a prompt includes concrete UI or workflow instructions, explicitly evaluate them against:

1.  **Established YtreeNova behavior:** You MUST NOT break local consistency without explicit, strong justification.
2.  **Lineage expectations:** Preserve XTree/ZTree muscle memory where that is clearly part of the feature intent.
3.  **Broader convention:** You MUST follow Linux/TUI conventions and common user expectations for prompts, menus, help, and key behavior.
4.  **Best practices:** Favor clarity, safety, and maintainability over clever but nonstandard interaction ideas.

Required agent behavior:

*   **Preserve the goal, question the detail:** Keep the user's intended outcome, but challenge interface details that appear nonstandard or weak.
*   **Recommend before encoding:** State the better conventional or best-practice choice before baking the literal wording into specs, prompts, or code.
*   **Explain the tradeoff:** When deviating from the user's literal wording, explain why the recommended version is stronger.
*   **Do not silently comply with weak specifics:** You MUST NOT turn a guessed interaction detail into lasting project behavior without explicit user review.
*   **Do not weaken ordinary language:** In maintainer and task language, `should`, `may`, `can`, and similar modal words express mandatory intent rather than optional permission unless the maintainer explicitly offers alternatives or asks a genuine question. Only an applicable quoted formal standard may supply a different modal definition.
*   **Establish the behavior before implementing:** Identify the nearest established behavior and its invariants, state a one-sentence behavioral contract for the requested outcome, and separate directly specified requirements from unresolved decisions. Implement specified behavior as an extension of the established pattern, not as a new interpretation or parallel interaction.
*   **Resolve new contracts before encoding them:** When a required behavior needs a new user-visible policy, error contract, architecture decision, or other durable choice that is neither specified nor established locally, list the unresolved choices, recommend the conventional outcome with its tradeoffs, and obtain approval before writing code, tests, help, or specifications. A regression test preserves an agreed or established contract; it MUST NOT convert an AI inference into a project requirement.
*   **Validate the outcome itself:** For an interactive, visual, performance, safety, or integration surface that can be exercised, run the supplied reproducer or equivalent real-runtime check, report the observed result to the maintainer before broad QA, and reconcile it with the original request and adjacent contracts before claiming readiness. Automated checks complement that evidence; they do not replace it.

### 1.4 UX Economy Gate (Mandatory)

Interactive flows must minimize interruption and decision depth.

Hard rule:

*   Common path should be `key -> Enter -> result`.
*   Maximum submenu depth on common path is 1.
*   If a flow exceeds one submenu, include explicit justification and provide an equivalent fast path.

This gate applies to architecture proposals, implementations, and code-audit findings.

### 1.5 QA Remediation Gate (Mandatory)

When a QA gate fails (build, static analysis, sanitizer, valgrind, or pytest), remediation must target root cause rather than symptoms.

Hard rule:

*   Fix the underlying defect causing the failure.
*   Do not patch around failures by weakening behavior or bypassing failing paths.
*   Do not add local suppressions (`NOLINT`, `xfail`, `skip`, ignore lists, disable flags) as a shortcut.
*   Test-only edits are allowed only when the test is demonstrably wrong against the Spec.
*   If temporary suppression is the only safe short-term option, discuss with the maintainer first and get explicit approval with a rollback plan.

### 1.6 Coverage Proof Gate (Mandatory)

Non-trivial tasks, migrations, and bugfixes must carry explicit proof that the work is complete within its intended scope.

Hard rule:

*   Before implementation, build an explicit in-scope inventory of affected surfaces. Include relevant code paths, tests, docs/trackers, and compatibility seams that could otherwise be forgotten.
*   For bugfixes, include the reproducer path plus adjacent failure surfaces that could share the same root cause.
*   Before opening a PR or declaring completion, reconcile the inventory item by item as addressed, intentionally unchanged with reason, or deferred/blocked with reason.
*   Do not treat one passing helper cluster or one green reproducer as sufficient proof. Completion requires a final sweep so no in-scope work remains unaccounted.

---

## 2. Shared Personas (The `.agent/rules/` directory)

The project maintains a set of "Persona Rules" in the `.agent/rules/` directory. These are Markdown files that define the behavior, constraints, and technical standards for different types of AI tasks.

*   **`architect.md`**: Used for planning and architecture design. Defines task boundaries and invariants without coding.
*   **`developer.md`**: Used for implementation. Applies approved changes while preserving architecture and safety constraints.
*   **`code_auditor.md`**: Used for adversarial quality review, fragility detection, and pass/fail gate findings.
*   **`tester.md`**: Used for generating Python-based TUI automation tests.
*   **`greybeard.md`**: Advisory persona for general engineering guidance, convention checks, and practical best-practice sanity checks. This is not a mandatory gate role.

### 2.1 Persona Activation and Skill Auto-Load

Use explicit persona switching in prompts:

*   Full form: `:at architect`, `:at developer`, `:at code_auditor`, `:at tester`, `:at greybeard`
*   Short form: `:at a`, `:at d`, `:at c`, `:at t`, `:at g`
*   Parse guardrail: persona switching triggers only when `:` is in column 1 and `:at` occupies columns 1-3 (`:at ...` at line start).
*   Default when no explicit switch is `architect`.
*   Assistant responses should begin with `<name>:`.

Skills are auto-loaded by active persona from `.ai/skills/*/SKILL.md` (no extra user command needed):

*   `architect` -> `architect-planning`
*   `developer` -> `developer-implementation`
*   `code_auditor` -> `code-auditor-gate`
*   `tester` -> `tester-regression-design`
*   `greybeard` -> `greybeard-meta-guidance`

Optional explicit skill controls:

*   `use skill <skill-name>` to force-load a skill
*   `skip skill <skill-name>` to suppress a skill
*   `only skill <skill-name>[,<skill-name>...]` to load only specific skills
*   `reset skills` to clear explicit overrides and return to defaults

Cross-cutting skill auto-load:

*   Bugfix work -> `bugfix-red-green-proof`
*   Feature-sized, major, and PR-update work -> `full-audit-gate-c`
*   QA-failure remediation work -> `qa-root-cause-remediation`
*   PTY/pexpect flake debugging -> `pty-pexpect-debug`
*   Ncurses rendering/redraw/color work -> `ncurses-render-safety`
*   Keybinding/menu/help key changes -> `keybinding-collision-check`
*   Manpage/usage doc sync tasks -> `manpage-sync`
*   UI workflow/menu-depth/interaction-economy design -> `ui-economy-navigation`
*   UI prompt-chain offender detection/audit -> `ui-flow-offender-audit`
*   Code-quality audit/remediation (clean-code/deslop-style) -> `code-quality` (blueprint: [CODE_QUALITY.md](CODE_QUALITY.md))
*   AI-writing-tell and prose de-slop tasks -> `ai-writing-tells`

Skill precedence (highest to lowest):

*   `only skill ...`
*   `use skill ...` and `skip skill ...`
*   Persona mapping
*   Cross-cutting auto-load

### 2.2 Dedup Contract (Persona vs Skill vs Docs)

Use this strict separation to avoid instruction drift:

1.  **Personas:** role boundaries, judgment posture, and communication style only.
2.  **Skills:** repeatable step-by-step procedures and checklists.
3.  **Docs:** policy, behavior contracts, and pointers to the right skills.

If procedural instructions appear in persona files, move them into skills and leave only a pointer.

---

## 3. The Agentic Loop

1.  **Direct Execution:** Run edits, builds, and tests in-repo to avoid manual copy/paste workflows.
2.  **Model Routing Principle:** Choose the available model/runtime by task risk and uncertainty: lower capability for mechanical edits, higher capability for ambiguous or high-impact decisions. Keep this as a capability decision, not a provider-name decision.
3.  **Semantic Context:** You MUST use **Serena** (symbol navigation) and **jCodeMunch** (repo indexing/search) for targeted context retrieval. You MUST NOT use raw bash tools like grep or find.
4.  **Cross-File State:** Keep terminal state, open-file context, and task history in one working loop.

### 3.1 Stateless Multi-AI Delivery Workflow (Non-Trivial Missions)

Use this workflow when a tracked bug or task needs architect-supervised implementation.
Prompt templates:
- **[TASK_PROMPT_TEMPLATE.md](TASK_PROMPT_TEMPLATE.md)**: architect-led implementation entrypoint for one tracked task or bugfix. The maintainer edits only the applicable `Work item:` selector line, and the AI derives title/scope from that selector, auto-consumes matching failed-audit relay files if present, and drives coherent batching plus completion-proof coverage.
- **[AUDIT_PROMPT_TEMPLATE.md](AUDIT_PROMPT_TEMPLATE.md)**: adversarial post-implementation audit entrypoint for one locked task or bugfix scope. The maintainer edits only the applicable `Audit target:` selector line, and the AI derives scope from that selector, writes failed-audit relay files only when follow-up work is needed, and may return PASS when the work is already satisfactory.

##### 3.1.0.1 MCP Config Bootstrap (Recommended)

To ensure local AI tooling picks up project defaults, run:

```bash
make mcp-doctor FIX=1
```

This bootstraps local MCP client configuration from repo-tracked defaults when missing, while preserving personal auth/session/history settings.
Principle: keep team-shared defaults in repo config and keep user-specific paths, credentials, and local overrides in user-local config.

##### 3.1.0.2 Periodic Codex/AI Tooling Refresh (Recommended)

If you use the Codex/AI workflow, run this update checklist periodically from the repository root:

```bash
# uvx-managed tools: force fresh resolve + latest
uvx --refresh codex-lb@latest --help
uvx --refresh --from git+https://github.com/oraios/serena@main serena --help
uvx --refresh --from git+https://github.com/jgravelle/jcodemunch-mcp.git jcodemunch-mcp --help

# GitHub MCP Docker image (only if you use that MCP server)
docker pull ghcr.io/github/github-mcp-server:latest

# Optional MCP config health check
make mcp-doctor
# If drift/missing config is reported:
make mcp-doctor FIX=1

cd ~/ytreenova
source .venv/bin/activate
pip-compile --upgrade -o scripts/requirements.txt scripts/requirements.in
```

Fuzz harnesses under `tests/fuzz/` are hand-maintained source files (not generated). Keep them in sync with their target modules:

- `tests/fuzz/fuzz_string_utils.c` -> `src/util/string_utils.c`
- `tests/fuzz/fuzz_path_utils.c` -> `src/util/path_utils.c`
- `tests/fuzz/fuzz_filter_core.c` -> `src/fs/filter_core.c`
- `tests/fuzz/fuzz_common.c` / `tests/fuzz/fuzz_common.h` -> shared helper layer used by all fuzz harnesses

When you change any of those target modules, update the matching fuzz harness(es) in the same change and run:

```bash
make qa-fuzz
```

#### 3.1.1 Workflow Contract (Mandatory)

1.  This workflow is mandatory for non-trivial missions.
2.  Work is executed as numbered work items that are:
    *   atomic and independently verifiable,
    *   sized as the largest coherent adjacent boundary-family batch that shares the same risk class and validation path,
    *   not fragmented into helper-by-helper or guard-by-guard micro-steps inside one boundary family unless a materially different validation path or risk split requires it,
    *   defined with an explicit coverage inventory for the in-scope files/symbols/tests/call paths before implementation starts,
    *   executed one work item at a time.
3.  Handoff artifacts under `.agent/handoffs/` are temporary relay state for active work only:
    *   create only the minimal files needed for the current work item,
    *   keep them current while that work item is still active,
    *   record the current coverage inventory and closure status in the active relay file whenever the mission spans multiple related surfaces or resumes across sessions,
    *   before starting a new work item, delete leftovers from any older completed work item so one task or bug never inherits another item's relay residue,
    *   delete them once the work item reaches a neutral stop state, so `.agent/handoffs/` is empty between work items.
4.  Failed-audit relay files live under `.agent/handoffs/` only until the next follow-up task consumes them:
    *   `audit.current.txt`: latest failed audit verdict for the most recently audited work item,
    *   `audit.task-<number>.txt` / `audit.bug-<number>.txt`: latest failed audit verdict for that specific roadmap task or bug.
    *   When a task prompt resumes from a failed audit, the AI must read the matching audit handoff automatically and choose the next coherent defect family itself rather than requiring maintainer triage.
5.  If `.agent/handoffs/` is absent or empty, the next mission must reconstruct from current repo state, git/GitHub state, and the selected tracker item rather than treating the missing relay as a blocker.

#### 3.1.2 Mission Definition Pass (Stateless Planning)

1.  Run a stateless planning session to define mission scope, constraints, and acceptance criteria.
2.  Output must include a prompt for a stateless `architect` pass.

#### 3.1.3 Architect Pass (Stateless, Branch Setup)

1.  Start on a dedicated feature branch (local + remote).
2.  Architect emits exactly one runnable developer unit at a time.
3.  Every unit definition must include:
    *   strict scope lock,
    *   inventory seed covering the intended files/symbols/tests/call paths to reconcile before completion,
    *   acceptance criteria,
    *   verification commands,
    *   blocker conditions,
    *   any temporary relay-file requirement needed for the active work item, or explicit confirmation that no relay file is needed.
4.  Architect status updates to maintainer must be delta-only and include concrete evidence handles.

#### 3.1.4 Developer Pass (Single Unit)

1.  Developer executes one scoped unit and produces a completion report.
2.  Verification cadence inside one atomic unit remains mandatory:
    *   initial pass: full verification set listed for the unit,
    *   correction/rework pass: rerun failing checks + directly impacted targeted tests,
    *   avoid full `make qa-all` during routine iteration unless maintainer explicitly requests it,
    *   rely on failing-check reruns + directly impacted targeted tests between implementation steps.
3.  Before declaring the unit complete, developer MUST reconcile the unit inventory item by item and record any intentionally unchanged or deferred items with explicit reasons.
4.  Developer MUST NOT treat a single passing helper cluster, call site, or reproducer as proof of completion when adjacent inventoried surfaces remain unresolved.
5.  Worker MUST NOT mark unit complete while required checks are failing.
6.  On success/failure/timeout, worker MUST emit explicit event log entries (no silent loops).
7.  Developer status line to maintainer must be delta-only: net-new state + next action + changed handles only.
8.  Developer/architect relay updates are facts-first:
    *   state completed work in past tense before planned next actions,
    *   include concrete evidence handles for each completion event (`report_handle`, event seq, command excerpt),
    *   keep updates delta-only: net-new state + next action + changed handles.

#### 3.1.5 Auditor Pass (Single Unit)

1.  Auditor runs only after developer evidence is available.
2.  Auditor workflow is evidence-first:
    *   validate code diff + verification evidence first,
    *   rerun commands only when evidence is incomplete, contradictory, or risk is high.
3.  Correction/rework iterations are separate atomic units and must be re-audited.
4.  Auditor output must include explicit pass/fail decision and severity-ranked risks.
5.  Repeated audits of the same work item may return PASS if no credible in-scope defects remain; the auditor is not required to manufacture findings to justify another iteration.

#### 3.1.6 Architect Validation, Commit, and Cleanup

1.  Architect validates durable run state plus developer/auditor evidence.
2.  Watchdog continuously enforces liveness:
    *   expired lease or stale heartbeat -> `stall_detected` event,
    *   requeue/reassign with bounded retry,
    *   terminal fail when retry budget is exhausted (no silent stop states),
    *   if worker creation is policy-blocked, retry once with a reduced subagent-safe prompt profile (minimal technical payload only) and do not pause maintainer for that recoverable path.
3.  Relay execution remains autonomous end-to-end; maintainer interruption is reserved strictly for `true_blocker_decision` and `commit_message_approval`.
    *   Workers must not be stopped/paused for routine process gating; stop/cancel is only for explicit maintainer stop requests or terminal failure recovery.
    *   Architect MUST record each spawned worker handle and proactively poll/wait all `active` workers until each transitions to `completed` or `blocked`; do not idle passively or wait for maintainer echo/re-send.
    *   After dispatching a worker, architect may do only genuinely non-overlapping local work before waiting. If no such local work remains, architect MUST enter the wait/poll loop immediately rather than returning an idle status update.
    *   Worker completion events/notifications are authoritative immediate loop triggers; the architect MUST start completion handling as soon as the completion event is observed.
    *   On each worker completion event, architect MUST immediately: read the completion report, run required validation checks, close the completed worker agent, proceed to the next step, and post a delta-only maintainer update using only `active|completed|blocked` status wording. Maintainer-pasted completion lines are fallback evidence only, not the expected trigger.
    *   When maintainer input is required, architect MUST emit exactly one standalone line:
        `ACTION NEEDED (maintainer): reply "<exact text to send>"`.
    *   When no maintainer input is required, architect MUST emit:
        `ACTION NEEDED (maintainer): none`.
4.  Canonical relay autonomy policy tokens (required in docs + guards):
    *   `policy_block_retry_once`: policy-blocked worker prompt failure is auto-retried once with reduced prompt profile and no maintainer interruption.
    *   `watchdog_stall_retry_terminal`: stale heartbeat/timeout must emit `stall_detected`, then bounded retry/reassign, then terminal escalation on retry exhaustion.
    *   `maintainer_pause_gate=true_blocker_decision|commit_message_approval`: pause gate allows maintainer interruption only for those two reasons.
    *   Runtime event naming should prefer explicit completion semantics (`worker_command_started`, `worker_command_completed`, `worker_command_failed`, `unit_completed`, `unit_failed`) so maintainers can distinguish done-vs-next without prompt interpretation.
    *   Maintainer-facing status wording should be constrained to `active`, `completed`, or `blocked`; avoid ambiguous runtime labels (for example `awaiting instruction`) in relay updates.
5.  Before merge to `main`, architect MUST ensure green PR full-QA CI evidence (`make qa-all` equivalent) for accepted branch state.
6.  Actual merge-safety gate is mandatory:
    *   immediately before merge, re-query the live PR state/checks for the current head SHA,
    *   treat required checks as not green if any required check is red, pending, cancelled, missing, or rerunning,
    *   require the branch freshness/up-to-date gate to be green at that moment when such a gate exists,
    *   if the head SHA or required-check set changes after the last green observation, restart the wait cycle and do not merge yet.
7.  If accepted:
    *   commit only code/doc files (no relay/runtime artifacts),
    *   use maintainer-approved commit message describing durable behavior (no task numbering),
    *   PR title/summary, commit wording, tracker-status prose, and other durable repo text must describe concrete behavior/problem and must not rely on volatile tracker IDs alone,
    *   when durable text needs a noun phrase, use the work item's actual title or the concrete defect family/behavior; do not write temporary workflow phrasing such as `a tracker number`, `a bug number`,  `this PR`, `remaining audit family`, `next slice`, or `until the rest lands`,
    *   include explicit work-item status text in the same commit (for example `Status: Confirmed.`, `Status: In Progress.`, or `Status: Fixed.`) so no status transition is left ambiguous, but keep any accompanying explanation durable rather than tracker-numbered,
    *   first push: `git push-fast-up`; tracked branch: `git push-fast`.
8.  If correction is needed for the same logical change set, amend and repush:
    *   `git commit --amend --no-edit`
    *   push with the branch rule above.
9.  Cleanup consumed transient artifacts after usefulness ends (for example `compile_commands.json`, `valgrind.log`, temporary `/tmp` files).

#### 3.1.7 Completion Gate, Merge, and Manual Fallback

1.  When preparing merge to `main`, require green PR full-QA CI gate (`make qa-all` equivalent).
2.  Immediately before merge, recheck live PR status on the current head SHA; if any required check is red, pending, cancelled, missing, rerunning, or freshness is no longer green, do not merge and restart the wait/remediation loop.
3.  Integrate branch to `main` using fast-forward only.
4.  For any bug or task, mark final status (Fixed/Completed) in the commit that is fast-forwarded to main; before that, status must stay non-final (Confirmed/In Progress).
5.  Delete temporary feature branch locally and on remote after merge.
6.  Verify only the intended tracked recovery checkpoint is committed; stale transient handoff artifacts must stay out of the change.
7.  Manual mode is default: one-unit-at-a-time architect -> developer -> auditor handoff.

#### 3.1.8 Practical Prompt-Template Finish Flow (Consecutive Role Order)

Use this when wrapping up a PROMPT_TEMPLATE-driven mission and returning the repo to a stable post-merge state.

1.  **Maintainer (local):** Run manual verification of the latest change first. Typical flow:
    *   `clear; make clean; sudo make uninstall; make; sudo make install`
    *   `ytnova ~`
    *   Manually exercise the changed behavior.
2.  **Maintainer -> Architect/AI:** If manual checks find issues, report failures; architect dispatches a new developer/auditor unit and repeats the loop until manual checks are green.
3.  **Architect/AI:** Clean stale task artifacts from the finished mission.
    *   Required before final commit: remove consumed relay files from `.agent/handoffs/` so no completed work item leaves residue behind, unless the maintainer explicitly asks to preserve a still-needed failed-audit handoff for immediate follow-up work.
4.  **Architect/AI:** Run quick local checks only (build + targeted smoke/tests for touched scope).
5.  **Architect/AI:** Stage intended changes only (exclude unrelated local edits and workflow artifacts).
6.  **Architect/AI:** Choose a Conventional Commit subject autonomously unless current repo policy or the maintainer explicitly requires approval.
7.  **Architect/AI:** Commit, push branch, and open/update a regular PR.
    *   PR title must use Conventional Commit format and describe the current atomic unit's durable aim, not a volatile task/bug number or the whole roadmap initiative.
    *   PR body should be concise: summary, scope, and local validation evidence. Do not duplicate obvious CI output or paste check transcripts into the body/comments.
    *   Do not use draft PRs by default. Use a draft PR only when the maintainer explicitly asks for draft status first, or when no reviewable unit exists yet and the AI has first warned that changing draft to ready later may restart long required CI.
    *   Before making or requesting any PR mutation that may restart required checks (for example draft/ready transitions, PR-body/title edits, base syncs, or similar GitHub-side actions), warn about the rerun cost and obtain explicit maintainer consent.
8.  **Maintainer (GitHub):** Review PR scope and evidence.
9.  **Architect/AI + Maintainer (GitHub):** Monitor PR CI full gate proactively (for example `gh pr checks <pr-number> --watch`). Do not wait passively for a separate reminder when checks change state.
    *   To avoid branch-CI babysitting during active implementation, use the branch repair loop. It watches the current branch head, writes a live GitHub failure packet, and launches a fresh Codex repair pass on each red run until checks go green or the retry budget is exhausted.
    *   Normal start (detached): `make ci-repair-start`
    *   Detached start with explicit handoff and tuned polling: `make ci-repair-start ARGS='--handoff .agent/handoffs/<task>.current.md --poll-seconds 120 --max-attempts 5'`
    *   Foreground/debug run: `make ci-repair-loop ARGS='--handoff .agent/handoffs/<task>.current.md'`
    *   Status: `make ci-repair-status`
    *   Recent log tail: `make ci-repair-log`
    *   If exactly one `*.current.md` handoff exists under `.agent/handoffs/`, the loop auto-discovers it and you do not need `--handoff`.
    *   Detached mode uses `tmux` when available; otherwise it starts a plain detached background process. In both cases the loop keeps running after you close the terminal, and writes state/log artifacts under `.agent/handoffs/`.
10. **Architect/AI:** If any checks are red, triage failing jobs immediately, fix CI failures root-cause-first, push updates, and repeat until required PR full-QA CI (`make qa-all` equivalent) is green.
    *   Do not request reviewers while checks are red unless the maintainer explicitly instructs it.
    *   Once checks are green on the current head SHA, avoid non-essential PR mutations before merge (for example draft/ready transitions, PR-body/title edits, or base-sync actions) because they may restart CI or invalidate freshness.
    *   If the red checks show self-caused collateral regressions outside the intended task surface, do not keep stacking repair commits as routine iteration. Reset/revert to the last green state, restart from a fresh agent/context, and only resume in-place repair when the remaining issue is a narrow residual miss rather than branch-wide blast radius.
11. **Architect/AI + Maintainer (GitHub):** If a draft/ready transition, PR metadata edit, branch sync, or any other PR mutation restarts required checks, restart the wait loop and do not merge against stale earlier green results.
12. **Maintainer (GitHub):** Merge PR to `main` only after checks are rechecked live as green on the current head SHA and review is satisfied. Never merge or close a PR while required checks are red, pending, cancelled, missing, rerunning, or freshness is stale.
13. **Maintainer (GitHub):** Delete remote branch:
    *   `git push origin --delete <branch>`
14. **Maintainer (local):** Sync local `main` to remote:
    *   `git checkout main`
    *   `git fetch origin`
    *   `git pull --ff-only`
15. **Maintainer (local):** Delete local feature branch:
    *   `git branch -d <branch>` (use `-D` only when needed)

### GitHub Source Preference

- For PRs, issues, comments, reviews, reactions, and check state, prefer the GitHub connector as the first source of truth.
- Use `gh` for GitHub Actions log retrieval, job-level failure inspection, or other run details that the connector does not expose cleanly.
- Treat the connector primarily as a remote PR/issue review and discussion surface inside ChatGPT/Codex; use the local checkout for code edits and tests.

## 4. Debugging Procedures

Never allow the AI to "guess" the cause of a bug. Use one of the following objective methodologies, described in detail in **[DEBUGGING.md](DEBUGGING.md)**.

### 4.1 Summary of Methodologies (by Usefulness)

1.  **Targeted Root Cause Analysis:** Lightweight, hypothesis-driven approach using semantic tools (**Serena**/**jCodeMunch**) to compare working vs. broken cases.
2.  **The "Hands-Off" Fix Mandate (Testing):** The mandatory procedure for agentic execution. Prove understanding by writing a failing `pytest`/`pexpect` test *before* editing code.
3.  **Instrumentation (The Discovery Loop):** Add `fprintf(stderr, ...)` to trace state. Used primarily for exploratory research in **AI Studio**.
4.  **External Expert Architecture Analysis:** High-reasoning audit for complex, multi-subsystem, or architectural bugs. Requires a fresh LLM context.

*   **Detailed Workflow:** See **[DEBUGGING.md](DEBUGGING.md)** for the complete step-by-step procedures for each method.

---

## 5. Audit Loop and Merge Gate

Follow **[../AUDIT.md](../AUDIT.md)** as the canonical process.

Cadence:
- Treat auditing as a continuous process during implementation, not an end-only step.
- Use focused build/test checks during each feature-sized change or PR iteration.
- Use PR full-QA CI (`make qa-all` equivalent) as the required pre-merge full gate; run local `make qa-all` only when the maintainer explicitly requests it or when extra local confidence is needed.
- Run the merge gate before merging.
- Do not run the full loop after every single prompt-level edit unless risk justifies it.

---

## 6. Resource Management & Usage Allowance Economy

AI computational resources (often referred to as tokens, usage allowance, context window limits, or quotas) are strictly finite. Wasted resources lead to shorter sessions, lost history, and increased costs.

### 6.1 Mandatory Usage Allowance Guard
*   **The AI MUST warn the user** when any requested action will unnecessarily consume massive amounts of context, generate immense output, or pull in disproportionately large data unsuited for the immediate task.
*   **The AI MUST always suggest the more economical alternative** before proceeding.
*   **The AI MUST NOT prevent** the user from proceeding if the user explicitly confirms they want to proceed with the expensive action anyway.

### 6.2 Avoid "Blind Exploration"
*   **Do not** ask the agent to "look for issues" or "summarize the file" to get started.
*   **The Semantic Advantage:** **jCodeMunch** `get_repo_outline` and **Serena** `get_symbols_overview` provide the necessary context in a fraction of the cost compared to reading raw files or directory listings.

### 6.3 Targeted Retrieval
*   **Do not** load multiple unrelated files into a single prompt.
*   **The Semantic Advantage:** You MUST use **Serena** `find_symbol` to extract only the relevant function or struct definition. This keeps the prompt focused and the model's reasoning sharp.

### 6.4 Minimize Redundant Calls
*   If a symbol has been read in the current session, do not re-read it.
*   Use **jCodeMunch** `search_text` for broad pattern matching across the entire project rather than manually grepping or opening dozens of files. This is strictly required over bash generic tools.

### 6.5 Batch Related Edits
*   Group related changes into a single prompt to avoid the overhead of full context reload for each minor edit.

### 6.6 Chat Pruning and Context
*   **New Chats for New Work Items:** Once a specific work item (bug or task) is done, close the chat and start a new one to drop old contexts.
*   **Exit Early on Hallucinations:** Stop and restart if the agent speculates. Do not waste usage allowance trying to correct a hallucinating model.
