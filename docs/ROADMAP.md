# **YtreeNova Roadmap**

---

Ordering policy (for all editors, including AI editors):
- Organize work as `Current Delivery Roadmap` and `Future Enhancements / Wishlist`, then by phase.
- Inside each phase: put items that are high-impact first after that order remaining items by ease of implementation.
- Insert new approved items at the correct priority position (do not append by default).
- In `Current Delivery Roadmap`, number `Task` items top-to-bottom in ascending order (`1` = highest priority).
- In `Future Enhancements / Wishlist`, use `Idea FE-*` IDs in top-to-bottom ascending order (`FE-1` = highest priority in wishlist).
- IDs are unstable labels and are likely to change often due to reprioritization/renumbering.
- `docs/ROADMAP.md` is forward-looking only (`planned`/`in-progress`).
- Completed items are removed after landing.
- For shipped outcomes, see [docs/CHANGELOG.md](CHANGELOG.md), which records only the most significant milestones, not every minor change.
- Use git history as the full archive.

---

## **Phase 1: Exploitability-First Security Hardening (Pre-Alpha Release Blocker)**
*This phase is first by priority for real-world security readiness. Ship only after these classes are closed or explicitly risk-accepted.*

## **Phase 2: Architecture and Clean-Code Guardrails (Early Prevention)**
*This phase codifies architectural and coding-discipline guardrails early so regressions are blocked before they become backlog debt.*


### **Task 1: Unified AppState Transition Machine + Projection Contract**
*   **Goal:** Move UI behavior from dispersed flags, redraw-side repair, and ad hoc restore paths to one explicit application state machine with authoritative ownership, validated transitions, and rendering as projection only.
*   **Rationale:** Repeated split/tree/file/window/focus regressions show that local fixes can pass narrow tests while leaving competing state owners alive. The durable fix is to define the machine first: every visible behavior and input target must have exactly one owner, and every UI-affecting action/event must pass through one transition boundary before render.
*   **Scope:** Application state ownership and transition architecture for split/single layout, panel focus, tree/file/small/big-file window shape, dotfile visibility, tagged/showall/global modes, viewport restore, command/modal state, footer/stats projection, and shared volume topology. This task is architectural and test-first; it must not become another symptom-specific split/F8 patch.
*   **Definition:** “UI-affecting” means any input, event, rebuild, refresh, filesystem mutation, modal action, resize/reflow, volume operation, visibility/filter change, or render-invalidation path that can change selection, focus, viewport, layout, mode, visibility, restore identity, tags, file-list contents, footer/stats output, or panel/volume binding.
*   **Target State Model:**
    *   The target is a hierarchical statechart, not only a collection of state structs. The architecture must define root state, child regions, legal substates, events/actions, guards, allowed transitions, blocked transitions, entry/exit effects, declared write sets, and generation effects.
    *   `AppState`: the single formal application-state root. During migration it must be explicitly mapped to the existing `ViewContext` root: either `ViewContext` is the storage representation of `AppState`, or `AppState` is embedded under `ViewContext`; there must not be two authoritative roots. Any legacy `ViewContext` mirror of AppState-owned fields is a compatibility shim subject to this task's shim rules.
    *   `GlobalConfigState`: default visibility, file-display options, key/profile configuration, and view preferences.
    *   `VolumeState[]`: shared directory tree model, logged/expanded topology, file payload cache, shared workflow state only where explicitly documented as non-panel-local, and generation/version counters. Panel-local tags, selection, focus, filters, and visibility must not be owned by `VolumeState`.
    *   `PanelState[2]`: panel-local current volume, selected directory/file identity, tree viewport identity, file cursor/viewport identity, focus owner (`tree`, `small-file`, `big-file`, `preview`, `command`), dotfile visibility, filter state, panel-local tags/tagged-path state, and restore snapshots. A specific transition may project a documented shared/global view only without changing panel ownership.
    *   `ModeState`: single/split layout, compare/copy/move/showall/global/archive modes, and modal ownership.
    *   `RenderState`: derived layout, dirty regions, and last projected screen. Render caches may be used only for invalidation/diffing and must never feed selection, focus, identity, visibility, or restore.
*   **Transition Contract:**
    *   Input/event flow must be: `decode action/event -> validate AppState -> run one registered transition -> produce new AppState -> derive read-only RenderProjection -> render projection`.
    *   Every UI-affecting input/event must have exactly one transition record: source state, event, guard, allowed/blocked result, target state, declared write set, generation changes, side effects, and render invalidation output.
    *   Invalid transitions must resolve through a registered deterministic outcome: no-op, blocked diagnostic, modal prompt, or registered fallback transition. They must not mutate unrelated state.
    *   Controllers dispatch actions; they must not repair state ad hoc or directly encode restore policy.
    *   Render code displays resolved state; it must not mutate owners, re-anchor viewports, infer focus, or choose selection from raw rows.
    *   Restore/rebind must use durable identity plus generation validation, never stale flat-list rows, stale pointers, footer text, or previous rendered shape.
    *   Inactive panel state is frozen across active-only actions. Shared topology changes may be mirrored only through explicit transition rules that rebind each panel by its own identity.
    *   Terminal resize/reflow is an explicit event. Previous screen geometry, rendered rows, and cached window shape must never be used as restore authority.
*   **Mechanism:**
    *   Produce a concrete ownership map naming the single owner for each UI/state field: dotfile visibility, active panel, focus owner, tree viewport, file-window shape, tags, filters, showall/global state, footer/stats projection, and modal command state.
    *   Inventory current competing owners and classify them as canonical, derived mirror, compatibility shim, or defect.
    *   Introduce a transition boundary/API for all UI-affecting actions. High-risk actions (`F8`, `Tab`, `Enter`, `Esc`, refresh, dotfile toggle, delete/mkdir, search/jump, showall/global/tagged-only, volume cycling/release) are the first migration batch, not the only required coverage.
    *   The transition matrix must include non-key UI-affecting events, including filesystem mutation results, live-refresh/watcher events, signal-flag handling such as resize/shutdown-visible cleanup, command completion/failure outcomes, and any rebuild/rebind callback that can affect visible state.
    *   Define stable identity schemas and generation domains for volume, directory, file, panel, focus shape, modal target, visibility/filter state, topology, file payload, volume lifecycle, and layout/reflow.
    *   Add debug invariant checks at owner boundaries: illegal inactive-panel mutation, render-side mutation, stale-generation restore, hidden-entry visible-navigation selection, and shared-state overwrite of panel-local state.
    *   Add dynamic/state-sequence tests that generate or enumerate action sequences and assert invariants after every transition, not only final screen snapshots.
    *   Add a transition-diff harness that snapshots state before/after every transition and render/reflow pass, then fails if any field outside the transition's declared write set changed.
    *   Migrate restore/render paths incrementally only through QA-visible compatibility wrappers; every accepted wrapper must remove, disable, or quarantine at least one old authority path in the same change.
    *   Compatibility shims must declare the old authority path they replace, whether they may read or write, the invariant checks protecting them, the owner, removal trigger, target replacement transition, and explicit follow-up roadmap task. Shims without those fields fail review/QA, and new bypasses around the transition boundary must fail QA.
*   **Acceptance Criteria:**
*   `docs/ARCHITECTURE.md` defines the AppState hierarchy, owner map, statechart contract, transition contract, blocked-transition semantics, and render-projection rule as the canonical target architecture.
*   A complete action-transition matrix exists for all keybindings, menu actions, modal actions, refresh/rebuild operations, volume operations, and terminal resize/reflow events.
*   The action-transition matrix is the canonical registry for UI-affecting actions, and QA fails if a keybinding/menu/modal/resize/rebuild event dispatches outside the registered transition boundary.
*   The transition registry covers both user actions and non-key UI-affecting events: filesystem mutation results, watcher/live-refresh events, signal-flag events, rebuild/rebind callbacks, and command completion/failure outcomes.
*   A state-transition test harness exists that can run scripted action sequences and check invariants after each step.
*   Dynamic tests validate intermediate state after every action, not only the final rendered screen, and fail if any transition mutates state outside its declared owner.
*   The invariant harness checks declared write sets after every transition and verifies render/reflow performs no owner-state mutation.
*   Tests cover at least these invariants: inactive panel unchanged unless targeted; render does not mutate owner state; hidden entries cannot be selected through visible navigation; focus restoration is panel-local; viewport identity survives rebuild when still visible; global/shared state cannot overwrite panel-local state; stale snapshots fail closed through deterministic fallback.
*   Blocked/invalid transitions are covered by tests and prove deterministic no-op/fallback behavior with no unrelated mutation.
*   High-risk flows are covered in the first migration batch: `F8`, `Tab`, `Enter`, `Esc`, dotfile reveal/conceal, refresh, delete/mkdir, search/jump, showall/global/tagged-only, file small/big transitions, volume cycling/release, and split close/reopen.
*   No UI-affecting action may mutate panel, volume, mode, focus, visibility, viewport, restore, or render-invalidation state except through the transition boundary, unless explicitly listed as a time-bounded compatibility shim.
*   No compatibility shim is accepted unless it appears in a documented shim registry with owner, old authority path, read/write permission, invariant checks, removal trigger, replacement transition, follow-up task, and QA enforcement that fails unregistered bypasses.
*   Existing split/viewport fixes are either routed through the transition boundary or explicitly marked as compatibility shims with removal tasks.
*   `docs/SPECIFICATION.md` and `docs/ARCHITECTURE.md` are updated or cross-linked so existing restore/split contracts do not preserve a narrower `F8`/`Tab`-only transition model, stale task references, or conflicting ownership language.
*   `make qa-all` / PR full-QA CI passes with the invariant test harness enabled.
*   - [ ] **Status:** Not Started.

### **Task 2: Remove Dead-History Comments + Add Anti-History Comment Gate**
*   **Goal:** Remove comments that describe removed code/history and prevent their reintroduction.
*   **Policy:** Source comments may describe only invariants, ownership/lifetime assumptions, aliasing constraints, or non-obvious design rationale.
*   **Forbidden Comment Classes:** commented-out declarations/code blocks, instruction-transcript comments, and strong explicit dead-history phrasing; ambiguous phrases like `used to`, `moved to`, `obsolete`, or `removed` require stronger contextual evidence before they count as dead history.
*   **Mechanism:** Add a QA guard script (wired into `qa-all`) that fails on forbidden dead-history comment patterns, with allowlist-only exceptions for migration-required cases.
*   **Acceptance Criteria:**
*   Existing dead-history comments in first-party code are removed or rewritten to durable design intent.
*   New guard fails on forbidden patterns and passes on current baseline.
*   `make qa-all` passes with the new guard enabled.
*   - [x] **Status:** Complete.

### **Task 3: Unified Clean-Code Compliance Gate**
*   **Goal:** Enforce clean-code rules continuously through one measurable gate instead of ad-hoc review.
*   **Scope:** Naming quality, function size/argument/side-effect discipline, duplication control, boundary-condition encapsulation, and test clarity/independence.
*   **Mechanism:** Add `qa-clean-code` (included in `qa-all`) that combines static checks + targeted meta-tests:
    *   **Naming checks:** fail on new ambiguous/abbreviated single-letter parameter identifiers outside accepted loop/index conventions unless they are explicitly allowlisted with owner + removal plan; fail on new threshold/comparison magic-number literals outside the same allowlist contract.
    *   **Function checks:** enforce max function-size budgets, max argument-count budgets, and fail on new flag-argument signatures unless the exception is explicitly recorded with owner + removal plan.
    *   **Boundary/duplication checks:** bundle `qa-module-boundaries` inside `qa-clean-code` so controller owner-boundary allowlists, controller-growth budgets, and the shared clean-code baseline-debt registry are enforced through one gate instead of ad-hoc review.
    *   **Test checks:** fail on shared mutable test globals and non-function-scope fixtures unless they are explicitly allowlisted; targeted meta-tests must cover the clean-code guard classifiers.
*   **Acceptance Criteria:**
*   `qa-clean-code` exists, is documented, and is wired into `qa-all`.
*   Baseline debt is explicitly allowlisted with owner + removal plan; no silent grandfathering.
*   New violations fail local gate and CI evidence.
*   `make qa-all` passes with `qa-clean-code` enabled on the current baseline.
*   - [x] **Status:** Complete.

### **Task 4: Compiler Warning Baseline + No-New-Warnings Gate**
*   **Goal:** Reduce `-Wall/-Wextra` warning debt to a maintained baseline and prevent warning regressions on supported toolchains.
*   **Rationale:** A low-noise warning profile improves signal quality and catches real defects earlier without forcing brittle all-or-nothing local builds.
*   **Scope:** Build/QA policy and warning remediation only; no feature behavior changes in this task.
*   **Acceptance Criteria:**
*   Define and document baseline warning counts for `gcc` and `clang` under current default flags.
*   Add strict QA mode (for example `STRICT=1`) that enables `-Werror` for CI/QA gates while preserving a portable default developer build.
*   Burn down existing warning debt in prioritized batches (safety/correctness first), keeping suppressions minimal and justified.
*   CI/QA fails on new warnings in strict mode for supported compilers.
*   - [x] **Status:** Completed.

### **Task 5: Code-Smell Gate (Audit + Detect + Block)**
*   **Goal:** Add explicit QA and merge-gate enforcement that audits current code smells and blocks new/reintroduced structural smell debt.
*   **Scope:** controller growth, god-function budgets, module-boundary violations, complexity hotspots, and architecture drift.
*   **Acceptance Criteria:** Smell baseline audit evidence exists, recurring smell checks are mandatory in `qa-all`/PR evidence, and merge is blocked on unapproved new smell violations.
*   - [x] **Status:** Completed.

#### **Task 5.1: Baseline Code-Smell Audit and Debt Register**
*   **Goal:** Audit current codebase for structural smells and categorize debt with explicit remediation sequencing.
*   **Deliverables:** baseline report covering hotspots, oversized controllers/functions, boundary exceptions, and tracked rationale for retained debt.
*   - [x] **Status:** Completed.

#### **Task 5.2: Strengthen Smell-Prevention Guards**
*   **Goal:** Prevent reintroduction of known smell patterns via automated policy checks.
*   **Mechanism:** Tighten module-boundary/controller-growth policies and require explicit approval paths for exceptions.
*   - [x] **Status:** Completed.

#### **Task 5.3: Smell Gate Evidence as Merge Prerequisite**
*   **Goal:** Ensure smell-audit results are part of mandatory merge evidence, not optional review notes.
*   **Mechanism:** Require successful smell checks in QA artifacts and block integration on unresolved unapproved violations.
*   - [x] **Status:** Completed.

### **Task 6: Recurring Code-Quality Burn-Down + Lean Simplicity Contract**
*   **Goal:** Run ongoing debt burn-down passes (not only one-off cleanup) while enforcing a durable simplicity contract: code should stay lean, readable, and non-obfuscated.
*   **Rationale:** Existing gates can stop regressions but do not automatically eliminate legacy bloat/smell debt. Recurring passes are required to steadily reduce old hotspots.
*   **Scope:** Controller/file/function bloat reduction, complexity hot-spot remediation, readability/simplicity standards, and corresponding documentation of rules/checklists.
*   **Mechanism:**
    *   Define a recurring cadence trigger set (for example every N merged feature tasks, and before milestone tags), not only pre-beta.
    *   In each pass, reduce a bounded batch of highest-impact hotspots (top-N by size/complexity) with behavior-preserving refactors.
    *   Enforce a simplicity contract: prefer fewer lines when clarity is preserved; avoid shorthand/obfuscated patterns and unnecessary recursion/indirection.
    *   Expand `docs/ai/CODE_QUALITY.md` into an actionable blueprint (smell classes, remediation heuristics, acceptance checklist, and evidence format).
*   **Acceptance Criteria:**
*   Recurring cadence policy is documented and tied to explicit triggers.
*   Each cadence run produces measurable debt deltas (before/after hotspot list).
*   No regressions in behavior or UX semantics for touched paths.
*   `make qa-module-boundaries` and required bundled QA gates remain green for each pass.
*   - [x] **Status:** Completed.

## **Phase 3: Build System, Documentation, and CI**
*This phase focuses on project infrastructure, developer experience, and release readiness.*

### **Task 7: Add Automated Coverage Reporting and CI Threshold Gate**
*   **Goal:** Integrate `gcov`/`lcov` into Makefile and CI to generate automated statement-coverage reports and enforce a minimum coverage threshold.
*   **Rationale:** Coverage reporting gives a measurable quality signal and prevents silent regression of test effectiveness.
*   **Scope Lock:** Coverage instrumentation, report generation, and CI gating only; no feature behavior changes in this task.
*   **Acceptance Criteria:**
*   Add reproducible coverage targets in Makefile for local and CI usage.
*   CI publishes coverage output as build artifacts and reports pass/fail status.
*   CI fails when measured statement coverage drops below 80% (or configured threshold).
*   Document how to run coverage locally and how threshold policy is enforced in CI.
*   Update `docs/AUDIT.md` in the same change so audit policy reflects implemented coverage commands/gates (not planned-only wording).
*   - [x] **Status:** Complete.

### **Task 8: Restructure and Expand Test Suite**
*   **Goal:** Tidy up existing test scripts into a coherent, modular structure and thoroughly expand the regression suite for comprehensive coverage.
*   **Rationale:** A well-structured test suite is easier to maintain and extend. Thorough, systematic coverage ensures reliability and prevents regressions across complex file operations.
*   **Scope Lock:** Test architecture, fixtures, and regression coverage expansion only; no runtime feature behavior changes in this task.
*   **Acceptance Criteria:**
*   Reorganize tests into contributor-friendly modules by domain (filesystem ops, archive ops, split/panel isolation, and UI interaction contracts).
*   Add file/archive integrity regression coverage for mutation workflows with deterministic pre/post file-count and content-hash assertions.
*   Add cancel/interruption/failure-path tests that assert no partial/corrupt leftovers, including archive rewrite paths.
*   Add edge-path coverage for overwrite/self-target, same-path, cross-device behavior, permission/no-space failures, odd filenames, and archive path edge cases.
*   PTY/TUI helper conventions favor event-driven wait helpers and line-snapshot reuse over fixed sleeps and repeated full-screen joins in polling loops.
*   Document fixture/helper conventions so new contributors can add mutation-integrity tests consistently.
*   - [x] **Status:** Completed.

### Task 90: **Test Contract Resilience Remediation**
*   **Goal:** Make the test suite resilient to legitimate changes in prose, translations, themes, terminal size, wrapping, footer packing, redraw timing, and internal implementation structure.  A test must prove a durable contract: an action produces its intended effect, an invariant is preserved, or a generated artefact is correct.  It must not fail merely because an editable presentation detail moved or was reworded.
*   **Status:** Complete. Test Contract Resilience Remediation now proves durable behavioural contracts across the reviewed boundary; authored F1 sources (`etc/help/f1.en.md` and `etc/help/f1.de.md`) remain unchanged.
*   **Definitions:**
    * **Durable assertion:** filesystem/resulting state, mode transition, selected entity, semantic role/style, generated-artifact equivalence, security property, or a user-visible capability reached through its documented action.
    * **Incidental assertion:** an exact editable sentence, translation, command-strip packing/order, terminal row/column, visual grid, wrap point, scroll offset, fixed key-press count, or a particular private function/call branch when another implementation can provide the same behaviour.
    * Exact text remains valid only when the text itself is the external contract: CLI diagnostics/options, a machine-readable format, an intentionally stable config/template syntax, or a documented security warning.  Source inspection remains valid only for a non-observable static property (unsafe API ban, generated-file synchronisation, module-boundary guard, or a security-sensitive construction that cannot be proved safely through runtime execution).
*   - [x] **Status:** Complete. The reviewed behavioural and static contracts now preserve durable action/result evidence across the resilience boundary.

#### Task 90.1 **Generate and Reconcile a Measurable Brittle-Pattern Baseline**
*   Generate a checked-in baseline report over every `tests/` Python file for: direct `time.sleep()`; polling/retry loops; hard-coded terminal rows/columns, screen slices, and visual grids; fixed navigation/key-press counts; source reads and implementation-string assertions; and exact user-facing prose assertions.  Each entry must record its file, enclosing test/helper where available, matched pattern, and disposition.
*   **Authority:** Add `tests/contract_resilience_baseline.json`, generated by `scripts/check_test_contract_resilience.py`.  The JSON schema version, pattern id, path, symbol/line, evidence, disposition (`remediated`, `retained`, or `out_of_scope`), and mandatory reason/owner fields are the authoritative inventory and exception mechanism.  The checker must fail when an in-scope match is absent, lacks a disposition/reason, or is a new unreviewed match.
*   This is a completion gate, not a sample list: every baseline match must be **remediated**, **retained with a specific durable-contract reason**, or **explicitly out of scope with a concrete owner/boundary reason**.  It must cover omitted candidates including `tests/test_attribute_prompt_flow.py` and all of `tests/test_theme_ui_contract.py`, not only F1-related portions.
*   Add a repeatable report/guard so new matches cannot silently enter the suite.  A broad regex suppression or blanket allowlist is not reconciliation.
*   Use `wait_for_condition`, `wait_for_text`, or an event/observable-state predicate.  Do not use `time.sleep()` or a retry loop as synchronization.  A bounded loop is permitted only to drive a known repeated user action until a semantic predicate succeeds; its bound must be a safety cap, not a claimed interaction count or ordering contract.
*   Never identify a popup or mode using a label that may also occur in the underlying application footer.  Use the popup title/frame, semantic style, or a state transition instead.
*   For TUI tests, replace row/column slices and full-screen grids with fuzzy/semantic helpers: visible text by role, selected item identity, modal presence, screen-state transition, file-system effect, and style-role comparison.
*   For menus, footers, help, and documentation, assert the action is available and has the correct effect; do not require exact English labels, the order in which entries wrap, or a fixed number of arrows/pages to reach it unless ordering is explicitly a published user contract.
*   Replace implementation-source assertions with focused runtime tests where the behaviour is observable.  Keep static guards only for the explicit exceptions above, and state the invariant in each retained guard's test name/message.
*   Preserve test intent.  Do not delete a regression merely because it is brittle: rewrite it to reproduce the same user-visible failure and verify the post-action state.
*   - [x] **Status:** Complete.

#### Task 90.2 **Remediate Waiting and Navigation Mechanisms**
*   Scope: `tests/tui_harness.py`, `tests/ytnova_control.py`, and every baseline match for sleeps, polling, fixed navigation, or popup detection, including `tests/test_attribute_prompt_flow.py`.  Build event-driven helpers that wait for semantic state and navigate to fixture identity rather than a terminal row or a number of key presses.  This is an independent validation boundary.
*   **Reference-only inventory — fixed-navigation tests:** `tests/test_f2_vols.py`, `tests/test_dir_window_dispatch_regressions.py`, `tests/test_panel_isolation.py`, `tests/test_display_layout.py`, `tests/test_compare_actions.py`, `tests/test_archive_exit_ui.py`, `tests/test_archive_ui.py`, `tests/repro_same_volume_home_mkdir_bug.py`, and `tests/repro_real_home_same_volume_split_bug.py`.  Their `range(8)`, `range(80)`, and `range(200)` navigation loops use target fixture identity/current-path helpers and only a diagnostic safety cap.
*   **Reference-only inventory — sleep/poll tests:** `tests/ytnova_control.py`, `tests/tui_harness.py`, `tests/test_panels.py`, `tests/test_vi_keys_mode.py`, `tests/test_f2_vols.py`, `tests/test_archive_exit_ui.py`, `tests/test_ui_layout.py`, `tests/test_ghost_bugs.py`, `tests/test_exit_empty_dir.py`, `tests/test_dir_window_dispatch_regressions.py`, `tests/test_ui_display.py`, `tests/test_file_window_dispatch_regressions.py`, `tests/test_panel_isolation.py`, `tests/test_filtering.py`, `tests/test_refresh_race.py`, `tests/test_compare_actions.py`, `tests/test_viewer_return_ui.py`, `tests/test_small_window.py`, `tests/test_state_collision.py`, `tests/test_core.py`, `tests/test_security_shell_paths.py`, `tests/test_display_layout.py`, `tests/test_commands_exhaustive.py`, `tests/test_tagged_action_regressions.py`, `tests/test_archive_write_parity.py`, and `tests/test_fileops_integrity.py`.  Their direct sleeps and time-driven polling use `tests/tui_harness.py` semantic predicates with timeout diagnostics.
*   - [x] **Status:** Complete. Event-driven configuration completion, orderly process-exit waits, observable startup and resize readiness, refresh-request deletion coordination, split-panel fixture transitions, panel-local selection preservation, and display target-identity navigation cover profile reload, history persistence, panel switching, and display actions. The final sweep leaves only the canonical event-driven PTY and control-session wait predicates as explicitly retained baseline rows.

#### Task 90.3 **Remediate Geometry, Presentation, and Runtime Interaction Contracts**
*   Scope: the remaining runtime baseline rows: help, command strips, footers, themes, layout, viewports, and modal presentation.  This is a separate validation boundary from 90.2 and **consumes, rather than reimplements**, the reconciled 90.2 waiting/navigation helpers.
*   The preceding fixed-navigation and sleep/poll inventories are outside this scope.  Only update their presentation assertions after their semantic action/state helpers are in place.
*   **Contextual help — `tests/test_help_text_contract.py` (entire family):** Rewrite the helper layer (`_scroll_help_to_text`, `_send_help_key_until_text`, `_open_help_detail`, `_follow_help_topic`) and all tests that depend on exact F1 body text, projected command rows, `Related help`, `Enter/Right open link`, footer copy, body coordinates, `HOME`/`END` results, or fixed arrow/page counts.  This includes the navigation, related-help, scrolling, wrapping, split-mode, prompt-help, Showall, output, index, picker, archive, page-key, and integrated-source tests in that file.  Stable replacement: F1 opens the contextual popup; an authored Markdown link is styled/selectable; Right **or** Enter follows it; Left returns; Esc returns to the invoking surface; footer styling follows the active locale/theme.  Do not assert editable body prose, link order, scroll restoration, or terminal geometry.
*   **Help schema/documentation contracts — `tests/test_help_source_schema.py`, `tests/test_help_generator.py`, and the F1/help portions of `tests/test_theme_ui_contract.py`:** Keep source-schema and generator validation, topic/context-id coverage, locale parity, and generated-output equivalence.  Remove assertions requiring particular explanatory facts, command descriptions, link wording, alphabetical presentation, or exact authored prose.  Do not inspect `runtime_help.c`/generated-topic implementation branches when a runtime context-opening test can prove the behaviour.
*   **Command strips and footer packing — `tests/test_command_strip_visibility.py`, `tests/test_display_layout.py`, `tests/test_archive_exit_ui.py`, `tests/test_compare_actions.py`, `tests/test_f2_vols.py`, `tests/test_ui_display.py`, and footer-related portions of `tests/test_panel_isolation.py`:** Replace exact footer rows, `lines[n]`, `footer_rows[n]`, string `.index()` order, spacing, and wrap/truncation grids with: command availability in the active mode, no stale commands after a transition, action invocation, and semantic style-role checks.  Retain a key order assertion only where a documented keyboard ordering contract exists; name that contract explicitly.
*   **Layout/viewport/grid tests — `tests/test_display_layout.py`, `tests/test_stats_panel.py`, `tests/test_ui_layout.py`, `tests/test_f7_preview.py`, `tests/test_modal_message_layout.py`, `tests/test_modal_color_taxonomy.py`, `tests/test_modal_severity_contract.py`, `tests/test_small_window.py`, `tests/test_panels.py`, and layout-heavy sections of `tests/test_panel_isolation.py`:** Remove hard-coded rows, columns, frame coordinates, exact blank-line counts, and character-grid comparisons.  Assert that the focused item remains discoverable, selected state follows navigation, a modal is usable, content is not lost/corrupted, and semantic colors/styles differ or match as intended.  Geometry may be tested only as a bounded capability (for example, "does not overflow/overlap at a supported minimum size"), not an exact drawing map.
*   - [x] **Status:** Complete. Contextual help, command-strip, footer, theme-role, layout, viewport, modal, and panel contracts now assert stable runtime actions, state, and semantic roles rather than editable prose or terminal geometry. The resilience baseline contains no geometry or presentation remediation rows. Authored F1 Markdown remains unchanged.

#### Task 90.4 **Preserve Documentation Semantics Without Coupling to Prose**
*   Retain language-independent checks for documented action-to-topic, action-to-link, and context-to-help relationships.  For example, F1 from each supported context must open its owned topic, an authored Markdown link must lead to its declared target, and navigation must return to the source context.  Do not require English wording, translations, line position, or presentation order.
*   - [x] **Status:** Completed.

#### Task 90.5 **Classify and Preserve Justified External Contracts**
*   **Implementation-coupled runtime tests — `tests/test_dir_window_dispatch_regressions.py`, `tests/test_file_window_dispatch_regressions.py`, `tests/test_archive_ui.py`, `tests/test_color_config.py`, `tests/test_theme_ui_contract.py`, `tests/test_modal_color_taxonomy.py`, `tests/test_modal_message_layout.py`, `tests/test_modal_severity_contract.py`, `tests/test_commands_exhaustive.py`, `tests/test_tagged_action_regressions.py`, `tests/test_command_strip_visibility.py`, `tests/test_help_text_contract.py`, and `tests/test_security_shell_paths.py`:** Review every `_read_source`, `read_repo_source`, `extract_function_block`, and literal implementation-string assertion.  Convert user-observable behaviour to runtime tests.  Keep only static guards for non-observable security, generation, or architecture invariants, with the reason documented in the assertion message.
*   **External command/config/template contracts:** `tests/test_cli_version_flags.py`, `tests/test_profile_template_sync.py`, `tests/test_theme_catalog_sync.py`, `tests/test_theme_config_paths.py`, `tests/test_mcp_doctor.py`, `tests/test_pre_push_guard.py`, `tests/test_ci_repair_loop.py`, and `tests/test_install_shadow_guard.py`.  Exact output/content can be valid where it is a published CLI, config, generated-file, or workflow interface.  Remove only incidental prose and whitespace checks; keep machine- or user-consumed contract checks.
*   **Security/static-analysis contracts:** `tests/test_c_unsafe_apis_guard.py`, `tests/test_security_gate_contract.py`, `tests/test_security_tempfiles.py`, `tests/test_security_shell_paths.py`, `tests/test_fuzz_harness_sync_guard.py`, and `tests/test_appstate_contract_guard.py`.  Source inspection is appropriate where runtime execution cannot safely prove the absence of an unsafe API, injection construction, forbidden boundary, or generated-contract drift.  Each retained test must identify the protected invariant rather than a private implementation spelling.
*   **Filesystem and archive effects:** `tests/test_core.py`, `tests/test_destination_prompt.py`, `tests/test_archive_backend.py`, `tests/test_archive_write_parity.py`, and `tests/test_fileops_integrity.py`.  Their primary assertions should remain filesystem/archive results, cleanup, and error handling.  Replace sleeps/position-dependent UI setup only; do not weaken end-state integrity assertions.
*   - [x] **Status:** Completed. Observable behavior is covered through stable action and filesystem/archive effects; retained external, security, generated, and architecture guards identify their non-observable invariant and why runtime proof cannot safely establish it.

#### Task 90.6 **Document Retained Static-Inspection Reasons**
*   For every retained baseline source inspection, including every classification under **90.5**, add or retain an assertion message that names the non-observable invariant and why a runtime test cannot prove it safely.
*   - [x] **Status:** Completed. Retained generated-source assertions identify the source/generated invariant and explain why runtime execution cannot reveal pre-build drift.

#### Task 90.7 **Prove and Validate Resilience**
1. Add/reuse semantic test helpers first: popup/modal detection, selected-item identity, footer command presence by action/key, style-role comparison, and event-driven state/file waits.  Do not add helpers that parse terminal coordinates or English prose.
2. Complete **90.2** first, then **90.3**, each with its own focused validation.  Do not edit either authored F1 Markdown source.
3. Complete **90.4** before changing explanatory-help assertions; retain semantic context/topic/link relationships while removing prose coupling.
4. Apply **90.5** and **90.6** to every source-inspection test: state why each retained static inspection is impossible or unsafe to prove at runtime.  Replace all others with observable behaviour tests.
5. Reconcile every baseline row using the authoritative **90.1** disposition values: **remediated**, **retained**, or **out_of_scope**, each with its required reason and owner/boundary data.
6. **Matrix authority:** add `tests/contract_resilience_matrix.json`, consumed by the selected behavioural tests.  Its locale catalog is the locale sources passed to `make help-assets` (currently `etc/help/f1.en.md` and `etc/help/f1.de.md`); its theme catalog is the deterministic test themes declared in that file, not every user-editable theme; and its supported size profiles are `constrained` = 24 rows x 80 columns and `normal` = 36 rows x 120 columns.  Any catalog/profile change must update this file and the matrix evidence.
7. Prove resilience positively: run the matrix-selected behavioural tests across every locale/theme/size entry.  The evidence must show F1/context navigation, representative footer/menu actions, and modal/prompt round-trips still work; it must not merely show that brittle assertions were deleted.
8. Run focused tests per mechanism from the repository root with the virtual environment activated and host permissions.  Do not run full QA for this documentation/audit item unless explicitly requested.  Before a PR, record the baseline reconciliation and targeted matrix evidence.
*   - [x] **Status:** Complete. The authoritative resilience matrix exercises contextual F1/link navigation, representative footer/menu actions, and modal prompt cancellation across every supported locale, deterministic test theme, and size profile; the baseline guard remains fully reconciled.

### Task 91: **Enforce Stable Behavioural Test Contracts**
*   **Goal:** Add a CI-enforced test-authoring guard that prevents new tests from coupling to volatile TUI presentation rather than observable behaviour.
*   **Policy:** The guard must reject—or require a narrowly documented exception for—static sleeps, fixed terminal coordinates/grids, full-screen snapshots, padding/wrapping/footer-order assertions, and assertions on editable copy or translations. Tests must use event-driven waiting and assert stable action/result contracts: state transitions, selected identity, filesystem or archive effects, modal capabilities, generated artefacts, or documented external interfaces.
*   **Input Contract:** Input sequences may be tested where they are themselves the user-facing contract, but tests must not depend on incidental menu position, fixed key counts, or presentation details. Exact text and geometry assertions are permitted only for explicitly documented stable contracts.
*   **Enforcement:** Enforce the policy through automated lint/contract checks, reviewable test helpers/templates, and a checked-in exception allowlist containing the test, rule, rationale, owner, and expiry/removal condition.
*   **Scope Lock:** Static detection is a first-line guard, not a substitute for semantic review. Keep the allowlist narrow, local, and reviewable; it must not become a blanket suppression mechanism.
*   - [x] **Status:** Complete. The CI quality gate rejects unreviewed volatile test patterns; every reviewed exception identifies its test, rule, rationale, owner, and removal condition.

### **Task 9: Finalize Documentation**
*   **Goal:** Update the `CHANGELOG`, `README.md`, and `CONTRIBUTING.md` files to reflect all new features and changes before a release.
*   **Rationale:** Ensures users and developers have accurate, up-to-date information about the project.
*   - [x] **Status:** Completed.

### **Task 10: Initialize Distributed Issue Tracking (git-bug)**
*   **Goal:** Configure `git-bug` to act as a bridge between the local repository and GitHub Issues. Migrate the contents of `BUGS.md` and `TODO.txt` into this system prior to public release.
*   **Rationale:** Allows the developer to maintain a simple local text-based workflow during heavy development, while ensuring that all tracking data can be synchronized to the public web interface when the project goes live.
*   - [x] **Status:** Completed.

### **Task 11: Configuration Integrity and Persistence**
*   **Goal:** Group configuration-source governance and config/history persistence hardening under one umbrella with ordered subtask delivery.

#### **Task 11.1: Config Source-of-Truth + Generation/Verification Gate**
*   **Goal:** Enforce one canonical editable default profile source and make generated artifacts deterministic and verifiable.
*   **Source-of-Truth Policy:** `etc/ytnova.conf` is the human-edited default runtime config source; `etc/ytnova.themes` is the separate human-edited default theme source; `etc/ytnova.commands` is the human-edited default active command-surface source; and any Task 11.5 locale/layout preset catalogs (for example `etc/commands/*.conf`) must follow the same source/generated discipline once introduced. Generated headers/templates are generated-only and consumed by `--init`.
*   **Mechanism:** Add reproducible generator paths for each canonical config surface (`etc/ytnova.conf` -> `src/core/default_profile_template.h`, `etc/ytnova.themes` -> theme catalog output, `etc/ytnova.commands` -> default command artifact(s), plus any Task 11.5 preset-catalog outputs) and a QA/CI check that fails when generated output is stale or hand-edited.
*   **Acceptance Criteria:**
*   `ytnova --init` output remains byte-equivalent to the canonical template semantics.
*   A single documented command regenerates the header deterministically.
*   `make qa-all` (or dedicated gate) fails on source/generated drift.
*   **Files to Modify:** `Makefile`, `scripts/*` (new/updated generator + verifier), `src/core/default_profile_template.h`, and contributor/docs references as needed.
*   - [x] **Status:** Completed.

#### **Task 11.2: Split Command Customization into `commands.conf` (i18n/l10n-Safe Layout)**
*   **Goal:** Separate user-visible command labels, displayed key tokens, input/action customization, and custom shell-command bindings from the main runtime profile using one XDG-first companion command surface, while keeping the canonical design safe for future gettext/i18n/l10n work.
*   **Rationale:** The current main profile still mixes core runtime settings with menu-text overrides and input/action customization. That blocks clean ownership boundaries and makes future localization fragile because whole rendered footer/menu lines are not stable translation units. Canonical user-editable command data must be keyed by stable action identity, while rendered footer keybinding/F1/menu lines must be assembled at runtime from localized labels plus current key tokens.
*   **Pre-Implementation Clarification Gate (mandatory):** A stateless AI or fresh maintainer pass must not jump straight to code on this task. Before implementation, stop and explicitly confirm the following decisions with the maintainer so the split is not inferred differently by different agents:
    *   the canonical home of command customization;
    *   the exact precedence order between `commands.conf`, legacy sections in `ytnova.conf`, and built-in defaults;
    *   the runtime assembly model for visible commands: stable action ID, resolved label, resolved key token, and availability/enabled state;
    *   the starter-file format and comments for `commands.conf`;
    *   what `--init` generates for each surface; and
    *   what `F10` edits/creates for each surface.
*   **Discussion-First Output Contract (mandatory):** Before coding, the active agent must summarize the proposed answers for those six points back to the maintainer in concrete file/path terms and get alignment on any ambiguous item. Silent assumption is forbidden for this task.
*   **Canonical Surface Split:**
    *   `etc/ytnova.conf` -> `~/.config/ytnova/ytnova.conf` (fallback `~/.ytnova`) remains the human-edited source for core runtime configuration only.
    *   `etc/ytnova.themes` -> `~/.config/ytnova/themes.conf` (fallback `~/.ytnova.themes`) remains the human-edited source for theme roles/palette only.
    *   New `etc/ytnova.commands` -> `~/.config/ytnova/commands.conf` (fallback `~/.ytnova.commands`) becomes the human-edited source for line-1/line-2 command bindings, shown tokens, labels, and custom shell-command bindings replacing the old `[MENU]`, `[DIRMAP]`, `[FILEMAP]`, `[DIRCMD]`, and `[FILECMD]` ownership model.
*   **Canonical Ownership Rules:**
    *   `ytnova.conf` must not remain the canonical home for `[MENU]`, `[DIRMAP]`, `[FILEMAP]`, `[DIRCMD]`, or `[FILECMD]`.
    *   `commands.conf` is the canonical user-editable command-customization surface.
    *   No canonical user-editable file may store fully rendered footer keybinding/F1/menu lines as the primary data model.
    *   Action IDs, localized/default labels, displayed key tokens, disabled-state logic, and line layout must remain separate concerns.
    *   Custom shell-command bindings must live in the same `commands.conf` surface rather than in an unrelated side file.
*   **Localization Contract:**
    *   Stable action IDs are the canonical identity for user-visible commands and must remain untranslated.
    *   Default labels come from code/gettext-ready message sources, not from hardcoded rendered footer lines.
    *   `commands.conf` stores plain labels, exact input bindings, shown key tokens, stable action IDs, and optional custom shell commands as separate fields.
    *   Footer/help/menu rendering must assemble visible command entries from action ID + label + key token + availability state at runtime.
    *   If a shown key token appears in the label, runtime must render the compact mnemonic form inline (for example `(C)opy` or `mo(V)edir`). If it does not appear in the label, runtime must render the token separately with a space before the label (for example `(J) compare`). Multi-token displays must render slash-separated highlighted tokens with an unhighlighted slash (for example `(M)/(^N) move`).
    *   Locale-specific word order, mnemonic placement, truncation, and line packing must not depend on raw `[MENU]` whole-line overrides.
*   **Compatibility Contract:**
    *   Legacy `[MENU]`, `[DIRMAP]`, `[FILEMAP]`, `[DIRCMD]`, and `[FILECMD]` sections in `ytnova.conf` may be accepted only as compatibility inputs during migration.
    *   Companion-file precedence must be: `commands.conf` -> legacy section in `ytnova.conf` -> built-in defaults.
    *   If both `commands.conf` and a legacy section define the same command binding or label, `commands.conf` wins deterministically.
    *   Legacy `[MENU]` is a compatibility shim only and must not remain the long-term canonical override model.
*   **Discovery and Bootstrap Contract:**
    *   For `ytnova.conf`, `themes.conf`, and `commands.conf`, runtime must prefer `~/.config/ytnova/*.conf`.
    *   Home-dotfile fallbacks (`~/.ytnova`, `~/.ytnova.themes`, `~/.ytnova.commands`) exist only for environments where the XDG-style home config path cannot be used; they are not the preferred location on supported Linux/BSD/illumos/Hurd targets.
    *   Missing user files fall back to packaged/compiled defaults without creating a user file.
    *   Only `--init` and explicit `F10` edit flows create starter files.
*   **Mechanism:**
    *   Add packaged default source `etc/ytnova.commands`.
    *   Add deterministic generator/verification paths for starter artifacts derived from that source, matching the existing source/generated policy used for `etc/ytnova.conf` and `etc/ytnova.themes`.
    *   Refactor config discovery so startup, `--init`, reload, and `F10` all use one shared path-resolution policy across config/theme/commands surfaces.
    *   Add an explicit loader/validator for `commands.conf` rather than growing one monolithic profile parser indefinitely.
    *   `commands.conf` starter comments must be concise, self-explanatory, and already populated with live examples. They must document the canonical per-section row order `binding | shown | label | action | command`, explain the `[DIR]` / `[FILE]` section headers, explain that uppercase and lowercase letters may be bound separately, explain that `Ctrl+letter` bindings are case-insensitive and therefore collide on the same chord, and warn that action IDs are internal names that users must not translate.
    *   Extend `F10` so the configuration surface can edit/create the active config, themes, and commands files independently while preserving the same XDG-first/fallback rules.
    *   Reload must be atomic across config + commands + themes: validation failure in any one surface keeps the previously working runtime state.
*   **Concrete Decisions This Task Must Lock Down:**
    *   **Canonical home:** `commands.conf` is the canonical editable home of command customization.
    *   **Precedence:** `commands.conf` -> legacy `ytnova.conf` section -> built-in default.
    *   **Runtime assembly model:** render footer keybinding/F1/menu/prompt command entries from `(action_id, label, key_token, availability_state)` rather than from pre-rendered line text.
    *   **Starter-file model:** `commands.conf` uses canonical per-context section headers such as `[DIR]` and `[FILE]`; inside each section entries use the canonical row columns `binding | shown | label | action | command`; alias bindings may be comma-separated only when they share the same section, shown token, label, action, and command payload.
    *   **`--init` contract:** generate/bootstrap the active starter files for config, themes, and commands using the same discovery policy and deterministic source/generated pipeline.
    *   **`F10` contract:** edit/create the active file for config, themes, and commands independently rather than routing command edits back through monolithic `ytnova.conf` text.
*   **Acceptance Criteria:**
*   `commands.conf` exists as a documented first-class config surface with packaged defaults and starter-file generation.
*   `ytnova.conf` no longer serves as the canonical editable source for label overrides, displayed key tokens, key/custom-command mappings, or footer/menu line text.
*   No canonical user-editable file stores raw rendered footer/menu lines as the primary override model.
*   Structured command resolution is keyed by stable action identity and remains independent from bound key tokens.
*   `commands.conf` documents the canonical sectioned row order `binding | shown | label | action | command` with concise comments and live examples.
*   XDG-first discovery and home-dotfile fallback behavior is consistent across config, themes, and commands.
*   Missing user `commands.conf` files fall back to built-in defaults without file creation.
*   `--init` can bootstrap all three user-editable surfaces without ambiguity.
*   `F10` can resolve, create, and edit the active file for each surface independently.
*   Legacy sections in `ytnova.conf` still load during the compatibility phase, but `commands.conf` overrides them deterministically.
*   Reload fails closed if any one of config/theme/commands is malformed or invalid, and the previous working state remains active.
*   `docs/SPECIFICATION.md`, manpage/USAGE docs, and contributor guidance describe commands as structured overrides rather than rendered-line text replacement.
*   Locale/layout-aware packaged command presets remain follow-on work tracked separately by Task 11.5; Task 11.2 establishes the action/label/token ownership model those presets build on.
*   - [x] **Status:** Completed.

#### **Task 11.3: Config/History Robustness Gate (Strict Parse, Validation, Atomic Persistence)**
*   **Goal:** Harden config/history reliability and corruption resistance without changing user-facing feature semantics.
*   **Scope:**
*   Strict parse rules for config/history input.
*   Value validation (type/range/enum/path sanity) with explicit diagnostics.
*   Atomic write path for persisted files.
*   **Mechanism (mandatory):**
*   Writes must use `temp file -> fsync -> atomic rename` for config/history persistence paths.
*   Parser must reject malformed lines deterministically and report actionable errors.
*   Invalid values must not partially apply; fallback behavior must be explicit and logged/notified.
*   **Acceptance Criteria:**
*   Corrupted or malformed config/history input does not crash runtime and does not leave partial in-memory state.
*   Persistence writes are crash-safe and do not produce truncated/half-written files.
*   Regression tests cover malformed input, invalid value ranges, interrupted-write simulation, and recovery behavior.
*   - [x] **Status:** Completed.

#### **Task 11.4: Implement F10 Configuration Hub + Raw-Text Edit Paths**
*   **Goal:** Implement a user-friendly configuration hub (activated by `F10`) that gives the split config, commands, and theme surfaces one coherent in-app entry point while preserving the direct raw-text editing workflow.
*   **Rationale:** Keeps configuration discoverable without forcing menu-heavy editing flows that do not fit the preferred Unix/console workflow for many users. Starter-commented text files remain the expert-friendly authority, while `F10` provides the shallow entry point and reload path.
*   **UI Contract:** The `F10` command strip is `(C)onfig  co(M)mands  (T)hemes  (R)eload  (Esc)/(Q)uit`. Default action is config editing so `F10 -> Enter` remains the common path. Reload exists only under `F10`, not as a global/main-UI key.
*   **File Contract:**
    *   Config editing targets the preferred runtime config at `~/.config/ytnova/ytnova.conf`; if startup loaded the legacy fallback `~/.ytnova`, `F10` acts as the migration path and should create/edit the XDG profile whenever that target path is usable, falling back to `~/.ytnova` only when the XDG-style target path cannot be used.
    *   Theme editing targets the active theme file (`~/.config/ytnova/themes.conf` or fallback `~/.ytnova.themes`).
    *   Commands editing targets the active commands file (`~/.config/ytnova/commands.conf` or fallback `~/.ytnova.commands`).
    *   The commands path owns preset selection plus per-action overrides; packaged locale/layout preset catalogs remain read-only shared data rather than extra per-user config files.
*   **Dependency Contract:**
    *   Sequence after Task 11.2 defines the canonical split surfaces and precedence rules.
    *   Sequence after Task 11.5 if preset selection is exposed in the UI; `F10` must not hardcode obsolete bindings/labels split assumptions.
    *   The hub/edit flow must preserve the same XDG-first/home-dotfile-fallback rules as startup and `--init`.
    *   Raw-text editing must not re-collapse structured label/action data back into legacy raw `[MENU]` whole-line text.
*   **Acceptance Criteria:**
*   `F10` exposes one coherent hub for config, commands, themes, and reload.
*   `F10 -> Enter -> result` still opens the main config as the common path.
*   Each `F10` action edits or creates the active runtime file for that surface using the same path-resolution rules as startup.
*   The commands path can edit command overrides without requiring users to rewrite pre-rendered footer/menu lines, and can expose preset selection once Task 11.5 lands.
*   Starter-commented raw-text editing remains the canonical expert path for advanced users.
*   Reload from the `F10` hub respects the atomic reload contract across config/theme/commands surfaces.
*   Footer/F1/manpage wording for `F10` stays synchronized with the split-surface model.
*   - [x] **Status:** Completed.

#### **Task 11.5: Locale/Layout-Aware Command Presets**
*   **Goal:** Add proper locale/layout-aware command preset catalogs without reopening Task 11.2's ownership model: shipped presets live as separate packaged command-map files, `commands.conf` remains the one active user-editable command surface, and users or packagers can choose a preset without rewriting core command-dispatch code.
*   **Rationale:** The structured `commands.conf` model from Task 11.2 solves ownership and footer keybinding/F1 assembly, but it does not by itself provide a conventional way to ship German/Lithuanian/Hindi-friendly mnemonic sets. Locale-aware command presets should be packaged like read-only data, selected by stable preset ID, and overridden locally only where needed.
*   **Scope Lock:** Preset discovery, selection, validation, and override layering only. Do not add automatic locale remapping, physical-scancode assumptions, or a second user-editable bindings/labels surface.
*   **Preset Catalog Contract:**
    *   Packaged preset sources live as separate files (for example `etc/commands/en.conf`, `etc/commands/de.conf`, `etc/commands/lt.conf`, `etc/commands/hi-latin.conf`) rather than as one giant multilingual catalog embedded in the active user file.
    *   Installed presets are read-only shared data (for example `/usr/share/ytnova/commands/<preset>.conf`); `commands.conf` remains the sole canonical user-editable commands surface.
    *   Preset IDs are stable untranslated identifiers such as `en`, `de`, `lt`, and `hi-latin`; translator-facing prose belongs in labels/help text, not in preset IDs.
    *   A preset ID selects a command-layout variant. That variant may encode localized labels, mnemonic choices, keyboard-layout accommodations, or a combination of those, but it does not by itself select the whole application language.
    *   Each preset file uses the same action-based row model as Task 11.2 (`binding | shown | label | action | command`) so footer keybinding/F1/menu rendering still resolves from stable action IDs plus current key tokens and labels.
    *   Every preset file begins with concise explanatory comment headers naming the preset ID, intended locale/layout, that the file is packaged read-only data selected from `commands.conf`, and that action IDs must remain untranslated.
*   **Selection and Override Contract:**
    *   `commands.conf` may select zero or one preset with a single stable selector line (for example `preset = en`) and may then apply local per-action overrides in the canonical sectioned row format.
    *   If no preset selector is present, runtime uses the packaged default active command map from `etc/ytnova.commands`; upstream may ship that default as English, while a package may explicitly replace that default for a localized build.
    *   Comment/uncomment language blocks is not the canonical model; preset selection is explicit data, not manual whole-file surgery.
    *   Runtime may seed a packaged default preset from the build/package environment, but there is no automatic runtime locale-to-keymap remapping heuristic.
    *   Missing or invalid preset IDs must fail closed with a clear diagnostic and retain the previous working command state rather than silently substituting a guessed locale.
*   **Validation Contract:**
    *   Collision/unbound-action validation remains action-surface-aware and must run after preset load plus local overrides.
    *   Locale mnemonic freedom must not break the universal core: arrows, Enter, Esc, function keys, and other non-linguistic shared controls remain independently bindable and documentable.
    *   Footer/help/F1 continue to consume only the resolved current bindings/tokens/labels; they must not special-case locale names or read packaged preset files directly.
    *   Section ownership is by stable runtime command-surface ID rather than by language or storage back-end name alone. Current canonical surfaces must cover at least directory/file and archive-directory/archive-file variants; future surfaces may add new stable IDs without changing the row grammar.
*   **Dependency Contract:**
    *   Sequence after Task 11.2 establishes the action/label/token split.
    *   Prefer to land before final F10 hub polish so `F10` can expose the final commands-surface model rather than a transient one.
    *   Task 41.2 and Task 44 consume the resolved active command state only; they should not need separate locale-specific layout logic once this task lands.
*   **Acceptance Criteria:**
*   Shipped command presets exist as separate packaged source files keyed by stable preset ID.
*   `commands.conf` remains the single canonical user-editable command surface and can select zero or one preset plus local overrides.
*   No user-editable surface requires multilingual `[english]` / `[deutsch]` block toggling to switch locale/layout behavior.
*   Packagers can ship a localized default preset choice without forking command-dispatch code, and users can override that choice later through `commands.conf` / `F10`.
*   Validation catches collisions and unresolved actions after preset + override resolution.
*   Preset files carry concise comment headers explaining their role and constraints.
*   `docs/SPECIFICATION.md`, `docs/ARCHITECTURE.md`, and F10/help docs describe command presets as read-only packaged data layered under one active commands file.
*   - [x] **Status:** Completed.

---

## **Phase 4: UI/UX Enhancements and Cleanup**
*This phase adds user-facing improvements, cleans up the remaining artifacts, and ensures a clean, modern, and portable codebase.*

### **Immediate Quick Wins**

### **Task 12: Footer Action Parity in Archive Mode (`Pipe`)**
*   **Goal:** Make archive-mode footer keybinding/F1 lines accurately reflect runtime-available actions, starting with `Pipe`.
*   **Rationale:** Footer/help is the primary discoverability surface; available actions must not be hidden.
*   **Scope Lock:** No command semantics or keybinding behavior changes; visibility/alignment only.
*   **Acceptance Criteria:**
*   Archive footer keybinding hints and F1 help show `Pipe` whenever it is available in that context.
*   Actions unavailable in archive mode remain absent from archive footer keybinding hints and F1 help.
*   A focused regression test (or existing footer keybinding/F1 test extension) verifies archive footer/action parity.
*   - [x] **Status:** Completed.

### **Task 13: Archive Virtual Filesystem Parity**
*   **Goal:** Make archives behave as filesystem-like volumes according to their runtime-probed operation capabilities.
*   **Scope:** Add recursive directory `Copy`, `PathCopy`, and `Move`; expose per-operation archive capabilities in the stats line and help; keep footer mutation hints truthful; and enforce canonical internal-path, collision, and subtree/self-target protections.
*   **Capability Contract:** Determine browse, extract/copy-out, add/copy-in, delete, rename, and move availability from the opened archive and installed libarchive support. Do not infer write support from an extension. Read-only archives remain browsable and support available copy-out actions, while unavailable mutations are omitted from the footer and reject clearly if invoked.
*   **Move Contract:** Intra-archive moves rewrite canonical member paths. Cross-archive moves rewrite and replace the destination before altering the source. If source-side removal then fails, retain duplicate data rather than lose data; cross-archive moves are not globally atomic.
*   **Directory Contract:** Directory operations are recursive without a confirmation prompt and place the selected directory beneath the destination using its basename. Reject destinations inside the selected subtree.
*   **Help Contract:** List common expected writable formats such as `.tar`, `.tar.gz`, `.tar.bz2`, `.tar.xz`, and `.zip`, while stating that actual availability depends on the installed libarchive and archive properties.
*   **Acceptance Criteria:**
*   Archive directory `Copy`, `PathCopy`, and `Move` work across archive and filesystem destinations according to available operation capabilities.
*   Capability UI and contextual help never promise unavailable mutations.
*   Canonical-path collision, traversal, and self-target cases are rejected safely.
*   Cross-archive move failure preserves the source until destination success and never loses data after a source-side failure.
*   - [ ] **Status:** In Progress.

### **Task 14: Path Message Formatting Audit (`//` Artifact Prevention)**
*   **Goal:** Audit user-facing message/path rendering and eliminate accidental double-slash artifacts in status/error/footer output.
*   **Rationale:** Message correctness is a trust surface; inconsistent path rendering invites avoidable bug reports and operator confusion.
*   **Scope Lock:** Message/path formatting and tests only; no navigation, keybinding, or filesystem behavior changes.
*   **Acceptance Criteria:**
*   Inventory and review path-formatting callsites used by user-visible message surfaces.
*   Route display-path formatting through a shared canonical formatter/contract (or equivalent centralized policy).
*   Add focused regression tests covering root paths, trailing-slash joins, archive/display paths, and no accidental `//` join artifacts.
*   Preserve valid POSIX-leading `//` semantics where intentional; do not blanket-collapse legitimate leading doubles.
*   - [x] **Status:** Completed.

### **Task 15: Copy Include-Paths Base/Result Preview Contract (Predictable Root Semantics)**
*   **Goal:** Make `Copy` with `Preserve ancestor paths` explicit and predictable by showing a compact computed preview of base root, relative segment, and resulting destination path.
*   **Rationale:** Users cannot infer include-path base semantics from UI alone, which makes destination depth feel arbitrary and increases wrong-target risk.
*   **Scope Lock:** Prompt/help/docs and regression coverage only; do not change underlying copy/sync semantics in this task.
*   **Acceptance Criteria:**
*   During `Copy` destination input when `Preserve ancestor paths` is enabled, show one compact computed line in the same flow (no extra submenu): `Base:<...>  Rel:<...>  -> <...>`.
*   The displayed base is the active volume root used by runtime path computation.
*   In the same prompt, `[` includes one more parent segment in the relative path (longer path) and `]` removes one parent segment (shorter path); preview updates immediately on each keypress.
*   `F2` destination-directory picker behavior remains unchanged.
*   Rendering stays concise and non-repetitive: single-line summary with deterministic clipping (middle truncation) when width is constrained.
*   Add focused regression tests for preview correctness and resulting destination path in representative filesystem scenarios (including nested roots and absolute destination input).
*   Update `docs/SPECIFICATION.md`, `etc/ytnova.1.md`, generated `docs/USAGE.md`, and F1/context help text so include-path root/relative/result contract and `[`/`]` controls are explicit and consistent.
 *   - [x] **Status:** Complete.

### **Task 16: Proactive Missing-Destination Directory Creation Prompt**
*   **Goal:** When a destination directory is missing in destination-driven workflows, detect it before execution and offer an explicit one-step create confirmation.
*   **Rationale:** Prevents avoidable late failures, reduces wrong-target mistakes from typos, and improves alpha-readiness of copy/move-style flows.
*   **Scope Lock:** Destination validation and confirmation behavior only; no command semantic/keybinding changes.
*   **Acceptance Criteria:**
*   In `Copy` and `Move` destination flows, if the resolved destination directory does not exist, show a single explicit prompt with full target path and default-safe choice: `Create missing directory? (y/N)`.
*   Choosing `y` creates the missing directory path deterministically before the operation continues.
*   Choosing `N`/`Esc` leaves filesystem state unchanged and returns control to destination input flow.
*   On creation failure (permissions/path errors), show a precise actionable error and do not continue the mutation command.
*   Add focused regression coverage for `yes`, `no/cancel`, and failure-path behavior.
*   Update `etc/ytnova.1.md` and regenerate `docs/USAGE.md` (`make docs`) when behavior lands.
*   - [x] **Status:** Completed.

### **Task 17: Add Inline `Shift+N` Create-Link Flow (Symlink/Hardlink)**
*   **Goal:** Add an in-app link creation command that mirrors existing `mkdir/newfile/copy` prompt ergonomics without requiring external `X` shell execution.
*   **Rationale:** Link creation is a core file-manager workflow; requiring shell fallback breaks interaction consistency and discoverability.
*   **Scope Lock:** Filesystem link creation UX/behavior only (`symlink` and `hardlink`); no unrelated command flow redesign.
*   **Acceptance Criteria:**
*   Add one primary keybinding: `Shift+N` (`N`) for `Create Link`.
*   The command is available in both directory and file contexts where filesystem mutations are valid, including showall/global file flows.
*   Flow is single-surface (no pre-step menu): first prompt is the link-target input and footer exposes live type toggle (`s`/`h`) with a default already set.
*   Prompt/header contract is explicit and concise (for example: `CREATE LINK [s=symlink h=hard] TARGET:`), and a second prompt captures `LINK NAME:`.
*   Default link type is `symlink`; pressing `s`/`h` in the first prompt switches mode inline without leaving the prompt.
*   Target prefill follows existing copy/newfile conventions for active selection context and remains overrideable by direct typing.
*   Destination resolution in showall/global targets the owner directory of the highlighted entry (not unrelated tree cursor state).
*   Existing `n`/`N` newfile behavior is remapped to preserve intuitive command grouping while keeping help/footer truthfully synchronized.
*   Add focused regression coverage for: symlink create, hardlink create, cancel/no-op behavior, showall/global owner-directory resolution, split-panel isolation, and error-path messaging.
*   Update `etc/ytnova.1.md` and regenerate `docs/USAGE.md` (`make docs`) when behavior lands.
*   - [ ] **Status:** Not Started.

### **Task 18: F7 Top Path Line Must Preserve Full `filename.ext`**
*   **Goal:** In F7 preview mode, the top line above the directory window must display file context as `path + filename.ext` for the selected file.
*   **Rationale:** In preview workflows, the selected file identity must remain explicit and unambiguous.
*   **Scope Lock:** F7 top-line rendering contract only; no preview navigation/keybinding changes in this task.
*   **Acceptance Criteria:**
*   F7 top line includes path context and the selected file name with extension.
*   When width is insufficient, truncate middle of path segment; keep full selected `filename.ext` visible.
*   The same identity-preservation rule applies in filesystem and archive preview contexts.
*   Add focused regression tests for F7 top-line truncation/identity behavior.
*   Update `etc/ytnova.1.md` and regenerate `docs/USAGE.md` (`make docs`) when behavior lands.
*   - [x] **Status:** Complete.

### **Task 19: Manual File-Column Width Controls (`[` Narrower, `]` Wider, `{` / `}` Reset)**
*   **Goal:** Add explicit keyboard controls for file-list column width so users can quickly trade density vs readability in the file window.
*   **Rationale:** Long-name workflows need fast, deterministic control over visible filename identity without terminal resize churn.
*   **Scope Lock:** File-window list column width controls only; no F7 split-preview width redesign in this task.
*   **Acceptance Criteria:**
*   `[` decreases file-column width in fixed-width list layouts.
*   `]` increases file-column width in fixed-width list layouts.
*   `{` / `}` resets to default auto-layout behavior.
*   Behavior is deterministic and static (no marquee/auto-scrolling text).
*   Footer keybinding hints and F1 help document these keys in file contexts where they apply.
*   Add focused regression coverage for width adjust left/right/reset behavior and bounds handling.
*   - [ ] **Status:** Not Started.

### **Task 20: Adjustable List/Preview Width in `F7` Mode**
*   **Goal:** Allow users to adjust the relative width of file-list and preview panes while in `F7` preview mode.
*   **Rationale:** Different file types and terminal sizes benefit from quick width tuning during inspect workflows.
*   **Scope Lock:** `F7` pane-width behavior only; no split-mode (`F8`) layout redesign.
*   **Acceptance Criteria:**
*   Provide portable primary resize keys in `F7` (`[` narrower list, `]` wider list, `0` reset default split).
*   Divider movement direction is explicit and intuitive: `[` always reduces file-list width and `]` always increases file-list width, regardless of which border visually moves.
*   Width changes preserve current file selection and preview scroll context.
*   Behavior is deterministic and static (no marquee/auto-scrolling text).
*   Footer keybinding hints, F1 help, and config docs are updated when behavior lands.
*   - [ ] **Status:** Not Started.

### **Task 21: Progress Indicators for Copy/Move/Delete/Archive Workflows**
*   **Goal:** Add consistent progress feedback for long-running mutation workflows (`Copy`, `Move`, `Delete`, archive create/extract/rewrite).
*   **Rationale:** Users need immediate confidence that work is active and not hung, especially during large operations.
*   **Scope Lock:** Progress signaling and UI/status messaging only; no changes to command semantics, confirmation policies, or keybindings.
*   **Acceptance Criteria:**
*   For measurable work totals, show progress bar + percent (and ETA where stable) for copy/move/delete/archive operations.
*   When total work is initially indeterminate, show spinner by default; transition to bar/percent/ETA only if total becomes measurable.
*   Progress rendering must not overwrite footer keybinding/prompt/F1 help surfaces; on constrained layouts, degrade to a compact indicator while preserving help readability.
*   Behavior follows the specification conventions for informative motion and static/non-decorative UI.
*   Footer/F1/manpage wording is updated where needed so behavior is discoverable and consistent.
*   Add focused regression coverage for progress-state selection (indeterminate vs measurable) and completion/error transitions.
*   - [ ] **Status:** Not Started.

#### **Task 21.1: Keep Progress Indicators from Clobbering Footer/Prompt/F1 Guidance**
*   **Goal:** Preserve footer, prompt, and `F1` help ownership while long-running operations update progress/spinner state.
*   **Rationale:** Help/trust regressions are not limited to static wording; progress rendering that overwrites guidance surfaces creates the same "UI is lying to me" failure mode during active work.
*   **Related Bug:** `BUG-11` — progress spinner can overwrite footer keybinding/prompt/F1 surfaces.
*   **Acceptance Criteria:**
*   Progress updates render in a dedicated non-obtrusive status surface and never overwrite active footer/prompt/F1 guidance.
*   When `F1` help is open, progress state degrades gracefully to a compact indicator or deferred repaint rather than seizing the help surface.
*   Focused regression coverage proves long-running operations cannot blank or corrupt contextual guidance surfaces.
*   - [ ] **Status:** Not Started.

### **Task 22: Redraw Coherence**
*   **Goal:** Ensure all related redraw-synchronization work ships under one coherent umbrella with deterministic scope boundaries.

#### **Task 22.1: Unify Stats + Main-Pane Frame Redraw Contract**
*   **Goal:** Eliminate intermittent split-brain rendering where stats and main panes update on different redraw lifecycles.
*   **Rationale:** UI trust depends on one coherent frame; partial redraw divergence creates stale/corrupted mixed states.
*   **Scope Lock:** Rendering/invalidation pipeline and regression coverage only; no command/keybinding semantics changes.
*   **Acceptance Criteria:**
*   Stats, path, dir, and file surfaces are drawn from one frame/layout snapshot and flushed in one update cycle.
*   Resize, mode-switch, and recoverable-error paths trigger deterministic full-surface invalidation and redraw.
*   No persistent mixed state where stats is fresh while main panes are stale (or vice versa) after redraw-triggering actions.
*   Add focused regression coverage for redraw coherence across resize/mode toggles and representative recovery paths.
*   - [ ] **Status:** Not Started.

#### **Task 22.2: Footer-Aware Redraw Synchronization Contract**
*   **Goal:** Footer/help/prompt surfaces must participate in the same redraw contract as stats/path/dir/file panes.
*   **Rationale:** Partial redraw of guidance surfaces creates trust loss even when content panes are correct.
*   **Scope Lock:** Redraw ordering and invalidation only; no keybinding or command behavior changes.
*   **Acceptance Criteria:**
*   Footer/help/prompt are rendered from the same frame snapshot as content panes.
*   Resize and mode transitions must not leave footer keybinding/F1 surfaces stale relative to the active context.
*   Focused regression coverage proves synchronized redraw across normal, split, and overlay transitions.
*   - [ ] **Status:** Not Started.

#### **Task 22.3: Unify Main-Screen Frame and Junction Ownership**
*   **Goal:** Replace patched line-drawing fixes with one architecturally clean main-screen frame-rendering contract.
*   **Rationale:** The current defect family is not just "wrong glyph here or there"; it comes from fragmented border ownership across layout code, stats rendering, preview-family rendering, and transition-time redraw helpers. As long as multiple paths can write the same seam cells, missing or overwritten junctions will keep returning in new layout combinations.
*   **Relationship to Task 22.1:** Task 22.1 aligns redraw timing; this subtask aligns frame ownership so the synchronized redraw has one authoritative border/junction source.
*   **Ownership Rule (mandatory):** The frame compositor owns every outer-border cell, divider cell, split-separator cell, stats-touching border cell, and every junction-bearing seam cell. Non-frame renderers may draw interior content and renderer-local internal separators only; they must not paint shared frame/seam cells.
*   **Scope Lock:** Main-screen frame composition, seam ownership, and regression coverage only; no keybinding, command-surface, or theme-design changes.
*   **Acceptance Criteria:**
*   One authoritative render owner chooses all main-screen frame glyphs, including the outer box, dir/file divider, split separator, stats-touching borders, preview-family frame seams, and all top/middle/bottom junctions.
*   No non-frame renderer paints shared frame/seam cells.
*   The Task 22.3.x subtasks land without introducing keybinding, command-surface, or theme-design drift.
*   - [ ] **Status:** Not Started.

##### **Task 22.3.1: Canonicalize Main-Screen Geometry**
*   **Goal:** Define one authoritative geometry model for the outer frame, dir/file divider, split separator, stats column boundary, preview-family border seams, and every junction-bearing seam cell before glyph selection occurs.
*   **Acceptance Criteria:** All main-screen border-bearing cells are derived from one shared layout model rather than recomputed independently by multiple render paths.
*   - [ ] **Status:** Not Started.

##### **Task 22.3.2: Introduce a Unified Frame Compositor / Junction Resolver**
*   **Goal:** Choose main-screen border glyphs from declarative edge connectivity instead of scattered imperative `ACS_*` writes.
*   **Mechanism:** Add one frame-composition path that resolves top/middle/bottom junctions, corners, and straight runs from the canonical geometry model and applies unchanged across single, split, and preview-family layouts.
*   - [ ] **Status:** Not Started.

##### **Task 22.3.3: Remove Shared Seam Ownership from Non-Frame Renderers**
*   **Goal:** Restrict stats and other non-frame renderers to interior content and renderer-local internal separators so frame-touching seam cells have one owner.
*   **Acceptance Criteria:** No shared seam cell is written by both the main layout/frame path and any non-frame renderer, including stats and preview-family content renderers.
*   - [ ] **Status:** Not Started.

##### **Task 22.3.4: Remove Transition-Time Border Fragment Repaints**
*   **Goal:** Eliminate mode/layout helpers that repaint border fragments directly instead of requesting a full recomposition from the frame owner.
*   **Acceptance Criteria:** Mode/layout transitions do not paint ad-hoc border fragments outside the unified frame-render path.
*   - [ ] **Status:** Not Started.

##### **Task 22.3.5: Add Seam-Family Regression Coverage and Final Ownership Documentation**
*   **Goal:** Prove the new ownership model across the full seam family and document the final architectural boundary.
*   **Acceptance Criteria:**
*   Focused regression coverage proves seam correctness across left-only/right-only/both/none stats combinations and representative small/large terminal geometries.
*   Single, split, and preview-family layouts use the same junction-resolution mechanism.
*   `docs/ARCHITECTURE.md` documents the final ownership boundary: frame composition owns shared border/junction cells, while non-frame renderers own interior content and renderer-local internal separators only.
*   - [ ] **Status:** Not Started.

### **Task 23: Clarify Internal `^V` Navigation for File vs Hit Traversal**
*   **Goal:** Make internal `View Tagged` (`^V`) navigation unambiguous by separating file-to-file movement from hit-to-hit movement.
*   **Rationale:** Current flow is easy to misinterpret (`Space` paging, `S` sort, and `^S` tagged search/filter context), which increases user friction during review workflows.
*   **Scope Lock:** Internal `^V` viewer behavior/help only; do not change tagged-filter semantics in file/archive list mode.
*   **Acceptance Criteria:**
*   Keep `n/p` for next/previous file.
*   Keep `Space` and page keys as page movement only.
*   Add hit navigation within current file set as `/` next-hit and `?` previous-hit.
*   In `TAGGEDVIEWER=external` mode, hit traversal remains pager-native (for example `less` keys) and is not remapped by ytnova.
*   Footer keybinding hints and F1 help in internal `^V` mode explicitly show file-nav keys and hit-nav keys.
*   `^S` remains the tagged-list search/filter action outside viewer mode and is documented distinctly.
*   Add focused regression coverage for key behavior and help discoverability in this mode.
*   - [x] **Status:** Complete.

### **Task 24: Replace `Write` with an Explicit `Output` / Hardcopy Flow**
*   **Goal:** Make file-output and hardcopy behavior immediately understandable by repurposing `O` as an explicit `Output` action.
*   **Rationale:** `Write` reads like in-place save/edit, while the current destination chooser hides the core distinction users care about: file output versus hardcopy.
*   **Scope Lock:** Keep the interaction shallow: no more than one chooser before the final destination prompt.
*   **Acceptance Criteria:**
*   The command strip, footer, F1 help, prompts, manpage, and generated usage docs relabel the current output/export action from `Write` to `Output`.
*   `O` becomes `Output`; user-facing wording for this feature uses `Output`, `File`, and `Hardcopy` rather than the misleading `Write` label.
*   The common path becomes `O -> Output to: File / Hardcopy -> destination prompt -> Enter`.
*   `Filter` keeps `*` as the established default meaning "all files" / no filter; blank input does not replace that contract.
*   The default filter prompt remains `FILTER: *`.
*   Pressing `Tab` inside the filter prompt toggles the scope between all files and tagged-only without opening another submenu.
*   The tagged-only state reuses the same prompt surface as `FILTER [tagged only]: *`.
*   File output still begins with the existing format chooser: `Format: Raw, Framed, Page break  Esc cancel`.
*   File output uses explicit prompts such as `Output file:` and defaults plain filename/path input to file output.
*   Hardcopy uses an explicit prompt such as `Printer command:` with examples/history suited to printer workflows.
*   Top-level `Output` wording does not reintroduce a competing generic `Command` destination label when `Pipe` and `eXecute` already own that mental model.
*   Expert shortcuts remain accepted where safe (for example `>path` or command aliases), but they are optional rather than required for discovery.
*   Runtime UI exposes only context-valid `Output` options in each context.
*   `Framed` and `Page break` prompts/options remain distinct and truthful; they must not reuse misleading wording.
*   Regression tests verify option visibility/behavior parity across at least filesystem + archive contexts.
*   Regression tests verify destination semantics (plain filename file-output default, hardcopy prompt behavior, and no-crash error paths).
*   F1/help text, footer labels, prompt text, and runtime behavior stay synchronized with the same contract.
*   `docs/SPECIFICATION.md`, `etc/ytnova.1.md`, and generated `docs/USAGE.md` are updated in the same delivery so docs match runtime behavior.
*   No crash on printer-command failure or destination-open failure.
*   - [x] **Status:** Completed.

### **Task 26: Add `Catalog` Output Mode to `Write`**
*   **Goal:** Extend the existing `Write` format dialog with a `Catalog` mode that exports a deterministic file/directory inventory (similar intent to `ls -1pR`) instead of file contents.
*   **Rationale:** Users need an in-app way to generate list/report output to command or file without dropping to shell-specific workflows.
*   **Scope Lock:** Add format behavior only; do not define or change keybindings in this task.
*   **Acceptance Criteria:**
*   `Write` prompt includes `Catalog` alongside existing formats.
*   Catalog output can be sent to command or file via existing `Write` destination flow.
*   Output contract is documented (recursion rules, directory markers, ordering, archive behavior).
*   Focused regression tests cover at least one filesystem case and one archive case.
*   - [ ] **Status:** Not Started.

### **Task 27: Audit and Collapse Unnecessary Prompt Bureaucracy**
*   **Goal:** Find and remediate prompts that add routine bureaucracy without adding meaning, while preserving prompts that capture genuinely separate user decisions or real safety exceptions.
*   **Rationale:** Prompt friction is not limited to submenu depth. Repeated approvals, redundant mode choosers, and batch-operation follow-up questions can still turn a nominally shallow flow into tedious bureaucracy even when the user already expressed clear intent.
*   **Scope Lock:** This task family covers prompt necessity, prompt sequencing, and prompt-compression correctness across runtime workflows. It complements Task 28's primary-action depth and surface audit rather than replacing it. Do not remove prompts that capture distinct meanings, independent data types, or true safety confirmations.
*   **Source-of-Truth Rule:** The prompt-bureaucracy rules belong in `docs/SPECIFICATION.md` alongside the Task 28 shallow-flow contract. This roadmap item must reconcile against that spec and add any missing prompt-necessity rules there instead of carrying a duplicate local contract in the roadmap.
*   **Delivery Model:** Complete through subtasks `26.1+`. `26.1` audits prompt bureaucracy offenders, `26.2` reconciles the complementary spec contract, and `26.3+` remediates one coherent offender family per subtask.
*   **Umbrella Acceptance Criteria:**
*   Audit prompts across filesystem, archive, split, `F7`, `F8`, `Showall`, `Global`, tagged workflows, chooser flows, overwrite flows, delete flows, and other multi-step command paths that can accumulate routine prompt friction.
*   Classify each prompt in audited flows as one of:
*   required input,
*   separate user decision that must remain explicit,
*   real safety confirmation,
*   prompt-local aid,
*   unnecessary bureaucracy.
*   Record offender families where runtime asks for approval, policy, or mode selection after the user's intent is already clear and no new meaning or safety boundary has been introduced.
*   For each offender family, define the collapsed common path and explicitly state which prompts remain necessary and why.
*   Reconcile the audit against `docs/SPECIFICATION.md` so the final rule set complements Task 28: compress bureaucracy, but do not merge distinct meanings into one opaque prompt.
*   Create remediation subtasks for every audited bureaucracy family and close this umbrella only after each inventoried family is marked addressed, intentionally unchanged with reason, or deferred/blocked with a concrete reason.
*   - [x] **Status:** Completed.

#### **Task 27.1: Audit and Rank Prompt Bureaucracy Offenders**
*   **Goal:** Build a complete inventory of prompts that are unnecessary, repetitive, or wrongly sequenced, and rank them by workflow cost and frequency.
*   **Scope Lock:** Audit, classification, checklist output, remediation planning, and tracker/spec reconciliation only; no runtime behavior changes in this subtask.
*   **Acceptance Criteria:**
*   Produce a prompt-bureaucracy matrix for the full Task 27 coverage set.
*   For each audited flow, record:
*   keybinding / entry path,
*   current prompt chain,
*   which prompts are required input versus true safety confirmations,
*   which prompts are separate user decisions that must remain explicit,
*   which prompts are unnecessary bureaucracy and why,
*   manual repro keys,
*   likely owner files/modules,
*   likely tests/docs to update.
*   Rank offender families by user cost, repetition frequency, and whether the extra prompt appears on the common path.
*   Identify coherent remediation-family boundaries so follow-up subtasks can land by owner/risk/validation surface rather than one prompt at a time.
*   - [x] **Status:** Completed.

#### **Task 27.2: Reconcile Prompt-Necessity Contract Against Spec**
*   **Goal:** Define the complementary prompt-bureaucracy rules in `docs/SPECIFICATION.md` so they sit alongside, and do not conflict with, the Task 28 shallow-flow contract.
*   **Scope Lock:** Spec reconciliation only; do not duplicate long-form rules in the roadmap and do not change runtime behavior in this subtask.
*   **Acceptance Criteria:**
*   Verify that `docs/SPECIFICATION.md` explicitly defines:
*   what makes a prompt required versus bureaucratic,
*   what counts as a separate user decision that must remain explicit,
*   which safety confirmations are legitimate,
*   when batch-mode follow-up prompts are redundant,
*   when prompt compression would hide meaning instead of removing bureaucracy.
*   Verify that the final spec wording complements Task 28 rather than duplicating it: Task 28 governs interactive depth and surface correctness; Task 27 governs whether the prompts in that flow are necessary at all.
*   If any required rule is missing or ambiguous, update `docs/SPECIFICATION.md` first and keep the roadmap text concise by reference rather than restating the full contract here.
*   - [x] **Status:** Completed.

#### **Task 27.3: Remediate Audited Prompt Bureaucracy Families**
*   **Goal:** Remove unnecessary prompts from each audited offender family while preserving distinct decisions and real safety checks.
*   **Scope Lock:** One coherent bureaucracy family per subtask; do not mix unrelated command families with different owner boundaries or validation paths.
*   **Acceptance Criteria (applies to each remediation subtask):**
*   The remediated flow removes routine prompt bureaucracy from the common path.
*   Distinct user decisions remain explicit when collapsing them would hide meaning instead of removing friction.
*   Real safety confirmations remain in place where needed, and only there.
*   Prompt/help/spec surfaces stay synchronized with the corrected flow.
*   Focused regression coverage prevents the offender family from regressing into repeated or unnecessary prompts.
*   - [x] **Status:** Completed.

#### **Task 27.3.1: Preserve the Already-Satisfied Prompt-Bureaucracy Fixes**
*   **Goal:** Keep the already-landed prompt-bureaucracy fixes for the first six audited families explicit and verifiable in one place.
*   **Scope Lock:** Roadmap reconciliation only for the already-satisfied families; do not reopen or renumber them into separate active subtasks.
*   **Acceptance Criteria:**
*   Filter keeps tagged-only scope on the live prompt instead of a second chooser.
*   Compare keeps target entry on one live prompt without an extra chooser.
*   Attribute date edits go straight from the chooser to the value prompt.
*   Copy/move/pathcopy keep the explicit `name -> destination` exception with only real safety confirmations afterward.
*   Tagged delete keeps one batch confirmation without a routine `confirm each file` policy prompt.
*   Output/export no longer uses a redundant standalone format chooser.
*   - [x] **Status:** Completed.

#### **Task 27.3.2: Collapse Tagged Overwrite Policy Prompt Bureaucracy**
*   **Goal:** Remove the pre-conflict overwrite policy prompt from tagged copy/move flows while preserving concrete overwrite safety prompts.
*   **Scope Lock:** Tagged copy/move overwrite prompting, conflict-policy help/spec wording, and related tests only.
*   **Acceptance Criteria:**
*   Tagged copy/move no longer asks `Ask for confirmation for each overwrite (Y/N)?` before showing the first concrete overwrite conflict.
*   The first overwrite conflict prompt still shows source/target context and keeps explicit conflict-resolution choices.
*   Focused overwrite regressions prove that repeated conflicts can still be applied safely across the remaining tagged set.
*   - [x] **Status:** Completed.

### **Task 28: Enforce One-Level Primary Action Depth (Prompt-Chain Audit)**
*   **Goal:** Audit and remediate primary interactive workflows so the common path stays `key -> Enter -> result` with at most one submenu/prompt layer.
*   **Rationale:** Deep prompt chains, misleading prompt surfaces, and context-mismatched command visibility increase friction, hide capability, and slow high-frequency workflows.
*   **Scope Lock:** This task family covers interaction depth, prompt/menu composition, context-surface command visibility, and prompt-surface correctness only; it does not change command semantics.
*   **Source-of-Truth Rule:** Shared shallow-flow rules, prompt-label/mnemonic normalization, prompt-surface visibility rules, and counting rules belong in `docs/SPECIFICATION.md`. This roadmap item must audit against that source of truth and update the spec first if any required rule is missing, stale, or ambiguous.
*   **Delivery Model:** Complete through subtasks `27.1+`. `27.1` inventories and ranks offenders, `27.2` reconciles and finalizes the governing spec contract, and `27.3+` remediate one audited offender family per subtask.
*   **Umbrella Acceptance Criteria:**
*   Inventory primary action flows across the full required coverage set: filesystem, archive, split, `F7`, `F8`, `Showall`, `Global`, tagged workflows, and active picker/prompt/dialog surfaces such as history, volumes, applications, compare prompts, and syntax-bearing command prompts.
*   Produce a complete `keybinding -> flow` audit for all keybindings that open submenus, prompts, pickers, modal choosers, overlays, or other interactive surfaces in that coverage set.
*   For each audited surface, record current chain steps, common-path step count, submenu depth, active context, return path, and manual repro keys.
*   Audit prompt-surface correctness in the same pass, including:
*   usable commands that are available in the active surface but not shown there,
*   commands shown in the active surface that are not actually usable there,
*   prompt/menu labels or mnemonics that violate the documented prompt-label contract.
*   Deliver the audit output as a manual QA checklist so every flagged surface can be exercised directly.
*   Reconcile the audited flows against `docs/SPECIFICATION.md` and update the spec only where the governing rule is missing, stale, or ambiguous.
*   Create remediation subtasks for every audited prompt-chain offender family and close this umbrella only after each inventoried family is marked addressed, intentionally unchanged with reason, or deferred/blocked with a concrete reason.
*   - [x] **Status:** Completed.

#### **Task 28.1: Audit and Rank Primary Action Flows**
*   **Goal:** Build the zero-based inventory of all primary interactive flows and rank prompt-chain offenders by depth, frequency, and operator cost.
*   **Scope Lock:** Audit, classification, checklist output, remediation planning, and tracker/spec reconciliation only; no runtime behavior changes in this subtask.
*   **Acceptance Criteria:**
*   Produce the full `keybinding -> flow` matrix for the complete Task 28 coverage set.
*   Record current chain, common-path decision count, submenu depth, visible command surface, hidden-but-usable commands, shown-but-unusable commands, return path, and manual repro keys for each audited flow.
*   Rank offender families by user impact and depth severity.
*   For each offender family, produce a remediation-ready compression plan that records:
*   current chain,
*   proposed compressed chain,
*   current vs proposed submenu depth,
*   equivalent fast path or justified exception if needed,
*   likely owner files/modules,
*   likely tests/docs to update,
*   focused validation path.
*   Identify family boundaries for remediation so follow-up subtasks can be delivered by coherent owner/risk/validation surface rather than prompt-by-prompt micro-slices.
*   - [x] **Status:** Completed.

#### **Task 28.2: Reconcile Shallow-Flow Contract Against Spec**
*   **Goal:** Confirm that the governing shallow-flow, prompt-surface, mnemonic, and counting rules are fully and correctly defined in `docs/SPECIFICATION.md` before remediation subtasks land.
*   **Scope Lock:** Spec reconciliation only; do not duplicate long-form design rules in the roadmap and do not change runtime behavior in this subtask.
*   **Acceptance Criteria:**
*   Verify that `docs/SPECIFICATION.md` explicitly defines:
*   what counts as a primary action,
*   how common-path steps are counted,
*   how submenu/prompt depth is counted,
*   whether prompt-local aids such as `F1`, `F2`, history, browse, and completion count toward depth,
*   which destructive or safety-critical confirmations are valid exceptions,
*   what qualifies as an equivalent fast path.
*   Verify that `docs/SPECIFICATION.md` also explicitly covers prompt-label/mnemonic normalization and prompt-surface command-visibility correctness.
*   If any required rule is missing or ambiguous, update `docs/SPECIFICATION.md` first and keep the roadmap text concise by reference rather than restating the full contract here.
*   - [x] **Status:** Completed.

#### **Task 28.3: Remediate Audited Offender Families**
*   **Goal:** Reduce each audited offender family to the documented shallow-flow budget or document a justified exception with an equivalent fast path.
*   **Scope Lock:** One coherent offender family per subtask; do not mix unrelated families with different owner boundaries or validation paths.
*   **Acceptance Criteria (applies to each remediation subtask):**
*   Compression removes bureaucracy, not meaning: do not collapse distinct user decisions into a merged prompt when that would make the flow less intuitive than the explicit version.
*   The remediated family meets the documented common-path depth rule, or the subtask documents why a deeper branch is unavoidable and provides an equivalent fast path.
*   Routine successful operations in the remediated family return directly to the working view; any useful completion summary is non-modal.
*   Prompt/menu surfaces for that family show the commands that are actually usable there and do not advertise commands that are unavailable in that surface.
*   Labels and mnemonics for that family conform to the prompt-label contract in `docs/SPECIFICATION.md`.
*   Focused regression coverage prevents the remediated family from regressing into deeper prompt chains or prompt-surface mismatches.
*   - [x] **Status:** Completed.

#### **Task 28.3.1: Compress Compare Prompt Chains**
*   **Goal:** Reduce compare workflows to the shallowest safe common path without changing compare semantics.
*   **Scope Lock:** Compare chooser/prompt composition, prompt visibility, and compare help updates only.
*   **Acceptance Criteria:**
*   Directory compare no longer requires more than one chooser layer before the target prompt on the common path.
*   External compare does not add an avoidable extra chooser layer on top of the compare entry path.
*   Compare target, basis, and tagged-result choices remain explicit, discoverable, and help-synchronized.
*   Successful compare runs do not stop on a blocking completion dialog; any completion summary stays non-modal.
*   - [x] **Status:** Completed.

#### **Task 28.3.2: Compress Attribute Editing Prompt Chains**
*   **Goal:** Reduce attribute-edit workflows, especially date edits, to the shallowest safe prompt sequence.
*   **Scope Lock:** Attribute chooser composition, date-scope selection, prompt help, and related tests/docs only.
*   **Acceptance Criteria:**
*   Attribute edits require at most one chooser layer before the value-entry prompt on the common path.
*   Syntax-bearing date edits advertise their format help on the active prompt surface.
*   Tagged/date variants remain explicit without reintroducing deeper menu chains.
*   - [x] **Status:** Completed.

#### **Task 28.3.3: Normalize Prompt-Aid Visibility, Chooser Labels, and F2 View-State Controls**
*   **Goal:** Make prompt-local aids and chooser commands visible, truthfully labeled, and behaviorally consistent with the active view state.
*   **Scope Lock:** History, F2 picker, volume menu, applications menu, and other chooser/prompt command-surface correctness, including F2 dotfile visibility behavior only.
*   **Acceptance Criteria:**
*   Prompt and chooser surfaces advertise every supported local action, including help, history, browse, and dotfile toggles where available.
*   Chooser/menu labels match the actual command behavior on the active surface.
*   Chooser-style footer strips follow the canonical low-noise order: `F1 help` first, truthful local actions in task-flow order, `Esc cancel` last, and generic list-navigation hints omitted unless they explain a distinctive local rule.
*   Standalone modal choosers get box lines. Embedded transient mini-pickers do not.
*   The applications chooser exposes its edit action on-screen and honors the standard list-navigation keys (`Home`, `End`, `PgUp`, `PgDn`) in addition to arrow navigation.
*   F2 browse/tree views inherit the current dotfile visibility state from the invoking view.
*   In F2 browse/tree views, the backtick command toggles dotfile visibility and the on-screen command strip advertises it.
*   Focused regression coverage prevents prompt-aid mismatches and F2 view-state drift.
*   - [x] **Status:** Completed.

#### **Task 28.3.4: Reconcile Multi-Input Target Prompt Families**
*   **Goal:** Document and preserve copy/move/pathcopy as the explicit multi-input exception: name first, destination second, then only real safety confirmations.
*   **Scope Lock:** Copy, move, pathcopy, rename-pattern, destination, and related confirmation flows only.
*   **Acceptance Criteria:**
*   Copy, move, and pathcopy keep the two-step `name -> destination` flow as an explicit exception to generic prompt-compression work.
*   The exception is justified in plain language: replacement name/pattern and destination directory are separate user decisions, so merging them would hide meaning instead of removing bureaucracy.
*   Only real safety confirmations remain after those two prompts, such as overwrite/replace conflicts or creating a missing destination directory.
*   Spec, help text, and regression tests make that exception explicit.
*   - [x] **Status:** Completed.

#### **Task 28.3.5: Compress Output Export Prompt Chains**
*   **Goal:** Reduce output/export prompting to the shallowest safe flow while removing redundant chooser layers without making output/export less intuitive.
*   **Scope Lock:** Output format/destination/separator prompts, help text, and related tests/docs only.
*   **Acceptance Criteria:**
*   Output/export no longer crosses more chooser layers than the Task 28 spec allows for its common path, but it does not collapse unlike decisions into one opaque “smart” prompt.
*   File output versus hardcopy remains explicit whenever that distinction changes behavior, and format stays explicit whenever folding it away would make the flow harder to understand.
*   Separator prompting appears only when the selected output mode actually needs it.
*   Advanced output options remain available without reintroducing hidden semantics or less-intuitive prompt merges.
*   File and hardcopy destinations remain explicit and help-synchronized.
*   - [x] **Status:** Completed.

### **Task 29: Persist Compare Mode Presets and Modal Contract**
*   **Goal:** Build on the Task 28 compare-flow baseline by making compare mode remember last-used settings and enforce a durable modal interaction contract.
*   **Rationale:** After compare prompt-chain compression is established, compare still needs stable option memory, strict modal ownership, and consistent compare-only key behavior to stay fast and predictable in repeated use.
*   **Dependency:** Sequence after Task 28.2 and after the Task 28 compare-family remediation subtask establishes the compressed compare flow baseline. If compare-mode state ownership still depends on split-panel restore/state work, land this after Task 31.
*   **Scope Lock:** Compare mode behavior only after the compare-family shallow-flow remediation exists; no fresh compare prompt-chain redesign here and no unrelated footer or global keybinding redesign.
*   **Acceptance Criteria:**
*   Compare remains a modal state and the compare footer keybinding/F1 surface owns the footer while active.
*   In compare mode, only compare keys are active; non-compare keys are silent no-ops.
*   No conflicting quick-key mappings are permitted in compare mode.
*   The compare flow inherited from Task 28 remains the baseline; this task must not reintroduce deeper prompt chains or bypass explicit compare target confirmation.
*   Persist last-used compare options across restart; config values seed defaults and runtime usage updates remembered defaults.
*   Default behavior remains unchanged when quick/preset config is absent.
*   Compare-mode prompt/menu/help surfaces stay synchronized with the persisted option model and modal key contract.
*   Add focused regression coverage for compare behavior and split-panel isolation.
*   Update compare docs/help text in `etc/ytnova.1.md` and regenerate `docs/USAGE.md`.
*   - [ ] **Status:** Not Started.

### **Task 30: Add Recursive Directory Compare in `J` Flow**
*   **Goal:** Support recursive directory-tree compare from the existing `J` compare flow.
*   **Rationale:** Recursive compare is a practical file-manager workflow and improves alpha usefulness for real tree-diff tasks.
*   **Scope Lock:** Add recursive compare capability and prompt/menu wiring only; do not redesign unrelated compare UI.
*   **UX Direction:** Keep `J` as the compare entry point. Use one submenu level or a direct compare prompt with explicit recursive choice (`Recursive: y/N`).
*   **Acceptance Criteria:**
*   Recursive and non-recursive directory compare are both available from the same `J`-entry compare flow.
*   The recursive choice is explicit and discoverable in compare prompts/help.
*   Compare target confirmation and split-panel isolation behavior remain unchanged.
*   `etc/ytnova.1.md` and generated `docs/USAGE.md` are updated when behavior lands.
*   - [ ] **Status:** Not Started.

### **Task 31: Unified Split-Panel State/Restore Architecture**
*   **Goal:** Make split-panel behavior deterministic by giving each panel one canonical UI state record and one canonical restore path so `F8`, `Tab`, `Enter`, release, reactivation, and visible-tree redraw all preserve stable identity, viewport, selection, visibility, and mode without re-deriving authority from raw rows or stale pointers.
*   **Rationale:** The BUG-3-family regressions are all the same root-cause class: split-panel state ownership and restore authority are fragmented, so small changes keep reintroducing viewport drift, selection drift, hidden-dotfile reanchor, and transient wrong-shape renders.
*   **Scope Lock:** Canonical panel/window UI state ownership, restore generation and fallback, split transition integrity, and regression coverage only. No keybinding redesign, no new features, and no unrelated overlay/submode rewrite.
*   **Implementation Rule:** Task 31 must follow `docs/SPECIFICATION.md` §2.3, §3.4, §5.1, §5.2, §5.3, and §5.5. If any implementation detail is still ambiguous after reading those sections, the spec must be updated before code changes are made.
*   **Pre-implementation Checklist:** Before coding starts, Task 31 must name the exact state schema, owner boundary, generation rules, identity-key rules, restore/transition entrypoints, fallback order, and regression-gate matrix.
*   **Coverage Target:** This umbrella is the primary stabilization track for BUG-3, BUG-3.1, BUG-3.2, BUG-3.3, BUG-3.4, BUG-3.5, BUG-4, BUG-5, and BUG-6, plus the related split-state regressions they expose.
*   **Execution Order (mandatory):** Deliver as sequenced subtasks: **30.1 -> 30.2 -> 30.3 -> 30.4**. Task 31 closes only after all subtasks are complete.
*   **Acceptance Criteria:**
*   One authoritative UI state record exists per panel/window; in split mode restore snapshots are keyed by `(panel, volume)`.
*   Shared `Volume` owns shared topology and payload only; it must not own panel-local selection, viewport, filter, or dotfile-visibility state.
*   Restore uses stable identity keys and deterministic fallback only: exact identity, then nearest visible ancestor, then next visible sibling, then previous visible sibling, then root visible node.
*   Restore code must not reconstruct authority from raw row math, `disp_begin_pos + cursor_pos`, or stale `DirEntry*` / `FileEntry*` pointers.
*   Redraw is projection only; it must not become the source of truth for stored state.
*   Reactivation must restore the recorded tree/small-file/big-file shape directly and must not flicker through the wrong shape first.
*   Invalidation and ordering are explicit: rebuild/mutation completes, generation advances, then restore rebinds or falls back deterministically.
*   Mandatory invariant checks exist in code for owner-boundary writes, restore authority, and inactive-panel freeze/resume behavior.
*   Mandatory regression coverage exists as a focused matrix for `Enter`, `Tab`, `F8`, hidden-dotfile reactivation, release/relog, generation mismatch, and split-panel restore paths.
*   Generation-mismatch restore checks are proven by tests and must fail if a stale snapshot is reused after invalidation.
*   Merge gate policy exists for F8/split-touching PRs: required invariant gate, required transition-matrix gate, and required no-direct-write split-authority check that blocks direct writes outside the canonical owner path.

#### **Task 31.1: Canonical Panel UI State Record + Ownership Map**
*   **Goal:** Define one explicit owner for panel-local frozen state and remove shadow ownership paths that let split panels drift apart.
*   **Mechanism:** Make the panel/window UI state record canonical for cursor, viewport origin, file selection, file cursor, filters, dotfile visibility, and saved focus/mode; keep shared topology in `Volume` only.
*   **Acceptance Criteria:**
*   All panel-local state classes are classified explicitly as owned, derived, or shared-topology-only.
*   Split panels can hold independent filters and dotfile visibility for the same logged volume.
*   No shared-buffer aliasing path remains that allows cross-panel leakage of panel-local state.
*   Owner-boundary assertions exist in the code and fail fast if a non-owner path attempts to mutate panel-local state.
*   - [ ] **Status:** Not Started.

#### **Task 31.2: Deterministic Restore/Rebind Engine + Generation Invalidation**
*   **Goal:** Make restore deterministic after rebuilds, visibility changes, renames, moves, symlink changes, and mount remaps by re-resolving stable identity instead of re-deriving state from row position.
*   **Acceptance Criteria:**
*   Restore rebinds by stable identity and advances generation before a reused snapshot can apply.
*   Exact fallback order is fixed and documented: exact identity, nearest visible ancestor, next visible sibling, previous visible sibling, then root visible node.
*   No restore path may use raw row math, stale pointers, or guessed viewport origin as authority.
*   No invalid transition may briefly render the wrong tree/file shape before converging.
*   Add focused regression coverage for Enter, Tab, hidden-dotfile reactivation, and split restore after rebuild/mutation.
*   Add an explicit generation-mismatch test that proves a stale snapshot cannot restore after invalidation.
*   Update `docs/SPECIFICATION.md` contract references if implementation details differ during delivery.
*   - [ ] **Status:** Not Started.

#### **Task 31.3: Atomic Split Transition Engine + Read-Only Render Contract**
*   **Goal:** Centralize split transitions so `F8` enter/exit and `Tab` handoff are transactional and rendering cannot mutate authoritative split state.
*   **Acceptance Criteria:**
*   One explicit split-state owner module/API exists for split/panel-mode authoritative mutation.
*   Direct split-state mutation outside the owner path is removed from production paths.
*   F8 and Tab transitions use one transaction flow: snapshot -> compute -> validate invariants -> commit/rollback.
*   Renderer paths are read-only and may only project the current state record.
*   Direct writes outside the canonical owner path fail through assertions and CI checks.
*   - [ ] **Status:** Not Started.

#### **Task 31.4: Enforceable Regression Gates + Spec Sync**
*   **Goal:** Keep the architecture from regressing by making the restore contract testable and merge-blocking.
*   **Acceptance Criteria:**
*   Mandatory invariant checks run for split restore/transition paths (active-only mutation, inactive freeze/resume, no cross-panel import, identity-based restore).
*   CI/QA merge gate exists for F8/split-touching PRs: invariant gate, transition-matrix gate, and no-direct-write split-authority check.
*   The regression matrix is focused on split-state flows and covers restore, reactivation, generation mismatch, and transition handoff cases.
*   Test evidence explicitly proves stale restore snapshots are rejected after generation changes.
*   `docs/SPECIFICATION.md` stays aligned with the implemented restore contract and fallback order.
*   Task 31 closure requires green evidence for all Task 31 subtasks.
*   - [ ] **Status:** Not Started.

#### **Task 31.5: Panel-Local Split Statistics**
*   **Goal:** Make `F6` statistics an independent view property of each `F8` split panel instead of a single global sidebar.
*   **Scope Lock:** Panel-owned stats visibility, split geometry, rendering projection, and focused regression coverage only. Do not alter single-panel statistics behavior.
*   **Acceptance Criteria:**
*   Entering `F8` initializes both panels with statistics hidden.
*   `F6` toggles only the active panel's statistics; switching with `Tab` neither changes nor redraws the other panel's state.
*   An enabled panel reserves a 24-column statistics strip at its own right edge; both strips can be visible simultaneously.
*   Stats rendering projects each owning panel's selection and view state without changing active-panel ownership or other panel state.
*   Focused PTY coverage proves left-only, right-only, both-visible, both-hidden, and split re-entry behavior with layout-resilient assertions.
*   Footer and F1 help describe the panel-local `F6` behavior when implementation lands.
*   - [x] **Status:** Completed.

### **Task 32: Enable Practical Command Subset in `F7` Preview (Keep `F8`/`Tab` Blocked)**
*   **Goal:** Finish `F7` as an in-place work mode: users can run common file actions without leaving preview, while `F8`/`Tab` stay blocked for preview-state safety.
*   **Rationale:** `F7` currently feels unfinished because common workflows still require repeated exits.
*   **Scope Lock:** `F7` command availability contract, help/footer parity, and regression coverage only; no split-layout redesign.
*   **Acceptance Criteria:**
*   Define and implement the core `F7` action set for inspect-and-act workflows (including tag/search/view results/compare/copy/move/rename, plus existing high-value file actions).
*   In `F7`, `^T` tag-all, `^S` search, and `^V` tagged/search-result view flows work without exiting preview mode.
*   Tagged search hits/results are visibly highlighted in `F7` preview.
*   `F8` and `Tab` are explicit no-ops in `F7` mode.
*   Footer keybinding hints and F1 help in `F7` accurately reflect allowed actions and blocked keys.
*   Add focused regression tests for allowed-command execution in `F7` and blocked-key enforcement (`F8`, `Tab`).
*   Update `etc/ytnova.1.md` and regenerate `docs/USAGE.md` when behavior lands.
*   - [ ] **Status:** Not Started.

### **Phase Follow-On Work**

### **Task 33: Harden Build Source Discovery (Recursive + Deterministic)**
*   **Goal:** Update build source discovery so all C files under `src/` are discovered recursively with deterministic ordering.
*   **Rationale:** Current discovery only covers up to one subdirectory level and will miss files after module reorganization.
*   **Scope Lock:** Build discovery and related guard/test updates only. No feature behavior changes.
*   **Acceptance Criteria:**
*   Build still succeeds with `make clean && make`.
*   Full QA gate still passes with `make qa-all`.
*   Source file list ordering is deterministic across runs.
*   - [ ] **Status:** Not Started.

### **Task 34: Reorganize Modules into Shallow Hierarchical Folders**
*   **Goal:** Group modules into shallow, purpose-based subfolders and update build/header/linkage references accordingly.
*   **Rationale:** Improves discoverability and ownership without changing behavior.
*   **Scope Lock:** File moves + include/path/build/script/test reference updates only. No feature behavior changes.
*   **Acceptance Criteria:**
*   `make clean && make` passes.
*   `make qa-module-boundaries` and `make qa-all` pass.
*   No runtime behavior changes.
*   Folder depth remains shallow (max one extra level under `src/ui` and `src/cmd`).
*   - [ ] **Status:** Not Started.

### **Task 35: Decompose Remaining Hotspot Modules (Atomic Subtasks)**
*   **Goal:** Reduce complexity in remaining hotspot files by extracting cohesive action families into focused modules while preserving behavior.
*   **Rationale:** These files remain risk hotspots after controller decomposition and slow safe feature delivery.
*   **Execution Rule:** Must be delivered one atomic subtask at a time (3.1 to 3.5), each with its own architect plan, developer pass, auditor pass, and QA evidence.
*   - [ ] **Status:** Not Started.

### **Task 36: Decompose `src/ui/ctrl_file_ops.c` (`handle_tag_file_action` focus)**
*   **Goal:** Extract large tagged-action branches from `handle_tag_file_action` into focused helpers/modules.
*   **Scope Lock:** Preserve all tagged-file behavior and command semantics.
*   **Acceptance Criteria:** Smaller dispatcher function, unchanged behavior, green QA.
*   - [ ] **Status:** Not Started.

### **Task 37: Decompose `src/ui/key_engine.c`**
*   **Goal:** Separate key mapping/dispatch concerns from input-loop mechanics and context-specific action routing.
*   **Action Name Cleanup:** Normalize tree-expand action identifiers so names match behavior: shallow expand (`+`) is `ACTION_TREE_EXPAND`, recursive expand (`*`) is `ACTION_TREE_EXPAND_RECURSIVE`, and any redundant tree-expand identifier is merged or removed. Update key/action mappings and related tests with no behavior change.
*   **Scope Lock:** No keybinding behavior change unless explicitly approved in a separate task.
*   **Acceptance Criteria:** Cleaner dispatch boundaries, consistent action naming, unchanged key behavior, green QA.
*   - [ ] **Status:** Not Started.

### **Task 38: Decompose `src/cmd/copy.c`**
*   **Goal:** Isolate copy conflict handling, path/precondition validation, and transfer orchestration into focused units.
*   **Scope Lock:** No copy/move/archive user-visible behavior changes.
*   **Acceptance Criteria:** Reduced complexity in core copy path, unchanged behavior, green QA.
*   - [ ] **Status:** Not Started.

### **Task 39: Decompose `src/cmd/profile.c`**
*   **Goal:** Split profile parsing, validation/defaulting, and apply/update logic into focused units.
*   **Scope Lock:** No configuration semantic changes.
*   **Acceptance Criteria:** Clear parser/apply separation, unchanged config behavior, green QA.
*   - [ ] **Status:** Not Started.

### **Task 40: Refactor Tab Completion for Command Arguments**
*   **Goal:** Update the tab completion logic in `src/util/tabcompl.c` to handle command-line arguments correctly and resolve ambiguous matches using Longest Common Prefix (LCP).
*   **Rationale:** Currently, the completion engine treats the entire input line as a single path. This causes failures when trying to complete arguments for commands (e.g., `x ls /us<TAB>` fails because it looks for a file named "ls /us"). It also fails to partial-complete when multiple matches exist (e.g., `/s` matching both `/sys` and `/srv`).
*   **Mechanism:**
    *   Tokenize the input string to identify the word under the cursor.
    *   Perform globbing/matching *only* on that specific token.
    *   If multiple matches are found, calculate the Longest Common Prefix and return that (standard shell behavior) instead of failing or returning the first match.
    *   Reassemble the command string (prefix + completed token) before returning.
*   - [ ] **Status:** Not Started.

### **Task 41: Responsive Adaptive Footer**
*   **Goal:** Replace hardcoded footer-line command placement with a structured, self-organizing footer layout engine that assembles command entries at runtime and keeps the function-key band fitting cleanly on the bottom footer line.
*   **Rationale:** The current footer model is too position-bound and too brittle for context changes, responsive width changes, and future localization. Footer content must be assembled from stable action entries rather than handwritten line strings so that key tokens, translated/default labels, disabled-state logic, and line packing can evolve independently.
*   **Scope:** Footer layout/packing/rendering only. This task defines how footer command entries are assembled, prioritized, wrapped, truncated, and fitted into the available footer area. Footer/F1 parity and wording accuracy remain coordinated with the dedicated parity task rather than being left implicit here.
*   **Architecture note:** Treat the left mode/toggle signpost and the footer command strip as separate render regions. The left label (for example `DIR`, `FILE`, `ARCHIVE`, `←─┘ File`, `←─┘ Tree`) is a context signpost, not part of the command-layout string. The footer command strip must start at an explicit command column determined by layout rules rather than by the rendered width of the left label text. Label wording, glyph rendering, and toggle/context naming must never silently shift the footer-command alignment.
*   **Self-Organizing Footer Contract:**
    *   The footer is built from structured command entries, not hardcoded rendered lines.
    *   The top footer lines must self-organize according to available width, command priority, context, and availability state.
    *   The function-key command band must fit on the bottom footer line through structured packing rules rather than fixed hand-authored text.
    *   Key tokens, labels, separators, disabled state, and visibility rules are independent fields in the layout model.
    *   Footer rendering must remain compatible with future gettext/i18n/l10n work: labels are not assumed to have fixed English width or fixed word order.
*   **Cross-Task Contract:**
    *   This task depends on the structured label/key-token split introduced by Task 11.2.
    *   Footer labels must come from stable action IDs plus current label resolution, not from legacy raw `[MENU]` whole-line overrides.
    *   Task 44 remains the footer/F1/help parity contract that verifies the footer layout is semantically honest once this layout engine exists.
*   **Acceptance Criteria:**
*   Footer command layout is generated from structured entries rather than hardcoded per-line binding strings.
*   The top footer lines auto-fit their visible commands according to width and context across the standard footer surfaces (directory, file, archive, and equivalent context-driven footer variants) without relying on fixed English-only line templates.
*   The function-key band fits on the bottom footer line through defined packing/truncation/priority rules across the same shared footer layout contract.
*   Key tokens remain independent from labels, so keymap changes and future localization do not require hardcoded footer-line rewrites.
*   Footer layout behavior is documented with deterministic ordering, overflow, truncation, and disabled-state rules.
*   The resulting footer architecture is explicitly compatible with Task 11.2 label/binding separation and with future gettext-based localization work.
*   - [ ] **Status:** Not Started.

#### **Task 41.1: Footer Auto-Fit Line Layout (No Hardcoded Per-Line Bindings)**
*   **Goal:** Define and implement the structured footer-entry model and the auto-fit packing rules for the shared runtime footer layout so every standard footer context reuses the same self-organizing top rows and bottom function-key band.
*   **Mechanism:** Introduce footer entries keyed by stable action identity, with independently resolved label, key token, visibility, enabled/disabled state, and priority; then pack those entries into available footer lines deterministically by rendered keybinding token order, including natural function-key sequencing and `Esc` after the function-key run, regardless of whether the active footer is directory, file, archive, or another context-specific variant of the shared footer surface. The two top command rows must choose wrap points that try to use the available width evenly rather than greedily filling row 1 first.
*   **Explicit render split:** The implementation must model and render the left footer signpost separately from the footer command strip it precedes. The signpost owns only context/toggle labeling and its own glyph/text rendering; the footer command strip owns only command packing and starts from a declared layout column, not from the measured width of the signpost string.
*   **Cross-Reference:** Labels must resolve through the Task 11.2 label architecture from stable action IDs plus current binding state, not from footer-line text assembly.
*   **Coverage note:** This shared footer layout applies wherever the runtime presents the standard footer surface; context changes swap signposts and command sets, not footer-line formatting rules.
*   - [x] **Status:** Completed.

#### **Task 41.2: Implement Responsive Adaptive Footer**
*   **Goal:** Extend the shared footer-layout work into footer-adjacent prompt takeover surfaces and other specialized status-area flows that do not yet reuse the standard footer renderer.
*   **Mechanism:** Replace remaining prompt-/dialog-specific footer-area strings with runtime packing/reflow where those surfaces still own the footer region, while preserving deterministic ordering and context-sensitive visibility rules.
*   **Cross-Reference:** Task 44 validates semantic parity between footer, help, and prompts after this layout work lands; Task 11.2 provides the structured label/key-token inputs that make localization-safe footer rendering possible.
*   **Delivered scope:** Prompt takeover rows now use adaptive command-strip rendering with ellipsis-based truncation instead of mid-token clipping, and remaining sort/attribute prompt strings route through structured command-strip layouts rather than hardcoded footer text.
*   - [x] **Status:** Completed.

### **Task 42: Implement Integrated Help System**
*   **Goal:** Create a pop-up, scrollable help window (activated by F1) that displays context-sensitive command information.
*   **Rationale:** Replaces the limited static help lines with a comprehensive and user-friendly help system, making the application easier to learn and use without consulting external documentation.
*   **Context Contract:** `F1` help must be contextual by active runtime surface, not a single generic command dump. The help surface must resolve against the currently active directory/tree view, file view, archive view, Showall/Global view, split/preview layout, and prompt/dialog state.
*   **Sequencing Note:** Prefer to land Task 43's final portable footer keybinding/F1 wording contract before this task so the integrated help system documents the intended low-noise runtime guidance surfaces rather than an abandoned transient-footer idea.
*   **Delivered Scope:** Shared modal help now opens from the main runtime surfaces plus active picker/prompt/dialog flows, stays brief enough for in-task consultation, and leaves longer semantics/examples to `etc/ytnova.1.md` and generated `docs/USAGE.md`.
*   - [x] **Status:** Completed.

### **Task 43: Refine In-App Help Text**
*   **Goal:** Review all user prompts and help lines to be clear and provide context for special syntax (e.g., `{}`). The menu should be decluttered by only showing a `^` shortcut if its action differs from the base key (e.g., `(C)opy/(^K)` is good; redundant duplicate bindings should not be listed).
*   **VI Mode Signaling**: Ensure footer keybinding lines dynamically reflect uppercase commands (e.g., `(K) Vol` instead of `(k) Vol`) when `VI_KEYS=1` is active to avoid navigation collisions.
*   **Footer Stability Decision:** The canonical live footer must stay stable across supported terminal paths and must not depend on transient modifier-state telemetry that terminals, multiplexers, and remote sessions may fail to expose consistently.
*   **Noise Budget Decision:** Do **not** replace the clean footer with permanently noisy fallback forms such as `Copy/^Copy`, `C/^Copy`, or a tagged-state footer variant that appears only after tagging. The common footer must stay clean enough for new users to operate without help-menu friction.
*   **Discoverability Contract:** Keep the live footer focused on always-relevant, low-noise bindings plus non-redundant alternates (for example `(C)opy/(^K)` and `(M)ove/(^N)` where the alternate actually differs). Put Ctrl-only tagged/search-oriented operations and their semantics in `F1` help and prompt wording instead of trying to surface every one of them in the footer.
*   **Rationale:** Fulfills the "No Hidden Features" principle and improves UI clarity by removing redundant information.
*   - [x] **Status:** Completed.

### **Task 44: Refine Contextual F1 Content and Footer-Parity Contract (gettext-ready)**
*   **Goal:** Ensure each contextual `F1` surface is concise, useful, complete, and readable under popup constraints: it must cover every footer command for the active surface while also clarifying the non-obvious behavior the footer cannot carry.
*   **Rationale:** Footer and `F1` are the primary in-app guidance surfaces. Pure parity without added clarification degenerates into "the footer again," while essay-length help steals attention from the task at hand. `F1` must stay contextual, plain-English, and newcomer-friendly; `etc/ytnova.1.md`/`docs/USAGE.md` remain the terser reference path.
*   **Related Bugs:** `BUG-10.1` / `BUG-10.2` / `BUG-10.3` / `BUG-10.4` — footer keybinding/F1/prompt mismatch (discoverability + confidence).
*   **Dependency:** Sequence after Task 41 establishes the structured footer layout engine and after Task 11.2 establishes the structured label/key-token split.
*   **Sequencing Note:** Land Task 43's portable low-noise footer keybinding/F1 wording decisions before this task so parity is enforced against the final portable footer contract rather than a transient modifier-held variant.
*   **Scope Lock:** Base help wording/structure, coverage matrix, and text-organization readiness only; no command-behavior changes. Hyperlink/index-style help navigation is tracked separately under Task 44.1, and progress/help coexistence is tracked separately under Task 21.1.
*   **Surface-Naming Contract:** Keep the help surfaces distinct in roadmap/spec/runtime language: the always-visible bottom strip is the **footer command strip**; the modal opened by `F1` is the **help popup**; the minimal action row inside that popup is the **help-popup hint line**; and any command/topic entry that exists mainly to branch into deeper explanation is a **help popup link**.
*   **Acceptance Criteria:**
*   For each supported context, every footer command appears in the matching `F1` help set, but `F1` need not mirror the footer verbatim if a clearer concise explanation is better. Displayed key tokens and command labels in those `F1` rows must remain truthful to the live runtime bindings, including symbolic and mnemonic-heavy rows.
*   `F1` adds short clarification for behaviors the footer cannot explain cleanly, especially Ctrl-only tagged/search flows, tagged variants of base commands, wildcard or other syntax-heavy command families, prompt syntax such as `{}`, numeric `1..9` display/info-band meanings, and split/archive/Showall/Global caveats that affect the current surface.
*   Main contextual help popups use a minimal hint line shaped like `Contents  Navigation  Esc/Quit` rather than restating the full footer command strip. Non-obvious help-only follow/back controls must remain explicit there instead of being left for the user to infer.
*   Main contextual command pages keep local navigation scoped to the active surface: directory help includes tree/directory movement only, file help includes file-window movement only, and split-specific controls remain on `F8` help instead of leaking into unrelated pages.
*   Prompt-local `F1` is selective rather than universal. Use it where the prompt itself carries non-obvious syntax or semantics; for trivial prompts, keep the footer simpler and let `F1` fall back to the owning surface/shared explanation instead of forcing a dedicated prompt help page.
*   If a prompt has its own `F1`, the prompt footer must advertise it explicitly (for example `F1 help`) so users are not expected to guess that prompt-local help exists.
*   First-pass required contexts: FS dir, FS file, VFS dir, VFS file, F7, F8, Showall, Global, tagged flows, prompt/dialog flows with genuinely non-obvious semantics, and `VI_KEYS=1` variants.
*   Help popup body text wraps within the available popup width; narrow layouts preserve readable content through wrapping and scrolling instead of right-edge truncation.
*   Numeric FileInfo band coverage is explicit: when the footer compresses `1..9 dir view` / `1..9 file view`, the matching `F1` help must decode each advertised number's active-surface meaning for Name, Attributes, Owner, Times, Compact, size units, Mini preview, File detail, and Git, while keeping the hidden unassigned `0` behavior out of the advertised band.
*   Context-sensitive actions keep short in-app summaries while full semantics remain in `etc/ytnova.1.md`/`docs/USAGE.md` (for example compare `J` modes, compare basis/tag/hash meaning, useful command-line editing, and archive/compress format behavior).
*   Shared operator topics such as filters, jump/list-jump behavior, wildcard/rename-pattern rules, search/fuzzy-matching semantics, command-line editing, `VI_KEYS=1`, `F10` config, theming/customization, and similar cross-cutting workflows are explained once and linked from local `F1` pages instead of being re-taught on every page.
*   The `Contents` topic acts as a complete alphabetical link index for operator-facing help topics, so users can discover the owning topic for conceptual, procedural, and semantic questions without guessing which local page might contain it.
*   In contextual command lists, every listed command term is a help popup link to a brief plain-English explainer page; the user must not be sent into a maze of recursive definitions.
*   Help text paths are structured for gettext extraction/reuse and future deduplication (no duplicated ad-hoc strings per view path, and no translator-facing requirement to edit C literals just to keep related help prose aligned).
*   Add regression checks that detect footer/F1 parity drift in covered contexts.
*   Add a keybinding parity audit gate that verifies active runtime keybindings remain consistently documented across footer, `F1`, and `etc/ytnova.1.md`/`docs/USAGE.md`.
*   - [~] **Status:** In Progress.

#### **Task 44.1: Add Contextual F1 Hyperlinks and Shared Explainer Pages**
*   **Goal:** Let contextual `F1` help treat every listed command term as a navigable link so repeated topics (for example useful command-line editing, tagged-flow semantics, or numeric FileInfo meanings) do not have to be duplicated verbatim on every page.
*   **Rationale:** As contextual help grows, repeated prose becomes harder to keep consistent. A lightweight mc-style link model keeps `F1` brief while still allowing a user to drill into a short explainer without leaving the modal help surface.
*   **Interaction Contract:** Keep `F1` modal and task-focused, not a separate browser. Link navigation must stay shallow: follow as few links as possible, with a hard cap of one or two link hops from the original contextual page before the user returns/backtracks. `Enter` follows the selected link, `Right` may also follow, `Left` goes back one page, and `Esc` closes the help surface from anywhere. Normal reading/navigation keys remain available inside the page: `Up`, `Down`, `PgUp`, `PgDn`, `Home`, and `End`. The `Contents` page should behave like a real topic index rather than a decorative intro, and help-only follow/back behavior must stay explicit in the hint line and Navigation topic rather than being implied by generic runtime-navigation prose.
*   **Theme/Config Contract:** Help-surface styling remains part of the theme catalog (`themes.conf` / runtime theme data), not `ytnova.conf`. Base help text continues to use the `help` reading surface, while hyperlink-capable help may add narrower theme roles such as `help_link` and `help_link_selection` for linked text and the active linked target.
*   **Orthodox Default Direction:** For the default orthodox-blue theme, prefer a restrained reading surface with distinct linked-text and active-link colors rather than picker-style row highlighting. If hyperlinks need dedicated colors, keep the defaults conservative (for example black-on-blue links and yellow-on-blue active-link emphasis) and avoid turning the whole help page into a loud picker.
*   **Acceptance Criteria:**
*   Each listed command term in a contextual `F1` command list can be followed as a help popup link.
*   Shared topics can be linked from multiple contextual pages without copying the same explanation string into each page body.
*   Link navigation stays shallow and predictable inside the modal help surface; users do not have to enter a separate browser/workspace to read a linked explainer.
*   Linked explainer pages can be reached and exited without losing the original contextual-help entry point.
*   The link model never grows into an unbounded history/browser stack; at most one or two explainer hops are reachable before the user backtracks.
*   Linked explainer pages use brief plain English and remain short enough to hold attention instead of becoming dense reference pages.
*   `Enter` (and optionally `Right`) follows, `Left` backs out one page, `Esc` closes from anywhere, and `Up`/`Down`/`PgUp`/`PgDn`/`Home`/`End` continue to serve page navigation/reading.
*   The `Contents` page is a complete alphabetical linked index of the operator-facing help topics intended for end-user discovery.
*   Links exist only where the target adds new explanatory value; avoid duplicate detail pages that merely restate the originating row or force unnecessary bureaucracy.
*   Theme support for linked text and linked-target emphasis is defined in the theme catalog path, not as one-off `[COLORS]` or `ytnova.conf` knobs.
*   Add focused regression coverage for link focus, follow, back, and close behavior.
*   - [x] **Status:** Completed.

#### **Task 44.2: Theme the Contextual F1 Reading Surface and Separate Footer Guidance Role**
*   **Goal:** Define and implement distinct theme-role behavior for the contextual `F1` reading surface and the always-visible footer guidance surface now that the base role-based theme system exists.
*   **Rationale:** Task 61 established the general theme architecture, but it intentionally left `help` overloaded across the footer and the `F1` reading surface. Contextual help now needs its own follow-on theming pass so the reading surface, linked text, and active linked target remain readable, restrained, and consistent across bundled themes while the footer keeps an independently tunable low-noise scheme.
*   **Theme Contract:** All footer and `F1` visual styling belongs in `etc/ytnova.themes` / runtime theme data, not `ytnova.conf`. Reserve `help` for the `F1` reading surface, keep the always-visible main footer on `footer`, give the popup strip its own `help_footer`, and allow narrower help-popup roles such as `help_heading`, `help_topic`, `help_attention`, `help_alert`, `help_link`, and `help_link_selection` where the bounded help semantics need them. Prompt/dialog surfaces remain separate concerns unless a later task explicitly gives them their own theme role.
*   **Scope Limit:** This task owns the surface-level help UI roles for the popup body, popup footer strip, frame lines, link text, active link emphasis, help-popup mnemonic emphasis, popup title emphasis, popup term/definition labels, and bounded authored attention/alert tiers. It does **not** open arbitrary free-form per-span theming or raw presentational directives inside help prose; broader semantic expansion remains deferred to Idea FE-9 after the content/IA settles.
*   **Orthodox Default Direction:** Keep the help page readable and quiet on the orthodox-blue theme: black-on-grey body text, black-on-cyan linked text, and yellow-on-cyan active-link emphasis are acceptable; ordinary body text must remain easier to read than navigation chrome.
*   The footer must remain concise and lower-noise than modal help while still allowing its own color treatment.
*   **Acceptance Criteria:**
*   The base `F1` reading surface has an explicit documented theme contract separate from picker/list surfaces and separate from the footer guidance strip.
*   Footer rendering resolves through a dedicated `footer` role rather than reusing `help`.
*   If hyperlink help is enabled, linked text and active-link emphasis resolve through dedicated theme roles rather than hardcoded colors.
*   Built-in themes document and ship coherent footer and `F1` help styling without requiring user edits to `ytnova.conf`.
*   Focused tests or source-contract checks prove footer and `F1` help can use different theme roles and do not fall back to picker/dialog styling.
*   - [x] **Status:** Completed.

#### **Task 44.3: Separate Contextual F1 Help Authorship from Man/Usage Reference Authorship**
*   **Goal:** Keep contextual `F1` help and man/USAGE reference prose in separate authored sources so each can serve its own audience without tone or structure compromises, then finish the remaining audit/parity cleanup around that pipeline.
*   **Rationale:** Much of this split-source pipeline already exists, so the remaining work is no longer "invent the mechanism" so much as "keep the mechanism authoritative, editable, and fully reconciled with runtime/footer/manpage behavior." The filter path remains the clearest example: pressing `F`/`f` for filter and then `F1` must open a short plain-English guide, while the manpage and `docs/USAGE.md` stay terser Unix-reference material.
*   **Separate-source contract:** The authored contextual-help source is dedicated to the help popup (`etc/help/f1.en.md`). The authored man/USAGE reference source is separate (`etc/help/man.en.md`). No compatibility shim or forced dual-purpose source remains in the final model.
*   **Projection contract:** The F1 source drives only runtime/contextual help assets. The man source drives only the manpage and generated `docs/USAGE.md`. Shared topic IDs and mappings are allowed, but shared prose is optional rather than required.
*   **Generator/runtime contract:** The in-repo Python generator is the sole correctness path for runtime help assets, generated headers, manpage output, and `docs/USAGE.md`, and runtime `F1` surfaces resolve through maintained context -> topic mappings backed by the F1 source rather than scattered ad-hoc C prose.
*   **Editability contract:** Both sources must stay easy for translators and maintainers to read and edit directly as ordinary markdown. Optional reusable fragments may exist only when naturally helpful; neither file must be shaped around clever DRY tricks that make ordinary editing harder.
*   **Context/topic contract:** Both authored sources use the same stable topic inventory (for example `contents`, `navigation`, `dir`, `file`, `archive-dir`, `archive-file`, `filter`, `compare`, `output`, `showall`, `global`, `f7`, `f8`, `command-line-editing`, `vi-keys`, `f10`, and `theming`) and the runtime maps active contexts/prompts to those topic IDs. Shared facts such as keybinding ownership and context mapping stay aligned through that inventory, not through forced shared prose.
*   **Contextual-F1 contract:** Contextual `F1` remains intentionally short and task-local. Prompt `F1` surfaces such as Filter/Compare/Output show only the relevant plain-English syntax/options/examples for that prompt, while the main directory/file/archive/F7/F8 surfaces explain the active footer commands and only the non-obvious caveats needed in that context. The help-popup hint line is a fixed minimal row for `Contents`, `Navigation`, and `Esc/Quit`, and every listed command term in the body is a help popup link to a brief explainer page.
*   **Reference-tone contract:** The man/USAGE source stays terse and reference-oriented. It is a markdown-authored Unix-style bite-sized reference, not a projection of the F1 tutorial voice.
*   **Generator portability contract:** Essential help generation must not depend on external markdown tooling that may be missing on supported targets. The project ships a small in-repo Python generator (Python is already a project requirement for tests) that runs on Linux, BSD, GNU Hurd, and illumos; external tools are not part of the correctness path.
*   **Ownership split:** User `commands.conf` remains a user override/customization surface for labels/bindings and is not the canonical source of translated help prose. Shipped help data owns explanatory text; key tokens and displayed command labels continue to resolve through keymap/command-preset/runtime rendering rules.
*   **Acceptance Criteria:**
*   Updating filter help for the tutorial popup and updating filter help for the man/USAGE reference are independent explicit edits; no hidden single-source prose coupling is required.
*   The schemas for `etc/help/f1.en.md` and `etc/help/man.en.md` remain strict enough for deterministic generation and simple enough for maintainers and translators to extend manually.
*   One generator command regenerates runtime help outputs from `etc/help/f1.en.md` and reference outputs from `etc/help/man.en.md`, and generation fails loudly on malformed topic blocks instead of silently dropping content.
*   A maintained context-to-source mapping exists for the first-pass `F1` surfaces (directory, file, archive, Showall, Global, F7, F8, and prompt/dialog help including Filter/Compare/Output).
*   Directory, file, archive-dir, archive-file, Filter, Compare, Output, Showall, Global, F7, and F8 `F1` paths all resolve through generated topic data.
*   Parity/audit coverage detects drift between active runtime keybindings, generated `F1` slices, and manpage/usage text.
*   `docs/SPECIFICATION.md` and `docs/ARCHITECTURE.md` stay aligned with the split-source help model, generator path, and runtime context/topic mapping contract.
*   The chosen split-source format remains compatible with the future gettext/po4a split tracked under Task 62 rather than creating an i18n dead end.
*   - [x] **Status:** Completed.

#### **Task 44.4: Split Cross-Cutting Operator Semantics from Local Context Pages**
*   **Goal:** Make contextual `F1` pages answer the immediate screen/prompt question first, while shared operator semantics live in dedicated explainer topics that can be reused across multiple surfaces.
*   **Rationale:** The current failure mode is not just missing prose; it is information-architecture blur. When local command summaries, shared rules, help-popup navigation, and reference-detail all compete on the same page, `F1` feels noisy and unreliable even when the facts are present. Users need a crisp distinction between "what this screen does now," "how this repeated ytnova feature family works in general," and "where the full reference lives."
*   **Comprehensiveness Contract:** A user pressing `F1` on any supported surface must be able to reach an answer for the active question without being stranded by omissions. If the first contextual page is intentionally short, it must still provide an obvious path to the owning shared topic or command explainer rather than silently assuming outside knowledge.
*   **Unix Documentation Principle:** ytnova must follow the Unix split between short in-app help and fuller external reference. `F1` is the contextual task-local path, while exhaustive semantics/configuration detail remain the job of the manpage and generated usage/reference docs. Centralized authored help sources are acceptable for maintenance, but the runtime popup must still present only small contextual slices rather than a giant manual.
*   **Scope Contract:** Treat repeated operator rules as first-class shared topics rather than smearing partial explanations across directory/file/archive/F7/F8/Showall/Global/prompt pages. The shared inventory must be broad enough to cover the recurrent question families users actually ask, including filters, jump/list-jump behavior, wildcard and rename-pattern rules, search/fuzzy-matching semantics, command-line editing, `VI_KEYS=1`, theming/customization, tagged-set semantics, compare/output syntax families, and operator-facing tips/tricks where they truly generalize.
*   **Implementation Reality Contract:** This task is not text-only. It owns the runtime/help-popup behavior needed to make the authored IA real: contextual command rows must be able to open their owning shared topic or command explainer instead of trapping the user in local inline detail only, and the focused regression tests must be updated to validate the intended workflow rather than preserving the older limited model by accident.
*   **Navigation Taxonomy Contract:** Explicitly separate **help-popup navigation** (`Up`/`Down`, `PgUp`/`PgDn`, `Home`/`End`, `Enter`/`Right`, `Left`, `Esc`/`Q`) from **runtime ytnova navigation** (tree/file movement, jump, prompts, split movement, preview movement, tagged flows, etc.). `F1` must not blur those two domains together on the same first-screen explanation.
*   **Page-Shape Contract:** Each contextual page must answer, in order: where the user is, the main actions available here, the few caveats/traps specific to this surface, and which shared topic explains the deeper rule. Shared topics may then teach the reusable semantics once, with concise examples, without forcing every local page to repeat them.
*   **Prompt-Help Scope Contract:** Do not create prompt-local pages by default for every command prompt. Prompt `F1` must exist only where the prompt itself adds syntax, scope, wildcard, placeholder, or similarly non-obvious semantics that the user needs at input time. Otherwise the user must get the answer from the parent surface/shared explanation path instead of maintaining a second thin prompt page.
*   **Hint-Line Contract:** The help-popup hint line may expand beyond the current minimal trio when needed to keep the shared-topic structure discoverable, but it must remain low-noise and semantically honest. If Back, Contents, Navigation, list/detail switching, or other cross-topic movement is part of the intended help workflow, the hint line must advertise enough of that workflow that the user is not expected to guess it.
*   **Documentation Contract:** The governing principles for contextual `F1` structure, shared-topic ownership, and translator-facing editability must be spelled out in the specification and in a translator-facing guide rather than living only as roadmap intent.
*   **Acceptance Criteria:**
*   Local contextual pages stay task-local and do not try to re-teach full filter, jump, wildcard, search, or theme semantics inline.
*   Repeated operator semantics are reachable through stable shared topics from every relevant local page.
*   Help-popup navigation is documented as a separate concept from runtime ytnova navigation, and users can tell which rules belong to which layer.
*   Pressing `F1` on a covered surface never leaves the user without either the answer on that page or a clearly signposted next hop to the owning shared topic.
*   Prompt-heavy rule families such as filter syntax, wildcard rename targets, search semantics, and command-line editing are maintained once per shared topic family rather than duplicated ad hoc across unrelated pages.
*   Prompt-local help exists only where it is justified by non-obvious input semantics, and any prompt that owns `F1` advertises that fact in its footer hints.
*   When tags change a command's behavior, the local command page explains that tagged variant inline for the current surface instead of relying on the shared Tagged topic as the only explanation path.
*   The shared-topic inventory is reviewed as an explicit user-question matrix rather than as an author-memory list, so missing recurrent questions are tracked as coverage defects.
*   The help-popup hint line is validated against the final IA: it remains concise, but it is expanded where necessary so the new structure is discoverable without hidden gestures.
*   Contextual command rows that need deeper explanation have a real follow path at runtime to the owning shared topic or command explainer; authored `topic:` links are not allowed to exist only on paper while the popup remains trapped in local inline detail.
*   Shared semantics with multiple current owners are reconciled to one owning topic family. At minimum, split-model rules, wildcard/rename-target rules, jump/list-jump semantics, and search/filter semantics are not maintained independently across every local mode page.
*   Regression tests stop hard-coding the pre-43.4 discoverability model. Focused help tests must validate the chosen open/follow/back/close workflow and the final hint-line affordances instead of asserting the old minimal trio when the new IA requires more explicit cues.
*   Any "tips and tricks" guidance admitted into `F1` is curated as reusable operator guidance rather than leaking one-off editorial advice into arbitrary local pages.
*   - [x] **Status:** Completed.

#### **Task 44.5: Make Contextual F1 Help Accurate, Direct, and Task-Focused**
*   **Goal:** Make `etc/help/f1.en.md` and `etc/help/f1.de.md` reliable, plain-language contextual help for using YtreeNova now. `F1` help must explain the current screen, prompt, or action directly without turning into a manual page.
*   **Rationale:** Task 44.4 established the runtime/topic structure, but structure alone does not make `F1` trustworthy. Users still lose confidence when contextual help is vague, misnamed, overly indirect, inconsistently scoped, or inaccurate about what the program actually does. This task makes the authored `F1` help read like operator-facing help instead of rough implementation notes while also reconciling the terminology, parity, and narrow presentation fallout needed to make that help true and usable.
*   **Standalone Ownership Contract:** This task fully owns the remaining contextual-`F1` prose pass and the narrow supporting cleanup required to complete it. Do not defer necessary `F1` terminology, topic-label, locale-parity, help-only styling, generator, or focused test fixes to the already-completed Task 44.4. If a side issue must be corrected for `F1` help to be accurate, direct, distinguishable, or readable, it is in scope here.
*   **Source Boundary:** Edit contextual `F1` help only in `etc/help/f1.en.md` and locale equivalents. `etc/help/man.en.md`, generated manpages, and `docs/USAGE.md` are separate reference documentation and are out of scope unless the maintainer explicitly approves cross-boundary edits.
*   **Permitted Supporting Scope:** Narrow runtime, generator, theme-role, and focused test changes are allowed when required to support corrected `F1` behavior or presentation. This includes terminology cleanup, topic/title renames, locale inventory reconciliation, `help_topic` / `help_footer` / `help_keybind` presentation fixes, and small generation/runtime adjustments that keep the resulting popup truthful and readable. Broad theming work, reference-doc rewrites, or unrelated architecture changes remain out of scope.
*   **Terminology Contract:**
    *   `Footer` means YtreeNova’s always-visible bottom command strip.
    *   `Help strip` means the command row at the bottom of the `F1` popup.
    *   `Help Index` means the `F1` topic index. Its help-strip command is `I Index`.
    *   `F1 Navigation` means how to use the help popup and `Help Index`.
    *   `YtreeNova Navigation` means how to move through YtreeNova lists and screens. These are separate topics.
    *   `Help topic` means an `F1` page or destination.
    *   Theme role `help_topic` means visually distinct term/definition labels and inline backticked terms in `F1` where users need to identify or distinguish them.
*   **Writing Contract:** Start with what the user can do, in ordinary language. Prefer `Enter` or `Right` opens the selected link over unexplained bare `open`. Avoid self-narration, internal jargon, vague abstractions, and filler such as `this page explains`, `surface`, `familiar list navigation`, or `topic index`. Keep sentences short, concrete, and scannable under popup constraints. A local page should answer the immediate question first, note only the local exception that matters there, and point once to the deeper owning explanation if needed.
*   **Navigation/Usage Contract:** Do not repeat program-navigation rules on the `F1 Navigation` page. The `YtreeNova Navigation` page owns list/navigation-key explanation, including `/` jump, keyboard-first design, incidental mouse behavior, terminal key collisions, `Alt` portability, and where `Tab` is specific to `F8` or prompts. Local pages should state only the local consequence of those rules unless a local exception materially changes behavior.
*   **Information-Architecture Contract:** `Help Index` lists `Navigation`, which opens the `YtreeNova Navigation` topic, not `F1 Navigation`. `F1 Navigation` is reached from the help strip’s `Navigation` command. `Help Index` does not show its own `I Index` command. Each local page must answer: what this screen or prompt is for; what the user can do here; what local exception or trap matters; and where the one deeper explanation lives if needed. A reusable explanation has one owner. Local pages give only the local consequence and one direct link. Do not create prompt pages unless the prompt owns non-obvious syntax, scope, placeholders, wildcard behavior, or special controls that users need at input time.
*   **Accuracy Audit Contract:** For every factual statement in English `F1` help, verify it against implementation, specification, or intentionally approved behavior. Correct misleading keys, defaults, conditions, paths, terminology, scope, and command descriptions rather than smoothing the prose around them. Mirror each meaning change in German `F1` help. If a claim cannot be verified, remove it, correct it, or leave the task explicitly blocked on that unresolved point rather than guessing.
*   **Locale-Parity Contract:** English and German `F1` sources must keep the same topic IDs, context ownership, and link targets even when wording differs by language. Terminology may be localized, but help structure and destination mapping must remain aligned.
*   **Presentation Contract:** `F1` text must remain readable at narrow popup widths. Backticked terms and distinct term/definition labels that users need to identify must visibly use `help_topic`. Mnemonic-highlighted help-strip commands must visibly use configured `help_keybind` styling. Plain-text help-strip key-token labels such as `Enter/Right` and `Esc/Quit` must use `help_footer`. Help presentation must stay contextual and readable without depending on right-edge clipping or hard-coded color assumptions.
*   **Acceptance Criteria:**
    *   English and German contextual `F1` help is accurate, direct, and operator-facing rather than manual-like or implementation-narrated.
    *   Every changed factual claim in English `F1` help has been verified against code, spec, or intentionally approved behavior during implementation.
    *   Misleading or unverified claims are corrected, removed, or explicitly left as blockers; none are papered over with vaguer prose.
    *   English and German `F1` sources have identical topic IDs, context ownership, and link targets.
    *   `Help Index`, `F1 Navigation`, and `YtreeNova Navigation` use the intended terminology and remain clearly distinct.
    *   `Help Index` lists `Navigation` as the entry label for the `YtreeNova Navigation` topic, while `F1 Navigation` remains a separate help-strip destination.
    *   Local pages stay task-focused, and reusable explanations are owned once rather than half-repeated across multiple pages.
    *   Prompt pages exist only where prompt-specific syntax or controls justify them.
    *   Tagged or syntax-sensitive command variants are explained where they materially change what the user can do.
    *   `F1` help remains distinct from manpage/reference prose and does not require man-source edits to complete this task.
    *   Required help-only styling and terminology fallout is resolved, including correct `help_topic`, `help_footer`, and `help_keybind` usage where needed for readable `F1` presentation.
    *   The `F1` popup remains readable at narrow widths.
    *   `make help-assets`, `make`, and focused help/theme tests pass.
- [ ] **Status:** Not Started.

#### **Task 44.6: Bring Manpage/Usage Reference Prose in Line with Unix Manpage Conventions**
*   **Goal:** Make the authored manpage/usage reference text follow expected Unix manpage conventions for structure, tone, scannability, and level of detail instead of reading like an over-dense dump of command rows, repeated boilerplate, or mechanically projected popup content.
*   **Rationale:** Task 44.3 completed the source split so contextual `F1` help and the long-form reference no longer need to share the same prose. That split is only valuable if the reference side now uses its freedom well. When the manpage collapses large command sets into breathless paragraph slabs, repeats obvious one-line commands that the live UI already shows, or reads like a regurgitated footer listing rather than a curated operator reference, it becomes noisy, hard to scan, and hostile to actual use.
*   **Reference-Writing Principle:** The manpage and generated `docs/USAGE.md` must remain reference-oriented, but “reference-oriented” does not mean “dump every command into one undifferentiated block.” A manpage should follow expected Unix manpage conventions: compact, structured, scannable, and deliberate about what deserves space. It groups related operations coherently, highlights the few distinctions that matter, and omits redundant restatement of what the live surface already makes obvious unless that restatement is needed for offline reference value.
*   **Noise-Reduction Contract:** Do not preserve giant dense command-dump sections that flatten twenty commands into one paragraph-like blob of tiny summaries. If a command family is obvious, repetitive, or already visible in the UI, the manpage must summarize it at the right level of abstraction instead of expanding every row into low-value filler. The reference should spend its space on distinctions, scope rules, syntax, ownership, caveats, environment/configuration interactions, and operator-relevant behavior that would not be obvious from merely seeing the footer labels.
*   **Structure Contract:** Long-form reference sections must be broken into readable subgroups with clear headings, bullets, or similarly scannable structure. Adjacent commands that share the same model should be described together once, with exceptions or variants called out explicitly, rather than repeating nearly identical one-line definitions for each row. Where a mode page still needs a command inventory, that inventory must be curated and formatted for human scanning rather than emitted as a dense prose slab.
*   **Layering Contract:** The manpage must not merely mirror the popup help, and it also must not merely transcribe the footer command strip. The reference layer owns durable operational semantics, option syntax, scope rules, tagged-behavior variants, environment/config details, and cross-surface distinctions. It may mention the existence of `F1`, but it must not devolve into tutorial voice or popup-navigation chatter.
*   **Editability Contract:** `etc/help/man.en.md` must remain an intelligible authored document that maintainers can improve directly topic by topic. Any generator-facing structure must continue to serve human-edited reference writing rather than trapping authors in awkward repetitive formatting that encourages copy-pasted sludge.
*   **German Reference Source Follow-On:** Add `etc/help/man.de.md` as the German reference/manual source, maintained from canonical `etc/help/man.en.md` rather than from generated outputs. The English authored manual remains the primary source of truth for topic inventory, structure, and link targets; the German manual tracks it as a localized authored source in the same spirit as `etc/help/f1.de.md`.
*   **Acceptance Criteria:**
*   Mode/reference sections no longer read as giant undifferentiated command-summary slabs.
*   Repetitive command families are grouped and summarized at the correct abstraction level instead of expanded into low-value one-line noise.
*   The manpage and generated `docs/USAGE.md` emphasize operator-relevant distinctions, syntax, scope, and caveats over restating obvious footer labels.
*   Long-form reference prose is scannable with deliberate headings/bullets/grouping and remains readable in plain-text manpage output.
*   The reference text is clearly distinct in tone and structure from contextual `F1` popup help and from live footer command strips, and reads like a proper Unix manpage rather than a projected UI dump.
*   Topic-by-topic editing in `etc/help/man.en.md` remains straightforward for maintainers and translators.
*   `etc/help/man.de.md` exists as a localized authored manual source that tracks canonical `etc/help/man.en.md` instead of any generated artifact.
*   The resulting reference prose remains compatible with Task 44.3's split-source/generator contract and does not reintroduce pressure to collapse `F1` and manpage text back into one shared corpus.
*   - [x] **Status:** Completed.

### **Task 45: Replace `^F` Mode Cycling with Unified Numeric `FileInfo` Band (`1..9`)**
*   **Goal:** Replace display-mode cycling with direct numeric `FileInfo` controls for the focused panel.
*   **Behavior Contract:**
*   `1` => Name only (default/baseline). This is also the reset-to-default selection.
*   `2` => Attributes, including `name -> target` symlink rows in file projections.
*   `3` => Owner.
*   `4` => Times.
*   `5` => toggle compact Name/full-width file-window rendering when the current `1` / Name base view is active. This replaces the old `B` Brief toggle.
*   `6` => toggle file-size units (`binary` vs `human-readable`) for directory/file row surfaces.
*   `6` composes with the currently selected file-info mode and does not reset other file-info toggles. Stats remain human-readable.
*   `7` => toggle Mini preview text-snippet view.
*   `8` => toggle file-type/summary view.
*   `9` => Git-focused file-info band (status-oriented file view) when the current scope is inside a Git worktree.
*   `0` => currently unused; silent no-op.
*   Number keys are grouped by ownership/scope for the active panel:
    *   **Panel-wide toggles (dir + file projections):** `` ` `` dotfiles (existing behavior) and `6` size-unit toggle.
    *   **Shared-by-default display modes:** `1..4` change the active panel's current view, and tree/directory + file windows follow each other by default.
    *   **Base-view reset:** selecting `1..4` returns the named base view and clears temporary overlay/compact view states for that file projection.
    *   **Repeat-select reset:** pressing the already-active `2`, `3`, or `4` view key again resets that context back to `1` / Name.
    *   **Startup baseline:** panels always start in `1` / Name view; old `FILEMODE` profile lines no longer override startup.
    *   **Optional separate display modes:** `SEPARATE_DIR_FILE_VIEWS=1` restores split behavior so dir-focus changes only the dir/tree view and file-focus changes only the file-window view.
    *   **Compact normalization:** `5` only works from the current `1` / Name base view. It always uses the Name file projection and is a silent no-op from `2`, `3`, or `4`.
    *   **File-projection toggles:** `5`, `7`, `8`, and `9` never change tree rows. They affect the panel's file projection instead, so in tree focus they update the embedded small file window and in file focus they update the file window.
*   Not active in `F7` preview mode.
*   If a requested mode is unsupported in the active context (for example VFS file mode `4`, or `9` outside a Git worktree), do a silent no-op (no beep).
*   Git band (`9`) defaults to off, uses cached/non-blocking status refresh, and must not stall list rendering in large repos.
*   Add `FILE_SIZE_UNITS=binary|human-readable` profile setting (default `human-readable`) as the seed for `6`.
*   Add `SEPARATE_DIR_FILE_VIEWS=0|1` profile setting (default `0`) to switch between shared and split `1..4` panel views.
*   **Keybinding Policy:** Remove `^F` and `B` from runtime behavior and help/manpage docs. This task is the explicit keybinding-change exception referenced by Task 40 scope lock.
*   **UX/Help Policy:** Footer stays concise (`1..9 dir view` / `1..9 file view`) and no longer carries a separate `Brief` item; stats name the active `5` state as `Compact`; full key semantics live in F1 help/manpage. Unassigned `0` remains a silent no-op but is not advertised in the footer command band or F1 help.
*   **Spec/Docs Sync Policy:** When delivered, update `docs/SPECIFICATION.md` and `etc/ytnova.1.md` (and regenerated `docs/USAGE.md`) with the same grouped ownership contract.
*   - [x] **Status:** Completed.

### **Task 46: Add Case-Sensitive Sort Toggle + Profile Default**
*   **Goal:** Add case-sensitivity as a sort option in the existing sort flow and profile defaults.
*   **Rationale:** Users need deterministic lexical control without introducing extra global keybindings.
*   **Scope Lock:** Sort comparison behavior only; no tree/file model changes.
*   **Acceptance Criteria:**
*   Add `SORT_CASE_SENSITIVE=0|1` profile setting (default `0`) and wire it to default sort behavior.
*   Existing sort prompt (`S` flow) includes a case-sensitivity toggle.
*   Footer/F1/help/manpage text are synchronized for the new sort option.
*   - [ ] **Status:** Not Started.

### **Task 47: Input Loop Determinism and Event Handling**
*   **Goal:** Group event-priority policy and multiplexing implementation under one umbrella to reduce recurring input-loop regressions.

#### **Task 47.1: Input Loop Determinism and Event-Priority Contract**
*   **Goal:** Make key handling deterministic across ESC sequences, resize events, watcher events, and prompt/overlay contexts.
*   **Rationale:** Recurring regressions originate from event-order ambiguity, not raw key decoding alone.
*   **Scope Lock:** Input/event ordering, dispatch priority, and regression coverage only; no keybinding changes.
*   **Acceptance Criteria:**
*   Event-priority order is explicit and enforced for: resize, watcher refresh, ESC-sequence normalization, and key dispatch.
*   Prompt/overlay contexts must consume input according to innermost-active-context rules before base-mode dispatch.
*   No double-processing or dropped-event regressions on rapid resize + key + watcher activity.
*   Focused regression matrix covers ESC timing, resize storms, watcher bursts, and split/overlay transitions.
*   - [ ] **Status:** Not Started.

#### **Task 47.2: Non-Blocking FD Multiplexing Implementation**
*   **Task:** Implement/maintain non-blocking input multiplexing (`select`/`poll`) for keyboard + watcher FDs as the concrete mechanism under Task 47.
*   **Scope Lock:** Mechanism-level implementation only.
*   **Acceptance Criteria:**
*   Multiplex loop behavior conforms to Task 47 event-priority contract.
*   Regression coverage confirms no blocking/starvation under mixed input/event load.
*   - [ ] **Status:** Not Started.

#### **Task 47.3: Create Watcher Infrastructure (`watcher.c`)**
*   **Task:** Create a new module `watcher.c` to abstract the OS-specific file monitoring APIs.
*   **Logic:**
    *   **Init:** Call `inotify_init1(IN_NONBLOCK)`.
    *   **Add Watch:** Implement `Watcher_SetDir(char *path)` which removes the previous watch (if any) and adds a new watch (`inotify_add_watch`) on the specified path for events: `IN_CREATE | IN_DELETE | IN_MOVE | IN_MODIFY | IN_ATTRIB`.
    *   **Check:** Implement `Watcher_CheckEvents()` which reads from the file descriptor. If events are found, it returns `TRUE`, otherwise `FALSE`.
    *   **Portability:** Guard everything with `#ifdef __linux__`. On other systems, these functions act as empty stubs.
*   - [ ] **Status:** Not Started.

#### **Task 47.4: Implement Live Refresh Logic**
*   **Task:** Connect the `refresh_needed` flag to the main window logic.
*   **Logic:**
    *   In `dirwin.c` (`HandleDirWindow`) and `filewin.c` (`HandleFileWindow`), inside the input loop:
    *   Check `if (refresh_needed)`.
    *   **Action:**
        1.  Call `RescanDir(current_dir)`.
        2.  Call `BuildFileEntryList`.
        3.  Call `DisplayFileWindow`.
        4.  Reset `refresh_needed = FALSE`.
    *   *Note:* We must ensure the cursor stays on the same file if possible (by saving the filename before rescan and finding it after).
*   - [ ] **Status:** Not Started.

#### **Task 47.5: Update Watch Context on Navigation (Current-Directory Auto-Refresh Context)**
*   **Task:** Ensure the watcher always monitors the *current* directory so the file list the user is looking at stays fresh without a manual reload.
*   **Logic:**
    *   In `dirwin.c`: Whenever the user moves the cursor to a new directory (UP/DOWN), update the watcher.
    *   *Optimization:** Only update the watcher if the user *enters* the File Window (Enter) or stays on a directory for > X milliseconds?
    *   *Decision:* For `ytnova`, the "Active Context" is the directory under the cursor in the Directory Window, OR the directory being viewed in the File Window. In user-facing terms, auto-refresh should follow the current working view.
    *   **Implementation:** Call `Watcher_SetDir(dir_entry->name)` inside `HandleDirWindow` navigation logic (possibly debounced) and definitely inside `HandleFileWindow`.
*   - [ ] **Status:** Not Started.

#### **Task 47.6: Implement Directory Filtering (Non-Recursive)**
*   **Description:** Extend Filter to support directory-pattern tokens identified by a trailing slash.
    *   `dir/` means include matching directories in the current tree view.
    *   `-dir/` means exclude matching directories in the current tree view.
    *   Directory tokens can be combined with existing file-pattern tokens in the same filter spec.
    *   This logic is non-recursive and visibility-only: it affects what is shown in the current view, not internal directory state.
*   - [ ] **Status:** Not Started.

### **Task 48: Add Configurable Bypass for External Viewers**
*   **Goal:** Add a configuration option to globally disable external viewers, forcing the use of the internal viewer.
*   **UI Note:** If a future guided `F10` config panel lands, expose this there without replacing the existing raw-text config path.
*   **Rationale:** Provides flexibility for cases where the user wants to quickly inspect the raw bytes of a file (e.g., a PDF) without launching a heavy external application.
*   **Coverage Clarification:** This task also covers single-file `V` parity with tagged viewing: users must be able to choose internal vs external behavior consistently for both single-file view and tagged-view workflows.
*   - [ ] **Status:** Not Started.

### **Task 49: Implement Auto-Execute on Command Termination**
*   **Goal:** Allow users to execute shell commands (`X` or `P`) immediately by ending the input string with a specific terminator (e.g., `\n` or `;`), without needing to press Enter explicitly.
*   **Rationale:** Accelerates command entry for power users who want to "fire and forget" commands rapidly.
*   - [ ] **Status:** Not Started.

### **Task 50: Standardize Internal Viewer Layout**
*   **Goal:** Ensure the internal viewer's layout geometry matches the main application (borders, headers, and footer).
*   - [ ] **Status:** Not Started.

### **Task 51: Nested Archive Traversal**
*   Allow transparently entering an archive that is itself inside another archive.
*   - [ ] **Status:** Not Started.

---

## **Phase 5: Permanent Security Gates**
*This phase is an enforcement gate for security risk classes: audit baseline debt, then detect and block introduced/reintroduced security findings on every non-trivial change.*

### **Task 52: Security Risk Gate (Audit + Detect + Block)**
*   **Goal:** Add explicit QA and merge-gate enforcement that audits the current codebase for security risks and blocks new or reintroduced security findings.
*   **Scope:** shell-command construction and escaping boundaries, archive path trust policy, tempfile lifecycle, and unsafe API usage.
*   **Acceptance Criteria:** Security baseline audit evidence exists, recurring security checks are mandatory in `qa-all`/PR evidence, and merge is blocked on unresolved blocker/high security findings.
*   - [x] **Status:** Complete.

#### **Task 52.1: Baseline Security Debt Audit and Classification**
*   **Goal:** Run and document a focused baseline audit of current security risk classes already in scope for Phase 0.
*   **Deliverables:** findings inventory with severity, owner, disposition (fix now vs tracked debt), and explicit residual-risk notes.
*   - [x] **Status:** Complete.

#### **Task 52.2: Runtime Execution Security Guardrail**
*   **Goal:** Group runtime execution security hardening and guard expansion under one umbrella with mandatory staged completion.

##### **Task 52.2.1: Expand Security Guard Coverage to Block Reintroduction**
*   **Goal:** Ensure banned/legacy security-sensitive APIs and patterns are explicitly rejected by automated guard scripts.
*   **Mechanism:** Extend guard checks for legacy unsafe escaping/runtime paths and other approved denylisted APIs/patterns.
*   - [x] **Status:** Complete.

##### **Task 52.2.2: Standardize Runtime Process Launch Hardening (`fork` + `execvp` + `waitpid`)**
*   **Goal:** Make runtime command execution deterministic and secure by using one mandatory process-launch path in app runtime code.
*   **Policy (mandatory):** Runtime launches **must** use `fork()` -> `execvp()` -> `waitpid()` only.
*   **Non-Goal:** This task **must not** introduce `posix_spawn()`.
*   **Rationale:** Remove mixed execution behavior (`system()`/`popen()` vs direct child-process execution), reduce shell-injection surface, and stabilize terminal restore behavior.

*   **Scope:**
    *   Add one shared launcher module for runtime child-process execution.
    *   Migrate existing runtime call sites that currently use `system()`/`popen()` (including `system.c`, `print_ops.c`, `ctrl_file_ops.c`, and equivalent runtime paths).
    *   Preserve existing UX flow: ytnova remains active, launched command completes, control returns to ytnova, curses state is restored.

*   **Implementation Rules (mandatory):**
    *   Parent process remains ytnova; ytnova **must not** replace itself.
    *   Child process **must** execute target via `execvp()`.
    *   Parent **must** reap child via `waitpid()` using an `EINTR`-safe wait loop.
    *   No new runtime `system()` or `popen()` usage is permitted.
    *   Any temporary migration shim/wrapper **must** be removed before task closure (see Task 64).

*   **Acceptance Criteria:**
    *   All runtime command-launch paths use the shared `fork`/`execvp`/`waitpid` implementation.
    *   Zero runtime `system()`/`popen()` call sites remain in production runtime paths.
    *   Regression coverage proves:
        *   launched command runs and exits correctly,
        *   ytnova returns to interactive control after command completion,
        *   terminal/curses state is restored correctly after command return.
    *   QA guard fails CI if new runtime `system()`/`popen()` usage is introduced.
    *   Shim cleanup is complete per Task 64.
*   - [x] **Status:** Complete.

#### **Task 52.3: Security Regression Gate in CI + Merge Workflow**
*   **Goal:** Make security verification non-optional in routine change flow.
*   **Mechanism:** Require security gate evidence for non-trivial PRs and keep merge blocked until gates pass.
*   - [x] **Status:** Complete.

### **Task 53: Add Security Fuzzing Harness for High-Risk Input Paths**
*   **Goal:** Add fuzzing coverage (for example libFuzzer) for archive parsing and shell-command construction paths to detect malformed-input crashes and security-critical edge cases early.
*   **Rationale:** Complements static checks and regression tests with adversarial input exploration.
*   **Scope Lock:** Harness, seed corpus, and reproducible crash-minimization workflow only; no feature UX changes in this task.
*   **Acceptance Criteria:**
*   Reproducible fuzz targets exist for archive parsing and command-construction boundaries.
*   QA documentation defines how to run fuzz smoke jobs and triage crashes.
*   Findings flow into the existing security gate workflow.
*   - [ ] **Status:** Not Started.

---

## **Phase 6: Current Delivery Completion Queue**
*This phase is still current-delivery scope and contains implementation work that is planned to land.*

### **Task 54: Implement Advanced Batch Rename**
*   **Goal:** Add a ytnova-native batch rename flow for tagged files with numbering support, casing changes (`Tab`), substring replacement, and pattern-based keep/remove operations.
*   **Rationale:** Essential power-user feature for managing large file sets without forcing one-by-one rename loops.
*   **Preview/Apply Contract:** Batch rename is preview-first. Show `old -> new` results before mutation and support per-item apply controls: `y` (apply current), `n` (skip current), `a` (apply all remaining), `Esc` (cancel remaining).
*   - [ ] **Status:** Not Started.

### **Task 55: Unify Copy Semantics and Add Directory Sync (`Y`)**
*   **Goal:** Define one clear `Copy` contract (with optional ancestor-path preservation) and add a guided directory-sync flow from dir footer `Y`, backed by `rsync` where practical.
*   **User-Facing Behavior:**
    *   **Copy (file/tagged files):** Non-recursive single-item copy behavior is explicit and predictable.
    *   **Copy (directory/tagged directories):** Recursive copy behavior is explicit and predictable.
    *   **Preserve ancestor paths (option):** Uses the same copy selection as `Copy`, but destination path preserves ancestor-relative path from the operation base root (logged/selected source root, never `/`).
    *   **Dir-footer sync entry:** In directory context, `Y` opens sync flow with explicit source/destination, preview-first execution, and clear completion outcomes.
    *   **Mirror / one-way synchronize:** Treat the selected files or source tree as the source of truth. Copy new files, replace changed files, and optionally delete destination files that do not exist in the source selection.
    *   **Execution model:** Where practical, delegate recursive synchronize/update work to `rsync` rather than reimplementing tree-sync logic inside ytnova.
    *   **Source-scope policy:** Unlogged directories are excluded from copy source scope by default unless explicitly selected/logged by the user.
*   **Rationale:** Users need one coherent copy model (source-type-based semantics) plus a reliable repeat-backup workflow; rsync-backed execution reduces reinvention risk.
*   **Acceptance Criteria:**
    *   Prompt/help text makes `Copy` semantics explicit before execution: file sources are non-recursive; directory sources are recursive.
    *   `Preserve ancestor paths` is documented as a `Copy` option with base-root semantics relative to logged/selected source root (never filesystem `/`).
    *   Dir footer exposes `Y` as sync entry with footer/F1/manpage parity.
    *   Sync flow supports both one-off option edits and quick recall of recent/pinned sync presets.
    *   Source and destination roles are explicit; this is one-way synchronization, not bidirectional merge logic.
    *   Deletion of destination-only files is opt-in and clearly confirmed.
    *   Unlogged-directory default-exclusion behavior is explicit and documented.
    *   The synchronize path prefers `rsync` for plain filesystem paths and does not require ytnova to own a new recursive sync engine.
*   - [ ] **Status:** Not Started.

### **Task 56: Implement Applications Menu (`F9`) with Safe Default Presets**
*   **Goal:** Implement `F9` Applications Menu as a visible, contributor-friendly command surface with sensible default entries.
*   **Semantics:** Entries are user commands/templates (with optional placeholders/parameters) that execute commands; this is not keystroke recording.
*   **Default Presets (initial set):**
    *   `wget` fetch preset (option-heavy fetch with URL prompt).
    *   `ssh` connect-to-known-host preset.
    *   Format-convert preset (for example `ffmpeg` or `pandoc` template).
*   **Rationale:** Users need discoverable access to repeat-heavy external workflows that are not built-in file-manager actions.
*   **Acceptance Criteria:**
    *   `F9` exposes user-configurable entries and ships with documented default presets.
    *   `F9` application entries live in a dedicated user-editable applications catalog rather than sharing `commands.conf` with command-strip bindings.
    *   The Applications menu edit path opens that dedicated applications catalog surface.
    *   Default presets prioritize non-builtin external workflows with clear input placeholders and safe defaults.
    *   User-added commands are not restricted, including commands that duplicate existing builtins.
    *   Prompt/help/F1/manpage text documents `F9` basics and preset intent in contributor-friendly language.
    *   Default `F9` launch returns to ytnova immediately, and the launched application continues independently.
    *   Prompt/help/starter-catalog text explains `{}` and `{input}` in plain English and distinguishes `F9` presets from ad hoc `eXecute`.
*   - [x] **Status:** Completed. `F9` now loads a dedicated applications catalog, bootstraps editable starter presets, launches presets as independent applications from the active selection context, and documents the launcher-vs-`eXecute` workflow across help/manpage surfaces.

### **Task 57: Define Extension Surface Contract (`F9` Apps + `F7` Preview Plugins)**
*   **Goal:** Define one explicit extension contract for external-tool integrations so command apps (`F9`) and preview plugins (`F7`) follow the same safety, UX, and fallback rules.
*   **Scope:** Contract/spec-only delivery for external execution surfaces (`X`, `P`, `W`, `FILEDIFF`, `F9`, and `F7` preview-helper boundary).
*   **Rationale:** ytnova should reuse mature external tools without accumulating ad-hoc one-off behavior per feature.
*   **Acceptance Criteria:**
    *   Contract defines provider types (`app`, `preview`) and shared lifecycle semantics.
    *   Contract defines placeholder/token policy, argument safety rules, and bounded command construction.
    *   Contract defines deterministic completion/failure reporting and fallback behavior.
    *   Footer/F1/manpage wording aligns with the new contract language.
*   - [ ] **Status:** Not Started.

### **Task 58: Implement Shared Provider Registry (Plugin-Lite, External-Tool-First)**
*   **Goal:** Implement a shared provider registry/runtime for extension providers instead of isolated one-off paths.
*   **Non-Goal:** Do not add in-process arbitrary binary/plugin loading; providers remain external-tool adapters.
*   **Rationale:** A unified provider runtime keeps behavior predictable and lowers maintenance risk while preserving Unix-style composability.
*   **Acceptance Criteria:**
    *   Shared provider model supports at least `app` and `preview` provider classes.
    *   Common execution/safety controls are centralized (timeouts, output caps, exit-code mapping, fallback policy).
    *   Config/profile format is documented and validated with focused regression tests.
*   - [ ] **Status:** Not Started.

### **Task 59: Add Optional Background App Execution (`bg`)**
*   **Goal:** Allow selected external commands to run in background so users can continue navigating immediately.
*   **Entry Direction:** Prefer `F9` as the primary UX surface, with optional command-prompt parity where it fits cleanly.
*   **Scope Lock:** External commands/apps only (no async copy/move/delete queue in this task).
*   **Rationale:** This captures high-value "run and continue" workflow speed without requiring an embedded subshell model.
*   **Acceptance Criteria:**
    *   Users can launch an app in foreground or background using explicit UI choice/marker.
    *   Background job state is visible and queryable (running/success/failure) with actionable completion messaging.
    *   Failed background runs return clear diagnostics without destabilizing curses state.
*   - [ ] **Status:** Not Started.

### **Task 60: Implement F7 Preview Helper Pipeline (Promote Preview-Helper Pipeline into Current Delivery)**
*   **Goal:** Deliver the beta-scope F7 helper pipeline with strict fallback guarantees.
*   **Baseline Contract:** `BINARY` (internal preview, no helpers) and `RENDER` (helper-rendered output with guaranteed fallback to `BINARY` on failure).
*   **Scope Lock:** Ship the baseline safety/fallback pipeline now; defer optional advanced renderer ergonomics until later phases.
*   **Rationale:** This provides practical plugin-like preview extensibility while keeping ytnova's internal preview as the reliability floor.
*   **Acceptance Criteria:**
    *   `F7` supports deterministic `BINARY` <-> `RENDER` mode toggling with stable footer labeling.
    *   Helper execution is bounded and safe (argv-first execution, timeout, output cap, failure fallback).
    *   Panel-local mode state is preserved in split mode.
    *   Config/docs/tests are synchronized for the delivered baseline behavior.
*   - [ ] **Status:** Not Started.

---

## **Phase 7: Internationalization and Configurability**
*   **Goal:** Refactor the application to support role-based themes, localization, and user-defined keybindings, moving away from hardcoded English-centric values and colors.

### **Task 61: Establish Role-Based Theme System and Restrained Default Palette**
*   **Goal:** Define and implement a role-based color/theme system with a restrained orthodox-blue default theme, a bash-black alternate theme, plain-text user-editable theme storage, and safe foreground/background handling that prevents color bleed between themes.
*   **Priority:** High. The default visual identity will strongly affect first impressions, screenshots, user trust, and whether users perceive ytnova as focused or noisy.
*   **Design Direction:**
    *   There is one canonical default look: blue background, bright readable content, restrained structure, grey selection/dialog surfaces, and road-sign alert colors.
    *   Use color for structure, state, and exceptional meaning, not decoration.
    *   Default public theme should be a restrained classic blue TUI:
        *   blue main background;
        *   bright white for filenames, paths, dynamic values, keybindings, changing text, and tree guide lines;
        *   white for static labels, fixed captions, and stats titles;
        *   cyan on blue for panel borders, separators, dialog boxes, and window frames;
        *   black on light grey for selections, bars, and neutral dialogs;
        *   bright white on blue for informational text;
        *   black on yellow for warnings;
        *   bright white on red for errors;
        *   black on yellow for search hits or rare standout emphasis.
    *   Avoid frequent yellow, green, or bright/cyan-heavy text in the default theme. Loud colors must be rare.
    *   Keep UI chrome quieter than file/content text.
    *   Global/branch/showall mode should be indicated through panel titles, status labels, frame accents, or subtle markers rather than making ordinary filenames loud.
    *   The default theme should preserve the xtree/ztree-derived feel while avoiding rainbow-style orthodox file-manager visual noise.
*   **Role Granularity Direction:**
    *   Provide enough semantic roles to control important visual categories without requiring users to configure every widget separately.
    *   Prefer broad reusable roles over per-window/per-line duplication.
    *   Minimum roles:
        *   `background`: default application background.
        *   `box_lines`: panel borders, separators, dialog boxes, and window frames.
        *   `tree_lines`: tree guide glyphs; default follows dynamic/content text rather than border chrome.
        *   `margin`: tree/file margins and status marker columns; inherits `dynamic_text` unless explicitly set by the active theme.
        *   `static_text`: fixed labels/captions and text that rarely changes.
        *   `dynamic_text`: filenames, paths, counts, sizes, timestamps, current mode values, tree names, and file names.
        *   `keybind`: footer/menu keybinding characters.
        *   `selection`: active highlighted row/bar; selection may use inverse video or explicit colors, but if explicit colors are used both foreground and background must be configurable.
        *   `dialog`: neutral prompt/dialog surface.
        *   `picker`: selectable-list surfaces such as F2, history/completion lists, and the volume menu.
        *   `help`: F1/context-help reading surface; keep distinct from selectable pickers so help text does not inherit row-picker styling.
        *   `info`: informational road-sign color.
        *   `warning`: warning road-sign color.
        *   `error`: error road-sign color.
        *   `search_hit`: standout search/current-hit highlight.
    *   Do not add a separate `critical` role; fatal failures are generally outside useful interactive rendering scope.
    *   Allow advanced themes to override narrower roles later, but default theme files should stay short and readable.
*   **Theme vs File-Type Coloring:**
    *   Themes and file-type coloring are separate concerns.
    *   A theme defines semantic UI roles.
    *   File-type coloring is an optional content-decoration layer defined by the active theme.
    *   Any theme may define its own file-type coloring palette.
    *   If the active theme defines no file-type palette rules, all filenames use the theme `dynamic_text`/filename role.
    *   File-type colors must never assume a specific theme background or reduce readability on the active theme.
*   **File-Type Palette Format Direction:**
    *   File-type palettes belong to themes, so different themes can define different extension colors.
    *   If the active theme defines no file-type palette rules, all filenames use the theme `dynamic_text`/filename role.
    *   If an extension is not listed in the active theme palette, it uses the default filename color.
    *   Prefer named, compact grouped rules over one line per extension. Do not use a bare color line followed by a bare extension line; every rule must be self-contained and labeled enough to be understandable. The group name before `=` is for human readability; matching is driven by the style and selectors after `=`:
        *   `archives = red: tar,tgz,arj,taz,lzh,zip,z,Z,gz,bz2,deb,rpm,jar,rar,7z,iso,img`
        *   `scripts = +cyan: sh,bash,zsh,py,pl,rb`
        *   `archives = 1: tar,tgz,zip` is acceptable for numeric-color users, but named colors are preferred in shipped examples.
    *   The style side must accept the same color syntax as theme roles, including optional background:
        *   `red`
        *   `+red`
        *   `red on blue`
        *   `+cyan on black`
    *   When a file-type rule omits a background, inherit the active theme filename background but still resolve to a complete foreground/background pair internally.
    *   Rules list extensions without `*.` by default. Special selectors may include `LINK` and `EXEC`; directories in the tree use theme roles rather than file-type palette rules.
    *   Rules are evaluated top-to-bottom and the first matching rule wins. Put more specific rules before generic rules; for example, place `scripts = +cyan: sh,bash,zsh,py,pl,rb` before `executables = green: EXEC` if executable scripts should keep the script color rather than the generic executable color.
*   **Configuration Direction:**
    *   Do not preserve confusing legacy color-key names as the user-facing model. Temporary internal compatibility shims may be used only during the rewrite to keep staged changes testable, and must be removed or isolated before Task 61 closure.
    *   Add `grey` / `gray` support for dark grey. User-facing docs and comments must use `grey`/`gray`, not `bright black`.
    *   Add `+grey` / `+gray` support for light grey.
    *   Add a bright-prefix syntax such as `+red`, `+yellow`, `+white`, and `+grey`.
    *   Prefer canonical plain-text style syntax such as:
        *   `+white on blue`
        *   `white on blue`
        *   `cyan on blue`
        *   `black on +grey`
        *   `black on yellow`
        *   `+white on red`
    *   Move larger theme definitions out of the main config. The main config selects the active theme; the theme file defines named themes and each theme's file-type palette.
    *   Packaged default sources are `etc/ytnova.conf` and `etc/ytnova.themes`; runtime binaries must not consult `etc/` directly.
    *   Preferred config-family paths are `$XDG_CONFIG_HOME/ytnova/ytnova.conf` and `$XDG_CONFIG_HOME/ytnova/themes.conf`; when `XDG_CONFIG_HOME` is unset they fall back to `~/.config/ytnova/ytnova.conf` and `~/.config/ytnova/themes.conf`.
    *   Legacy fallback user paths are `~/.ytnova` and `~/.ytnova.themes` only when the XDG-style targets cannot be used.
    *   Future config-family catalogs such as bindings and labels follow the same `.../ytnova/` config-directory policy.
    *   Command history is session state, not config: prefer `$XDG_STATE_HOME/ytnova/ytnova.hst`, fall back to `~/.local/state/ytnova/ytnova.hst` when `XDG_STATE_HOME` is unset, and use `~/.ytnova-hst` only as a compatibility fallback or migration source.
    *   If the user theme catalog is missing, runtime loads packaged or compiled-in default theme data without creating `~/.config/ytnova/themes.conf`.
    *   The theme file may contain an unlimited number of named themes. Used theme definitions are uncommented; unused bundled or user themes can be prefixed with `#` to comment them out.
    *   The format should remain friendly to user edits and future contributed themes, such as light variants, beige themes, or alternate black-background themes.
*   **F10 Config Surface and Reload Direction:**
    *   `F10` opens a shallow configuration command surface, not a single hardwired raw-file editor.
    *   The command strip is exactly: `(C)onfig  co(M)mands  (T)hemes  (R)eload  (Esc)/(Q)uit`.
    *   Common path remains `F10 -> Enter -> edit config`; direct expert paths are `F10 -> C`, `F10 -> M`, `F10 -> T`, and `F10 -> R`.
    *   Reload is available only under `F10`; do not add a top-level/global reload key.
    *   Policy: `F10` edits the active user file for that surface (XDG or home-dotfile fallback); if runtime is using built-in defaults for that surface, `F10` creates the XDG file for that surface and edits it.
    *   Successful reload silently repaints using the new config/theme/commands state. Failed reload keeps the previous working config/theme/commands state and reports the parse/load error in the footer/status area only.
*   **Default Palette Direction:**
    *   `background = blue`
    *   `box_lines = cyan`
    *   `tree_lines = +white`
    *   `margin = dynamic_text`
    *   `static_text = white`
    *   `dynamic_text = +white`
    *   `keybind = +white`
    *   `selection = black on white`
    *   `dialog = white`
    *   `picker = black on cyan`
    *   `help = white`
    *   `info = +white`
    *   `warning = black on yellow`
    *   `error = +white on red`
    *   `search_hit = black on yellow`
*   **Implementation Direction:**
    *   Audit existing color options for duplicate severity aliases or unused compatibility entries.
    *   Audit `src/ui/color.c` and all window background/border drawing paths for reversed color-pair use, unintended `A_REVERSE`, foreground-only styling, and stale background attributes.
    *   Replace ad-hoc foreground-only coloring with complete role resolution where each rendered style resolves to a foreground and background.
    *   Ensure `cyan,blue` renders cyan glyphs on blue background, never blue glyphs on cyan background.
    *   Ensure panel borders and stats borders draw cyan line glyphs on blue background; they must not set the whole panel fill to cyan.
    *   Ensure stats titles render as white on blue and stats dynamic values render as bright white on blue.
    *   Split stats rendering roles explicitly: section titles and fixed labels use `static_text`/title styling, changing values use `dynamic_text`, and box lines use `box_lines`.
    *   Ensure tree and file names render as bright white on blue in the orthodox-blue theme.
    *   Ensure switching from black-theme coloring to blue-theme coloring cannot leave black-background or incompatible file-color attributes behind.
    *   Replace misleading menu/keybinding strings with token-aware rendering. The required F2 footer wording is `(L)og  (<)/(>) Cycle`; key tokens `L`, `<`, and `>` use `keybind`, while translated/descriptive text uses the surrounding role.
    *   Volume-menu keybindings must use the same integrated keybinding grammar as the rest of the UI; do not use detached labels such as `D Delete`. The volume-menu command strip is exactly: `Select (Up)/(Down)  Switch (Enter)  (Esc)/(Q)uit  (D)elete`.
    *   Prepare for Task 62 by storing menu/help entries as structured command labels plus key tokens, not as one translated display string. Example: command `COPY` has label `Copy` and key token `C`, allowing English `(C)opy`; a German keymap/locale can use label `Kopieren` and key token `K`, allowing `(K)opieren`, without translators editing raw punctuation to expose the shortcut.
    *   Map any leftover severity color aliases only through temporary migration paths and keep them out of the final user-facing theme model.
    *   Set window background once per refresh path and clear/redraw safely; avoid background changes inside per-row rendering loops.
    *   Keep file-type color application as a distinct optional layer after base theme role resolution.
*   **Theme Set:**
    *   Provide at least two built-in themes:
        *   `orthodox-blue`: restrained public/default theme.
        *   `bash-black`: power-user black-background theme with optional richer file coloring.
    *   Each built-in theme must carry its own file-type palette. A theme may intentionally define an empty file-type palette, in which case ordinary filenames use the theme filename/`dynamic_text` role.
    *   Future in-app theme editing should operate on semantic roles and the separate file-type coloring layer rather than exposing unrelated one-off color knobs.
*   **Out of Scope / Follow-On:**
*   Full modal-placement redesign is separate interaction work, but Task 61 must not introduce modal success/noise for theme/config reload. Successful theme/config reload must silently repaint without a success message. Non-obvious errors use footer/status text only. Destructive confirmations and real choice pickers may still use prompt/dialog surfaces until a later interaction task replaces them.
*   Narrower follow-on roles such as `help_link`, `help_link_selection`, or `shadow` may be added later for hyperlink help and modal chrome, but they still belong in the theme catalog path rather than `ytnova.conf`.
*   **Acceptance Criteria:**
    *   The default theme is readable, restrained, and suitable for screenshots.
    *   Normal filenames, tree names, tree lines, paths, keybindings, and dynamic values are bright white or otherwise high-contrast in the orthodox-blue theme.
    *   Static labels and stats titles are white or otherwise clearly readable in the orthodox-blue theme.
    *   Error text uses bright white on red.
    *   Warning and search-hit styling uses black on yellow.
    *   Yellow is reserved for warnings/search/rare emphasis and is not used for frequent ordinary states.
    *   File-type coloring can be omitted from a theme, leaving filenames on the default filename/`dynamic_text` role.
    *   Any theme can define its own file-type coloring palette.
    *   File-type colors do not bleed assumptions from one theme into another theme.
    *   `grey`/`gray`, `+grey`/`+gray`, and `+color` bright-prefix parsing are documented and tested.
    *   User-facing theme/config examples use `grey`/`gray`, never `bright black`.
    *   Runtime theme lookup uses XDG `themes.conf` first, then legacy `~/.ytnova.themes`; missing user theme catalogs are satisfied from packaged/compiled defaults without creating a user file, only --init / explicit edit flows create starter files.
    *   F2 shows `(L)og  (<)/(>) Cycle` with only key tokens styled as keybindings.
    *   F10 exposes `(C)onfig  co(M)mands  (T)hemes  (R)eload  (Esc)/(Q)uit`; reload is not exposed as a global/main-UI key.
    *   The volume menu shows `Select (Up)/(Down)  Switch (Enter)  (Esc)/(Q)uit  (D)elete` with only key tokens styled as keybindings.
    *   F1/context help uses the `help` role, while F2/history/completion/volume selectable lists use `picker`.
    *   Theme implementation proves foreground/background pair correctness.
*   `docs/SPECIFICATION.md` documents the user-visible theme/color contract.
*   `docs/ARCHITECTURE.md` documents the rendering/config invariants.
*   Legacy profile `[COLORS]` / `[FILE_COLORS]` parsing is not a runtime theme path; theme files are authoritative for semantic roles and file-type palettes.
*   - [x] **Status:** Complete.

#### **Task 61.1: Propagate Active Theme to Supported Terminal Helpers**
*   **Goal:** Propagate the active YtreeNova theme to supported terminal helpers so configured `EDITOR`, `PAGER`, and `TAGGEDVIEWER=external` flows can launch with a matching or near-matching color preset when the helper supports non-invasive startup theming.
*   **Rationale:** Task 61 made YtreeNova itself themeable, but external terminal helpers still break visual continuity when the main UI is blue-on-white and the launched helper falls back to unrelated defaults. Supported helpers should be able to follow the active theme without requiring users to hand-maintain per-theme shell startup hacks.
*   **Scope Contract:** This is an adapter/preset task for known terminal helpers, not a promise to theme arbitrary external commands or GUI applications. Unsupported helpers must continue to launch normally with no theme injection rather than receiving brittle guessed arguments.
*   **Launch Policy:** Prefer per-launch arguments, environment variables, temporary helper config files, or repo-managed wrapper/adaptor scripts that are selected by helper name/profile. Do **not** auto-edit persistent user shell startup files such as `.bashrc`, editor dotfiles, or pager rc files. Theme changes should apply on the next helper launch without a manual revert step because no persistent user config mutation occurred.
*   **Documentation Policy:** If a helper cannot be themed well through transient launch-time inputs alone, document an optional user-managed setup path in `etc/ytnova.1.md` / generated `docs/USAGE.md`, but keep that as opt-in guidance rather than automatic mutation. The docs must clearly distinguish between built-in transient presets and user-owned persistent helper customization.
*   **Tagged Viewer Contract:** `TAGGEDVIEWER=external` participates in this task when the selected external pager/helper is one of the supported terminal helpers. Pager-native behaviors such as hit traversal and search highlighting remain helper-owned unless a supported preset explicitly maps them; YtreeNova must not fight helper-native search-hit semantics just to force visual parity.
*   **Acceptance Criteria:**
*   At least the shipped supported-helper set for one editor family and one pager family (for example vim-like and less-like helpers) can be launched with a theme preset that tracks the active YtreeNova theme.
*   Active-theme changes apply on subsequent helper launches without requiring a revert pass through shell/editor/pager dotfiles.
*   Unsupported external helpers degrade safely to normal launch behavior with no broken command lines, no silent shell-dotfile edits, and no persistent side effects.
*   `TAGGEDVIEWER=external` uses the same supported-helper preset path when applicable and otherwise degrades safely to normal external launch behavior.
*   The manpage/usage docs explain the supported-helper contract, the non-invasive launch policy, and any optional user-managed helper setup for cases where transient theming is insufficient.
*   Focused regression or source-contract coverage proves helper theming is adapter-driven, opt-in by supported helper identity, and does not mutate persistent user shell/editor/pager startup files.
*   - [ ] **Status:** Not Started.

### **Task 62: Externalize UI Strings with GNU gettext (i18n Foundation)**
*   **Description:** Replace hardcoded user-facing strings with gettext-backed message lookups (`gettext`/`_()`), initialize locale/domain at startup, and add a standard catalog workflow (`.pot` -> `.po` -> compiled catalogs). Keep default locale as English while enabling translation packs.
*   **Documentation i18n split:** Use `po4a` for help/manpage/doc translation workflow from the canonical authored help sources defined by Task 44.3 (`etc/help/f1.en.md` and `etc/help/man.en.md`); generated man/usage/help assets stay derived artifacts. Use gettext for runtime UI surfaces (footer labels/help, prompts, status/error/info text) that are not emitted from the canonical help-source pipeline.
*   **Canonical-source contract:** `etc/help/f1.en.md` and `etc/help/man.en.md` remain the canonical authored English sources. Localized help/man content follows those English sources; localized runtime UI follows gettext catalogs; locale-specific mnemonic layouts follow shipped preset/keymap data.
*   **Keybinding token contract:** Translate human command labels only. Key tokens come from the active keymap and punctuation comes from the renderer. For example, English can render key token `C` + label `Copy` as `(C)opy`, while German can render key token `K` + label `Kopieren` as `(K)opieren`. Translators must not be required to preserve raw strings like `(C)opy` for shortcut visibility.
*   **Preset boundary:** Locale-specific command labels and mnemonic bindings that belong to packaged command presets remain data-owned by preset/keymap files (for example `etc/commands/*.conf`); gettext owns non-preset runtime UI prose.
*   **Translator workflow contract:** Contributors should be able to localize runtime UI by editing standard `.po` catalogs and localize authored help/man content by editing the canonical `po4a` inputs, without touching C source or renderer punctuation. Locale-specific mnemonic/keybinding layouts remain defined in shipped preset/keymap data rather than in gettext strings.
*   **Translator-guide contract:** Ship a translator-facing guide that explains the split between canonical help-source translation, runtime gettext strings, and command/keymap preset data; the guide must also explain how locale-specific mnemonic/keybinding conflicts are resolved in shipped presets/keymaps rather than by manually rewriting rendered shortcut prose.
*   **Runtime help-text organization:** Contextual `F1` help prose must come primarily from the canonical help-source pipeline in Task 44.3 rather than scattered ad-hoc C literals; remaining non-generated footer/prompt/help text must flow through a small predictable set of gettext contexts/catalog entries so translators can find related strings together and keep repeated explanations consistent.
*   **Current F1 quality debt:** The existing authored `F1` prose is structurally adequate for the pipeline but not yet at the intended quality bar. Preserve the canonical source/topic contract while allowing later prose rewrites to improve clarity, scannability, and usefulness without reopening the runtime/context mapping model.
*   **Translation path policy:** Define default translation discovery paths for system and user installs (for example system locale catalogs under `/usr/share/locale/.../LC_MESSAGES/ytnova.mo` with a user-level override path), and document contributor workflow for adding a language.
*   **Pilot locale:** Use the existing German (`de`) help/man sources and German command preset as the initial reference locale baseline, and complete the missing runtime gettext/catalog path so German becomes the first end-to-end supported non-English locale. New locales may use German as a structural example, but English remains the canonical source of meaning.
*   **Rationale:** For C/POSIX terminal software, GNU gettext is the most conventional and broadly understood approach. It has mature tooling, standard translator workflow, and broad ecosystem familiarity; a custom loadable language-file system would add avoidable maintenance and onboarding cost.
*   - [x] **Status:** Complete.

### **Task 63: Implement Configurable Keymap**
*   **Description:** Abstract all hardcoded key commands (e.g., 'm', '^N') into a configurable keymap loaded from a separate keymap profile file. The core application logic will respond to command identifiers (e.g., `CMD_MOVE`), not raw characters. This will allow users to customize their workflow and resolve keybinding conflicts.
*   **Sequencing dependency:** Implement after Task 43's portable footer keybinding/F1 wording cleanup. Prefer completing Task 47 parity gate first so keymap work lands on a stable footer/F1 contract.
*   **Config contract:** Select a keymap profile via `ytnova.conf` (opt-in). Locale-oriented profiles are allowed as explicit user choices, for example an English mnemonic profile can bind `C` to `Copy`, while a German mnemonic profile can bind `K` to `Kopieren` and `L` to `Löschen`. The shipped default keymap must remain portable and internally consistent, but compatibility with old confusing UI wording is not a reason to preserve that wording.
*   **Display contract:** Footer/help text must render active key tokens plus localized command labels together (for example active binding `C` + translated label `Copy` -> `(C)opy`) so runtime hints always match active bindings. Key tokens are data from the keymap, labels are data from localization, and punctuation/styling are renderer-owned.
*   **Legacy menu override contract:** The existing `[MENU]` text override only changes displayed text and does not change keyboard behavior. It may remain as an expert display override during migration, but it is not the final localization/keybinding model and must not be used as a substitute for real keymap-driven labels.
*   **Canonicalization/validation contract:** Normalize terminal byte aliases during keymap load (`^M`=`Enter`/`CR`, `^J`=`LF`/newline enter path, `^I`=`Tab`, `^[`=`Esc`) and reject profiles that map alias-equivalent inputs to different commands. Alias-equivalent inputs mapping to the same command are valid.
*   **Portability fallback contract:** Require workflow-level fallback for core actions (reachable without fragile terminal-specific modifiers). This is not a per-key duplication mandate; tagged/single-item variants may share menu/mode-driven paths when direct keyspace is exhausted.
*   **Behavior stability contract:** Default shipping keymap remains portable and stable; custom overrides are opt-in and must pass collision/unbound-action validation before activation.
*   - [ ] **Status:** Not Started.

---

## **Phase 8: Final Polish (Post-Alpha, Pre-v1.0.0)**
*This phase focuses on release polish. Security, module-boundary, and quality gates remain continuous from earlier phases and are not deferred to this phase.*

### **Task 64: Remove Temporary Compatibility Shims (Global Cleanup Gate)**
*   **Goal:** Eliminate temporary compatibility shims introduced during staged migrations and prevent shim accumulation as permanent architecture debt.
*   **Scope:** Applies to all migration tasks, including process-launch hardening and overlay/submode state unification.
*   **Policy (mandatory):**
    *   Temporary compatibility shims **must** be tagged at introduction with owner task ID and removal condition.
    *   Temporary compatibility shims **must** be removed when migration acceptance criteria are met.
    *   No temporary compatibility shim **may** remain in production paths after task completion.
*   **Acceptance Criteria:**
    *   A tracked shim inventory exists (file, symbol, owner task, removal condition).
    *   All shims owned by completed tasks are removed.
    *   CI/QA gate fails if orphaned/expired shim markers exist.
*   - [ ] **Status:** Not Started.

### **Task 65: UI/UX Snappiness Polish (Targeted Optimization)**
*   **Goal:** Improve perceived responsiveness in high-frequency flows using profiling-driven optimizations.
*   **Rationale:** Premature optimization is avoided; final polish applies targeted improvements where bottlenecks are measured.
*   - [ ] **Status:** Not Started.

### **Task 66: Source Comment Hygiene Pass**
*   **Goal:** Tidy comments for clarity and maintainability before v1.0.0.
*   **Policy:** Keep comments for invariants and design rationale; remove redundant narration of obvious control flow.
*   **Check:** Verify banner comments are only used where they add design/invariant context.
*   **Targeted K/A/R audit list (banner comments):**
*   **K (Keep):** Top-of-file purpose banners in `src/ui/display_utils.c`, `src/ui/stats.c`, `src/ui/ctrl_file_ops.c`, `src/fs/freesp.c`, and `src/fs/tree_read.c`.
*   **A (Add):** Missing top-of-file purpose banners in `src/ui/archive_payload.c` and `src/core/sort.c`.
*   **R (Remove):** Purely decorative in-body separator banners in `src/ui/display_utils.c`, `src/ui/ctrl_file_ops.c`, `src/fs/freesp.c`, and `src/fs/tree_read.c` when they do not state design/invariant context.
*   **Excluded:** Do not modify third-party `uthash.h`.
*   - [ ] **Status:** Not Started.

### **Task 67: Final Consistency Sweep (Style, Docs, UX Wording)**
*   **Goal:** Run a final consistency pass across style-sensitive surfaces (code style guardrails, docs wording, and help/footer terminology).
*   **Rationale:** Multi-contributor consistency is enforced continuously via guardrails and review; this task is a final convergence pass.
*   - [ ] **Status:** Not Started.

### **Task 68: Add Modal Window Shadows**
*   **Goal:** Add a restrained lower/right shadow treatment to modal windows so dialogs read as layered popups rather than flat border boxes.
*   **Rationale:** A subtle mc-style shadow gives visual depth and makes help/info/error dialogs easier to parse at a glance without changing modal behavior.
*   **Scope Lock:** Visual chrome only; no modal workflow, severity semantics, or keybinding changes.
*   **Acceptance Criteria:**
*   Modal/help/dialog surfaces can render a clipped lower/right shadow where terminal space permits, without obscuring modal content or corrupting underlying layout.
*   Shadow styling is theme-controlled (directly or through a dedicated semantic role) rather than hardcoded as one-off reverse-video tricks.
*   Focused rendering tests cover edge clipping and ensure shadow drawing does not bleed into non-modal surfaces.
*   - [ ] **Status:** Not Started.

### **Task 69: Multi-Round Adversarial Security Review**
*   **Goal:** Perform a pre-v1.0.0 multi-round security review using adversarial and AppSec perspectives.
*   **Examples:** Senior AppSec reviewer, penetration-tester mindset, and insider-knowledge threat modeling.
*   **Rationale:** Final pre-release pressure test on top of continuous Phase 2 security gates.
*   - [ ] **Status:** Not Started.

---

## **Beta: Stabilization and Performance**
*This phase follows alpha delivery phases and precedes wishlist work. Place stabilization tasks here: bug fixes, regressions, reliability, and performance. Defer non-essential feature work to wishlist phases.*

### **Task 70: Stabilize and Unify Overlay/Submode State Model (Compatibility-First)**
*   **Goal:** Make overlay/submode behavior deterministic by moving to one unified state model while preserving current user-visible behavior.
*   **Why now (Beta):** Split/mode/node state is explicit and stable, but overlay/submode behavior is still distributed across flags/controller paths.
*   **Precondition:** Current bug queue and planned current-delivery tasks are completed and green.

*   **Scope:**
    *   Add explicit enum-based `overlay_state` and `submode_state` fields to the authoritative runtime context.
    *   Unify state handling for:
        *   active overlay/context (normal/help/config/app-menu/autoview/fullview/diff/hexedit/destination-chooser),
        *   command submode (regular/tag/alt),
        *   overlay return/cancel chain behavior.
    *   Use compatibility-first migration with temporary adapters while preserving behavior parity.
    *   Migrate incrementally by context path; one-shot rewrite is out of scope.

*   **Scope Lock (mandatory):**
    *   No keybinding changes.
    *   No command-surface changes.
    *   No UX wording changes except correctness fixes required for parity.
    *   No new features.

*   **Post-migration Cleanup (mandatory):**
    *   Temporary compatibility shims **must** be removed once migration acceptance criteria are met.
    *   No compatibility shim may remain as permanent architecture.
    *   Shim cleanup is mandatory per Task 64 before closure.

*   **Acceptance Criteria:**
    *   One authoritative overlay/submode state path exists in runtime logic.
    *   Overlay entry/exit/cancel behavior is parity-validated for help, config, app menu, autoview, fullview, diff, hexedit, and destination chooser.
    *   Split behavior (`F8`/`Tab`) and active/inactive panel isolation remain unchanged.
    *   Existing split-panel/state-transition regression suites remain green.
    *   New regression coverage exists for overlay/submode transitions and cancel-chain restoration.
    *   Legacy overlay/submode flag/controller dispatch paths are removed from production paths.
    *   Zero compatibility shims remain for overlay/submode dispatch in production paths.
    *   `docs/ARCHITECTURE.md` is updated to document the unified model and migration endpoint.
*   - [ ] **Status:** Not Started.

---

## **Future Enhancements / Wishlist**
*Ideas that are not planned for inclusion at this stage. They are worth keeping a record of so they are not lost, but there is no promise or obligation to ever implement them. If you would like to take one on later, you are very welcome to do so.*
*IDs in this section use `Idea FE-*` and are explicitly non-priority/non-commitment markers.*

### **Future Phase 1: Post-Baseline Configurability Follow-On**

### **Idea FE-1: Optional Hidden-Child Restore on Re-Expand (`RESTORE_HIDDEN_CHILD=0|1`)**
*   **Goal:** Add an opt-in tree-navigation behavior that can restore the previously selected hidden child when a collapsed parent is re-expanded.
*   **Config Direction (`ytnova.conf`):** `RESTORE_HIDDEN_CHILD=0|1` (default `0`).
*   **Behavior Contract:**
    *   When `0` (default), keep current deterministic behavior: collapse invalidates child selection and selection remains at the fallback target (typically parent) after re-expand.
    *   When `1`, re-expand restores the last hidden child only if it still exists and is visible/valid; otherwise use deterministic fallback order (nearest ancestor, next/previous sibling, root visible node).
    *   Behavior applies to general tree navigation, not only split mode.
*   **Rationale:** Supports users who prefer sticky child selection after collapse/expand without changing default deterministic semantics.
*   - [ ] **Status:** Not Started.

### **Idea FE-2: Optional Re-log Destructive Warning Guard (`RELOG_WARN=0|1`)**
*   **Goal:** Keep default Unix-style relog behavior (execute immediately, no forced prompt) while adding an opt-in safety guard for users who want interruption before destructive relog resets.
*   **Config Direction (`ytnova.conf`):** `RELOG_WARN=0|1` (default `0`).
*   **Behavior Contract:**
    *   When `0` (default), current behavior remains unchanged: relog proceeds immediately and resets to the default view/state.
    *   When `1`, relog of an already-logged volume/path requires one explicit warning confirmation before execution.
    *   Warning flow mirrors existing delete-confirmation ergonomics (clear target context, default-safe cancel path, single prompt surface).
*   **Rationale:** Preserves Unix expectation that explicit commands run as requested by default, while offering an opt-in guard for users who may accidentally discard carefully curated logged-state context.
*   - [ ] **Status:** Not Started.

### **Idea FE-3: Explicit Accessibility Mode (Screen-Reader-First Terminal Behavior)**
*   **Goal:** Introduce an opt-in explicit accessibility mode focused on stable, low-noise behavior for screen-reader workflows.
*   **Research Gate (Required Before Implementation):**
    *   Audit current redraw/cursor-update hotspots (clock, spinner, status-line, dialogs, preview loops) for assistive-tech impact.
    *   Validate behavior with real screen-reader workflows (e.g., Speakup/NVDA terminal usage patterns) before locking UX contracts.
    *   Define measurable acceptance criteria (reduced cursor churn, reduced unsolicited announcements, no input-lag regressions).
*   **Implementation Direction (Post-Research):**
    *   Add a dedicated runtime/config toggle (not ad-hoc flags).
    *   Suppress/de-rate non-essential dynamic redraws in accessibility mode (e.g., spinners/timers) and prefer deterministic refresh cadence.
    *   Favor linear, prompt/result interaction paths where feasible; keep existing default behavior unchanged when mode is off.
*   **Rationale:** Terminal UI is not automatically accessible; explicit mode-level contracts are needed to avoid redraw/cursor noise regressions.
*   - [ ] **Status:** Not Started.

### **Idea FE-4: Portable Keyboard Capability Probe + `.ytnova` Key Workarounds**
*   **Goal:** Add startup-time terminal key-capability probing and user-configurable key overrides/workarounds in `~/.ytnova`.
*   **Behavior Direction:**
    *   Probe optional key availability once at startup (cache results; no per-keystroke probing overhead).
    *   Add explicit config overrides for problematic terminals/layouts so users can remap missing/ambiguous keys without code changes.
    *   Keep the default keymap stable and portable, with overrides as opt-in compatibility tools.
*   **Rationale:** Improves old-terminal portability while keeping runtime input handling fast.
*   - [ ] **Status:** Not Started.

### **Idea FE-5: Low-Risk Locale Expansion via Keymap/Docs Follow-On**
*   **Description:** Follow-up locale expansion beyond Task 11.5 that can land anytime without major UI architecture changes because it stays inside the current left-to-right ncurses model and the existing preset/help/manpage surfaces.
*   **Localized keymap profiles:** The core packaged-preset model is tracked by Task 11.5; any later work here must build on that shared action-based preset architecture rather than invent a second parallel keymap format.
*   **Can ship incrementally anytime:** These locales are mainly packaged-preset, label, help-text, manpage, and collision-validation work rather than new rendering architecture.
*   **Implementation order (widest practical audience first):** `es`, `fr`, `pt-BR`, then `it`, `ru`, `tr`, `pl`, `nl`, `id`.
*   **Scope examples:** Richer import/export tooling, advanced diagnostics UX, `F1` text updates, manpage/help updates, and migration notes for users or packagers.
*   **Best-practice guardrails:** Preserve a universal core of stable bindings (function keys/Ctrl/digits/arrows), allow locale mnemonic aliases where safe, and enforce strict collision/unbound-action validation with clear diagnostics.
*   - [ ] **Status:** Not Started.

### **Idea FE-6: Complex-Width Locale Readiness Before Higher-Risk Translation Expansion**
*   **Description:** Future work only. Current ytnova continues unchanged for now; do not partially reshape existing UI flows just to chase individual locale issues before a deliberate width-aware pass is planned.
*   **Due to difficulty with:** CJK width behavior, terminal cell accounting, compact labels, truncation/clipping policy, footer fit, prompt field rendering, and cursor positioning in mixed-width text.
*   **Need to do first:** Follow established terminal-application convention: make shared rendering/input surfaces width-aware by display columns rather than bytes, keep text storage separate from screen-geometry calculations, harden truncation/prompt/footer behavior centrally, and add compensating fixes plus regression coverage before shipping higher-risk non-RTL locales.
*   **Primary target locales after this work:** `ja`, `ko`, `zh-CN`, `zh-TW`.
*   **Rationale:** These locales do not require bidirectional UI, but they are still more likely than Latin/Cyrillic locales to expose layout assumptions that would otherwise cause regressions.
*   - [ ] **Status:** Not Started.

### **Idea FE-7: Bidirectional / RTL UI Capability Before Arabic and Hebrew**
*   **Description:** Future work only. Current ytnova continues unchanged for now; do not treat Arabic/Hebrew as ordinary translation work and do not auto-mirror the whole application unless later design work proves a specific surface should do so.
*   **Due to difficulty with:** Bidirectional ordering, punctuation in mixed RTL/LTR strings, prompt/edit-field cursor expectations, footer-strip ordering, truncation of mixed-direction text, and the question of whether any surfaces should mirror.
*   **Need to do first:** Follow established terminal-application convention: keep text in logical order, add display-time bidi handling with explicit base-direction rules for affected text regions, keep technical strings stable, and make prompts/help/footer surfaces bidi-safe before offering `ar` or `he`.
*   **Primary target locales after this work:** `ar`, `he`.
*   **Rationale:** Arabic and Hebrew are the highest-difficulty locales for current ytnova because the present UI is left-to-right and terminal-native bidi behavior is not something ordinary translation alone can solve.
*   - [ ] **Status:** Not Started.

### **Idea FE-8: Optional Guided Common-Options Config Panel**
*   **Goal:** Add an optional shallow guided editor for a small set of common options without replacing the current `F10` hub or the raw-text config/theme/commands authority.
*   **Behavior Contract:**
    *   `F10` keeps the current common path (`F10 -> Enter -> edit config`) and the existing config/commands/themes/reload hub.
    *   Any guided panel must stay shallow and strictly optional; it must not force menu-diving for users who prefer direct text editing.
    *   Raw-text files remain canonical for full fidelity, comments/examples, version control, and advanced edits.
    *   Guided edits must write back through the same split-surface files and preserve Task 11.2/11.5 structured command ownership.
*   **Rationale:** Leaves room for a friendlier common-options surface later without replacing the current Unix-style text-edit workflow.
*   - [ ] **Status:** Not Started.

### **Idea FE-9: Semantic F1 Help Styling Without Theme Bloat**
*   **Goal:** Allow authored `F1` help topics to request a small bounded set of semantic text styles while keeping `ytnova.themes` compact and stable.
*   **Dependency/Sequencing Note:** Evaluate this only after Task 44.4 has stabilized the help information architecture/content shape and Task 44.2 has settled the base help/footer surface-role contract. FE-9 is a follow-on enhancement for proven emphasis needs, not a prerequisite for making `F1` useful.
*   **Design Direction:**
    *   Use semantic markup roles in `etc/help/f1.en.md` rather than raw color/attribute requests.
    *   Initial role set should stay intentionally small (for example `help_text`, `help_key`, `help_code`, `help_heading`, `help_note`, `help_warning`).
    *   `ytnova.themes` maps those semantic roles to ncurses-supported attributes/colors.
    *   Missing theme entries must fall back deterministically to the normal help-text style.
*   **Non-Goal:** Do not allow arbitrary per-span foreground/background pairs or unlimited raw `bold`/`inverse`/`underline` directives directly in help source; that would balloon theme surface area and couple authored help text to presentation internals. FE-9 does not replace Task 44.2's surface-role ownership; it only adds a bounded semantic layer within already-settled help surfaces.
*   **Rationale:** Users may want richer help emphasis, but the safe path is a bounded semantic layer so help authors describe meaning and the theme decides appearance.
*   - [ ] **Status:** Not Started.

### **Future Phase 2: UI/UX Enhancements and Cleanup**

### **Idea FE-10: Configurable VCS Provider for `0` FileInfo Band**
*   **Goal:** Keep `0` as one stable VCS info band while allowing users to choose which backend powers it.
*   **Config Direction (`ytnova.conf`):** Add a single-provider selector (for example `VCS_PROVIDER=off|git|hg|svn|fossil|auto`).
*   **Behavior Contract:**
    *   Only one VCS provider is active at a time for `0`; no mixed multi-provider rendering in one view.
    *   Default remains off for performance/noise control.
    *   If the selected provider is unavailable in the current path/repo, `0` performs a silent no-op.
*   **Rationale:** Preserves key stability and avoids renumbering while keeping a path open for non-Git users.
*   - [ ] **Status:** Not Started.

### **Idea FE-11: Typed Filter Modes (`glob` default, `re:`, `fz:`)**
*   **Goal:** Extend file filtering with explicit typed terms while preserving today's glob-first behavior and key flow.
*   **User-Facing Behavior:**
    *   Keep existing glob syntax as default (`*.c`, `*.c,*.h`, `-*.tmp`).
    *   Add typed terms:
        *   `re:<expr>` for POSIX ERE regex.
        *   `fz:<text>` for simple fuzzy subsequence matching (case-insensitive).
        *   `glob:<pattern>` as explicit glob alias (optional but accepted).
    *   Keep exclusion semantics explicit and deterministic: exclusion matches always win.
    *   Matching target remains basename (`fe->name`) to preserve current expectations.
*   **Parsing/Validation Direction:**
    *   Support quoted terms so commas can be used inside a term (for example `re:"^x{1,3}$",*.c`).
    *   Treat malformed specs as invalid (for example `,,`, trailing comma, unmatched quote, empty `re:`/`fz:`/`glob:` term, or bare `-` term).
*   **UX/Help Direction:**
    *   Keep `FILTER:` prompt flow unchanged (`key -> Enter -> result`).
    *   Add lightweight inline hint text only (for example `glob(default) | re: | fz:`), without using `?` (reserved for backward search).
    *   Put full syntax/examples in `F1` help and manpage source (`etc/ytnova.1.md`).
*   **Rationale:** Adds regex/fuzzy power in a Unix-style, scriptable format without breaking existing wildcard workflows or adding submenu friction.
*   - [ ] **Status:** Not Started.

### **Idea FE-12: Prompt Input Decode Hardening (curses-first, legacy ESC fallback)**
*   **Goal:** Replace prompt-path manual ESC sequence parsing with curses/terminfo-first decoding, while keeping legacy manual ESC parsing as controlled fallback (or config-gated compatibility mode).
*   **Rationale:** Reduces xterm-specific assumptions in prompt entry and improves cross-terminal correctness on older UNIX environments.
*   - [ ] **Status:** Not Started.

### **Idea FE-13: Input Portability Regression Matrix (`TERM`)**
*   **Goal:** Expand UI regression coverage with a terminal-profile matrix and action-level assertions for keyboard behavior.
*   **Initial Matrix Target:** `xterm`, `vt100`, `screen`, `tmux`, `linux`.
*   **Rationale:** Existing UI tests prove behavior well in xterm-like sequences, but matrix runs provide stronger evidence for old/variant terminal compatibility.
*   - [ ] **Status:** Not Started.

### **Idea FE-14: Extended `sYsinfo` in Directory-Window Mode**
*   **Goal:** Add an on-demand extended stats/system-info surface (`sYsinfo`) for directory-window workflows without replacing the default compact stats panel.
*   **Rationale:** Advanced disk/system context is useful for planning operations, but should stay opt-in to avoid clutter in normal navigation.
*   **Keybinding Direction:** Keep context-specific `Y` behavior collision-free: directory-window `Y` may expose `sYsinfo`; file-window `Y` may expose sync workflow entry.
*   **Scope Lock:** Extended stats/sysinfo rendering and help/footer discoverability only; no copy/sync semantic redesign in this task.
*   **Acceptance Criteria:**
*   Extended stats view is reachable from directory mode and visually distinct from default stats.
*   Footer/F1/manpage wording explicitly documents context split where `Y` differs by mode.
*   - [ ] **Status:** Not Started.

### **Idea FE-15: Implement Mouse Support**
*   **Goal:** Add mouse support for core navigation and selection actions within the terminal (e.g., click to select, double-click to enter, wheel scrolling).
*   **Rationale:** In capable terminal environments, mouse support can improve speed and ease of use for navigation and selection without changing the keyboard-first design.
*   - [ ] **Status:** Not Started.

### **Idea FE-16: Configurable Split Header Path Display (`active` or `both`)**
*   **Goal:** Add a user option for split-mode header path display so users can choose active-panel-only path or both-panel paths.
*   **Rationale:** Active-only header is cleaner by default, while dual-path header can improve orientation for users managing two distant locations.
*   **Scope Lock:** Header display policy only; no split navigation, selection, or command behavior changes.
*   **Acceptance Criteria:**
*   Default mode remains `active` (current behavior).
*   Optional mode `both` renders left/right panel paths in split mode with deterministic truncation/clipping and no wrapping.
*   Active panel remains visually obvious in both modes.
*   Footer keybinding hints, F1 help, and config docs are updated when the option lands.
*   - [ ] **Status:** Not Started.

### **Idea FE-17: Prompt Path Entry, Shell-Style Completion, and ncurses-Native Input Editing**
*   **Goal:** Replace the current history-biased prompt input with a first-class path-entry workflow that is good enough for deep navigation, destination entry, and command prompts.
*   **Scope:** This task subsumes the previous separate ideas for shell-style tab completion, deep path jump, and advanced ncurses-native command-line editing.
*   **Behavior to Deliver:**
    *   **Shell-style completion:** `Tab` completes file and directory names in prompts instead of only recalling history.
    *   **Deep path entry and navigation:** Users can type or complete absolute paths, relative paths, and archive paths directly in a prompt (for example `/mnt/backups/../daily/2026-04-11/archive.tar.gz`) and jump there without changing `/` list-jump semantics.
    *   **Rich inline editing:** Full cursor movement (left/right, home/end, word-by-word), insert/delete/backspace, clear-to-start/end, and persistent prompt history accessible via arrow keys.
    *   **Prompt reuse:** The same editing/completion behavior should apply consistently to Log, Copy, Move, Rename, Filter, and command-entry prompts.
*   **Rationale:** Prompt entry should be strong enough that common path-based workflows stay direct: "type path -> complete/adjust -> Enter -> result" without forcing a separate browser/menu detour.
*   - [ ] **Status:** Not Started.

### **Idea FE-18: Tagged-Only Results View**
*   **Goal:** Add a tagged-only filter mode that shows only tagged files without altering the tag set itself.
*   **User-Facing Behavior:**
    *   In file lists, Showall, Global, and archive file views, users can open `Filter`, then press `Tab` when tagged files exist to toggle a **Tagged-Only** filter that temporarily narrows the visible list to currently tagged items.
    *   Leaving the tagged-only filter restores the normal file/filter view; tags remain unchanged.
    *   This composes cleanly with existing filters, grep-on-tagged workflows, and compare/tag workflows.
*   **Rationale:** After tagging, compare, or grep operations, users often want a focused "show me only the files I marked" result view instead of manually navigating through the full list.
*   - [x] **Status:** Completed.

### **Idea FE-19: Investigate Recursive Tagging vs Existing Showall/Global Workflow**
*   **Goal:** Determine whether recursive tagging provides enough real workflow benefit over the current `log dir -> Showall/Global -> tag` path to justify added complexity.
*   **Rationale:** Recursive tagging may reduce steps in some trees, but can also add command ambiguity and accidental broad-selection risk.
*   **Investigation Output:** Document concrete user workflows, interaction-depth impact, and safety tradeoffs; propose either (a) no change, or (b) a minimal, default-safe recursive tagging design with clear scope/confirmation semantics.
*   - [ ] **Status:** Not Started.

### **Idea FE-20: Richer Compare Result Views**
*   **Goal:** Extend compare workflows so the result can be viewed directly, not just turned into tags on the active side.
*   **User-Facing Behavior:**
    *   After comparing two directories/trees, users can narrow the result to categories such as **left/source only**, **right/target only**, **newer**, **older**, **size different**, **content different**, or **identical**.
    *   The result should be explorable as a list/view mode, not only as tag side effects.
    *   Compare output should stay explicit about which side is being shown and why an entry appears.
*   **Rationale:** Current compare behavior is useful but blunt. A richer result view makes compare a practical review tool rather than only a tag generator.
*   - [ ] **Status:** Not Started.

### **Idea FE-21: Recent-Directory Bookmarks and Pinned Favorites**
*   **Goal:** Add a first-class recent-directory and pinned-favorites picker for fast return to commonly visited locations.
*   **User-Facing Behavior:**
    *   Show a compact list of recently visited directories together with user-pinned favorites.
    *   Selecting an entry should log or activate that directory directly.
    *   Reuse existing log/history persistence where practical, but present this as a navigation surface, not merely raw prompt history text.
    *   Prefer a portable key or prompt hook (for example `F3`) over browser-style back/forward semantics unless a stronger need emerges later.
*   **Rationale:** Prompt history helps when the user remembers what they typed. A dedicated recent-directory/favorites list helps when the user remembers the place, not the exact command string.
*   - [ ] **Status:** Not Started.

### **Idea FE-22: Dual-Preview Split Mode**
*   **Goal:** Let `F7` from an `F8` split enter a stacked dual-autoview layout: each panel previews its selected file while retaining independent preview and list state.
*   **User-Facing Behavior:**
    *   `F8` continues to create the normal vertical two-panel split. Pressing `F7` from that split enters dual preview for both panels, changing the presentation to a stacked layout.
    *   Each preview uses its owning panel's selected file and preserves its own scroll position, selection, and return-to-list state.
    *   `Tab` switches the active preview panel without merging or resetting either panel's state.
    *   `F7` or `Esc` exits dual preview to the normal vertical split, restoring both list panels exactly as they were before preview.
    *   Active/inactive indicators make it unambiguous which preview will receive `Enter`, `Tab`, and `F7`.
*   **Rationale:** `J`/`FILEDIFF` is the direct file-comparison path. Dual preview is complementary: it makes logs, reports, and configuration files convenient to inspect side by side without repeatedly toggling state.
*   **Scope Lock:** This is an advanced split/preview state feature only. It does not require a broader orthodox-style layout redesign and should preserve ytnova's existing xtree/unixtree/ztree-derived interaction style.
*   **Acceptance Criteria:**
*   Both panels enter dual preview together, retain independent preview/list state, and do not leak state across the split boundary.
*   `Tab`, `F7`, `Esc`, and return-to-list behavior are deterministic and documented in footer/F1/manpage text.
*   Leaving dual preview restores the normal vertical split without altering either panel's selection, viewport, or focus state.
*   Split-panel active/inactive indicators remain unambiguous throughout dual preview.
*   Focused regression coverage proves layout transitions, per-panel state retention, panel switching, and exit/return behavior.
*   - [ ] **Status:** Not Started.

### **Idea FE-23: Directory-Focus Small-File Peek Navigation (`Shift` + Nav Keys)**
*   **Goal:** In directory focus, allow `Shift+Up/Down/Page/Home/End` to scroll the small file window for the selected directory without switching to full file-window focus.
*   **Rationale:** This gives a fast "peek and keep tree focus" workflow and mirrors the existing `Shift`-navigation feel used in `F7` preview.
*   **Scope Lock:** Directory-focus small-file-window navigation only; no new submenu flow, no change to normal unshifted tree navigation, and no change to `F7` preview behavior.
*   **Acceptance Criteria:**
*   In directory focus with the small file window visible, `Shift+Up/Down/Page/Home/End` moves the small-window file selection/offset deterministically.
*   Common path remains direct (`Shift+key -> immediate movement`) with zero submenu depth.
*   `/` list-jump remains directory-window scoped in this mode (no hidden mode switch of jump target).
*   If there are no files in scope, shifted navigation is a silent no-op (no modal/beep).
*   Footer/F1/manpage text clearly documents where shifted navigation applies.
*   Add focused regression coverage for shifted small-window navigation bounds/offset behavior and isolation from directory navigation.
*   - [ ] **Status:** Not Started.

### **Idea FE-24: Unified `N Create` Entry Point (Capability-Filtered by Backend)**
*   **Goal:** Replace the narrow `NewFile` entry point with a single explicit `Create` chooser whose available options are filtered by the active backend and context.
*   **User-Facing Behavior:**
    *   Where creation is supported, `n`/`N` opens `Create:` with only the actions that are valid for the active backend/context.
    *   In local filesystem contexts, the chooser should expose `Create: [f]ile [d]irectory [s]ymlink`.
    *   Pressing `Enter` at the chooser defaults to `[f]ile` when file creation is available.
    *   `f` preserves the current empty-file creation flow.
    *   `d` opens directory creation from the same top-level entry point where directory creation is supported.
    *   `s` opens a native symlink flow where symlink creation is supported, rather than relying on shell escape commands.
    *   Keep `M` as a direct `Make Directory` alias for backward-compatible speed; `N Create` becomes the canonical discoverable path.
    *   In file view, if a selected entry makes the symlink target unambiguous, prefer a shallow flow that prompts only for the link name/path; otherwise prompt explicitly for target and link destination.
    *   On read-only backends (for example ISO-style browsing), `N Create` does not appear at all.
*   **Rationale:** Keybindings are scarce. A one-submenu `Create` chooser is more discoverable and contributor-friendly than adding another top-level key or hiding mode switches inside a `MAKE FILE:` prompt, and capability-filtering keeps backend differences explicit instead of misleading.
*   **Scope Lock:** This task defines the common `N Create` entry point and capability-filtered option exposure. It does not require identical create semantics across all current or future backends. Hard-link creation stays out of scope unless a later roadmap item proves enough demand to justify the extra constraints and error handling.
*   **Acceptance Criteria:**
*   Footer/F1/help/manpage wording uses `Create` for the `N` entry point rather than `NewFile`.
*   Where creation is supported, the common path remains one submenu deep: `N` -> choice -> prompt -> result.
*   In local filesystem contexts, `Enter` at the chooser behaves as `f` and preserves current file-creation semantics.
*   `M` still performs direct directory creation.
*   The chooser shows only backend-valid create options; unavailable create types are omitted rather than advertised and rejected later.
*   Read-only backends expose no `N Create` entry in the footer keybinding hints or F1 help.
*   Symlink creation is available natively where supported, with explicit prompts and focused regression coverage for both selected-target and explicit-target flows.
*   - [ ] **Status:** Not Started.

### **Idea FE-25: Per-Window Filter State (Split Screen Prerequisite)**
*   Decouple the file filter (`file_spec`) from the `Volume` structure and move it into a new `WindowView` context. This architecture is required to support F8 Split Screen, enabling two independent views of the same volume with different filters (e.g., `*.c` in the left panel versus `*.h` in the right).
*   - [ ] **Status:** Not Started.

### **Idea FE-26: State Preservation on Reload (`^L`)**
*   Modify the Refresh command to preserve directory expansion states. Cache open paths prior to the re-scan and restore the previous view structure instead of resetting to the default depth.
*   - [ ] **Status:** Not Started.

### **Idea FE-27: Preserve Tree Expansion on Refresh**
*   Modify the Refresh/Rescan logic (`^L`, `F5`) to cache the list of currently expanded directories before reading the disk. After the scan is complete, programmatically re-expand those paths if they still exist.
*   - [ ] **Status:** Not Started.

### **Idea FE-28: Scroll Bars**
*   On left border of the file and directory windows to indicate the relative position of the highlighted item in the entire list (configurable to char or line).
*   - [ ] **Status:** Not Started.

### **Idea FE-29: Callback API Constification Cleanup (cppcheck strict mode)**
*   `cppcheck` suggests const-qualifying callback `user_data`, but doing this correctly likely requires changing callback typedef/API signatures (e.g., `RewriteCallback`) and related call sites. Defer this to a focused API pass to avoid scattered casts and partial churn.
*   - [ ] **Status:** Not Started.

### **Future Phase 3: Long-Horizon Experiments**

### **Idea FE-30: Implement VFS Abstraction Layer** (Use the Architect persona here)
*   **Goal:** Replace hardcoded filesystem logic with a driver-based architecture. This allows `ytnova` to treat any data source (Local FS, Archive, SSH, SQL) uniformly as a `Volume`.
*   **Context:** Currently, `log.c` decides between "Disk" and "Archive". We will change this so `log.c` asks a Registry: "Who can handle this path?"
*   **Follow-on Direction:** Include remote logging backends under this VFS model (FTP/SFTP candidates), with final protocol choice deferred until security and maintenance review.

### **Idea FE-31: Define VFS Interface & Volume Integration** (Use the Architect persona here)
*   **Goal:** Define the `VFS_Driver` contract (struct of function pointers) and update the `Volume` struct to hold a pointer to its active driver.
*   **Mechanism:**
    *   Create `include/ytnova_vfs.h`.
    *   Define function pointers: `scan`, `stat`, `lstat`, `extract`, `get_path` (for internal addressing).
    *   Update `include/ytnova_defs.h` to add `const VFS_Driver *driver` and `void *driver_data` to `struct Volume`.

### **Idea FE-32: Implement VFS Registry** (Use the Architect persona here)
*   **Goal:** Create the core logic to register drivers and probe paths.
*   **Mechanism:**
    *   Create `src/fs/vfs.c`.
    *   Implement `VFS_Init()` (registers built-in drivers).
    *   Implement `VFS_Probe(path)` which iterates drivers asking "Can you handle this?" and returns the best match.

### **Idea FE-33: Implement "Local" VFS Driver** (Use the Architect persona here)
*   **Goal:** Wrap the existing POSIX `opendir`/`readdir` logic into a `VFS_Driver`.
*   **Mechanism:**
    *   Create `src/fs/drv_local.c`.
    *   Move logic from `src/fs/tree_read.c` into the driver's `.scan` method.
    *   Ensure it populates `DirEntry` structures exactly as before.

### **Idea FE-34: Implement "Archive" VFS Driver** (Use the Architect persona here)
*   **Goal:** Wrap the existing `libarchive` logic into a `VFS_Driver`.
*   **Mechanism:**
    *   Create `src/fs/drv_archive.c`.
    *   Move logic from `src/fs/archive_read.c` and `src/fs/archive_write.c` into the driver.
    *   Implement `.extract` to handle the temporary file creation for viewing/copying.

### **Idea FE-35: Switch `LogDisk` to VFS** (Use the Architect persona here)
*   **Goal:** Update the main entry point to use the new system.
*   **Mechanism:**
    *   Refactor `src/cmd/log.c`.
    *   Replace the `stat`/`S_ISDIR` check with `VFS_Probe(path)`.
    *   Call `vol->driver->scan()` instead of calling `ReadTree` or `ReadTreeFromArchive` directly.

### **Idea FE-36: Refactor Consumers (Polymorphism)** (Use the Architect persona here)
*   **Goal:** Remove `if (mode == ARCHIVE)` from the rest of the codebase.
*   **Mechanism:**
    *   Update `view.c`, `copy.c`, `execute.c`.
    *   Replace specific calls with `vol->driver->extract(...)` or `vol->driver->stat(...)`.

### **Idea FE-37: Database Browsing and Editing via Virtual Filesystem Drivers**
*   **Goal:** After the driver-based VFS abstraction exists, allow ytnova to browse supported database formats as navigable virtual filesystems and eventually edit them through driver-defined operations.
*   **User-Facing Direction:** Treat a database as a structured volume (for example database -> tables -> rows/records or exported views) rather than as one opaque file blob.
*   **Rationale:** This is a specialized extension of the VFS model, not a core file-manager requirement. Keep it as a future experiment until a clear driver design and real use-case exist.
*   - [ ] **Status:** Not Started.

### **Idea FE-38: Implement Recursive Directory Watching**
*   **Goal:** Keep visible tree and file-list state fresh by watching all currently expanded filesystem directories, not only the active cursor directory.
*   **Rationale:** Without recursive watch coverage, edits in visible sibling/child directories can leave the UI stale until manual refresh.
*   **Scope Lock:** Filesystem watcher behavior only; no archive-internal recursive watching.
*   **Mechanism:**
    *   In `watcher.c`, maintain a `wd -> DirEntry*` map (for example via `uthash`) so events can be routed to the correct tree node.
    *   On `ReadTree` (expand), add watch descriptors for newly expanded directories.
    *   On `UnReadTree` / `DeleteTree` (collapse/free), remove corresponding watches immediately.
    *   On watch-limit failure (`ENOSPC`), degrade gracefully to active-directory-only watch mode without crashing or UI corruption.
*   **Archive Boundary:** For archives, watch the container file timestamp and trigger virtual tree reload; do not add recursive in-archive watches.
*   **Acceptance Criteria:**
    *   Expanded dirs update automatically when changed externally.
    *   Collapsing/removing nodes cleans up watches deterministically (no fd/watch leaks).
    *   `ENOSPC` fallback is explicit, stable, and non-fatal.
*   - [ ] **Status:** Not Started.

### **Idea FE-39: Implement Shell Script Generator**
*   **Goal:** Generate a shell script from tagged files using user-defined templates (e.g., `cp %f /backup/%f.bak`), replacing the "Batch" concept.
*   **Rationale:** Offers complex templating logic that goes beyond simple pipe/xargs, and critically allows the user to review/edit the generated script before execution for safety.
*   - [ ] **Status:** Not Started.

### **Idea FE-40: Keyboard Macros (F12 Record/Playback)**
*   **Goal:** Record and replay simple keystroke sequences.
*   **Rationale:** Useful for repeating safe, local interaction sequences.
*   **Status:** Deferred.
*   **Note:** Revisit only after a safe design exists that cannot turn traces into a secret-capturing scripting surface.

### **Idea FE-41: Enhance Built-In Viewer**
*   **Goal:** Evolve ytnova's internal viewer from a basic fallback inspector into a more capable built-in viewing tool for normal terminal workflows.
*   **Builds On:** Current-delivery viewer work such as `Add Configurable Bypass for External Viewers` and `Standardize Internal Viewer Layout`.
*   **Candidate Scope:**
    *   Stronger text viewing modes such as plain text, wrapped text, and hex/dump mode with consistent navigation.
    *   Better in-view search, jump-to-offset or jump-to-line behavior, and clearer file identity/status in the header/footer.
    *   Improved parity between single-file view, tagged-file view, and `F7` preview behavior where that makes sense.
    *   Optional lightweight conveniences such as line numbers, bookmarks, or simple gather/copy/export behavior if someone later proves the use-case.
*   **Non-Goal:** Do not turn ytnova into a native all-format viewer for images, PDFs, office files, multimedia, or GUI-centric content. External helper programs remain the preferred Unix-style answer for those cases.
*   **Rationale:** A stronger built-in viewer would make ytnova more self-contained for terminal inspection work, while still keeping the project focused on file management rather than format-specific rendering.
*   - [ ] **Status:** Not Started.

### **Idea FE-42: Investigate Optional Enhanced Terminal Input Protocols**
*   **Goal:** Investigate whether opt-in enhanced keyboard/input protocols can safely improve ytnova's TUI input model without replacing the portable baseline path.
*   **Input-protocol spike:** Start with kitty keyboard protocol and evaluate whether richer key events can distinguish collided control inputs such as `^M` versus `Enter`.
*   **Fallback contract:** If enhanced keyboard negotiation is unavailable, rejected, or stripped by the active terminal path, keep the current portable bindings and help semantics (for example `^N` for tagged move) rather than making any enhanced protocol a requirement.
*   **Scope boundary:** Treat this as an optional capability layered above the normal terminal path, not as a prerequisite for core navigation or command workflows.
*   **Rationale:** This is an input-capability investigation intended to determine whether optional terminal features can relieve current control-key collisions while keeping ytnova portable.
*   - [ ] **Status:** Not Started.

### **Idea FE-43: Investigate Replacing ncurses with a Better TUI Backend**
*   **Goal:** Investigate whether ytnova should replace or meaningfully decouple from ncurses in favor of a better TUI/runtime layer while preserving current interaction semantics.
*   **Investigation scope:** Evaluate candidate backends on portability, rendering/control over redraw behavior, input handling, testability, packaging friction, and migration risk for the current architecture.
*   **Compatibility contract:** Any replacement path must preserve the portable baseline terminal workflow and must not require a single terminal family or GUI-specific runtime stack.
*   **Rationale:** This is a platform/runtime architecture effort intended to determine whether ncurses remains the right long-term foundation for ytnova's TUI.
*   - [ ] **Status:** Not Started.

### **Idea FE-44: Implement "Safe Delete" (Trash Can)**
*   **Goal:** Add optional trash-backed delete where the active filesystem/backend supports it.
*   **Config:** Add a `ytnova.conf` switch for trash-delete with default `1` (enabled).
*   **Fallback:** If trash-delete is disabled or unsupported for the active backend, use permanent delete with explicit confirmation.
*   - [ ] **Status:** Not Started.

### **Idea FE-45: Port to other platforms**
*   **Validation:** Currently practical via WSL and QEMU
*   **Possible:** OmniOS (illumos), GNU Hurd, FreeBSD
*   **Possible but impractical for maintainers right now:**  macOS, AIX, OpenVMS, Solaris, Redox OS
*   **Out of scope:** Windows (ZTreeWin exists), legacy UNIXes including HP-UX
*   - [ ] **Status:** Not Started.

### **Idea FE-46: Forward Text Search in Contextual F1 Help**
*   **Goal:** Add a `/query` search to contextual `F1` popups that moves to the first matching rendered help text, including ordinary words that are not links.
*   **Design Requirements:** Search input, cancellation, backspace, no-match feedback, link-selection interaction, and locale/UTF-8 matching must be designed and covered by focused tests.
*   **Rendering Requirement:** Search must update the existing popup without closing/reopening it or causing redraw flicker.
*   **Scope Lock:** Future work only; current contextual-help behavior remains unchanged until this feature has a complete design and implementation.
*   - [ ] **Status:** Not Started.
