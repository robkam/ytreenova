# **System Architecture**
> **Purpose:** This document defines the internal design of `ytnova`. It serves as the authoritative guide for maintaining the codebase's structural integrity.

## **1. Core Quality Principles**
To maintain architectural stability throughout YtreeNova's development, all changes must adhere to these foundational rules:

*   **Code Quality (DRY):** All development must adhere to the "Don't Repeat Yourself" principle. Code must be modular, reusable, and free of redundancy.
*   **Architectural Integrity (Anti-Patching):** Do not apply superficial fixes for deep architectural problems. If a bug is caused by fragmented state or logic, **STOP**. Refactor the architecture to unify the logic before fixing the specific bug. It is better to break one thing to fix the system than to patch the system and break everything.
*   **Single Responsibility (SRP):** Enforce strict modularity. Each file (module) must serve exactly one purpose. Maintain a hard separation between the **UI** (View), **File System** (Model), and **Commands** (Controller) to ensure the codebase remains testable and maintainable.
*   **Use Established Libraries:** Prefer mature, well-supported libraries (e.g., `libarchive`) instead of creating custom replacements.
*   **Module Ownership (Feature Containment):** A feature that can be self-contained **MUST** be self-contained in its own module. It is **FORBIDDEN** to implement a new feature as a sub-function within an existing controller (`ctrl_*.c`) unless that logic is exclusively and inseparably part of that controller's input/event loop. The canonical test: *"Could this function be called from a different context without modification?"* If yes, it does not belong in a controller. Before adding any new function to `ctrl_dir.c` or `ctrl_file.c`, you MUST first ask: which module owns this logic? If no suitable module exists, create one. Controllers are terminal sinks - they dispatch to modules; they do not house modules.

### 1.1 Module Boundary Contract (Enforced)

The project uses a structural QA guard (`make qa-module-boundaries`) to catch architectural drift early. This guard is intentionally concrete and deterministic:

*   **No implementation includes:** `#include "*.c"` is forbidden.
*   **Per-directory dependency policy:** each source directory may depend only on approved layer(s). The target policy is:
    *   `core -> core`
    *   `util -> core, util`
    *   `fs -> core, fs, util`
    *   `cmd -> core, cmd, fs, util`
    *   `ui -> core, cmd, fs, ui, util`
*   **Legacy exceptions are explicit:** pre-existing violations are enumerated in `scripts/check_module_boundaries.py` as a temporary debt list. The guard fails on any new violation and also fails if an exception becomes stale and is not removed.
*   **Controller growth budget:** `src/ui/ctrl_dir.c` and `src/ui/ctrl_file.c` have line-count budgets as anti-regression tripwires to prevent feature creep back into controller modules.
*   **Controller top-level allowlists:** `src/ui/ctrl_dir.c` and `src/ui/ctrl_file.c` are pinned to approved top-level function sets. Any newly introduced top-level helper fails the guard unless architecture explicitly approves an allowlist update.
*   **God-function anti-growth budgets:** `HandleDirWindow` and `HandleFileWindow` have explicit line budgets; growth beyond baseline fails the guard and must be addressed by moving separable logic into dedicated modules.

### 1.2 Controller Ownership Rule (Dispatch-Only)

Controllers (`ctrl_*.c`) are orchestration boundaries, not ownership modules.

Acceptable controller contents:
*   Input/event dispatch loops.
*   Prompt/confirmation wiring that is inseparable from event flow.
*   UI-state coordination that cannot be reused outside the controller loop.

Unacceptable controller contents:
*   Reusable business logic (copy/move/compare/path transforms).
*   Generic helper utilities that could be called from non-controller modules.
*   Feature logic that can live in `src/cmd`, `src/fs`, `src/ui` helper modules, or a new dedicated module.

Canonical ownership test:
*   If a function could be called from another context without modification, it must not be introduced as a top-level controller function.

This does **not** replace architecture review. It is a fitness function: mechanical checks that fail fast when structure drifts.

## **2. Architectural Overview**
This document outlines the architectural design of `ytnova`. The codebase utilizes a modular, context-oriented C99 design.

The primary objective is to maintain a **predictable, high-integrity state machine**. Every component is designed to uphold the **Focus vs. Freeze** logic and the specific hierarchy of modal priorities inherited from the XTree lineage.

---

## 2. Execution & Concurrency Model
`ytnova` is strictly **SINGLE-THREADED**. This ensures deterministic state transitions and prevents race conditions within the Ncurses rendering pipeline.

*   **Sequential Logic:** All application logic, filesystem I/O, and UI rendering execute sequentially in the main thread.
*   **Signal Handling:** Signals (e.g., `SIGINT`, `SIGWINCH`) set atomic flags. No complex logic, I/O, or Ncurses calls are permitted inside signal handlers.
*   **Context-Passing Design:** All functions receive explicit context pointers (`ViewContext *ctx`, `YtreeNovaPanel *`, or `Volume *`) as their first argument. Global mutable state is prohibited. See **Section 3** for detailed rules and exemptions.

---

## 3. Context-Passing Architecture

`ytnova` follows a strict **context-passing** (also called "context-oriented") architecture. This is the most important structural property of the codebase — it governs how every function accesses application state.

### 3.1 The Rule

> **Every function receives the state it operates on as an explicit parameter. No function may read or write application state through global variables.**

In practice, this means every function signature begins with `ViewContext *ctx` (or a more specific context like `YtreeNovaPanel *` or `Volume *`). The `ViewContext` is the root session object; it is allocated once in `main()`, passed by pointer into every call chain, and owns all application state through its member hierarchy.

This pattern provides:
*   **Testability** — Functions can be called with synthetic contexts.
*   **Panel independence** — Two panels cannot accidentally share state through hidden globals.
*   **Auditability** — Every data dependency is visible in the function signature.
*   **LLM/tooling clarity** — Static analysis and AI tools can trace data flow without resolving global symbol tables.

### 3.2 `ViewContext` — The Session Root

The `ViewContext` struct (defined in `include/ytnova_defs.h`) is the root of all application state:

```
ViewContext (The Session)
├── left   → YtreeNovaPanel (Panel: cursor, scroll, window state, tags)
├── right  → YtreeNovaPanel (Panel: cursor, scroll, window state, tags)
├── active → points to left or right
├── volumes_head → Volume linked list (Model: shared DirEntry trees, statistics)
└── viewer, layout, mode flags, etc.
```

No component reaches "upward" or "sideways" through globals to find its siblings. All cross-component access goes through `ctx`.

### 3.3 Permitted Global Exceptions

Exactly **three** global variables exist in the codebase. Each has a specific technical justification and must not be extended:

| Variable | Type | File | Justification |
|---|---|---|---|
| `ui_colors[]` | `UIColor[]` | `src/ui/color.c` | Color palette table — mutated only by startup and F10 reload commit paths after strict config/theme validation. Failed reloads restore the previous working palette before reporting status. |
| `NUM_UI_COLORS` | `int` | `src/ui/color.c` | Derived from `sizeof(ui_colors)` — effectively a compile-time constant. |
| `ytnova_shutdown_flag` | `volatile sig_atomic_t` | `src/core/main.c` | Set by the `SIGTERM`/`SIGINT` signal handler. POSIX signal handlers cannot receive context pointers; an atomic global flag is the mandated pattern for signal-to-mainloop communication. |

> **For contributors:** Adding new global variables is not permitted. If you need shared state, add a member to `ViewContext` (or `YtreeNovaPanel` / `Volume` as appropriate) and pass it through the call chain.

### 3.4 Historical Note

The original `ytnova` codebase used pervasive global state (`CurrentVolume`, `statistic`, `dir_entry_list`, etc.) and functions with `void` parameter lists that silently operated on globals. Between 2024–2025, all 228+ function signatures were refactored to receive explicit context pointers, all global state was migrated into `ViewContext`, and the compatibility bridge (`GlobalView` pointer) was subsequently removed.

## 4. Core Architectural Data Hierarchy

### 4.1 Data Ownership
The application state is strictly hierarchical:

1.  **`ViewContext` (The Session):**
    *   The root object representing the application instance.
    *   Owns pointers to `left` and `right` panels and the `active` panel focus pointer.
    *   Owns the `volumes_head` registry of all loaded volumes.

2.  **`YtreeNovaPanel` (The View):**
    *   Represents a single UI panel.
    *   Owns **Panel-Local State**: cursor position, scroll offset, file-view anchor, focus/window mode, filespec/filter, and selected/tagged files for that panel.
    *   Holds a reference to a `Volume`.
    *   Independent panels may point to the same `Volume` while maintaining different panel-local state. Panel-local state must not be inferred from a shared tree index after the tree is rebuilt.

3.  **`Volume` (The Model):**
    *   Represents a filesystem (Physical Disk or Archive).
    *   Owns **Shared Data**: `DirEntry` tree topology, logged/unlogged memory state, file payload cache, `Statistic` metadata, and path info.
    *   Contains no panel-local UI state: no active focus, cursor, file-view anchor, selected file, filespec/filter, or panel-local tags.

### 4.2 Dual-Panel Context Isolation (F8 Logic)
The Split-Screen architecture treats each panel as an independent instance of a volume manager.

*   **Active Panel:** Owns keyboard focus and initiates all operations.
*   **Inactive Panel:** It does not process direct input and does not mutate panel-local frozen state in response to activity in the active panel. Shared-topology changes may still be mirrored and redrawn non-authoritatively.
*   **State Persistence (Tab-Switch):** The `Tab` key is the bridge. Switching panels restores the exact state held when that panel last had focus.
*   **Panel vs. Volume Rule:** Sharing a `Volume` only shares logged tree topology and file payload cache. It never shares panel-local tags, file-window anchors, cursor identity, or focus/mode state.

#### 4.2.1 Split-Panel Ownership Map (Task 1 Guardrail)
Split-transfer/switch code must classify each field before copying or restoring:

| State Class | Owned By | Examples | Forbidden Cross-Panel Behavior |
|---|---|---|---|
| **Panel-Local** | `YtreeNovaPanel` | `cursor_pos`, `disp_begin_pos`, `start_file`, `file_cursor_pos`, `file_dir_entry`, `saved_focus`, `saved_big_file_view`, `file_selection_name`, `file_selection_dir_path`, `tagged_paths`, `hide_dot_files` | Active-only commands must not mutate the inactive panel's values. |
| **Derived / Restore-Mirror** | `ViewContext` mirrors and `Volume` restore breadcrumbs | `saved_tree_index`, `saved_focus` | May shadow panel-local state for compatibility or restore, but must never become the authoritative source of truth when a panel-local copy exists. |
| **Shared-Topology** | `Volume` (possibly referenced by both panels) | `vol_stats.tree`, `dir_entry_list`, directory expansion/logging topology | Panel code may mirror topology visibility updates, but must re-anchor each panel by identity/path and must not infer panel-local selection by shared index. |
| **Session-Only** | `ViewContext` session scope | `is_split_screen`, layout windows, session options other than panel-local visibility state | Session toggles must not be treated as panel-local transfer state; split hand-off code must not use them to overwrite inactive panel-local snapshots. |

Boundary implementations in `src/ui/dir_ops.c` and `src/ui/ctrl_file_ops.c` reference this map in invariant comments and debug assertions.

#### 4.2.2 Unified Window/Panel UI State Record
The target architecture is not a greenfield rewrite. It is the canonicalization of the panel-owned state that already exists in `YtreeNovaPanel` and helper code: make one authoritative UI state record per window or panel, then route selection, viewport, and focus restore through that record instead of re-deriving them from visible rows or raw indices. Rendering is only a projection of that record.

The record must capture, at minimum:
*   **Identity and mode:** whether the container is a single-window session or a split-panel instance, plus the active focus shape.
*   **Stable identity keys:** directory identity as a stable path-based key scoped to the current volume or archive, and file identity as a stable path/name key within that same scope.
*   **Tree state:** selected directory identity, cursor position, viewport origin, and any stable tree anchor needed to restore the visible selection. Expand/collapse tree topology itself remains shared `Volume` state and must not be duplicated as panel-local expansion depth.
*   **File state:** selected file identity, file-window cursor position, file-window anchor, and the last visible file selection.
*   **Visibility state:** dotfile visibility and any other visibility filter that changes which rows are rendered.
*   **Context state:** per-volume anchors, per-panel filter text, statistics-strip visibility, and any mode flag that affects how the same tree/file model is presented.

Ownership rules:
*   `YtreeNovaPanel` owns the live UI state record for its window/panel instance.
*   `Volume` owns shared topology and payload cache; per-volume restore breadcrumbs may live there, but they are not shared-topology authority.
*   `ViewContext` owns only session-wide routing and layout references plus derived mirrors required by legacy helpers, not authoritative tree or file selection.
*   Any duplicate or shadow copy of the same state in `ViewContext` or helper paths must be either explicitly derived-only or removed; stable path-based identity is the restore key, not transient row indices or stale pointers.
*   In split mode, restore snapshots are keyed by `(panel, volume)` so each panel restores its own volume-specific state and cannot import the opposite panel's snapshot.

Update rules:
*   User navigation mutates the owned record directly.
*   Redraw/reflow paths may compute a temporary render projection from the record, but they must not replace the record with a new guess.
*   Restore/reactivation paths must rehydrate from the saved record and use deterministic fallback only when the original identity is no longer valid.
*   The invalid-selection fallback order is fixed: exact identity if still valid, then nearest visible ancestor, then next visible sibling, then previous visible sibling, then the root visible node.
*   Restore code must not reconstruct selection from `disp_begin_pos + cursor_pos` or from stale `DirEntry*`/`FileEntry*` pointers.
*   Reactivation must restore the recorded tree/small/big-file shape directly; it must not briefly render a different shape before converging.
*   Restore invalidation is generation-based: any tree rebuild, visibility change, rename, move, symlink change, or mount remap that can invalidate a saved identity must advance the panel/volume restore generation before any snapshot is reused.
*   Restore ordering is explicit: rebuild/mutation completes, generation updates, then restore helper re-resolves the saved identity or falls back deterministically. No helper may read selection from raw row math while a rebuild is in flight.
*   There must be one canonical restore helper per container type; alternate code paths must not synthesize their own restore logic or bypass the helper.
*   Verification is mandatory: regressions must prove no flicker, no reanchor, and no transient wrong-shape render on Enter/Tab/F8 and hidden-dotfile reactivation paths, including split-panel restore cases.

This record is the implementation-side counterpart to the contract stated in `docs/SPECIFICATION.md`. Future stateless agents must treat it as the canonical UI state model.

Canonical restore boundary: `CapturePanelAnchorPath`, `FindDirIndexByPath`, `FindDirIndexByPathOrAncestor`, `PositionPanelAtIndex`, `RestorePanelAnchorPath`, and `EnsurePanelAnchorVisible` in `src/ui/panel_anchor.c` / `include/ytnova_panel_anchor.h` are the intended restore helpers for this contract. Other modules may request restore through that API, but they must not invent alternate restore authority or re-derive panel-local selection/viewport state independently.

#### 4.2.3 AppState Transition Contract
`AppState` is the single formal application-state root for the transition contract. During migration, the runtime `ViewContext` remains the concrete carrier that maps to `AppState`: `ViewContext` owns session routing and layout references, `YtreeNovaPanel` owns panel-local UI state records, and `Volume` owns shared topology and payload cache. Runtime code must not introduce a second root or let render-derived values become state authority while this mapping is incomplete.

The formal child regions are:
*   **Session region:** active panel routing, split/single-window mode, modal state, command state, message state, and render invalidation flags. Owner: `ViewContext`.
*   **Panel regions:** one region for each window/panel instance with stable identity keys, focus shape, tree/file cursor and viewport anchors, visibility/filter state, restore snapshot, and panel generation. Owner: the corresponding `YtreeNovaPanel`.
*   **Volume regions:** logged volume/archive namespace, shared directory topology, payload cache, shared visibility/topology generation, and model statistics. Owner: `Volume`.
*   **Modal/command subregions:** transient prompts, menus, confirmations, external command completion, and operation results. Owner: `ViewContext`, with writes to panel or volume state only through an allowed transition commit.
*   **Render projection region:** dirty surfaces, layout geometry, and ncurses window handles. Owner: `ViewContext`; this region may project state but must not select new authoritative identities.

Every transition record must declare its category, source state, event, guard, allowed and blocked results, target state, owner, write set, generation effect, side effects, render invalidation, migration boundary status, and follow-up notes. The machine-readable registry in `registry/appstate/appstate_transition_matrix.json` is the source for this metadata until runtime transition objects exist. Required categories include keybinding, menu action, modal action, refresh/rebuild, volume operation, terminal signal/resize, filesystem mutation result, command completion, rebuild/rebind callback, and render reflow.

Transition execution follows this statechart contract:
1.  Capture the current `AppState` region snapshots needed by the event.
2.  Evaluate the transition guard before mutating any authoritative region.
3.  If allowed, write only the declared owner/write-set fields.
4.  Apply generation effects before restore or render consumers observe the change.
5.  Rebind stale identities through the canonical restore helpers or use the deterministic fallback order.
6.  Mark only the declared render surfaces dirty, then render from projection data.

Blocked transitions are fail-closed. A blocked transition must leave authoritative panel and volume records unchanged, except for explicitly declared message/modal fields needed to communicate a user-visible constraint. It must not partially advance generations, mutate inactive panel snapshots, perform hidden filesystem side effects, or repair state by row-index guesses.

Generation metadata is part of transition correctness:
*   `panel_generation` advances when panel-local selection, viewport, focus shape, filter, visibility, or restore snapshot authority changes.
*   `volume_generation` advances when shared topology, payload identity, logged/unlogged state, visibility set, or namespace mapping changes.
*   A generation mismatch forces stable-identity re-resolution before any snapshot is applied.
*   Rendering alone never advances either generation.

Rendering is projection only. Render/reflow paths may compute temporary row positions and clipped viewports from settled `AppState`, but those temporary values are discarded after drawing. A renderer must not choose a new tree/file selection, overwrite a saved viewport, or synthesize focus shape from visible rows. If projection cannot be computed safely, rendering must degrade or skip while leaving authoritative state intact.

Compatibility shims are retired from the current AppState contract. Any legacy mirror or alternate authority path is a defect that must be removed rather than documented as an accepted AppState boundary.

### 4.3 Inter-Panel Operations (The Directional Rule)
*   **Targeting:** Copy and Move operations occur directionally: **Source (Active Panel) to Destination (Inactive Panel)**.
*   **Read-Only Bridge:** The active panel reads the path of the inactive panel to set a default destination without altering the inactive panel's state.

---

## 5. Behavioral Protocols

### 5.1 Protocol A: Directory Entry and "No Files" Constraint
*   **Transition Invariant:** A directory can only be entered (Tree to File Mode) if it contains at least one file.
*   **Selection Memory (Breadcrumbs):** When returning from File Mode to Tree Mode and later re-entering the same directory, the panel restores the cursor to the last highlighted file.
*   **Navigation Stability:** Moving through the Tree never automatically triggers a transition into File Mode.
*   **Tag-View Scope Rule:** `i/I` (invert tags) and the `Filter` prompt's tagged-only toggle (`F`, then `Tab`) operate on the active panel's current file-list scope, regardless of whether focus is in tree or file window.
*   **Root Boundary Rule:** `Left` on an expanded root performs the same node-local reset as `-` (collapse + release/unlog). Further `Left` on an already-unlogged root is a no-op.

### 5.2 Protocol B: Archive and Volume Lifecycle
*   **Lifecycle Management:** The active panel handles Logging new volumes, Cycling through logged volumes, and Releasing (unlogging) volumes.
*   **Volume Menu Selection Rule:** Selecting an already-active loaded volume preserves its current in-memory expansion/collapse/tag/log state (no implicit relog).
*   **Explicit Relog Rule:** Logging an already logged volume/path performs a fresh reload of that volume state and reanchors selection at the volume root.
*   **Independent Rooting:** Changing the root or volume in the active panel has no impact on the inactive panel.

### 5.3 Protocol C: F7 Autoview
*   **Contextual Logic:** F7 displays content for files and shows a file list temporarily for directories.
*   **Dynamic Background Navigation:** While F7 is active, Up/Down keys move the cursor; the preview updates in real-time.
*   **Undo Protocol:** Pressing `F7` or `Esc` destroys the overlay and returns the user to the **exact** position and mode held before the preview. No state changes persist.

---

## 6. Visual and Rendering Standards
*   **Terminal Integrity:** UI updates are staged using `wnoutrefresh()` and committed atomically via `doupdate()` to prevent visual artifacts.
*   **Theme Configuration Boundary:** Runtime theme selection is read from the main profile, but role definitions and file-type palettes live in separate theme files. Packaged defaults are `etc/ytnova.conf` and `etc/ytnova.themes`; user discovery prefers XDG paths and falls back to home-directory dotfiles only when the XDG targets cannot be used.
*   **Edit Authority Rule:** `F10` edits the active user file for the selected surface (XDG or home-dotfile fallback); when runtime is using built-in defaults for that surface, `F10` materializes the XDG file for that surface and edits it. For command customization, `F10` edits only the active `commands.conf` surface; it does not edit packaged preset files in shared app-data paths.
*   **Command Preset Boundary:** Locale/layout-aware command presets are packaged read-only data keyed by stable preset ID and action ID. `etc/ytnova.commands` is the packaged default active command map; `commands.conf` may select zero or one packaged preset and then apply local overrides. Presets may seed the active command map, but they are not a second editable user config surface and they must not bypass the structured command-resolution path used by footer keybinding/F1/menu rendering.
*   **Command Surface Identity Rule:** Command rows belong to stable runtime command-surface IDs, not to languages or storage back-end names. Canonical surfaces include at least directory/file and archive-directory/archive-file variants, and future surfaces may add new stable IDs without changing the row grammar or footer keybinding/F1 assembly pipeline.
*   **Footer Packing Boundary:** Footer packing is a runtime layout algorithm, not a label-specific lookup table. The packing stage preserves stable command order, measures rendered entry widths, and chooses row splits by minimizing the width delta between the two top command rows. Any label-aware preference is allowed only as a tiebreaker between otherwise equivalent balanced fits; it must not become the primary wrap authority.
*   **Canonical Help Source Boundary:** Authored contextual-help prose belongs to `etc/help/f1.en.md`, and authored man/USAGE reference prose belongs to `etc/help/man.en.md`, not to scattered ad-hoc runtime literals and not to the generated long-form outputs. The in-repo generator `scripts/generate_help_assets.py` projects those sources into `etc/ytnova.1.md`, `docs/USAGE.md`, the build manpage output, and the generated runtime help asset `src/core/generated_help_topics.h`.
*   **Help Topic Schema Rule:** Every canonical help topic block uses an exact `## topic:<id>` heading, an immediate `ytnova-help-meta` fence with `title:` and `contexts:`, required `### Contextual F1` and `### Long form` sections, and optional `### Explainer links` entries written as `[Label](topic:<id>)`. Link-only explainer pages declare `contexts: none`.
*   **Help Topic Mapping Boundary:** Stable topic IDs are content identities; stable runtime context/prompt IDs live only in topic metadata and the generated runtime lookup path. Reuse happens by mapping multiple contexts to one topic or by linking to shared topics, not by cloning the same prose into separate authored files.
*   **Runtime Help Consumer Boundary:** `src/ui/runtime_help.c` is the sole UI consumer of `src/core/generated_help_topics.h`. It resolves context IDs to generated topics, appends runtime-owned footer/prompt command rows for parity, and lets popup footer link commands hop across shallow explainer topics without re-embedding help prose in controllers.
*   **User Persistence Family Rule:** Config-like editable surfaces (`ytnova.conf`, `themes.conf`, and `commands.conf`) live under `$XDG_CONFIG_HOME/ytnova/` or `~/.config/ytnova/`; packaged command preset catalogs live under shared read-only app data (for example `/usr/share/ytnova/commands/`); session state such as command history lives under `$XDG_STATE_HOME/ytnova/` or `~/.local/state/ytnova/`. Home-directory dotfiles are compatibility fallbacks only when those XDG-style targets cannot be used.
*   **Preset Packaging Rule:** Every packaged preset file carries concise top-of-file comments naming the stable preset ID, intended locale/layout audience, packaged read-only status, and the invariant that action IDs remain untranslated.
*   **Junction Grammar:** Ncurses junctions (T-pieces, crosses) are used only for horizontal boundary lines. Vertical separators must remain clean.
*   **Tree State Rendering Contract:** Unlogged state is rendered in the dedicated tree status-margin column; directory names do not carry a `+` suffix, while `/` may still be shown when the directory has subdirectories.
*   **Micro-Consistency:** Mode flags must be synchronized with the layout before any redraw.
*   **Background Ownership:** Set a window background once per refresh path, then clear before drawing. Entry renderers may set temporary attributes but must not change the window background inside item loops.
*   **Semantic Role Projection:** Renderers consume semantic role pairs. F1/context help uses `help`; picker surfaces use `picker`; tree status columns use `margin`; tree guide glyphs use `tree_lines`; directory tree names stay on `dynamic_text`; severity errors use `error`; search-hit spans use `search_hit` and then restore their surrounding content role.
*   **Theme Authoring Rule:** Shipped/default themes must omit redundant backgrounds on ordinary content roles when they are meant to follow the theme background, so changing `background` repaints the shared surface without requiring users to edit every text role.
*   **File-Type Palette Boundary:** File-type palette rules are optional per-theme filename decoration. They are evaluated top-to-bottom as first-match-wins rules, resolve to full foreground/background pairs, and must not style directory tree rows.
*   **Frame/Fill Separation:** Window fills use the content/background role for their surface, while frames and separators use `box_lines`; do not map a border role onto the full window fill.
*   **Stats Role Split:** In stats rendering, box lines use `box_lines`, stats titles and fixed labels use `static_text`, and changing stats values use `dynamic_text`.

---

## 7. Directory Structure
*   **build/**: Compiled binary outputs.
*   **docs/**: Project documentation and specifications.
*   **etc/**: Default configuration files.
*   **include/**: C header files.
*   **obj/**: Intermediate object files.
*   **src/**: Source code:
    *   `src/core/`: Initializers and session management.
    *   `src/fs/`: File system and archive handling.
    *   `src/ui/`: Ncurses rendering and window management.
    *   `src/cmd/`: User command implementations.
    *   `src/util/`: Utilities and history_utils management.
*   **tests/**: Behavioral TUI tests.
