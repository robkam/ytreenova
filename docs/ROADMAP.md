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

### **Task 2: Add Inline `Shift+N` Create-Link Flow (Symlink/Hardlink)**
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
*   Update `etc/help/man.en.md` and regenerate `docs/USAGE.md` (`make help-assets`) when behavior lands.
*   - [ ] **Status:** Not Started.

### **Task 3: Manual File-Column Width Controls (`[` Narrower, `]` Wider, `{` / `}` Reset)**
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

### **Task 4: Adjustable List/Preview Width in `F7` Mode**
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

### **Task 5: Progress Indicators for Copy/Move/Delete/Archive Workflows**
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

#### **Task 5.1: Keep Progress Indicators from Clobbering Footer/Prompt/F1 Guidance**
*   **Goal:** Preserve footer, prompt, and `F1` help ownership while long-running operations update progress/spinner state.
*   **Rationale:** Help/trust regressions are not limited to static wording; progress rendering that overwrites guidance surfaces creates the same "UI is lying to me" failure mode during active work.
*   **Related Bug:** `BUG-6` — progress spinner can overwrite footer keybinding/prompt/F1 surfaces.
*   **Acceptance Criteria:**
*   Progress updates render in a dedicated non-obtrusive status surface and never overwrite active footer/prompt/F1 guidance.
*   When `F1` help is open, progress state degrades gracefully to a compact indicator or deferred repaint rather than seizing the help surface.
*   Focused regression coverage proves long-running operations cannot blank or corrupt contextual guidance surfaces.
*   - [ ] **Status:** Not Started.

### **Task 6: Redraw Coherence**
*   **Goal:** Ensure all related redraw-synchronization work ships under one coherent umbrella with deterministic scope boundaries.

#### **Task 6.1: Unify Stats + Main-Pane Frame Redraw Contract**
*   **Goal:** Eliminate intermittent split-brain rendering where stats and main panes update on different redraw lifecycles.
*   **Rationale:** UI trust depends on one coherent frame; partial redraw divergence creates stale/corrupted mixed states.
*   **Scope Lock:** Rendering/invalidation pipeline and regression coverage only; no command/keybinding semantics changes.
*   **Acceptance Criteria:**
*   Stats, path, dir, and file surfaces are drawn from one frame/layout snapshot and flushed in one update cycle.
*   Resize, mode-switch, and recoverable-error paths trigger deterministic full-surface invalidation and redraw.
*   No persistent mixed state where stats is fresh while main panes are stale (or vice versa) after redraw-triggering actions.
*   Add focused regression coverage for redraw coherence across resize/mode toggles and representative recovery paths.
*   - [ ] **Status:** Not Started.

#### **Task 6.2: Footer-Aware Redraw Synchronization Contract**
*   **Goal:** Footer/help/prompt surfaces must participate in the same redraw contract as stats/path/dir/file panes.
*   **Rationale:** Partial redraw of guidance surfaces creates trust loss even when content panes are correct.
*   **Scope Lock:** Redraw ordering and invalidation only; no keybinding or command behavior changes.
*   **Acceptance Criteria:**
*   Footer/help/prompt are rendered from the same frame snapshot as content panes.
*   Resize and mode transitions must not leave footer keybinding/F1 surfaces stale relative to the active context.
*   Focused regression coverage proves synchronized redraw across normal, split, and overlay transitions.
*   - [ ] **Status:** Not Started.

#### **Task 6.3: Unify Main-Screen Frame and Junction Ownership**
*   **Goal:** Replace patched line-drawing fixes with one architecturally clean main-screen frame-rendering contract.
*   **Rationale:** The current defect family is not just "wrong glyph here or there"; it comes from fragmented border ownership across layout code, stats rendering, preview-family rendering, and transition-time redraw helpers. As long as multiple paths can write the same seam cells, missing or overwritten junctions will keep returning in new layout combinations.
*   **Relationship to Task 6.1:** Task 6.1 aligns redraw timing; this subtask aligns frame ownership so the synchronized redraw has one authoritative border/junction source.
*   **Ownership Rule (mandatory):** The frame compositor owns every outer-border cell, divider cell, split-separator cell, stats-touching border cell, and every junction-bearing seam cell. Non-frame renderers may draw interior content and renderer-local internal separators only; they must not paint shared frame/seam cells.
*   **Scope Lock:** Main-screen frame composition, seam ownership, and regression coverage only; no keybinding, command-surface, or theme-design changes.
*   **Acceptance Criteria:**
*   One authoritative render owner chooses all main-screen frame glyphs, including the outer box, dir/file divider, split separator, stats-touching borders, preview-family frame seams, and all top/middle/bottom junctions.
*   No non-frame renderer paints shared frame/seam cells.
*   The Task 6.3.x subtasks land without introducing keybinding, command-surface, or theme-design drift.
*   - [ ] **Status:** Not Started.

##### **Task 6.3.1: Canonicalize Main-Screen Geometry**
*   **Goal:** Define one authoritative geometry model for the outer frame, dir/file divider, split separator, stats column boundary, preview-family border seams, and every junction-bearing seam cell before glyph selection occurs.
*   **Acceptance Criteria:** All main-screen border-bearing cells are derived from one shared layout model rather than recomputed independently by multiple render paths.
*   - [ ] **Status:** Not Started.

##### **Task 6.3.2: Introduce a Unified Frame Compositor / Junction Resolver**
*   **Goal:** Choose main-screen border glyphs from declarative edge connectivity instead of scattered imperative `ACS_*` writes.
*   **Mechanism:** Add one frame-composition path that resolves top/middle/bottom junctions, corners, and straight runs from the canonical geometry model and applies unchanged across single, split, and preview-family layouts.
*   - [ ] **Status:** Not Started.

##### **Task 6.3.3: Remove Shared Seam Ownership from Non-Frame Renderers**
*   **Goal:** Restrict stats and other non-frame renderers to interior content and renderer-local internal separators so frame-touching seam cells have one owner.
*   **Acceptance Criteria:** No shared seam cell is written by both the main layout/frame path and any non-frame renderer, including stats and preview-family content renderers.
*   - [ ] **Status:** Not Started.

##### **Task 6.3.4: Remove Transition-Time Border Fragment Repaints**
*   **Goal:** Eliminate mode/layout helpers that repaint border fragments directly instead of requesting a full recomposition from the frame owner.
*   **Acceptance Criteria:** Mode/layout transitions do not paint ad-hoc border fragments outside the unified frame-render path.
*   - [ ] **Status:** Not Started.

##### **Task 6.3.5: Add Seam-Family Regression Coverage and Final Ownership Documentation**
*   **Goal:** Prove the new ownership model across the full seam family and document the final architectural boundary.
*   **Acceptance Criteria:**
*   Focused regression coverage proves seam correctness across left-only/right-only/both/none stats combinations and representative small/large terminal geometries.
*   Single, split, and preview-family layouts use the same junction-resolution mechanism.
*   `docs/ARCHITECTURE.md` documents the final ownership boundary: frame composition owns shared border/junction cells, while non-frame renderers own interior content and renderer-local internal separators only.
*   - [ ] **Status:** Not Started.

### **Task 7: Add `Catalog` Output Mode to `Write`**
*   **Goal:** Extend the existing `Write` format dialog with a `Catalog` mode that exports a deterministic file/directory inventory (similar intent to `ls -1pR`) instead of file contents.
*   **Rationale:** Users need an in-app way to generate list/report output to command or file without dropping to shell-specific workflows.
*   **Scope Lock:** Add format behavior only; do not define or change keybindings in this task.
*   **Acceptance Criteria:**
*   `Write` prompt includes `Catalog` alongside existing formats.
*   Catalog output can be sent to command or file via existing `Write` destination flow.
*   Output contract is documented (recursion rules, directory markers, ordering, archive behavior).
*   Focused regression tests cover at least one filesystem case and one archive case.
*   - [ ] **Status:** Not Started.

### **Task 8: Persist Compare Mode Presets and Modal Contract**
*   **Goal:** Build on the Enforce One-Level Primary Action Depth (Prompt-Chain Audit) compare-flow baseline by making compare mode remember last-used settings and enforce a durable modal interaction contract.
*   **Rationale:** After compare prompt-chain compression is established, compare still needs stable option memory, strict modal ownership, and consistent compare-only key behavior to stay fast and predictable in repeated use.
*   **Dependency:** Sequence after Reconcile Shallow-Flow Contract Against Spec and after the Enforce One-Level Primary Action Depth (Prompt-Chain Audit) compare-family remediation subtask establishes the compressed compare flow baseline. If compare-mode state ownership still depends on split-panel restore/state work, land this after Task 11.
*   **Scope Lock:** Compare mode behavior only after the compare-family shallow-flow remediation exists; no fresh compare prompt-chain redesign here and no unrelated footer or global keybinding redesign.
*   **Acceptance Criteria:**
*   Compare remains a modal state and the compare footer keybinding/F1 surface owns the footer while active.
*   In compare mode, only compare keys are active; non-compare keys are silent no-ops.
*   No conflicting quick-key mappings are permitted in compare mode.
*   The compare flow inherited from Enforce One-Level Primary Action Depth (Prompt-Chain Audit) remains the baseline; this task must not reintroduce deeper prompt chains or bypass explicit compare target confirmation.
*   Persist last-used compare options across restart; config values seed defaults and runtime usage updates remembered defaults.
*   Default behavior remains unchanged when quick/preset config is absent.
*   Compare-mode prompt/menu/help surfaces stay synchronized with the persisted option model and modal key contract.
*   Add focused regression coverage for compare behavior and split-panel isolation.
*   Update compare docs/help text in `etc/help/man.en.md` and regenerate `docs/USAGE.md`.
*   - [ ] **Status:** Not Started.

### **Task 9: Add Recursive Directory Compare in `J` Flow**
*   **Goal:** Support recursive directory-tree compare from the existing `J` compare flow.
*   **Rationale:** Recursive compare is a practical file-manager workflow and improves alpha usefulness for real tree-diff tasks.
*   **Scope Lock:** Add recursive compare capability and prompt/menu wiring only; do not redesign unrelated compare UI.
*   **UX Direction:** Keep `J` as the compare entry point. Use one submenu level or a direct compare prompt with explicit recursive choice (`Recursive: y/N`).
*   **Acceptance Criteria:**
*   Recursive and non-recursive directory compare are both available from the same `J`-entry compare flow.
*   The recursive choice is explicit and discoverable in compare prompts/help.
*   Compare target confirmation and split-panel isolation behavior remain unchanged.
*   `etc/help/man.en.md` and generated `docs/USAGE.md` are updated when behavior lands.
*   - [ ] **Status:** Not Started.

### **Task 10: Integrated File Comparison View**
*   **Goal:** Make `J` provide an in-app, read-only file-comparison view rather than requiring a separate interactive comparison utility.
*   **User-Facing Behavior:**
    *   `J` retains its existing target-selection flow, then opens an integrated side-by-side comparison of the selected source and target files in two independently framed boxes with their full identities in the headers.  A narrow non-content gutter between the boxes shows aligned-line relationship markers; it is not a shared border.
    *   The comparison opens side-by-side by default.  A session-only, context-local orientation action switches the two framed boxes to stacked and back without changing target identity, selected hunk, shared aligned scroll position, or focused comparison row.
    *   A terminal too narrow for the current orientation produces a clear outcome without silently changing orientation; the user may choose the other orientation or exit.
    *   The view makes additions, removals, changes, equal regions, file identities, navigation, and exit behavior unambiguous; it never changes either file.  Its navigation follows established ytnova viewer/edit navigation rather than introducing a compare-specific navigation model.
    *   The footer exposes only next/previous difference, orientation, contextual `F1` help, and quit actions in the first delivery.
    *   Directory and tree comparisons retain their existing result/tag and configured external-helper contracts unless separately redesigned.
*   **Implementation Contract:** Use a maintained, well-known system diff provider as the comparison engine (with `diff -u` as the project baseline) and render its result inside ytnova.  Invoke the provider through `fork`/`exec` with an argument vector and absolute source/target paths; use `--` only when the selected provider documents support for it.  Do not vendor or invent a diff algorithm.
*   **Provider and Data Contract:** Initially accept only bounded regular text files.  Define limits for input bytes, captured provider output bytes, parsed hunk/line count, and renderable pane width.  Binary or NUL-containing input produces the explicit `Non-text file` outcome and does not open a comparison view.  Parse unified-diff file headers, hunk headers, and hunk-body prefixes (` `, `-`, `+`) only; treat other provider text as unsupported metadata rather than arbitrary comparator output.
*   **Scope Lock:** File comparison rendering, provider invocation/fallback, navigation, and focused tests only; no Task 13 dual-preview behavior, directory/tree compare redesign, or file mutation.
*   **Acceptance Criteria:**
*   The default supported system provider produces a deterministic in-app comparison for representative identical, added, removed, and changed text files.
*   The alignment gutter and focused-line styling make matching, added, removed, and changed line relationships unambiguous without relying on fixed screen coordinates.
*   Focused coverage proves the default side-by-side layout and session-only orientation switching preserve the selected hunk, aligned scroll position, and focused comparison row.
*   Provider exit `0` means identical, exit `1` means differences, and exec, signal, or other exit failures have distinct explicit outcomes.  Add/delete-only hunks, zero-length ranges, and no-final-newline markers render deterministically.
*   Provider absence/failure, non-text input, input/output limit exhaustion, unsupported metadata, narrow terminals, and resize have explicit safe outcomes and do not corrupt the normal UI state.
*   The integrated view is visually distinct from Task 13 dual-preview inspection: comparison uses side-by-side framed boxes, whereas Task 13 defaults to stacked framed preview boxes.
*   Focused regression coverage proves target identity, no-mutation behavior, provider/fallback handling, navigation, exit/restore, and layout-resilient rendering.
*   - [ ] **Status:** Not Started.



### **Task 11: Unified Split-Panel State/Restore Architecture**
*   **Goal:** Make split-panel behavior deterministic by giving each panel one canonical UI state record and one canonical restore path so `F8`, `Tab`, `Enter`, release, reactivation, and visible-tree redraw all preserve stable identity, viewport, selection, visibility, and mode without re-deriving authority from raw rows or stale pointers.
*   **Rationale:** The split-panel state-isolation and restore-authority regressions are all the same root-cause class: split-panel state ownership and restore authority are fragmented, so small changes keep reintroducing viewport drift, selection drift, hidden-dotfile reanchor, and transient wrong-shape renders.
*   **Scope Lock:** Canonical panel/window UI state ownership, restore generation and fallback, split transition integrity, and regression coverage only. No keybinding redesign, no new features, and no unrelated overlay/submode rewrite.
*   **Implementation Rule:** Task 11 must follow `docs/SPECIFICATION.md` §2.3, §3.4, §5.1, §5.2, §5.3, and §5.5. If any implementation detail is still ambiguous after reading those sections, the spec must be updated before code changes are made.
*   **Pre-implementation Checklist:** Before coding starts, Task 11 must name the exact state schema, owner boundary, generation rules, identity-key rules, restore/transition entrypoints, fallback order, and regression-gate matrix.
*   **Coverage Target:** This umbrella is the primary stabilization track for Split-Panel State Isolation and Restore Authority Family, Split `Tab` Transition Can Trigger Obvious Wrong-Surface Refresh, Volume Switch Can Lose Per-Volume File Context (`SMALLWINDOWSKIP=1`), Tree Viewport Reanchors Unexpectedly During Navigation and Panel Reactivation, Mkdir Triggers Unnecessary Relog and Resets Tree State, BUG-2, F8 Dotfiles Toggle Leaks Across Panels, F8 Dotfiles Toggle Causes Inactive Selection Jitter, and F8 + SMALLWINDOWSKIP=0 Tab Can Force Inactive Panel into Wrong Focus, plus the related split-state regressions they expose.
*   **Execution Order (mandatory):** Deliver as sequenced subtasks: **11.1 -> 11.2 -> 11.3 -> 11.4**. Task 11 closes only after all subtasks are complete.
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

#### **Task 11.1: Canonical Panel UI State Record + Ownership Map**
*   **Goal:** Define one explicit owner for panel-local frozen state and remove shadow ownership paths that let split panels drift apart.
*   **Mechanism:** Make the panel/window UI state record canonical for cursor, viewport origin, file selection, file cursor, filters, dotfile visibility, and saved focus/mode; keep shared topology in `Volume` only.
*   **Acceptance Criteria:**
*   All panel-local state classes are classified explicitly as owned, derived, or shared-topology-only.
*   Split panels can hold independent filters and dotfile visibility for the same logged volume.
*   No shared-buffer aliasing path remains that allows cross-panel leakage of panel-local state.
*   Owner-boundary assertions exist in the code and fail fast if a non-owner path attempts to mutate panel-local state.
*   - [ ] **Status:** Not Started.

#### **Task 11.2: Deterministic Restore/Rebind Engine + Generation Invalidation**
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

#### **Task 11.3: Atomic Split Transition Engine + Read-Only Render Contract**
*   **Goal:** Centralize split transitions so `F8` enter/exit and `Tab` handoff are transactional and rendering cannot mutate authoritative split state.
*   **Acceptance Criteria:**
*   One explicit split-state owner module/API exists for split/panel-mode authoritative mutation.
*   Direct split-state mutation outside the owner path is removed from production paths.
*   F8 and Tab transitions use one transaction flow: snapshot -> compute -> validate invariants -> commit/rollback.
*   Renderer paths are read-only and may only project the current state record.
*   Direct writes outside the canonical owner path fail through assertions and CI checks.
*   - [ ] **Status:** Not Started.

#### **Task 11.4: Enforceable Regression Gates + Spec Sync**
*   **Goal:** Keep the architecture from regressing by making the restore contract testable and merge-blocking.
*   **Acceptance Criteria:**
*   Mandatory invariant checks run for split restore/transition paths (active-only mutation, inactive freeze/resume, no cross-panel import, identity-based restore).
*   CI/QA merge gate exists for F8/split-touching PRs: invariant gate, transition-matrix gate, and no-direct-write split-authority check.
*   The regression matrix is focused on split-state flows and covers restore, reactivation, generation mismatch, and transition handoff cases.
*   Test evidence explicitly proves stale restore snapshots are rejected after generation changes.
*   `docs/SPECIFICATION.md` stays aligned with the implemented restore contract and fallback order.
*   Task 11 closure requires green evidence for all Task 11 subtasks.
*   - [ ] **Status:** Not Started.

### **Task 12: Enable Practical Command Subset in `F7` Preview (Keep `Tab` Blocked)**
*   **Goal:** Finish `F7` as an in-place work mode: users can run common file actions without leaving preview, while `Tab` stays blocked for preview-state safety and `F8` follows the Task 13 dual-preview transition contract when that feature lands.
*   **Rationale:** `F7` currently feels unfinished because common workflows still require repeated exits.
*   **Scope Lock:** `F7` command availability contract, help/footer parity, and regression coverage only.  Task 13 owns the dual-preview layout and transition design.
*   **Acceptance Criteria:**
*   Define and implement the core `F7` action set for inspect-and-act workflows (including tag/search/view results/compare/copy/move/rename, plus existing high-value file actions).
*   In `F7`, `^T` tag-all, `^S` search, and `^V` tagged/search-result view flows work without exiting preview mode.
*   Tagged search hits/results are visibly highlighted in `F7` preview.
*   Until Task 13 lands, `F8` and `Tab` are explicit no-ops in `F7` mode.  Task 13 replaces the `F8` no-op with its compositional dual-preview transition; `Tab` remains a no-op in ordinary single-preview mode.
*   Footer keybinding hints and F1 help in `F7` accurately reflect allowed actions, blocked keys, and the Task 13 transition when available.
*   Add focused regression tests for allowed-command execution in `F7`, blocked-`Tab` enforcement, and the Task 13 `F8` handoff when that feature lands.
*   Update `etc/help/man.en.md` and regenerate `docs/USAGE.md` when behavior lands.
*   - [ ] **Status:** Not Started.

### **Phase Follow-On Work**

### **Task 13: Dual-Preview Split Mode**
*   **Goal:** Combine `F7` preview and `F8` split compositionally in either order, yielding a dual-preview layout whose panels retain independent preview and list state.
*   **User-Facing Behavior:**
    *   `F8` continues to create the normal side-by-side two-panel split.  `F7` from that split enters dual preview when both panel selections are previewable.
    *   `F7` from a single panel enters ordinary preview.  `F8` from that preview enters the same dual-preview mode when its split-panel selections are previewable.
    *   The transitions compose as a mode stack: repeating `F7` removes the preview layer and repeating `F8` removes the split layer.  Thus `F8` then `F7`, followed by `F7`, returns to split; `F7` then `F8`, followed by `F8`, returns to single preview.  `Esc` removes the most recently added layer.
    *   The default dual-preview presentation is stacked (top/bottom), with one independently framed preview box per panel, following the Unixtree convention.  The same session-only, context-local orientation action used by Task 10 switches the two framed boxes to side-by-side and back without changing either panel's identity or state.
    *   Each preview uses its owning panel's selected file and preserves its own scroll position, selection, and return-to-list state.
    *   `Tab` switches the active preview panel without merging or resetting either panel's state.
    *   The orientation action's binding must be a portable, collision-audited key.  Simultaneous `F7`+`F8` chords are not a reliable terminal input contract and must not be used as the binding.
    *   Active/inactive indicators make it unambiguous which preview will receive `Enter`, `Tab`, and `F7`.
*   **Rationale:** `J` is the direct comparison path.  Dual preview is complementary: it provides independent inspection of two files without creating diff output or leaving the file-manager UI.
*   **Scope Lock:** This is an advanced split/preview state feature only. It does not require a broader orthodox-style layout redesign and should preserve ytnova's existing xtree/unixtree/ztree-derived interaction style.
*   **Acceptance Criteria:**
*   Both panels enter dual preview together, retain independent preview/list state, and do not leak state across the split boundary.
*   `Tab`, `F7`, `F8`, `Esc`, orientation switching, and return-to-list behavior are deterministic and documented in footer/F1/manpage text.
*   Removing either layer restores its immediately preceding mode without altering either panel's selection, viewport, or focus state.
*   Split-panel active/inactive indicators remain unambiguous throughout dual preview.
*   Focused regression coverage proves both entry orders, compositional exits, orientation switching, per-panel state retention, panel switching, and exit/return behavior.
*   - [ ] **Status:** Not Started.



### **Task 14: Harden Build Source Discovery (Recursive + Deterministic)**
*   **Goal:** Update build source discovery so all C files under `src/` are discovered recursively with deterministic ordering.
*   **Rationale:** Current discovery only covers up to one subdirectory level and will miss files after module reorganization.
*   **Scope Lock:** Build discovery and related guard/test updates only. No feature behavior changes.
*   **Acceptance Criteria:**
*   Build still succeeds with `make clean && make`.
*   Full QA gate still passes with `make qa-all`.
*   Source file list ordering is deterministic across runs.
*   - [ ] **Status:** Not Started.

### **Task 15: Reorganize Modules into Shallow Hierarchical Folders**
*   **Goal:** Group modules into shallow, purpose-based subfolders and update build/header/linkage references accordingly.
*   **Rationale:** Improves discoverability and ownership without changing behavior.
*   **Scope Lock:** File moves + include/path/build/script/test reference updates only. No feature behavior changes.
*   **Acceptance Criteria:**
*   `make clean && make` passes.
*   `make qa-module-boundaries` and `make qa-all` pass.
*   No runtime behavior changes.
*   Folder depth remains shallow (max one extra level under `src/ui` and `src/cmd`).
*   - [ ] **Status:** Not Started.

### **Task 16: Decompose Remaining Hotspot Modules (Atomic Subtasks)**
*   **Goal:** Reduce complexity in remaining hotspot files by extracting cohesive action families into focused modules while preserving behavior.
*   **Rationale:** These files remain risk hotspots after controller decomposition and slow safe feature delivery.
*   **Execution Rule:** Deliver one hotspot module at a time, each with its own architect plan, developer pass, auditor pass, and QA evidence.
*   - [ ] **Status:** Not Started.

### **Task 17: Decompose `src/ui/ctrl_file_ops.c` (`handle_tag_file_action` focus)**
*   **Goal:** Extract large tagged-action branches from `handle_tag_file_action` into focused helpers/modules.
*   **Scope Lock:** Preserve all tagged-file behavior and command semantics.
*   **Acceptance Criteria:** Smaller dispatcher function, unchanged behavior, green QA.
*   - [ ] **Status:** Not Started.

### **Task 18: Decompose `src/ui/key_engine.c`**
*   **Goal:** Separate key mapping/dispatch concerns from input-loop mechanics and context-specific action routing.
*   **Action Name Cleanup:** Normalize tree-expand action identifiers so names match behavior: shallow expand (`+`) is `ACTION_TREE_EXPAND`, recursive expand (`*`) is `ACTION_TREE_EXPAND_RECURSIVE`, and any redundant tree-expand identifier is merged or removed. Update key/action mappings and related tests with no behavior change.
*   **Scope Lock:** No keybinding behavior change unless explicitly approved in a separate task.
*   **Acceptance Criteria:** Cleaner dispatch boundaries, consistent action naming, unchanged key behavior, green QA.
*   - [ ] **Status:** Not Started.

### **Task 19: Decompose `src/cmd/copy.c`**
*   **Goal:** Isolate copy conflict handling, path/precondition validation, and transfer orchestration into focused units.
*   **Scope Lock:** No copy/move/archive user-visible behavior changes.
*   **Acceptance Criteria:** Reduced complexity in core copy path, unchanged behavior, green QA.
*   - [ ] **Status:** Not Started.

### **Task 20: Decompose `src/cmd/profile.c`**
*   **Goal:** Split profile parsing, validation/defaulting, and apply/update logic into focused units.
*   **Scope Lock:** No configuration semantic changes.
*   **Acceptance Criteria:** Clear parser/apply separation, unchanged config behavior, green QA.
*   - [ ] **Status:** Not Started.

### **Task 21: Refactor Tab Completion for Command Arguments**
*   **Goal:** Update the tab completion logic in `src/util/tabcompl.c` to handle command-line arguments correctly and resolve ambiguous matches using Longest Common Prefix (LCP).
*   **Rationale:** Currently, the completion engine treats the entire input line as a single path. This causes failures when trying to complete arguments for commands (e.g., `x ls /us<TAB>` fails because it looks for a file named "ls /us"). It also fails to partial-complete when multiple matches exist (e.g., `/s` matching both `/sys` and `/srv`).
*   **Mechanism:**
    *   Tokenize the input string to identify the word under the cursor.
    *   Perform globbing/matching *only* on that specific token.
    *   If multiple matches are found, calculate the Longest Common Prefix and return that (standard shell behavior) instead of failing or returning the first match.
    *   Reassemble the command string (prefix + completed token) before returning.
*   - [ ] **Status:** Not Started.

### **Task 22: Add Case-Sensitive Sort Toggle + Profile Default**
*   **Goal:** Add case-sensitivity as a sort option in the existing sort flow and profile defaults.
*   **Rationale:** Users need deterministic lexical control without introducing extra global keybindings.
*   **Scope Lock:** Sort comparison behavior only; no tree/file model changes.
*   **Acceptance Criteria:**
*   Add `SORT_CASE_SENSITIVE=0|1` profile setting (default `0`) and wire it to default sort behavior.
*   Existing sort prompt (`S` flow) includes a case-sensitivity toggle.
*   Footer/F1/help/manpage text are synchronized for the new sort option.
*   - [ ] **Status:** Not Started.

### **Task 23: Input Loop Determinism and Event Handling**
*   **Goal:** Group event-priority policy and multiplexing implementation under one umbrella to reduce recurring input-loop regressions.

#### **Task 23.1: Input Loop Determinism and Event-Priority Contract**
*   **Goal:** Make key handling deterministic across ESC sequences, resize events, watcher events, and prompt/overlay contexts.
*   **Rationale:** Recurring regressions originate from event-order ambiguity, not raw key decoding alone.
*   **Scope Lock:** Input/event ordering, dispatch priority, and regression coverage only; no keybinding changes.
*   **Acceptance Criteria:**
*   Event-priority order is explicit and enforced for: resize, watcher refresh, ESC-sequence normalization, and key dispatch.
*   Prompt/overlay contexts must consume input according to innermost-active-context rules before base-mode dispatch.
*   No double-processing or dropped-event regressions on rapid resize + key + watcher activity.
*   Focused regression matrix covers ESC timing, resize storms, watcher bursts, and split/overlay transitions.
*   - [ ] **Status:** Not Started.

#### **Task 23.2: Non-Blocking FD Multiplexing Implementation**
*   **Task:** Implement/maintain non-blocking input multiplexing (`select`/`poll`) for keyboard + watcher FDs as the concrete mechanism under Task 23.
*   **Scope Lock:** Mechanism-level implementation only.
*   **Acceptance Criteria:**
*   Multiplex loop behavior conforms to Task 23 event-priority contract.
*   Regression coverage confirms no blocking/starvation under mixed input/event load.
*   - [ ] **Status:** Not Started.

#### **Task 23.3: Create Watcher Infrastructure (`watcher.c`)**
*   **Task:** Create a new module `watcher.c` to abstract the OS-specific file monitoring APIs.
*   **Logic:**
    *   **Init:** Call `inotify_init1(IN_NONBLOCK)`.
    *   **Add Watch:** Implement `Watcher_SetDir(char *path)` which removes the previous watch (if any) and adds a new watch (`inotify_add_watch`) on the specified path for events: `IN_CREATE | IN_DELETE | IN_MOVE | IN_MODIFY | IN_ATTRIB`.
    *   **Check:** Implement `Watcher_CheckEvents()` which reads from the file descriptor. If events are found, it returns `TRUE`, otherwise `FALSE`.
    *   **Portability:** Guard everything with `#ifdef __linux__`. On other systems, these functions act as empty stubs.
*   - [ ] **Status:** Not Started.

#### **Task 23.4: Implement Live Refresh Logic**
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

#### **Task 23.5: Update Watch Context on Navigation (Current-Directory Auto-Refresh Context)**
*   **Task:** Ensure the watcher always monitors the *current* directory so the file list the user is looking at stays fresh without a manual reload.
*   **Logic:**
    *   In `dirwin.c`: Whenever the user moves the cursor to a new directory (UP/DOWN), update the watcher.
    *   *Optimization:** Only update the watcher if the user *enters* the File Window (Enter) or stays on a directory for > X milliseconds?
    *   *Decision:* For `ytnova`, the "Active Context" is the directory under the cursor in the Directory Window, OR the directory being viewed in the File Window. In user-facing terms, auto-refresh should follow the current working view.
    *   **Implementation:** Call `Watcher_SetDir(dir_entry->name)` inside `HandleDirWindow` navigation logic (possibly debounced) and definitely inside `HandleFileWindow`.
*   - [ ] **Status:** Not Started.

#### **Task 23.6: Implement Directory Filtering (Non-Recursive)**
*   **Description:** Extend Filter to support directory-pattern tokens identified by a trailing slash.
    *   `dir/` means include matching directories in the current tree view.
    *   `-dir/` means exclude matching directories in the current tree view.
    *   Directory tokens can be combined with existing file-pattern tokens in the same filter spec.
    *   This logic is non-recursive and visibility-only: it affects what is shown in the current view, not internal directory state.
*   - [ ] **Status:** Not Started.

### **Task 24: Add Configurable Bypass for External Viewers**
*   **Goal:** Add a configuration option to globally disable external viewers, forcing the use of the internal viewer.
*   **UI Note:** If a future guided `F10` config panel lands, expose this there without replacing the existing raw-text config path.
*   **Rationale:** Provides flexibility for cases where the user wants to quickly inspect the raw bytes of a file (e.g., a PDF) without launching a heavy external application.
*   **Coverage Clarification:** This task also covers single-file `V` parity with tagged viewing: users must be able to choose internal vs external behavior consistently for both single-file view and tagged-view workflows.
*   - [ ] **Status:** Not Started.

### **Task 25: Implement Auto-Execute on Command Termination**
*   **Goal:** Allow users to execute shell commands (`X` or `P`) immediately by ending the input string with a specific terminator (e.g., `\n` or `;`), without needing to press Enter explicitly.
*   **Rationale:** Accelerates command entry for power users who want to "fire and forget" commands rapidly.
*   - [ ] **Status:** Not Started.

### **Task 26: Standardize Internal Viewer Layout**
*   **Goal:** Ensure the internal viewer's layout geometry matches the main application (borders, headers, and footer).
*   - [ ] **Status:** Not Started.

### **Task 27: Nested Archive Traversal**
*   Allow transparently entering an archive that is itself inside another archive.
*   - [ ] **Status:** Not Started.

---

## **Phase 5: Permanent Security Gates**
*This phase is an enforcement gate for security risk classes: audit baseline debt, then detect and block introduced/reintroduced security findings on every non-trivial change.*

### **Task 28: Add Security Fuzzing Harness for High-Risk Input Paths**
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

### **Task 29: Implement Advanced Batch Rename**
*   **Goal:** Add a ytnova-native batch rename flow for tagged files with numbering support, casing changes (`Tab`), substring replacement, and pattern-based keep/remove operations.
*   **Rationale:** Essential power-user feature for managing large file sets without forcing one-by-one rename loops.
*   **Preview/Apply Contract:** Batch rename is preview-first. Show `old -> new` results before mutation and support per-item apply controls: `y` (apply current), `n` (skip current), `a` (apply all remaining), `Esc` (cancel remaining).
*   - [ ] **Status:** Not Started.

### **Task 30: Unify Copy Semantics and Add Directory Sync (`Y`)**
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

### **Task 31: Define Extension Surface Contract (`F9` Apps + `F7` Preview Plugins)**
*   **Goal:** Define one explicit extension contract for external-tool integrations so command apps (`F9`) and preview plugins (`F7`) follow the same safety, UX, and fallback rules.
*   **Scope:** Contract/spec-only delivery for external execution surfaces (`X`, `P`, `W`, `FILEDIFF`, `F9`, and `F7` preview-helper boundary).
*   **Rationale:** ytnova should reuse mature external tools without accumulating ad-hoc one-off behavior per feature.
*   **Acceptance Criteria:**
    *   Contract defines provider types (`app`, `preview`) and shared lifecycle semantics.
    *   Contract defines placeholder/token policy, argument safety rules, and bounded command construction.
    *   Contract defines deterministic completion/failure reporting and fallback behavior.
    *   Footer/F1/manpage wording aligns with the new contract language.
*   - [ ] **Status:** Not Started.

### **Task 32: Implement Shared Provider Registry (Plugin-Lite, External-Tool-First)**
*   **Goal:** Implement a shared provider registry/runtime for extension providers instead of isolated one-off paths.
*   **Non-Goal:** Do not add in-process arbitrary binary/plugin loading; providers remain external-tool adapters.
*   **Rationale:** A unified provider runtime keeps behavior predictable and lowers maintenance risk while preserving Unix-style composability.
*   **Acceptance Criteria:**
    *   Shared provider model supports at least `app` and `preview` provider classes.
    *   Common execution/safety controls are centralized (timeouts, output caps, exit-code mapping, fallback policy).
    *   Config/profile format is documented and validated with focused regression tests.
*   - [ ] **Status:** Not Started.

### **Task 33: Add Optional Background App Execution (`bg`)**
*   **Goal:** Allow selected external commands to run in background so users can continue navigating immediately.
*   **Entry Direction:** Prefer `F9` as the primary UX surface, with optional command-prompt parity where it fits cleanly.
*   **Scope Lock:** External commands/apps only (no async copy/move/delete queue in this task).
*   **Rationale:** This captures high-value "run and continue" workflow speed without requiring an embedded subshell model.
*   **Acceptance Criteria:**
    *   Users can launch an app in foreground or background using explicit UI choice/marker.
    *   Background job state is visible and queryable (running/success/failure) with actionable completion messaging.
    *   Failed background runs return clear diagnostics without destabilizing curses state.
*   - [ ] **Status:** Not Started.

### **Task 34: Implement F7 Preview Helper Pipeline (Promote Preview-Helper Pipeline into Current Delivery)**
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

### **Task 35: Propagate Active Theme to Supported Terminal Helpers**
*   **Goal:** Propagate the active YtreeNova theme to supported terminal helpers so configured `EDITOR`, `PAGER`, and `TAGGEDVIEWER=external` flows can launch with a matching or near-matching color preset when the helper supports non-invasive startup theming.
*   **Rationale:** Establish Role-Based Theme System and Restrained Default Palette made YtreeNova itself themeable, but external terminal helpers still break visual continuity when the main UI is blue-on-white and the launched helper falls back to unrelated defaults. Supported helpers should be able to follow the active theme without requiring users to hand-maintain per-theme shell startup hacks.
*   **Scope Contract:** This is an adapter/preset task for known terminal helpers, not a promise to theme arbitrary external commands or GUI applications. Unsupported helpers must continue to launch normally with no theme injection rather than receiving brittle guessed arguments.
*   **Launch Policy:** Prefer per-launch arguments, environment variables, temporary helper config files, or repo-managed wrapper/adaptor scripts that are selected by helper name/profile. Do **not** auto-edit persistent user shell startup files such as `.bashrc`, editor dotfiles, or pager rc files. Theme changes should apply on the next helper launch without a manual revert step because no persistent user config mutation occurred.
*   **Documentation Policy:** If a helper cannot be themed well through transient launch-time inputs alone, document an optional user-managed setup path in `etc/help/man.en.md` / generated `docs/USAGE.md`, but keep that as opt-in guidance rather than automatic mutation. The docs must clearly distinguish between built-in transient presets and user-owned persistent helper customization.
*   **Tagged Viewer Contract:** `TAGGEDVIEWER=external` participates in this task when the selected external pager/helper is one of the supported terminal helpers. Pager-native behaviors such as hit traversal and search highlighting remain helper-owned unless a supported preset explicitly maps them; YtreeNova must not fight helper-native search-hit semantics just to force visual parity.
*   **Acceptance Criteria:**
*   At least the shipped supported-helper set for one editor family and one pager family (for example vim-like and less-like helpers) can be launched with a theme preset that tracks the active YtreeNova theme.
*   Active-theme changes apply on subsequent helper launches without requiring a revert pass through shell/editor/pager dotfiles.
*   Unsupported external helpers degrade safely to normal launch behavior with no broken command lines, no silent shell-dotfile edits, and no persistent side effects.
*   `TAGGEDVIEWER=external` uses the same supported-helper preset path when applicable and otherwise degrades safely to normal external launch behavior.
*   The manpage/usage docs explain the supported-helper contract, the non-invasive launch policy, and any optional user-managed helper setup for cases where transient theming is insufficient.
*   Focused regression or source-contract coverage proves helper theming is adapter-driven, opt-in by supported helper identity, and does not mutate persistent user shell/editor/pager startup files.
*   - [ ] **Status:** Not Started.

### **Task 36: Implement Configurable Keymap**
*   **Description:** Abstract all hardcoded key commands (e.g., 'm', '^N') into a configurable keymap loaded from a separate keymap profile file. The core application logic will respond to command identifiers (e.g., `CMD_MOVE`), not raw characters. This will allow users to customize their workflow and resolve keybinding conflicts.
*   **Sequencing dependency:** Implement after Kitty Keyboard Protocol Move Binding establishes enhanced key identity and input capability state, after Refine In-App Help Text's portable footer keybinding/F1 wording cleanup, and preferably after the Task 23 parity gate.
*   **Config contract:** Select a keymap profile via `ytnova.conf` (opt-in). Locale-oriented profiles are allowed as explicit user choices, for example an English mnemonic profile can bind `C` to `Copy`, while a German mnemonic profile can bind `K` to `Kopieren` and `L` to `Löschen`. The shipped default keymap must remain internally consistent.
*   **Display contract:** The footer must render active key tokens plus localized command labels together (for example active binding `C` + translated label `Copy` -> `(C)opy`) so runtime hints always match active bindings. Key tokens are data from the keymap, labels are data from localization, and punctuation/styling are renderer-owned.
*   **Legacy menu override contract:** The existing `[MENU]` text override only changes displayed text and does not change keyboard behavior. It may remain as an expert display override during migration, but it is not the final localization/keybinding model and must not be used as a substitute for real keymap-driven labels.
*   **Canonicalization/validation contract:** Normalize terminal byte aliases during keymap load (`^m`=`Enter`/`CR`, `^j`=`LF`/newline enter path, `^[`=`Esc`) and reject profiles that map alias-equivalent inputs to different commands. A confirmed protocol path may distinguish `^m` from Enter.
*   **Capability contract:** The active keyboard-input capability selects the tagged-move binding. A protocol path uses `^m`; the silent fallback uses `^n`.
*   **Behavior stability contract:** Custom overrides are opt-in and must pass collision/unbound-action validation before activation.
*   - [ ] **Status:** Not Started.

---

## **Phase 8: Final Polish (Post-Alpha, Pre-v1.0.0)**
*This phase focuses on release polish. Security, module-boundary, and quality gates remain continuous from earlier phases and are not deferred to this phase.*

### **Task 37: UI/UX Snappiness Polish (Targeted Optimization)**
*   **Goal:** Improve perceived responsiveness in high-frequency flows using profiling-driven optimizations.
*   **Rationale:** Premature optimization is avoided; final polish applies targeted improvements where bottlenecks are measured.
*   - [ ] **Status:** Not Started.

### **Task 38: Add Modal Window Shadows**
*   **Goal:** Add a restrained lower/right shadow treatment to modal windows so dialogs read as layered popups rather than flat border boxes.
*   **Rationale:** A subtle mc-style shadow gives visual depth and makes help/info/error dialogs easier to parse at a glance without changing modal behavior.
*   **Scope Lock:** Visual chrome only; no modal workflow, severity semantics, or keybinding changes.
*   **Acceptance Criteria:**
*   Modal/help/dialog surfaces can render a clipped lower/right shadow where terminal space permits, without obscuring modal content or corrupting underlying layout.
*   Shadow styling is theme-controlled (directly or through a dedicated semantic role) rather than hardcoded as one-off reverse-video tricks.
*   Focused rendering tests cover edge clipping and ensure shadow drawing does not bleed into non-modal surfaces.
*   - [ ] **Status:** Not Started.

### **Task 39: Multi-Round Adversarial Security Review**
*   **Goal:** Perform a pre-v1.0.0 multi-round security review using adversarial and AppSec perspectives.
*   **Examples:** Senior AppSec reviewer, penetration-tester mindset, and insider-knowledge threat modeling.
*   **Rationale:** Final pre-release pressure test on top of continuous Phase 2 security gates.
*   - [ ] **Status:** Not Started.

---

## **Beta: Stabilization and Performance**
*This phase follows alpha delivery phases and precedes wishlist work. Place stabilization tasks here: bug fixes, regressions, reliability, and performance. Defer non-essential feature work to wishlist phases.*

### **Task 40: Stabilize and Unify Overlay/Submode State Model (Compatibility-First)**
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
    *   Shim cleanup is mandatory per Remove Temporary Compatibility Shims (Global Cleanup Gate) before closure.

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
*   **Description:** Follow-up locale expansion can land anytime without major UI architecture changes because it builds on the shipped locale/layout-aware command presets and stays inside the current left-to-right ncurses model and existing preset/help/manpage surfaces.
*   **Localized keymap profiles:** The core packaged-preset model is the shipped locale/layout-aware command-preset architecture; later work here must build on that shared action-based model rather than invent a second parallel keymap format.
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
    *   Guided edits must write back through the same split-surface files and preserve the shipped config source-of-truth and locale/layout-aware command ownership.
*   **Rationale:** Leaves room for a friendlier common-options surface later without replacing the current Unix-style text-edit workflow.
*   - [ ] **Status:** Not Started.

### **Idea FE-9: Semantic F1 Help Styling Without Theme Bloat**
*   **Goal:** Allow authored `F1` help topics to request a small bounded set of semantic text styles while keeping `ytnova.themes` compact and stable.
*   **Dependency/Sequencing Note:** Evaluate this only after the shipped cross-cutting help information architecture and help/footer surface-role contracts have settled. FE-9 is a follow-on enhancement for proven emphasis needs, not a prerequisite for making `F1` useful.
*   **Design Direction:**
    *   Use semantic markup roles in `etc/help/f1.en.md` rather than raw color/attribute requests.
    *   Initial role set should stay intentionally small (for example `help_text`, `help_key`, `help_code`, `help_heading`, `help_note`, `help_warning`).
    *   `ytnova.themes` maps those semantic roles to ncurses-supported attributes/colors.
    *   Missing theme entries must fall back deterministically to the normal help-text style.
*   **Non-Goal:** Do not allow arbitrary per-span foreground/background pairs or unlimited raw `bold`/`inverse`/`underline` directives directly in help source; that would balloon theme surface area and couple authored help text to presentation internals. FE-9 does not replace the shipped surface-role ownership contract; it only adds a bounded semantic layer within already-settled help surfaces.
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
    *   Put full syntax/examples in `F1` help and manpage source (`etc/help/man.en.md`).
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


### **Idea FE-18: Investigate Recursive Tagging vs Existing Showall/Global Workflow**
*   **Goal:** Determine whether recursive tagging provides enough real workflow benefit over the current `log dir -> Showall/Global -> tag` path to justify added complexity.
*   **Rationale:** Recursive tagging may reduce steps in some trees, but can also add command ambiguity and accidental broad-selection risk.
*   **Investigation Output:** Document concrete user workflows, interaction-depth impact, and safety tradeoffs; propose either (a) no change, or (b) a minimal, default-safe recursive tagging design with clear scope/confirmation semantics.
*   - [ ] **Status:** Not Started.

### **Idea FE-19: Richer Compare Result Views**
*   **Goal:** Extend compare workflows so the result can be viewed directly, not just turned into tags on the active side.
*   **User-Facing Behavior:**
    *   After comparing two directories/trees, users can narrow the result to categories such as **left/source only**, **right/target only**, **newer**, **older**, **size different**, **content different**, or **identical**.
    *   The result should be explorable as a list/view mode, not only as tag side effects.
    *   Compare output should stay explicit about which side is being shown and why an entry appears.
*   **Rationale:** Current compare behavior is useful but blunt. A richer result view makes compare a practical review tool rather than only a tag generator.
*   - [ ] **Status:** Not Started.

### **Idea FE-20: Recent-Directory Bookmarks and Pinned Favorites**
*   **Goal:** Add a first-class recent-directory and pinned-favorites picker for fast return to commonly visited locations.
*   **User-Facing Behavior:**
    *   Show a compact list of recently visited directories together with user-pinned favorites.
    *   Selecting an entry should log or activate that directory directly.
    *   Reuse existing log/history persistence where practical, but present this as a navigation surface, not merely raw prompt history text.
    *   Prefer a portable key or prompt hook (for example `F3`) over browser-style back/forward semantics unless a stronger need emerges later.
*   **Rationale:** Prompt history helps when the user remembers what they typed. A dedicated recent-directory/favorites list helps when the user remembers the place, not the exact command string.
*   - [ ] **Status:** Not Started.


### **Idea FE-21: Directory-Focus Small-File Peek Navigation (`Shift` + Nav Keys)**
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

### **Idea FE-22: Unified `N Create` Entry Point (Capability-Filtered by Backend)**
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

### **Idea FE-23: Per-Window Filter State (Split Screen Prerequisite)**
*   Decouple the file filter (`file_spec`) from the `Volume` structure and move it into a new `WindowView` context. This architecture is required to support F8 Split Screen, enabling two independent views of the same volume with different filters (e.g., `*.c` in the left panel versus `*.h` in the right).
*   - [ ] **Status:** Not Started.

### **Idea FE-24: State Preservation on Reload (`^L`)**
*   Modify the Refresh command to preserve directory expansion states. Cache open paths prior to the re-scan and restore the previous view structure instead of resetting to the default depth.
*   - [ ] **Status:** Not Started.

### **Idea FE-25: Preserve Tree Expansion on Refresh**
*   Modify the Refresh/Rescan logic (`^L`, `F5`) to cache the list of currently expanded directories before reading the disk. After the scan is complete, programmatically re-expand those paths if they still exist.
*   - [ ] **Status:** Not Started.

### **Idea FE-26: Scroll Bars**
*   On left border of the file and directory windows to indicate the relative position of the highlighted item in the entire list (configurable to char or line).
*   - [ ] **Status:** Not Started.

### **Idea FE-27: Callback API Constification Cleanup (cppcheck strict mode)**
*   `cppcheck` suggests const-qualifying callback `user_data`, but doing this correctly likely requires changing callback typedef/API signatures (e.g., `RewriteCallback`) and related call sites. Defer this to a focused API pass to avoid scattered casts and partial churn.
*   - [ ] **Status:** Not Started.

### **Future Phase 3: Long-Horizon Experiments**

### **Idea FE-28: Implement VFS Abstraction Layer** (Use the Architect persona here)
*   **Goal:** Replace hardcoded filesystem logic with a driver-based architecture. This allows `ytnova` to treat any data source (Local FS, Archive, SSH, SQL) uniformly as a `Volume`.
*   **Context:** Currently, `log.c` decides between "Disk" and "Archive". We will change this so `log.c` asks a Registry: "Who can handle this path?"
*   **Follow-on Direction:** Include remote logging backends under this VFS model (FTP/SFTP candidates), with final protocol choice deferred until security and maintenance review.

### **Idea FE-29: Define VFS Interface & Volume Integration** (Use the Architect persona here)
*   **Goal:** Define the `VFS_Driver` contract (struct of function pointers) and update the `Volume` struct to hold a pointer to its active driver.
*   **Mechanism:**
    *   Create `include/ytnova_vfs.h`.
    *   Define function pointers: `scan`, `stat`, `lstat`, `extract`, `get_path` (for internal addressing).
    *   Update `include/ytnova_defs.h` to add `const VFS_Driver *driver` and `void *driver_data` to `struct Volume`.

### **Idea FE-30: Implement VFS Registry** (Use the Architect persona here)
*   **Goal:** Create the core logic to register drivers and probe paths.
*   **Mechanism:**
    *   Create `src/fs/vfs.c`.
    *   Implement `VFS_Init()` (registers built-in drivers).
    *   Implement `VFS_Probe(path)` which iterates drivers asking "Can you handle this?" and returns the best match.

### **Idea FE-31: Implement "Local" VFS Driver** (Use the Architect persona here)
*   **Goal:** Wrap the existing POSIX `opendir`/`readdir` logic into a `VFS_Driver`.
*   **Mechanism:**
    *   Create `src/fs/drv_local.c`.
    *   Move logic from `src/fs/tree_read.c` into the driver's `.scan` method.
    *   Ensure it populates `DirEntry` structures exactly as before.

### **Idea FE-32: Implement "Archive" VFS Driver** (Use the Architect persona here)
*   **Goal:** Wrap the existing `libarchive` logic into a `VFS_Driver`.
*   **Mechanism:**
    *   Create `src/fs/drv_archive.c`.
    *   Move logic from `src/fs/archive_read.c` and `src/fs/archive_write.c` into the driver.
    *   Implement `.extract` to handle the temporary file creation for viewing/copying.

### **Idea FE-33: Switch `LogDisk` to VFS** (Use the Architect persona here)
*   **Goal:** Update the main entry point to use the new system.
*   **Mechanism:**
    *   Refactor `src/cmd/log.c`.
    *   Replace the `stat`/`S_ISDIR` check with `VFS_Probe(path)`.
    *   Call `vol->driver->scan()` instead of calling `ReadTree` or `ReadTreeFromArchive` directly.

### **Idea FE-34: Refactor Consumers (Polymorphism)** (Use the Architect persona here)
*   **Goal:** Remove `if (mode == ARCHIVE)` from the rest of the codebase.
*   **Mechanism:**
    *   Update `view.c`, `copy.c`, `execute.c`.
    *   Replace specific calls with `vol->driver->extract(...)` or `vol->driver->stat(...)`.

### **Idea FE-35: Database Browsing and Editing via Virtual Filesystem Drivers**
*   **Goal:** After the driver-based VFS abstraction exists, allow ytnova to browse supported database formats as navigable virtual filesystems and eventually edit them through driver-defined operations.
*   **User-Facing Direction:** Treat a database as a structured volume (for example database -> tables -> rows/records or exported views) rather than as one opaque file blob.
*   **Rationale:** This is a specialized extension of the VFS model, not a core file-manager requirement. Keep it as a future experiment until a clear driver design and real use-case exist.
*   - [ ] **Status:** Not Started.

### **Idea FE-36: Implement Recursive Directory Watching**
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

### **Idea FE-37: Implement Shell Script Generator**
*   **Goal:** Generate a shell script from tagged files using user-defined templates (e.g., `cp %f /backup/%f.bak`), replacing the "Batch" concept.
*   **Rationale:** Offers complex templating logic that goes beyond simple pipe/xargs, and critically allows the user to review/edit the generated script before execution for safety.
*   - [ ] **Status:** Not Started.

### **Idea FE-38: Keyboard Macros (F12 Record/Playback)**
*   **Goal:** Record and replay simple keystroke sequences.
*   **Rationale:** Useful for repeating safe, local interaction sequences.
*   **Status:** Deferred.
*   **Note:** Revisit only after a safe design exists that cannot turn traces into a secret-capturing scripting surface.

### **Idea FE-39: Enhance Built-In Viewer**
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

### **Idea FE-40: Investigate Optional Kitty Protocol Enhancements**
*   **Goal:** Investigate optional Kitty protocol and terminal-UI capabilities beyond enhanced keyboard input that can improve ytnova workflows without changing the portable baseline.
*   **Candidate scope:** Evaluate protocol-backed image/preview rendering, terminal-native tabs or window/workspace integration, hyperlinks, clipboard, notifications, and other capabilities that make GUI-like interaction useful within a terminal session.
*   **Investigation scope:** For each candidate, establish concrete user benefit, terminal-path compatibility, fallback behavior, security implications, testability, packaging impact, and whether the integration belongs in ytnova rather than the terminal.
*   **Architecture contract:** Keep protocol support behind a narrow capability boundary. Do not spread Kitty-specific state or APIs through controllers, core models, or rendering ownership; the boundary must remain replaceable by a future Rust, notcurses, or other backend path.
*   **Compatibility contract:** Every capability must be negotiated, independently optional, and fully usable through the existing POSIX terminal workflow when unavailable.
*   **Non-Goal:** Do not make Kitty, a GUI runtime, or any single terminal family a requirement for ytnova.
*   **Rationale:** Kitty can provide richer terminal-native interaction, but adoption must be evidence-led and must preserve ytnova's terminal portability and future architecture choices.
*   - [ ] **Status:** Not Started.

### **Idea FE-41: Investigate Replacing ncurses with a Better TUI Backend**
*   **Goal:** Investigate whether ytnova should replace or meaningfully decouple from ncurses in favor of a better TUI/runtime layer while preserving current interaction semantics.
*   **Investigation scope:** Evaluate candidate backends on portability, rendering/control over redraw behavior, input handling, testability, packaging friction, and migration risk for the current architecture.
*   **Compatibility contract:** Any replacement path must preserve the portable baseline terminal workflow and must not require a single terminal family or GUI-specific runtime stack.
*   **Rationale:** This is a platform/runtime architecture effort intended to determine whether ncurses remains the right long-term foundation for ytnova's TUI.
*   - [ ] **Status:** Not Started.

### **Idea FE-42: Implement "Safe Delete" (Trash Can)**
*   **Goal:** Add optional trash-backed delete where the active filesystem/backend supports it.
*   **Config:** Add a `ytnova.conf` switch for trash-delete with default `1` (enabled).
*   **Fallback:** If trash-delete is disabled or unsupported for the active backend, use permanent delete with explicit confirmation.
*   - [ ] **Status:** Not Started.

### **Idea FE-43: Port to other platforms**
*   **Validation:** Currently practical via WSL and QEMU
*   **Possible:** OmniOS (illumos), GNU Hurd, FreeBSD
*   **Possible but impractical for maintainers right now:**  macOS, AIX, OpenVMS, Solaris, Redox OS
*   **Out of scope:** Windows (ZTreeWin exists), legacy UNIXes including HP-UX
*   - [ ] **Status:** Not Started.

### **Idea FE-44: Forward Text Search in Contextual F1 Help**
*   **Goal:** Add a `/query` search to contextual `F1` popups that moves to the first matching rendered help text, including ordinary words that are not links.
*   **Design Requirements:** Search input, cancellation, backspace, no-match feedback, link-selection interaction, and locale/UTF-8 matching must be designed and covered by focused tests.
*   **Rendering Requirement:** Search must update the existing popup without closing/reopening it or causing redraw flicker.
*   **Scope Lock:** Future work only; current contextual-help behavior remains unchanged until this feature has a complete design and implementation.
*   - [ ] **Status:** Not Started.
