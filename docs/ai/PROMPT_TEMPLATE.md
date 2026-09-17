# Implementation Prompt Template

Maintainer instructions:
- Delete the `Work item:` line that does not apply, then fill in the missing number on the line you keep.
- Do not rewrite the rest unless you intentionally want custom behavior.
- A future maintainer may have only this repository clone. The AI must rely on repo state, git/GitHub state, and any currently-present transient relay files under `.agent/handoffs/`, not on external chat history.

## Merge prompt to use only after CI is green

Use this only after the PR is already open and CI is green. Do not include this block in the starter prompt.

Re-check the live PR head SHA and current GitHub merge state against the current GitHub state. If and only if all required checks are green on the current head SHA, the PR is mergeable, and the branch is up to date as required, merge the PR with rebase-merge.
Do not edit the PR body, push new commits, update the branch, sync with base, or perform any other PR mutation before merging, because those actions may restart CI or invalidate freshness.
If this merge closes the selected work item, update the in-repo source-of-truth tracker status appropriately on local `main` only if that status change does not require opening another PR; otherwise just report it as remaining follow-up.
After merge, clean up only non-PR-opening post-merge residue: discard any uncommitted local handoff/relay leftovers, note any tracked stale handoff artifact still on `main`, and do not open another PR unless explicitly instructed.
Delete the merged feature branch locally and confirm the remote branch is gone.
Fast-forward local `main` to `origin/main`, leave the checkout clean, and report only completed state, current next action, and changed handles.

## Starter prompt

You are continuing ytreenova work as a stateless AI.

Repo: /home/rob/ytreenova
Work item: docs/BUGS.md Bug <number>
Work item: docs/ROADMAP.md Task <number>

Derive the exact title, acceptance target, and completion objective from the kept `Work item:` line. If the selector points at a bug, treat the mission as "fix this bug completely." If it points at a roadmap task, treat the mission as "complete this roadmap item correctly."

Startup requirements:
- Read AGENTS.md only as the discovery stub.
- Read .ai/codex.md and .ai/shared.md before any codebase research or edits.
- Use the repo-required MCP semantic tools for codebase exploration.
- Follow all repo commit, branch, PR, QA, and handoff rules.

Recovery and source of truth:
- Before starting the selected work item, inspect `.agent/handoffs/` for leftovers from any other task or bug.
- If relay files for an unrelated completed work item remain and no active PR/branch still depends on them, delete them immediately before continuing. Do not let one work item inherit another work item's residue.
- If relay files appear to belong to some other still-active work item, stop and report the conflict instead of mixing contexts.
- If `/home/rob/ytreenova/.agent/handoffs/` is absent or empty, reconstruct from current repo state, git/GitHub state, and the selected tracker item alone.
- If relevant relay files for this work item are present under `/home/rob/ytreenova/.agent/handoffs/`, use only the minimum necessary ones.
- Create or update only the minimal relay files needed for the active work item.
- To anticipate clone-based resume, commit a durable resume handoff for that work item instead of relying only on transient local relay state.
- Delete all consumed, completed, or otherwise stale relay/handoff files for this work item after merge/cleanup or after a PASS audit closes it, so `.agent/handoffs/` returns to empty between work items.
- Automatically derive the canonical source-of-truth docs, trackers, tests, codebase registry files, and directly relevant runtime surfaces from the selected `Work item:` line.
- If either of these audit handoff files exists for the selected work item, read it automatically and treat it as part of the source of truth:
  - `/home/rob/ytreenova/.agent/handoffs/audit.current.txt` if it clearly names the same work item,
  - `/home/rob/ytreenova/.agent/handoffs/audit.task-<number>.txt` for roadmap tasks or `/home/rob/ytreenova/.agent/handoffs/audit.bug-<number>.txt` for bugs.
- If an audit handoff exists and contains multiple defect families, you MUST group and prioritize them yourself. Choose the next highest-value coherent family automatically, record the chosen family plus deferred families in the active relay file if one is needed, and ask the maintainer only if the split is genuinely ambiguous.
- Use git and GitHub to discover the current branch, active PR, merged PRs, and stale branches.
- Do not rely on branch names, PR numbers, run IDs, or any external chat history.

Primary goal:
- Finish this work correctly and completely.
- Optimize for coherent, reviewable slices with explicit proof of coverage.
- Prevent false completion, silent omission, and tiny-slice churn.

Batching rules:
- Before starting a branch, identify the next largest coherent in-scope work family by shared owner boundary, runtime area, defect family, feature surface, migration boundary, or validation path.
- Default to one PR per coherent work family, not one PR per helper, guard, tiny fix, or narrow symptom.
- Batch together adjacent remaining code, tests, docs, guards, registry surfaces, dispatch surfaces, compatibility seams, and supporting changes that share the same risk class and focused validation path.
- Do not stop at the first passing helper cluster or obvious call site if adjacent work in the same family can safely land in the same PR.
- If a proposed batch touches fewer than about 3 adjacent surfaces, assume it is too small and expand it unless there is a concrete reason not to.
- Split a family only if one of these is true:
  - blocked dependency,
  - materially different validation path,
  - materially different risk class,
  - materially different subsystem owner.
- Do not mix unrelated risk classes just to reduce PR count.

Coverage and completeness rules:
- Before editing a work family, build an explicit in-scope inventory.
- The inventory must list all relevant items in scope, including where applicable:
  - source-of-truth roadmap/spec/tracker entries,
  - runtime helpers,
  - guards,
  - registry surfaces,
  - dispatch surfaces,
  - transition/state/projection surfaces,
  - related runtime modules,
  - directly affected tests,
  - directly affected docs,
  - legacy compatibility seams, replacement paths, or call paths touching that work family.
- For bugfixes, the inventory must also include:
  - the reproducer path,
  - adjacent failure surfaces that may share the same root cause,
  - any stale workaround, fallback, or duplicate behavior path related to the bug.
- Record the inventory in the live handoff before or alongside implementation.
- Do not start coding until the inventory is explicit enough to prove scope.

Closure rules:
- Before opening a PR, reconcile the inventory item by item.
- Every inventoried item must be marked as exactly one of:
  - addressed,
  - intentionally unchanged with reason,
  - deferred/blocked with reason.
- Silent omission is forbidden.
- "Done for now" is not an allowed status.
- If an item is left for later, the active relay file must name the concrete split reason. If no valid split reason exists, include it in the current PR.
- If the current work came from an audit handoff, the active relay file must also name:
  - the audit-selected defect family being fixed now,
  - any remaining deferred defect families from the same audit,
  - why those deferred families are not being mixed into the current PR.

Anti-premature-completion rules:
- Do not treat one passing helper cluster, one green reproducer, or one updated call site as proof that the work family is complete.
- Do not stop after the first green focused validation if adjacent inventoried surfaces in the same family remain.
- Do not treat "I changed the obvious places" as proof of completeness.
- The required proof of completeness for a work family is the reconciled inventory plus a final sweep.

Global audit rules:
- After every merge, re-scan:
  - the primary roadmap/spec/tracker source for this work,
  - directly relevant docs and tests,
  - the live handoff,
  - relevant code surfaces for the current work family.
- Use that scan to identify the next highest-value remaining work family and catch stale assumptions or missed adjacent work.
- If previously missed work belongs to the same family, prioritize folding it into the next coherent batch.

Task completion rules:
- Do not declare this task or bugfix complete until all of the following are true:
  - all relevant source-of-truth roadmap/spec/tracker items for this work are resolved or explicitly deferred with reason,
  - the live handoff lists no unaccounted in-scope work for this mission,
  - every identified work family has an inventory and closure status,
  - a final audit finds no remaining unmigrated, unfixed, or unaccounted in-scope surfaces,
  - any intentionally deferred items are explicitly named and justified.
- "No obvious next helper" is not completion.
- "Current PR merged" is not completion.
- Completion requires an explicit final sweep and explicit proof that no in-scope work families remain unaccounted.

Execution policy:
- Work autonomously.
- Do not ask for commit message approval unless current repo policy explicitly requires it.
- Choose compliant, durable Conventional Commit messages yourself when allowed.
- Keep commits and PRs coherent and outcome-focused.
- Prefer medium-sized or larger coherent PRs over micro-PRs.
- Optimize for fewer, fuller PRs while preserving reviewable coherence.

PR and wording rules:
- Do not put volatile task numbers, bug numbers, branch names, PR numbers, run IDs, or workflow IDs in commit messages, PR titles, or durable change descriptions unless explicitly required.
- Commit subjects and PR titles must describe durable outcomes only.
- PR titles must use Conventional Commit style: `<type>(<scope>): <durable outcome>`.
- Keep title style consistent across the series.
- Durable prose in PR bodies, tracker updates, and handoff summaries must name either the selected work item's actual title or the concrete defect family/behavior being changed.
- Do not write durable prose that says things like `Task <number>`, `Bug <number>`, `this PR`, `remaining audit family`, `next slice`, `next step`, or `until the rest lands`.
- If you update a tracker/status line and need to explain why work remains open, say only `Status: In Progress.` plus a durable explanation based on the title or defect family; never explain it with temporary numbering or workflow jargon.
- Use real Markdown newlines and code-formatted validation commands in PR bodies.
- Put red proof as a bullet under `## Validation`.
- Omit repository template boilerplate and courtesy preambles from PR bodies unless repo policy requires otherwise.
- Never merge, close, or mark ready an open PR while any required check is red or pending. Re-check actual GitHub PR status immediately before ready/merge actions; do not rely on stale assumptions or earlier summaries.

Validation rules:
- Run focused local validation only unless repo policy or the maintainer explicitly requires more.
- For bugfixes, follow strict red-green: prove the failure first, then implement the fix, then rerun to green.
- Before implementation, identify the nearest established behavior and invariants, state a one-sentence behavioral contract, and separate directly specified requirements from unresolved durable decisions. Extend the established pattern for specified behavior; do not invent a parallel interpretation.
- For an unresolved user-visible policy, error contract, architecture choice, or other durable decision, present the conventional recommendation and tradeoffs, then obtain maintainer approval before encoding it in code, tests, specifications, or help. Tests preserve agreed or established behavior; they must not make an AI inference into a requirement.
- When the requested runtime surface can be exercised, run the supplied reproducer or equivalent manual check, report the observed outcome before broad QA, and reconcile it with the original request and adjacent contracts. Automated tests complement this evidence; they do not replace it.
- For all work, do not implement from inference. Inspect the exact current behavior and relevant implementation path before editing, then make the smallest change that directly produces the requested result. Do not add speculative support layers or report success until the requested result itself has been checked. Complete the requested task autonomously, making all changes necessary to deliver the requested result correctly, including directly affected tests and documentation. Preserve existing behavior, interfaces, documentation, and tests outside the requested scope. Seek approval only when no safe, backward-compatible way exists to avoid a genuinely out-of-scope change.
- When updating active guidance or user/product documentation, write only about current behavior. Do not mention removed, replaced, superseded, or non-existent features, commands, or workflows unless a migration or release-compatibility instruction requires it; describe the current feature in its own right.
- Do not present routine test passes as progress.
- Use validation evidence only where required for PR, handoff, or merge decisions, and keep it concise.
- Do not stream full CI logs or paste long test output.

CI wait rule:
- After CI starts or restarts, check PR status every 5 minutes until required checks are green or red.
- If CI is still running or pending, keep polling every 5 minutes.
- If I explicitly report CI is red or green, stop waiting and handle that state immediately.
- When checking CI, use only a compact pass/fail/running summary via `--jq`.
- Fetch detailed check or log output only if a check is red.
- Keep maintainer-facing updates to one short line.
- If a pushed fix changes the PR, restart the wait cycle from the beginning.
- If PR metadata edits, branch sync, base update, or any other PR mutation restarts required checks, restart the wait cycle from the beginning.
- A PR is mergeable only when required checks are actually green at the moment you act. If an allegedly green PR is red or pending when rechecked, treat it as not ready and continue remediation/waiting.
- Recheck the live PR state against the current head SHA immediately before merge. Treat the PR as not ready if any required check is red, pending, cancelled, missing, rerunning, or if freshness/up-to-date is no longer green.

Continuation and stop rules:
- After every merge, resume the next largest coherent remaining work family without waiting for my go-ahead, unless I later tell you to pause or stop.
- Resume from the latest global audit and handoff, not from the smallest next helper or symptom.
- If I later give a stop, pause, or stop-after condition, that overrides all prior autonomous-loop instructions.
- After satisfying a stop condition:
  - cancel or mark blocked any active persistent goal,
  - do not start another PR,
  - do not poll CI,
  - do not resume automatically,
  - wait for an explicit new user command such as "resume work".

What to do now:
1. Inspect current git/GitHub state.
2. If there is an active PR:
   - Follow the CI wait rule.
   - Once required checks are green on the current head SHA, do not edit the PR body, push new commits, update the branch, sync with base, or perform any other PR mutation before merging unless a real fix is required, because those actions may restart CI.
   - If checks are green when rechecked live, merge when allowed, clean up stale remote/local branch state, delete or refresh any consumed relay files as required by the outcome, then continue according to the continuation rule unless a superseding stop condition applies.
   - If checks failed, inspect only the failure-relevant summary or log tail. Fix only clearly repo-side, in-scope failures.
   - If the failure is external, inconclusive, cancelled, or not clearly repo-side, stop and report the PR URL, check summary, and resume instruction.
3. If there is no active PR:
   - If the selected work item already reconciles cleanly in its source-of-truth tracker and the latest matching audit handoff is a PASS with no remaining credible work, delete any now-stale relay files for that work item, then stop and report completion instead of opening another branch or PR.
   - Select the next largest coherent in-scope work family from the current source-of-truth docs/trackers/codebase registries and handoff context.
   - If an audit handoff is present, choose the next highest-value coherent defect family from that audit automatically instead of asking the maintainer to triage the findings.
   - Build the explicit inventory first.
   - Implement the full coherent batch through the repo's developer/auditor/PR workflow.
   - Run focused local validation only.
   - Reconcile the inventory before opening the PR.
   - Push a regular PR.
   - Update any active relay file needed for recovery, including the inventory and closure status.
   - Follow the CI wait rule.
   - When CI is green when rechecked live, merge when allowed, clean up stale remote/local branch state, delete or refresh relay files so `.agent/handoffs/` does not retain stale work-item residue, then continue with the next largest coherent work family unless a superseding stop condition applies.

Reporting discipline:
- Do not provide broad recaps unless asked.
- Report only:
  - completed state,
  - current next action,
  - new or changed handles.

Stop only:
- to fix red until green,
- if you have a blocker or needed question,
- if you hit a genuine work-family split decision that materially affects batching or risk,
- if the completeness inventory cannot be reconciled without human clarification,
- or if I tell you to pause or stop.
