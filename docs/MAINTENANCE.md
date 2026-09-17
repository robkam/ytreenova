# Recurring Maintenance Register

This register indexes active continuing obligations and owns each family's trigger and **Active**/**Retired** state. Normative behavior remains in [SPECIFICATION.md](SPECIFICATION.md), implementation and ownership rules remain in [ARCHITECTURE.md](ARCHITECTURE.md), and detailed commands and audit procedures remain in [AUDIT.md](AUDIT.md).

Finite remediation discovered by a pass belongs in [BUGS.md](BUGS.md) when it is a confirmed defect or architectural violation, or in [ROADMAP.md](ROADMAP.md) when it is planned improvement work.

## Execution Evidence

For every triggered pass, record the trigger, scope, responsible maintainer or reviewer, procedures used, checks and semantic review performed, findings and dispositions, and a durable PR, audit, or release reference. For count- or time-based cadences, that evidence must identify the last completed pass and the next due condition. **Active** describes an obligation; it does not prove that a due pass occurred.

## Module Cohesion and Ownership Purity

**State:** Active

**Triggers:** Every structural change; the recurring code-quality burn-down; before a milestone or release.

**Tasks:**

- Review implementations against the module-boundary contract in [ARCHITECTURE.md](ARCHITECTURE.md). Keep each `.c` file responsible for one cohesive implementation concern. Self-contained means clear ownership behind an explicit interface, not zero dependencies or a requirement to make every file a standalone program.
- Keep exactly one authoritative implementation owner for each responsibility. Do not split the same policy across unrelated files, copy business rules between modules, or leave old and replacement paths reachable in parallel.
- Put genuinely shared behavior behind the owning module's declared API and header rather than reproducing it at call sites.
- Keep controllers as dispatch and event-loop boundaries. Move reusable command, filesystem, comparison, state, and utility logic into focused owner modules.
- Review cross-module calls for circular ownership, divergent sibling semantics, hidden backchannels, and changes to one responsibility that require unrelated module edits.
- Run `make qa-module-boundaries` and `make qa-clean-code`, then supplement their mechanical checks with semantic ownership review; passing size and allowlist checks alone does not prove module purity.

**Evidence:** Record the inspected owner map, overlaps found, consolidations made, remaining exceptions, and focused validation in the delivery PR or audit report.

## Structural Quality and Debt Burn-Down

**State:** Active

**Triggers:** After every five merged structural PRs affecting `src/`, `include/`, or code-quality guards; before a milestone or release; whenever a budget or legacy exception changes.

**Tasks:** Re-rank complexity and coupling hotspots, reduce a coherent high-value debt family, reconcile boundary exceptions, and verify that simplification does not create parallel abstractions or behavior drift.

**Evidence:** Attach comparable before/after hotspot measurements and the focused checks required by [AUDIT.md](AUDIT.md).

## Compiler and Toolchain Health

**State:** Active

**Triggers:** Compiler, standard-library, dependency, build-flag, or supported-platform changes; before releases.

**Tasks:** Review warning baselines, investigate new diagnostics, exercise supported compiler paths, and keep source discovery and build ordering deterministic. Do not normalize new warnings into the baseline without a documented reason and owner.

## Test Contract and Coverage Resilience

**State:** Active

**Triggers:** New behavior, UI or prose changes, test-harness changes, coverage movement, recurring flakes, or release preparation.

**Tasks:** Keep tests tied to stable behavior rather than incidental wording/layout, reconcile the resilience baseline, inspect coverage gaps by risk, and replace timing-dependent or implementation-coupled assertions where an observable contract exists.

## Interaction Economy and Surface Trust

**State:** Active

**Triggers:** Every new or materially changed command, prompt, menu, picker, dialog, or interactive workflow; before releases.

**Tasks:** Audit the common path against the interaction-economy contract in [SPECIFICATION.md](SPECIFICATION.md), justify every prompt and interactive layer, require an equivalent fast path for approved deeper flows, and verify that command surfaces advertise exactly the actions available in their context.

**Evidence:** Record the audited action flow, decision count, submenu depth, retained exceptions, surface-parity result, and focused runtime validation.

## Security Baseline and Adversarial Review

**State:** Active

**Triggers:** New process-launch, archive, filesystem, provider, parser, configuration, or other trust-boundary work; before releases; when threat assumptions or dependencies change.

**Tasks:** Reassess the security baseline and threat assumptions, review guard coverage, exercise adversarial inputs, confirm least-privilege and fail-closed behavior, and perform bounded adversarial review appropriate to the changed attack surface.

**Findings:** Record confirmed security defects in [BUGS.md](BUGS.md) and finite hardening projects in [ROADMAP.md](ROADMAP.md).

## Compatibility and Migration Debt

**State:** Active

**Triggers:** Whenever a migration introduces an adapter, mirror, legacy path, deprecation, or temporary exception; before releases.

**Tasks:** Verify every compatibility path has one owner, invariant protection, a removal condition, and a replacement path. Remove expired shims and reject permanent parallel authority.

## Source and Documentation Hygiene

**State:** Active

**Triggers:** Source-comment, user-visible behavior, help, localization, configuration, terminology, or generated-documentation changes; before releases.

**Tasks:** Remove stale or historical source comments, keep comments limited to durable rationale and invariants, maintain footer/help/manpage behavior parity, check terminology consistency, and verify generated projections from their canonical authored sources. Before a release, reconcile `README.md`, `CONTRIBUTING.md`, milestone-level `CHANGELOG.md` entries, `TRUST.md`, and release-status wording with the actual release scope.

## QA and CI Gate Health

**State:** Active

**Triggers:** Before any CI-policy change; at least once per calendar quarter while CI gates are maintained; after repeated flakes or infrastructure failures; after material duration, cache, queue-time, or coverage-overlap changes; whenever a gate is added or removed.

**Tasks:** Inspect historical duration, queue time, duplicate coverage, flake rate, cache hit rates, and unique failure detection before retaining, streamlining, rescheduling, adding, or removing a check. Fix repository defects at their root and do not weaken gates merely to obtain green results.

## Deep Runtime Health

**State:** Active

**Triggers:** Scheduled deep-runtime runs; after memory-, cleanup-, integer-, or undefined-behavior-sensitive changes; after major refactoring; periodically as a health check; before releases.

**Tasks:** Execute the applicable sanitizer, Valgrind, and maximum-depth procedures from [AUDIT.md](AUDIT.md), classify every failure before rerunning, and preserve the resulting evidence or tracked remediation.

## Active Tracker Reconciliation

**State:** Active

**Triggers:** When planned work lands, a confirmed defect is fixed or reclassified, a maintenance pass discovers finite work, and before releases.

**Tasks:** Keep [ROADMAP.md](ROADMAP.md) limited to unfinished planned improvements, [BUGS.md](BUGS.md) limited to unresolved confirmed defects and architectural violations, and this register limited to recurring obligations. Remove completed tracker records and use git history for detailed archival history.

**Evidence:** Record added, removed, reclassified, and intentionally unchanged records in the delivering PR or audit report.

## Release Readiness

**State:** Active

**Triggers:** Every release candidate and milestone requiring release-depth confidence.

**Tasks:** Run the release-appropriate checks in [AUDIT.md](AUDIT.md), perform the bounded release review, classify every failure, and open a concrete bug or remediation item for work that cannot be closed safely within release preparation.

## Retirement Rule

A family may be marked **Retired** only when the obligation itself no longer applies or another named canonical process fully owns it. A clean run, completed remediation, or newly automated guard is evidence that the family is healthy, not evidence that recurring maintenance is finished.
