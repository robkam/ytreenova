# Code Quality Burn-Down Blueprint

Use this doc when a session invokes `code-quality` explicitly or when a code-quality / clean-code remediation request auto-loads that skill.

This file is the canonical code-quality policy for recurring burn-down work:

- smell classes and prioritization,
- the lean simplicity contract,
- recurring cadence triggers,
- the before/after evidence format,
- and the acceptance checklist for each pass.

## Prevention-First Posture

Code-quality work is not permission for cosmetic rewrites.

Before editing code:

1. Reconfirm the behavior/spec that must remain true.
2. Build an explicit inventory of the in-scope surfaces.
3. Prefer root-cause fixes over cleanup that only rearranges symptoms.
4. Keep the batch coherent: one owner boundary, one validation path, one risk class.
5. Preserve architectural invariants (explicit context passing, panel isolation, deterministic single-threaded behavior).

Use this prompt starter when needed:

```text
use skill code-quality
Treat this as prevention-first work, not cleanup.
Before code edits, enforce:
- spec-first behavior check
- red/green regression proof for bugfixes
- module ownership check
- root-cause QA remediation (no bypass/suppressions)
- UX economy gate for interactive flows
- focused validation during iteration
- conceptual integrity check
- architecture conformance check
- drift/erosion check
- shotgun-surgery check
- duplicate/parallel implementation check
- performance regression check
- event-loop determinism / signal-safety check
Then report/implement only approved P0-P1 issues, plus any approved P2 burn-down batch.
```

## Lean Simplicity Contract

Every burn-down pass must leave the code leaner **without** making it harder to read or debug.

Mandatory rules:

1. Prefer the simplest clear implementation that preserves behavior and invariants.
2. Fewer lines is good only when readability and diagnosability stay equal or improve.
3. Avoid shorthand or clever constructs that hide control flow, ownership, or side effects.
4. Use explicit names for ownership and state transitions; avoid ambiguous abbreviations outside tiny local loops.
5. Favor straightforward control flow (guard clauses over deep nesting where practical).
6. Do not introduce speculative indirection, generic helpers, or flag-heavy APIs just to shrink one hotspot.

### Anti-Obfuscation Checks

Treat these as smells even when they technically work:

- dense expression chains that hide branching or mutation,
- nested ternaries or compressed control flow,
- helpers that multiplex unrelated behavior via boolean flags,
- hidden shared-state mutation without an explicit ownership contract,
- unnecessary indirection layers introduced before a clear rule-of-three need,
- recursion outside clearly bounded hierarchy traversal.

If recursion is retained or introduced, record a one-line justification and its termination/base-case note in the review evidence.

## Smell Classes and Prioritization

### Severity

- `P0`: security risk, corruption, data loss, destructive surprises.
- `P1`: correctness risk, broken invariants, high-likelihood regression.
- `P2`: maintainability drift that materially slows normal feature work.
- `P3`: readability/polish issue with low immediate risk.
- `P4`: optional stylistic cleanup.

Raise severity one level for a safe quick win. Lower it one level when change risk is high and coverage is weak.

### Canonical Smell Classes

| Class | What to look for | Typical priority |
| --- | --- | --- |
| Conceptual integrity | duplicate or parallel implementations, half-migrations, divergent sibling semantics, architectural drift/erosion | P1-P2 |
| Module boundaries | controller-owned logic that belongs in modules, mixed-responsibility files, cross-panel coupling, shotgun surgery | P1-P2 |
| Ownership and lifetime | unclear buffer/pointer ownership, asymmetric cleanup, borrowed references outliving owners | P0-P1 |
| Error handling | silent failure, inconsistent contracts, fail-late behavior, missing validation | P1-P2 |
| Event-loop determinism | redraw lag, blocking main-loop work, resize/input nondeterminism, signal-safety violations | P1-P2 |
| UI/TUI economy | extra submenu hops, prompt chains, inconsistent key behavior, stale redraw state | P2-P3 |
| API and data shape | long parameter lists, primitive obsession, flag-heavy signatures, duplicated business rules | P2 |
| Complexity and readability | deep nesting, mixed abstraction levels, wrong abstractions, dead code, comment-as-deodorant | P2-P3 |
| Tests and QA | missing fail-first proof, timing hacks, spec drift, suppressions that bypass root-cause fixes | P1-P2 |
| Docs and comments | stale comments, duplicated process guidance, missing invariant rationale | P2-P3 |

## Recurring Burn-Down Cadence

A burn-down pass is mandatory when **any** of these triggers fires:

1. **Five merged structural PRs since the last pass:** five merged feature/refactor PRs that touch `src/`, `include/`, or code-quality guard scripts under `scripts/`.
2. **Before milestone or release tags:** run a fresh pass before any milestone tag or release cut.
3. **When a structural baseline is intentionally lowered:** after shrinking a guarded controller/file/function budget or retiring a legacy boundary exception, record the new baseline in the same effort or immediately after it.

### Batch Size and Selection Rules

- Default to the **top 3-5 hotspots** by size/complexity/smell impact in one owner boundary.
- Do not mix unrelated subsystems solely to hit a count.
- If a hotspot batch touches controller/file/function budgets, the pass must include `make qa-module-boundaries`.
- Use the guarded hotspot report plus direct audit judgment together. The report ranks known budgeted hotspots; maintainers still need to spot duplicate logic, drift, or UX regressions that a size-only metric cannot see.

## Evidence Format for Each Pass

Create a measurable before/after record for the same hotspot rows:

1. Save a **before** snapshot:

   ```bash
   python3 scripts/report_code_quality_hotspots.py --format json > /tmp/ytnova-hotspots-before.json
   ```

2. Land the bounded burn-down batch.
3. Save the **after** comparison:

   ```bash
   python3 scripts/report_code_quality_hotspots.py \
     --baseline /tmp/ytnova-hotspots-before.json \
     --format markdown \
     --top 5
   ```

4. Attach:
   - the before/after hotspot table,
   - the delta summary (what shrank, what stayed flat, what was deferred),
   - behavior-preservation notes,
   - focused validation results.

### Minimum Validation Evidence

Every burn-down pass must include:

- `make`
- focused `pytest` for touched behavior (with `.venv` activated)
- `make qa-module-boundaries`

Add these when their surfaces change:

- `make qa-clean-code` when touching naming/function-budget/magic-number/test-independence guard surfaces or the clean-code allowlist
- `make qa-code-quality` when touching the broader bundled guard set, AI config, or generated catalog/template surfaces
- `make qa-fileops-integrity` for file/archive mutation flows
- `make qa-split-panel-gates` for split-panel invariants

`docs/clean_code_allowlist.json` is the canonical baseline-debt registry for `qa-clean-code`. Every retained exception must name the owner boundary plus a concrete removal plan; do not add silent inline waivers in scripts or tests.

Local `make qa-all` remains optional unless maintainer-requested; green PR full-QA CI remains the pre-merge gate.

## Acceptance Checklist for One Burn-Down Pass

Do not call a pass complete until all of these are true:

- The selected hotspot batch is explicit and coherent.
- The same hotspot rows appear in both before and after evidence.
- Any unchanged or deferred hotspot is named with a reason.
- Lean-simplicity rules were followed; no clever/obfuscated replacement code was introduced.
- Behavior and UX semantics for touched paths were preserved.
- `make qa-module-boundaries` and the required bundled QA gates are green for the touched surfaces.

## Repo Guardrails to Keep Active

1. Conventional commit-msg hook policy enforcement.
2. Mandatory focused verification during implementation; green PR full-QA CI before merge.
3. QA remediation gate: fix root cause, do not patch around failures.
4. Module ownership gate: keep controller files dispatch-oriented when logic can live elsewhere.
5. UX economy gate for interactive paths.

## Notes

- Skill name is `code-quality`.
- This filename remains intentionally short and discoverable: `CODE_QUALITY.md`.
