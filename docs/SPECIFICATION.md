# **Functional Specification**
> **Purpose:** This document defines the behavioral "Contract of Truth" for `ytnova`. It specifies how the UI must respond to input, how the filesystem is represented, and the design philosophy that governs the user experience.
> This specification defines behavior contracts only; detailed regression-test inventories and case matrices belong in roadmap/planning artifacts and the test suite, not in this file.

## **Normative Language**

In this specification, normative words are literal requirement levels:

*   **must** = required behavior. Implementations and follow-on changes are not allowed to ignore it.
*   **should** = preferred behavior that may be violated only with an explicit, documented justification.
*   **may** = optional behavior.

Ambiguity is not acceptable in normative wording. If a rule is intended to be mandatory, it must use `must` rather than softer phrasing.

## **1. Design Philosophy**
The `ytnova` interface is built to make the power of the Unix filesystem accessible through a high-speed, intuitive terminal interface.

*   **Unix-First Design:** Prioritize a user experience tailored for Unix power users, emphasizing shell integration, standard POSIX conventions, and scriptability.
*   **YtreeNova makes filesystem work self-evident:** Users must not need command-line fluency or Unix jargon to succeed; core actions must be visible, named plainly, and understandable from the interface itself.
*   **Interaction Economy (Minimize Friction):** `ytnova` is designed to minimize the distance between user intent and execution. Avoid unnecessary confirmations for safe operations and ensure the common path is always `key -> Enter -> result`.
*   **Direct Access (No Menu Diving):** High-speed keyboard access is superior to hierarchical navigation. Core functionality must be accessible via single-key or simple combinations; UI depth must never exceed one level for primary actions.
*   **No Hidden Features:** All functionality, especially syntax like the `{}` placeholder, must be explained in context within the UI (e.g., in help lines or prompts).

## **2. The User Interface Architecture**

### View-State Ownership Overview
`ytnova` uses one state model across both single-window mode and split-panel mode. When `F8` is off, the active container is a **window**; when `F8` is on, each side is a **panel**. `F8` changes the layout container, not the meaning of the stored state.

Each window or panel owns its own frozen selection, viewport origin, focus shape, and dotfile visibility. Reactivation, redraw, and restore paths must reuse that frozen state; they must not re-derive selection or viewport from raw tree indices or visible-row assumptions.

Hidden versus shown dotfiles is an orthogonal visibility setting. It can change which rows are visible, but it must not by itself cause re-anchoring, expansion changes, or viewport jumps unless the user explicitly navigates or the current selection is truly invalid.

Directory and file identity must be defined by stable path-based keys scoped to the current volume or archive, not by transient row positions or pointer identity. Rename, move, symlink, and mount-remap operations can change whether an identity still resolves, but they must not change the restore contract: if the stored identity still resolves, restore it; if it does not, use the deterministic fallback order in §5.5.5.

This section states the intended contract. Where current behavior differs, the contract is the target to converge on, not a description of the present implementation.

The architectural state model that backs this contract is described in `docs/ARCHITECTURE.md` as the per-window / per-panel UI state record and its ownership rules.

The formal AppState transition contract is defined in `docs/ARCHITECTURE.md` §4.2.3 and backed by the machine-readable AppState registries under `registry/appstate/`. This specification states user-visible behavior; transition ownership, write-set, generation, blocked-transition, render-projection, and related contract metadata belong to the architecture contract and must not be duplicated here.

### 2.1 Input Semantics

ytnova separates **view-state toggles** from **one-shot actions**:

*   **`Enter` toggles Tree/File focus states** in normal navigation flow.
*   **`F7`/`F8` are toggle view modes:** Preview and Split Screen are stateful layout modes toggled by repeating the same key.
*   **`s`/`g` are mode-entry keys, not same-key toggles:** they enter Showall/Global file-list states; behavior of repeated `s`/`g` follows that state's local keymap.
*   **Actions are one-way:** repeating the same key can run another action, insert input into an active prompt, or do nothing; it never means undo.
*   **Esc** is universal cancel/return. It does **not** undo completed filesystem mutations.

### 2.2 The Layout Grid
The screen is divided into non-overlapping zones. Geometry is calculated dynamically, except for the Stats Panel.

| Zone | Geometry | Content | Behavior |
| :--- | :--- | :--- | :--- |
| **Header** | Row 0 | Volume, Path, Clock, Version | Updates on every navigation event. |
| **Tree View** | Top-Left | Visual directory hierarchy | Primary Navigation anchor. |
| **File View** | Bottom-Left | File list of selected directory | Shows "** No files **" if empty; "** Unlogged **" if unread. |
| **Stats Panel** | Right Column (**Fixed 26**) | Metadata, Filters, Disk Stats | Context-aware. Always visible in Standard Mode. |
| **Command Area** | Bottom 3 Rows | Menu, Prompts, Messages | Handles all user interaction feedback. |

### 2.3 Visual Grammar (The "XTree Look")
*   **Junction Grammar:** Ncurses junctions (T-pieces, crosses) must **only** be used for horizontal boundary lines. Vertical separators must remain clean, unbroken lines to avoid visual clutter.
*   **Empty State:** If a directory contains no files, the File View window must display the text: `** No files **`.
*   **Small-Window Name Column Alignment:** In the small File View, reserve the first post-border cell for tag (`*` or space), the second as a spacer, and start all row text at the same column. This applies to regular names, symlink labels, and placeholders (e.g., `No files`, `Unlogged`).
    Untagged: `│  check_xml_integrit`, `│  @current`, `│  No files`, `│  Unlogged`
    Tagged: `│* check_xml_integrit`, `│* @current`, `│  No files`, `│  Unlogged`
*   **Single-Row List Invariant:** Tree/File/Showall/archive list rows must never wrap to a second terminal line.
*   **Informative Truncation Policy:** When width is insufficient, content must be truncated (not wrapped) using a deterministic strategy that preserves the most useful identity cues. Prefer `prefix…suffix` elision when both ends carry meaning (for example filename stem + extension or path tail); use one-sided clipping only when the omitted side is low-value in that context.
*   **Static Text Rule:** Truncated UI labels are static and stable while focused; marquee/auto-scrolling text is not permitted for core list and attribute surfaces.
*   **Motion-Only-When-Informative Rule:** UI animation is avoided by default. Motion is permitted only when it conveys live operational state (for example scanning/copying progress, spinner/ETA/progress counter) rather than decorative movement.
*   **Progress Lifecycle Rule:** Every filesystem or virtual-filesystem operation that can block the event loop MUST show immediate activity in the footer's reserved spinner cell. If the operation is still running after approximately one second, also show a compact progress bar in the main display. The footer spinner remains live so a temporarily stationary bar never looks hung.
*   **Progress Indicator Selection Rule:** Use an indeterminate bar only while a trustworthy total is being discovered. A bounded, read-only tree preflight is permitted before mutation when it validates the same source tree and provides the operation total without decoding payload data twice. Once totals are installed, the bar MUST change to determinate mode exactly once, show at least one filled cell after positive work begins, advance monotonically, and remain determinate until the terminal result. Multi-phase totals MUST cover every blocking phase owned by the operation, including archive scan/rewrite, payload transfer, refresh, and Move source removal, so the bar cannot report completion while work is still running.
*   **Progress Detail Rule:** A promoted determinate bar reports `Progress`, `Elapsed`, `Left`, and `Rate`, in that order. Durations use compact adaptive units: seconds alone below one minute (`8s`), minutes plus two-digit seconds below one hour (`1m 08s`), and hours plus two-digit minutes and seconds thereafter (`1h 02m 03s`). Elapsed time starts with the outer operation, and remaining time is derived from total completed work when progress has begun. Byte rate is shown for byte-bearing work and item rate for item-only work. When byte and item totals describe different phases of one operation, completion and remaining time account for both dimensions. An indeterminate bar reports unavailable values as dashes and shows the truthful work-unit count instead of inventing a percentage, remaining time, or rate.
*   **Progress Surface Ownership Rule:** The spinner belongs only to its reserved footer cell. A promoted progress bar belongs to a separately owned window centered in the main display. Neither surface may overwrite or hide footer keybinding hints, active prompt text, or F1 help. If layout is constrained, omit the centered bar and retain the footer spinner instead of replacing those surfaces.
*   **Regression Guard:** No-wrap/truncate behavior is a required regression-test contract across normal and archive view modes.
*   **Micro-Consistency:** UI state flags (e.g., `big_window`, `split_mode`) must be synchronized with the internal state machine before any call to `doupdate()`.
*   **Viewport Ownership Rule:** Each panel owns its own tree viewport (`disp_begin_pos` and `cursor_pos`). Split, tab, and Home/End navigation MUST only mutate the active panel's viewport; the inactive panel's tree viewport must remain unchanged.
    *   Panel handoffs and redraws MUST resolve the selected directory through the same visible-selection logic used by rendering.
    *   Do not recompute or “correct” tree selection with a raw `disp_begin_pos + cursor_pos` index in callers.
    *   Do not duplicate viewport-placement policy in multiple call sites; use the shared helper so hidden-dotfile trees and split-screen redraws follow one canonical scroll rule.
    *   Hidden dot directories do not earn extra viewport shifts. If the target row is already visible, the panel must keep its current viewport origin.

### 2.4 Tree Status Column
The first character column of the Tree View serves as the Memory State Indicator:
*   `+` : **Unlogged.** The directory entry is visible in the tree, but its file list is not in memory. The File View must display `**Unlogged**`.
*   ` ` (Blank): **Logged.** The file list for this directory is resident in memory.
*   Directory-name suffix contract: `+` is a status-margin marker (not a name suffix). A trailing `/` may still be shown on a directory name to indicate subdirectory presence.

---

## 3. Navigation & Focus Logic

### 3.1 Focus Flow (`SMALLWINDOWSKIP`)
The behavior of the `Enter` key on a directory node is governed by the configuration:

*   **State gate (all configs):** If the selected directory is unlogged/not-yet-scanned, `Enter` performs one-level log/reveal (same as `+`) and keeps focus in Tree View.
*   **Bypass Mode (`SMALLWINDOWSKIP=1`):**
    *   `Enter` on a logged directory in Tree -> **Instant Zoom**. File Window expands to full height.
    *   `Enter` or `Esc` on Zoomed Window -> Returns focus to the **Tree View**.
*   **Staged Navigation (`SMALLWINDOWSKIP=0`):**
    *   `Enter` on a logged directory in Tree -> **Focus Shift**. Focus moves to the File View (Small Window). Tree remains visible.
    *   `Enter` on Small Window -> **Zoom**. File Window expands to full height.
    *   `Enter` on Zoomed Window -> Returns focus to the **Tree View**.
*   **Navigation Stability:** Moving the cursor through the Tree must **never** automatically trigger a transition into File Mode or Zoom.

### 3.2 Directory Protocols
*   **Logging vs. Entry:** "Logging" is the act of scanning a directory branch. Any directory can be logged and exist in the Tree. However, **Entry** (transitioning focus from Tree to File View) is strictly prohibited if the directory contains zero files.
*   **Selection Memory (Breadcrumbs):** When returning from File Mode to Tree Mode and later re-entering the same directory, the panel must restore the cursor to the **last highlighted file**.
*   **Split-Panel File Ownership:** In split mode, each panel preserves its own file-view snapshot (`start_file`, `cursor_pos`, and file-selection anchors). `F8` seeds the new peer from the active panel's current file cursor, and `Tab` can switch panes without importing or resetting the inactive pane's file cursor.
    *   Exiting file mode returns only the active panel to tree focus.
    *   The inactive panel keeps its file snapshot intact for later reactivation.

### 3.3 Directory Memory Commands (Structural Controls)
*   **`+` or `=` (Expand):** Expand using configured `TREEDEPTH` behavior for the node context. `=` is a convenience alias (unshifted `+` on most keyboard layouts).
*   **`*` (Asterisk):** Deep Log. Recursively scans the entire branch.
*   **`M` (Make Directory):** Directory creation updates the current tree in place. It must not implicitly relog, reset expansion depth, or reanchor the viewport when the current selection remains valid and visible; only minimal bounds correction is allowed if the mutation would otherwise leave the current cursor or viewport offset invalid.
*   **`-` (Minus / Collapse):** Collapsing a directory node is a state reset for that node. When `-` collapses a currently expanded node, that subtree is released/unlogged. Re-expanding starts from normal configured depth behavior instead of restoring prior ad-hoc expansion history.

### 3.4 Arrow Key Navigation (Spatial Controls)
Arrow keys provide spatial, cursor-oriented navigation through the tree. They are distinct from the structural `+`/`-`/`*` controls:
*   **`→` (Right Arrow / Drill Down):** Progressive depth navigation. If the node is collapsed: expand one level. If already expanded: move cursor to the first child.
*   **`←` (Left Arrow):** If the selected directory is expanded, collapse it. Otherwise, move selection to its parent directory. Collapsing with `Left` is a state reset for that node; after reset at filesystem/archive root, further `Left` is a no-op.
*   **Tree Up/Down Edge-Scroll Rule:** `Up`/`Down` move the tree selection within the current visible viewport without changing the viewport origin while the target row is still visible. The tree scrolls by one visible row only when movement would pass above the top visible row or below the bottom visible row.
*   **Tree Home/End Visibility Rule:** `Home`/`End` move to the first/last visible tree row, but they must preserve the current viewport origin whenever the target row is already visible; viewport movement is allowed only when needed to keep the target row visible. Hidden dotfile rows do not grant extra viewport shifts.
*   **Enter/Restore Viewport Rule:** `Enter`, `Tab`, and other panel restore flows must preserve the current tree viewport origin whenever the active tree selection is already visible. They must reanchor the viewport only to preserve visibility, never as a redraw side effect.
    *   When a split is closed or a panel is restored from a same-volume handoff, the surviving panel must re-resolve its own tree selection from its panel-local anchors; it must not inherit the opposite panel's cursor row if the preserved selection is still visible.
    *   Split-close preserves the active panel's current focus and shape. If the right panel is active, its tree/file state is donated into the surviving left-side storage so single-panel mode continues from the same cursor, viewport, file selection, dotfile visibility, and tree/small-file/big-file focus.
*   **Hidden-Prefix Selection Accounting Rule:** When dotfiles are hidden, the panel's cursor position is interpreted against the visible tree rows. Any conversion back to a raw directory index must walk the visible rows from the current viewport origin instead of assuming `disp_begin_pos + cursor_pos` is the selected entry.

### 3.5 Preview Mode (`F7`) Contract
`F7` is the primary inspect mode for viewing file contents while retaining list-oriented navigation.
*   **Activation/Toggle:** `F7` enters preview mode and `F7` again exits preview mode.
*   **Layout Contract:** Preview mode presents a file-list pane and a content-preview pane simultaneously.
*   **List-First Navigation:** Standard list navigation changes selection in the list pane; preview content updates to the selected file.
*   **Preview-Scroll Modifiers:** Shift-modified navigation keys (and configured equivalents such as `^P`/`^N`) scroll preview content without changing list selection.
*   **Mode Safety Rule:** Split-navigation controls are not active while preview mode is active (for example `F8` and `Tab` must not perform split/layout switching from inside preview mode).
*   **Command Availability Rule:** Preview mode must expose only a defined in-context command subset; unavailable commands must not trigger unintended mode/layout transitions.

---

## 4. Keyboard Interaction Taxonomy

The `ytnova` input system follows a layered model designed for high-speed interaction and contextual efficiency.

### 4.1 Input Principles
*   **Case-Sensitivity:** Keys are **case-insensitive** by default. Lowercase notation is used for letter-based commands (e.g., `c` for copy). The Ctrl key is shown by the `^` symbol.
*   **Standard Conventions**: Function keys use the `F1`-`F12` (uppercase prefix) notation. Control keys use the `^key` (e.g., `^l`) lowercase notation.
*   **Alt-Key Portability Rule:** `Alt`/Meta key sequences are terminal-dependent and are not part of supported key contracts. Core workflows must use non-`Alt` bindings.
*   **Control-Alias Canonicalization Rule:** Terminal-equivalent aliases (`^M`/Enter/CR, `^J`/LF enter path, `^I`/Tab, `^[`/Esc) are a single canonical input for binding and validation; mapping alias forms to different commands is invalid.
*   **Keyboard Portability Baseline:** ytnova keyboard semantics follow curses `getch`/`KEY_*` behavior with terminfo capability mapping (practical references: [`curs_getch(3x)`](https://man7.org/linux/man-pages/man3/curs_getch.3x.html) and [`terminfo(5)`](https://man7.org/linux/man-pages/man5/terminfo.5.html)).
*   **Contextual Logic:** The effect of a key depends on whether focus is on the Tree View or File View.

### 4.2 Interaction Layers

| Category | Definition | Behavioral Persistence |
| :--- | :--- | :--- |
| **Linguistic Mnemonics** | Keys bound to command strings (e.g., `c`=copy, `m`=move). | Primary candidates for locale/layout-aware preset remapping. |
| **Structural Controls** | Positional keys (`+`/`=`, `-`, `*`) that manipulate the tree. | Static; universal regardless of locale. |
| **Spatial Navigation** | Arrow keys (`←`, `→`) for cursor-oriented tree traversal. | Fixed; directional drill-down / retreat. |
| **TUI Conventions** | Universal terminal muscle memory (`/`, `^l`, `^v`, `^q`). | Fixed; standard Unix utility behavior. |
| **State Toggles** | Binary or stateful switches (`` ` ``, `0`, `F7`, `F8`). | Stateful; toggles UI display modes. |
| **Control Aliases** | ASCII Control characters as functional aliases. | Fixed at the terminal protocol level (e.g., `^m` = Enter). |
| **Help Popup** | The contextual `F1` overlay and its linked explainer pages. | Highest transient precedence; while open it consumes its own navigation/follow/close keys before prompt, picker, preview, split, or base-mode dispatch. |
| **Prompt Interactions** | Contextual shortcuts active only during text prompts. | Specialized editing and browsing tools. |

### 4.3 Key Behavioral Rules
*   **The Minus Rule (`-`):** Collapsing an expanded node is a node-local state reset: the subtree is released/unlogged, and later expand does not restore prior ad-hoc expansion history.
*   **The Right Arrow Rule (`→`):** Progressive drill-down. Expand collapsed → move to child. Always takes the user one step deeper into the tree.
*   **The Root-Left Rule (`←` at root):** `Left` on expanded root performs the same node-local reset (collapse + release/unlog). Further `Left` on already-unlogged root is a no-op.
*   **The Plus/Equals Rule (`+`/`=`):** Explicit expand using configured `TREEDEPTH`. No cursor movement. `=` is the unshifted alias for `+`.
*   **The Tree Marker Rule (`+` status):** Unlogged state is rendered only in the dedicated tree status margin column; directory names do not carry a `+` suffix.
*   **The Volume Menu Rule (`K` menu):** Selecting the already-active volume preserves its current in-memory state (no implicit relog/reload).
*   **The Explicit Relog Rule (`L` on current path):** Logging an already logged volume/path performs a fresh relog/reload of that volume state and reanchors selection at volume root.
*   **The Invert Rule (`i`/`I`):** In both tree and file windows, invert tags applies to the active panel's current file-list scope (filesystem and archive contexts).
*   **The Only-Tagged Rule (`F`, then `Tab`):** In both tree and file windows, the filter prompt can toggle tagged-only file-list view for the active panel's current scope. In that prompt, text before the colon names the filter scope (`all files` or `tagged only`) and text after the colon is the filespec/filter expression; `Tab` toggles only the scope and never changes the current expression or tag state.
*   **The Archive/Global Jump (`\`):** In Archive Mode, jumps to the archive root. in Global/Showall views, jumps to the highlighted file's directory.
*   **Numeric FileInfo Band (`1..9`):** Number keys are the canonical file-display controls in normal list contexts (not active in `F7` preview). The footer presents this as `1..9 dir view` in tree/directory focus and `1..9 file view` in file focus. `1` is the simple default/baseline file or directory view, and it is also the reset-to-default selection for temporary FileInfo extras. By default, `1..4` change the active panel's shared view so tree/directory and file windows follow each other. Pressing the already-active `2`, `3`, or `4` again resets that context back to `1` / Name. Selecting `1..4` also returns that file projection to its named base view, clearing temporary compact/overlay state there. `2` is the Attributes view and now owns `name -> target` symlink detail in file projections. `SEPARATE_DIR_FILE_VIEWS=1` restores split behavior so `1..4` change only the currently focused context. `5` toggles the compact Name file rendering variant (the replacement for Brief) only when the current focused `1` / Name base view is active; it does not create compact Attributes/Owner/Times variants and is a silent no-op from `2`, `3`, or `4`. `6` toggles binary vs human-readable size units for directory/file rows only; stats remain human-readable. `7` toggles Mini preview detail that shows the start of readable file contents on every visible file row. `8` toggles File detail (`file`-style type-summary text, with a coarse built-in fallback when external type text is unavailable) on every visible file row. `9` toggles the Git status band for filesystem file lists inside Git worktrees. `5`, `7`, `8`, and `9` never change tree rows; they update the panel's file projection instead, so in tree focus they affect the embedded small file window and in file focus they affect the file window. `6` changes size formatting across the panel's directory/file row surfaces. Extra view states do not stack in the stats label; the current file/directory section names the one visible active state (`Compact`, `Mini preview`, `File`, or `Git`) so users do not have to decode the numeric band from the footer alone. `9` remains a silent no-op outside Git worktrees.
*   **Archive File Information Key (`0`):** On a filesystem-backed active panel, `0` is a silent no-op; `F6` alone shows or hides the active panel's stats. On an archive-backed active panel, `0` toggles `Size`, `Packed`, and `Ratio` details on each visible file row. `Size` is the original member size, `Packed` is the member's space inside the archive, and `Ratio` is the percentage of space saved (`100 × (1 - Packed / Size)`). A dash means the backend cannot derive a trustworthy per-member packed size or the ratio is undefined. Trustworthy packed metadata is captured as part of loading each archive generation, so pressing `0` only changes the file-row projection and never starts archive I/O. Split-panel behavior is resolved from the active panel volume only.
*   **Vi-Key Collision Policy:** When `VI_KEYS=1`, lowercase `h/j/k/l` are reserved for navigation. Uppercase `H/K/L/J` are used for commands (Hex, Volume, Log, Compare).
*   **Tagged Actions**: `^u` (Untag All) and `^d` (Delete All Tagged) provide batch operations across the visible scope.
*   **Quit to Directory (`^q`):** Exits `ytnova` to the currently highlighted directory (requires shell-level support to finalize the shell path).
*   **Localized Command-Preset Rule:** Locale/layout adaptation for linguistic mnemonics is provided by explicit command presets keyed by stable preset ID and action ID, not by runtime guesses about physical keyboard layout. A preset selects a command-layout variant; it may encode localized labels, mnemonic choices, keyboard-layout accommodations, or a combination of those, but it does not by itself select the whole application language. A package can choose a default preset, and the user can override it, but runtime command resolution still starts from curses/terminfo key events plus the resolved active command preset.

### 4.4 Function Key Blueprint (F1-F12)
*   **`F1`**: help.
*   **`F2`**: directory picker (Prompts Only).
*   **`F5`**: refresh.
*   **`F6`**: unassigned.
*   **`F7`**: autoview toggle.
*   **`F8`**: split-screen toggle.
*   **`F9`**: Applications menu.
*   **`F10`**: configuration.

### 4.4.1 Applications Menu Contract
*   **Launcher Role:** `F9` is the named-preset launcher for repeat-heavy external workflows. It is not the ad hoc shell prompt and it is not a keystroke recorder.
*   **`eXecute` Separation Rule:** `eXecute` (`X`) remains the one-off shell command surface with history and terminal-style output. `F9` is for saved launchers, helper scripts, and repeatable application workflows.
*   **Immediate Return Rule:** The common-path `F9` flow is `F9 -> Enter -> launched application -> working view`. After a preset starts, runtime MUST return to the TUI immediately without a blocking `PRESS ENTER` acknowledgment step.
*   **Independent Process Rule:** The launched preset process MUST continue independently of the ytnova TUI. If a preset needs its own terminal, the preset command must open that terminal itself (directly or through a helper script).
*   **Selection Placeholder Rule:** In the applications catalog, `{}` means the file or folder currently selected in ytnova.
*   **Prompt Placeholder Rule:** In the applications catalog, `{input}` means the text the operator typed when the preset asked for extra input.
*   **Working-Directory Rule:** Application presets MUST start in the active selection's directory context. In file focus that means the selected file's parent directory; in tree focus that means the selected directory itself. Presets without `{}` still inherit that working directory.
*   **No-Implied-Target Rule:** If a preset omits `{}`, runtime MUST NOT silently append a selected path argument. The selection still defines the preset's working directory under the Working-Directory Rule.
*   **Plain-English Help Rule:** Runtime help and starter-catalog comments MUST explain `{}` and `{input}` in plain English rather than shell-implementation jargon.

### 4.5 Prompt Interaction Standards
When a text prompt is active, specialized conventions ensure a refined editing experience:
*   **Line Editing**: `^a` / `^e` (start/end), `^k` / `^u` (kill to end/start), `^w` (kill word).
*   **History**: `^p` or `Up` arrow recalls contextual history (e.g., previous filters).
*   **Browsing**: `F2` or `^f` opens the directory selection browser.
*   **Prompt Guidance Layout Rule**: Prompt-owned command rows use structured command-strip rendering with adaptive truncation; if a prompt row cannot fit every visible action, runtime truncates the final visible entry with an ellipsis instead of clipping a key token or label mid-word.

### 4.6 Primary Action Depth Contract
*   **Primary-Action Definition:** A primary action is any user-invoked command that opens a prompt, menu, picker, overlay, modal chooser, or immediately commits a user-visible operation or mode/result change from the active runtime surface.
*   **Common-Path Rule:** The documented fast path for a primary action MUST stay `key -> Enter -> result` unless the action genuinely needs additional user choices or safety confirmation.
*   **Decision Count Rule:** Count one common-path decision for each required chooser selection or text acceptance after the initial key. Optional exploration, cancellation, or advanced branches do not change the baseline count.
*   **Interactive-Layer Count Rule:** Count one interactive layer each time the common path enters a distinct prompt, menu, picker, chooser, overlay, or confirmation surface after the initial key. Sequential hand-offs (`prompt A -> prompt B -> prompt C`) count cumulatively even when only one surface is visible at a time.
*   **Depth Budget Rule:** Ordinary primary actions MUST NOT require more than one interactive layer on their common path. If a command needs richer input, runtime MUST compress those choices into one prompt/menu layer where that can be done safely.
*   **Prompt-Necessity Rule:** A common-path prompt is allowed only when it captures required input, a separate user decision that must remain explicit, or a real safety confirmation. If a prompt does none of those jobs, runtime MUST remove it or absorb it into an existing surface.
*   **Required-Input Rule:** A prompt is required input only when the operator must supply data that runtime cannot infer safely from the active command, current selection, documented defaults, or a same-layer toggle on the owning prompt.
*   **Separate-Decision Rule:** A prompt remains explicit only when it captures a meaningfully different decision than the previous step, such as a different target, destination, route, or scope. Re-asking for approval or policy after the operator already chose the underlying action is bureaucracy, not a separate decision.
*   **Safety-Confirmation Rule:** A confirmation is legitimate only when it protects a destructive, conflicting, externally effectful, or otherwise irreversible boundary with concrete source/target context and a default-safe choice. Routine success acknowledgments and abstract policy pre-prompts are not safety confirmations.
*   **Batch Follow-Up Rule:** After the operator approves a batch action, runtime MUST NOT ask generic follow-up questions such as whether to confirm each item or each overwrite. Later prompts are allowed only when they describe a concrete per-item conflict or exception that needs an explicit decision at that moment.
*   **Bureaucracy Rule:** A prompt is unnecessary bureaucracy when it asks for mode, policy, or approval after the user's intent is already clear and no new meaning or safety boundary has appeared. Bureaucratic prompts MUST be removed instead of documented as normal flow.
*   **Meaning-Preservation Rule:** Prompt compression MUST remove bureaucracy, not collapse distinct user decisions into one less-intuitive surface. If combining inputs would blur different meanings or make the flow harder to reason about, runtime MUST keep those decisions explicit instead of forcing a “smart” merged prompt.
*   **Prompt-Local Aid Rule:** Prompt-local aids such as `F1`, `F2`, history recall, browse pickers, and completion lists do not increase the owning primary action's depth budget when they return to the same pending prompt with the partially edited value preserved. They are same-layer aids, not extra required layers.
*   **Aid Discoverability Rule:** When a prompt/menu/picker/dialog supports prompt-local aids, the active surface MUST advertise the usable aids there or in its immediate `F1`/hint surface. Hidden prompt-only capability is a defect.
*   **Exception Rule:** Extra interactive layers are allowed only for destructive or safety-critical confirmation, or when the action truly requires multiple independent data types that cannot be expressed safely on one surface. Such exceptions MUST remain explicit, justified, and paired with the shallowest equivalent fast path the workflow can support.
*   **Equivalent Fast-Path Rule:** An equivalent fast path is a documented path that reaches the same operation outcome without increasing mandatory decision count or interactive depth for the common case. Optional advanced toggles, defaults, or in-prompt mode switches satisfy this rule; an extra submenu does not.
*   **Output Export Rule:** `Output` keeps file-versus-hardcopy explicit with one route chooser, then collects the final destination on one prompt. The file destination prompt keeps the current format explicit and may cycle `Raw`, `Framed`, and `Page break` in place; when `F3` selects framed or page-break output, the separator prompt appears before returning to the final file destination prompt. Hardcopy asks only for the printer command and does not expose file-format cycling.
*   **Prompt Label Rule:** Prompt and menu titles MUST describe the user decision in plain language, not an internal implementation step. Labels and mnemonics stay normalized to the live command word and render as full words with in-place mnemonic emphasis rather than key-prefix jargon.
*   **Prompt-Surface Correctness Rule:** Active prompt, menu, picker, dialog, and overlay surfaces MUST show the commands that are actually usable there, MUST NOT show commands that are unavailable there, and MUST keep `Enter`/`Esc`/return-path semantics explicit.
*   **Routine-Success Return Rule:** Routine successful operations MUST return directly to the working view without a blocking acknowledgment step. If a short summary is useful, present it as a transient non-modal status message rather than a `PRESS ENTER` confirmation dialog.
*   **Bulk-Confirmation Rule:** Bulk actions such as tagged delete MUST use one batch confirmation on the common path. Runtime MUST NOT insert a routine follow-up chooser like `confirm each item?`; later prompts are allowed only when they are true safety exceptions or concrete per-item conflicts.

---

## 5. Split-Screen (F8) & Session Model

### 5.1 The Active-Inactive Rule
Split mode is a two-panel session entered/exited by `F8`.
*   **Activation:** `F8` toggles split mode on/off.
*   **Focus Switch:** `Tab` switches the active panel.
*   **Active Panel:** Owns keyboard focus and receives command/navigation input.
*   **Inactive Panel:** Does not process direct input while inactive; its own cursor/selection context is retained until focus is switched back.
*   **Freeze/Resume Rule:** When a panel loses focus, its panel-local state is frozen; when it becomes active again (via `Tab`), it must resume exactly where it left off.
*   **Active-Only Mutation Rule:** Commands mutate only the active panel's panel-local state. Cross-panel updates are limited to shared topology mirroring defined in §5.3.
*   **Statistics Visibility:** Entering split mode initializes both statistics strips as hidden. `F6` toggles only the active panel's 24-column strip, `Tab` preserves both visibility states, and either or both panel-local strips may be visible. Single-panel statistics retain their session-wide behavior.

### 5.2 State Persistence
Switching panels via `Tab` must restore the exact state held when that panel last had focus for panel-local state:
*   **Volume Context:** Logged volume or archive.
*   **Cursor & Offset:** Highlighted entry and scroll position.
*   **Selection:** Tags are specific to the panel session. Collapsing a directory (`Left` or `-`) or explicitly unreading/releasing it (`--`) discards saved tags beneath that directory; reloading it must not resurrect stale tags.
*   **Compare Tagging Rule:** Compare workflows can report difference counts, but they do not implicitly mutate per-file tag state; tags change only through explicit tagging actions.
*   **Filter (Filespec):** Independent search/filter strings.
*   **Panel-Volume State Key Rule:** In split mode, panel-local restore state is keyed by `(panel, volume)`. On `Tab`/volume-cycle transitions, a panel restores its own snapshot for that volume and must not import state from the opposite panel.
*   **Window/Mode Context:** Directory/File/Showall-style panel-local focus context.
*   **File-Window Shape Context:** In split mode, each panel owns its own file-window shape (`tree`, `small file`, `big file`). `Tab` and `F8` transitions must restore that panel-local shape exactly on reactivation.
*   **No Transient Fallback Rule:** Reactivating a panel must not briefly render a different shape (for example tree-first then file, or small-first then big) before correcting. The recorded shape must be restored directly.

### 5.3 Shared Tree Topology Contract
The split panels share one logged tree topology contract for a given logged volume:
*   **Shared Logging State:** Logged/unlogged directory-memory state is shared.
*   **Shared Structural State:** Expand/collapse/release tree-shape changes are shared.
*   **Mirror Rule:** Structural tree changes triggered in the active panel must be reflected in the inactive panel immediately.
*   **Selection Retention Rule:** Mirrored structural updates must not move the inactive panel's cursor/selection when its selected node remains visible/valid.
*   **Non-Invalidating Changes Rule:** Adding siblings/ancestors, or changing sibling structure that does not invalidate the inactive selected node, must not move the inactive selection.
*   **Render Stability Rule:** Inactive-panel rendering must not mutate the panel's stored tree viewport origin as a side effect of redraw; its only allowed redraw-side computation is a temporary render position.
*   **Frozen File-View Anchor Rule:** A panel left in file view is restored from its saved directory path and selected filename, not from the current flattened tree index. If shared-tree rebuilding leaves that directory as a visible but unloaded placeholder, the directory payload must be reloaded before the panel is rendered or resumed so the file window cannot degrade to an empty or unrelated listing.
*   **Rebind-After-Rebuild Rule:** Restore paths must not persist raw `DirEntry*`/`FileEntry*` pointers across rebuild/rescan/volume-cycle boundaries. Persist stable identity keys (path/name) and re-resolve after rebuild.
*   **Render Is Not Authority Rule:** Rendering can display the saved panel state, but it must not decide a new panel selection from whatever entry currently occupies the saved numeric index after a shared tree rebuild.
*   **Restore Safety Guard Rule:** Before post-restore list/index dereference, validate volume/list presence and bounds. If invalid/empty, use deterministic fallback behavior rather than dereferencing transient state.
*   **Restore Ordering Rule:** Any rebuild, rebind, or visibility transition that can invalidate a saved anchor must complete first; restore must run after the current topology and visibility state are settled. Restore must not race ahead of an in-progress rebuild or visibility mutation.
*   **Deterministic Fallback Rule (When Selected Node Becomes Invalid):**
    *   Keep exact node if still visible/valid after mirror update.
    *   Else move to nearest visible ancestor of the previously selected node.
    *   Else move to next visible sibling in display order.
    *   Else move to previous visible sibling.
    *   Else move to the root visible node.
    *   Raw `disp_begin_pos + cursor_pos` math is not a restore authority and must not be used to reconstruct selection when a helper can resolve visible selection by identity.
*   **Generation Discipline Rule:** Restore snapshots must be accepted only against the current panel/volume generation. If the generation has changed because topology or visibility mutated, the snapshot must be re-resolved from stable identity keys before any state is applied.
*   **No Surprise Parent-Jump Rule:** Collapsing a parent/grandparent in the active panel must not force the inactive selection to jump to parent unless the previously selected node is no longer visible/valid.

### 5.4 Modal Search Behavior
*   **Persistence:** The search string is retained after the mode is exited.
*   **Sticky Cursor:** If a character is typed that produces no match, the cursor remains at the last successful match.
*   **Implicit Exit:** Pressing a key associated with a file operation (Copy, Delete, Move) confirms the current search match and immediately executes that command.

### 5.5 Canonical Panel Restore Contract
Task 31 must follow this section. If implementation needs behavior that is not covered here, this specification must be updated before the implementation is considered complete.

#### 5.5.1 Canonical Panel State Record
Each window or panel must own one canonical frozen UI state record. The record must contain:

| Field | Required meaning |
| :--- | :--- |
| `panel_key` | Stable panel/window identity for the current session (`window`, `left`, or `right`). |
| `volume_key` | Stable identity of the current logged volume or archive namespace. |
| `tree_selection_key` | Stable path-based identity for the selected directory in the tree. |
| `tree_cursor_pos` | Cursor position within the visible tree rows. |
| `tree_viewport_origin` | The saved top-row origin for the tree viewport. |
| `file_selection_key` | Stable path-based identity for the selected file or file-anchor directory. |
| `file_cursor_pos` | Cursor position within the visible file rows. |
| `file_viewport_origin` | The saved top-row origin for the file viewport/list. |
| `focus_shape` | The saved panel shape (`tree`, `small file`, or `big file`). |
| `filter_text` | The current panel-local filespec/filter string. |
| `dotfile_visibility` | The current panel-local hidden-dotfile visibility setting. |
| `panel_generation` | The panel-local restore generation. |
| `volume_generation` | The shared topology/visibility generation for the current volume or archive namespace. |

The canonical record must be treated as the only restore authority for that panel. Raw row indices, transient pointers, and redraw-derived guesses are not authority.

#### 5.5.2 Identity Keys and Normalization
Restore identities must be path-based and scoped to the current `volume_key`.

*   **Tree identity:** the normalized directory path inside the current volume/archive namespace.
*   **File identity:** the normalized directory path plus the selected file name inside the same namespace.
*   **Archive identity:** the archive container identity and internal path prefix together form the namespace for restore keys.
*   **Remap behavior:** rename, move, symlink target change, or mount remap can invalidate a saved identity by changing the resolved namespace. If a stored identity still resolves in the current namespace, it must be reused; otherwise the deterministic fallback order applies.
*   **Forbidden authority:** inode values, row numbers, and pointer identity must not be used as the restore key.

#### 5.5.3 Generation and Invalidation Rules
Restore snapshots are valid only while both the saved `panel_generation` and `volume_generation` still match the current authoritative values.

*   `panel_generation` must increment whenever panel-local frozen state changes in a way that can affect restore: cursor movement that changes selection, viewport origin changes, focus-shape changes, filter changes, dotfile visibility changes, or any other panel-local mutation that changes the saved record.
*   `volume_generation` must increment whenever shared topology or visibility changes in a way that can affect restore: log/relog, release/unlog, collapse/expand that changes the shared tree shape, rename, move, symlink change, mount remap, or any rebuild that changes the visible set or its path identities.
*   If either generation no longer matches the snapshot, the snapshot must be re-resolved from stable identity before any state is applied.
*   Restore ordering must be explicit: mutation/rebuild completes, generation advances, then restore rebinds or falls back. Restore must not race ahead of an in-progress rebuild or visibility mutation.

#### 5.5.4 Restore Entry Point and Transition Entry Point
The implementation must expose one canonical restore path and one canonical split-transition path.

*   All restore requests must route through the canonical panel-anchor restore helpers in `src/ui/panel_anchor.c` and `include/ytnova_panel_anchor.h`; other modules may call these helpers, but they must not synthesize their own restore authority.
*   All `F8`/`Tab` split transitions must use one deterministic transaction flow: snapshot -> compute -> validate invariants -> commit/rollback.
*   Rendering is projection only. Redraw paths can compute a temporary render position, but they must not pick a new authoritative selection or viewport origin.

#### 5.5.5 Deterministic Fallback Order
If a saved identity no longer resolves, the fallback order must be deterministic and must be applied exactly in this order:

1. exact identity if still valid
2. nearest visible ancestor
3. next visible sibling
4. previous visible sibling
5. root visible node

Sibling choice must follow deterministic display order. If the current selection is still visible and valid, no fallback is allowed.

#### 5.5.6 No-Flicker / No-Wrong-Shape Rule
Reactivation must restore the recorded `focus_shape` directly.

*   A panel must not briefly render a different shape before converging.
*   Tree/file viewport restoration must not create a transient wrong-shape flash or a viewport jump while the saved state is still valid.
*   `Enter`, `Tab`, `F8`, release/relog, hidden-dotfile reactivation, and file-mode restore paths must all obey the same canonical restore contract.

#### 5.5.7 Scope Boundary for Nearby Flows
The canonical restore contract applies whenever these flows touch panel-local state:

*   `Enter`
*   `Tab`
*   `F8`
*   release/relog
*   volume cycling
*   file-mode restore
*   hidden-dotfile reactivation
*   preview mode (`F7`) when it reuses panel-local state

These flows can differ in user-facing action, but they must not use different restore rules.

---

## 6. Notification & Messaging Tiers
`ytnova` distinguishes between three primary locations for communication:

### 6.1 Footer Messages (Command Area)
*   **Transient:** Non-critical status (e.g., "File copied"). Appears in the Message row. Disappears on the next keystroke.
*   **Sticky/Warning:** Requires acknowledgment or input (e.g., "Delete file? Y/N" or "Path not found"). Stays in the footer until the user responds or hits a key to clear the warning.
*   **Outcome Clarity Rule:** Successful commands may remain quiet, but ytnova MUST NOT appear successful while doing nothing. No-op/skip/error outcomes must be explicit and user-visible.
*   **Portable Footer Rule:** The default footer MUST remain portable across the supported terminal target set and MUST NOT depend on transient modifier-state telemetry. A Ctrl-key variant with a reliable terminal byte MAY appear beside its normal-key variant; unavailable or colliding terminal chords MUST NOT be advertised as distinct controls.
*   **Footer Key-Variant Notation Rule:** The footer advertises normal and tagged Ctrl-key variants compactly. When both variants share one command word, render the normal key, slash, Ctrl key, and remainder of the word as one entry (for example, `C/^Kopy`). When the command uses a key prefix rather than an embedded mnemonic, render both keys before the command word (for example, `Z/^Z archive`). When the Ctrl chord invokes a distinct command, render it as a separate entry after the normal command (for example, `Sort  ^Search`). Highlight every displayed normal key token and every complete Ctrl token, including its `^` prefix.
*   **Layout Split Rule:** The default runtime footer is three rows: two auto-fit command rows plus one bottom function-key band. Directory, file, archive, and other standard footer contexts share this layout and vary only by their context-specific signposts and visible command entries. Each row has an independent left signpost region and a shared command column. Signpost wording/glyph changes must not shift the command column.
*   **Auto-Fit Rule:** Visible footer entries are ordered by stable key class: numeric first, alphabetic second, symbolic last. Within each class, entries are ordered by their rendered keybinding token rather than by command label text. Single-letter mnemonic keys sort lexically by key token; function-key tokens sort by natural numeric suffix (`F9` before `F10`); named bottom-band keys such as `Esc` sort after the function-key run. Runtime then packs the resulting full-label entries left-to-right.
*   **Wrap Rule:** The shared standard footer wraps by whole entry, not by character, across its available command rows. An entry is the complete rendered key/command label, including any internal spaces. Runtime preserves the established order, evaluates the legal wrap points for the two top command rows, and chooses the split that keeps the two used row widths as close to each other as possible instead of greedily saturating row 1 first. Label-specific preferences may break ties between otherwise equivalent balanced splits, but they MUST NOT become hard-coded primary split points. Runtime continues packing onto the next allowed row before truncating anything. When the available rows are exhausted, runtime truncates only the final visible entry and marks the truncation with an ellipsis. Runtime MUST NOT truncate earlier visible entries, reorder entries, or rewrite the footer as a handcrafted narrow variant.
*   **Bottom-Band Rule:** The bottom function-key band uses the same key-class ordering and full-label packing model but is limited to a single row. When `Esc` is visible in that band it sorts after the function-key entries, so wide layouts normally place it at the far right. When the current width cannot show every visible entry, runtime truncates only the final visible entry and marks the truncation with an ellipsis. Runtime MUST NOT truncate earlier visible entries, reorder entries, or rewrite the band as a handcrafted narrow variant.

### 6.2 Modal Messages (Centered Box)
A bordered pop-up box that overlays the center of the screen, used for:
*   **Info:** Detailed system information or multi-line status.
*   **Warning:** Significant operational warnings that require explicit dismissal.
*   **Error:** Critical failures (e.g., "Permission Denied" or "Archive Corrupt").
*   **Constraint:** Modals must be dismissed with `Esc` or `Enter` before any other navigation can occur.

### 6.3 Audible Feedback Policy
`ytnova` interaction is completely silent. Navigation boundaries, unsupported keys, and input validation must remain silent. If an event is expected during ordinary workflow, it must not trigger an audible cue.

### 6.4 Context Help Contract (Footer <-> F1)
*   **Parity Rule:** For any active context, commands shown in the footer keybinding hints MUST appear in that context's F1 help set. Missing footer commands in F1 are defects.
*   **Concision Rule:** F1 content is concise, contextual, and beginner-friendly. The help popup teaches the active surface in plain English; terse reference-style detail belongs in the manpage source and generated `docs/USAGE.md`.
*   **Unix Documentation Layering Rule:** ytnova follows the Unix split between in-app help and full reference. `F1` is the short contextual path for the active task, `--help`/usage stays terse, and the manpage/reference docs own exhaustive behavior and configuration detail. Authored contextual help stays in `etc/help/f1.en.md`, authored reference prose stays in `etc/help/man.en.md`, and runtime `F1` MUST render only small contextual slices rather than presenting one giant browsing document.
*   **Answerability Rule:** Pressing `F1` on any covered surface MUST leave the user with an answer path. The answer may be present on the opening contextual page or on one clearly signposted help-popup link hop to the owning explainer/shared topic, but the user must not be stranded by omitted semantics or hidden structure.
*   **Surface Naming Rule:** Keep the four help surfaces distinct in docs, tests, and code comments: the always-visible bottom-of-screen strip is the **footer command strip**; the modal opened by `F1` is the **help popup**; the minimal action row embedded in that popup is the **help-popup hint line**; and any command/topic entry that exists specifically to branch into deeper explanation is a **help popup link**.
*   **Footer Ordering Rule:** Footer command strips use a stable low-noise order. `F1 help` is always first. `Esc cancel` is always last when cancel is available. Between them, show only truthful surface-local actions in a consistent task-flow order, and spell the live verb the user will get (`select`, `switch`, `edit`, `release`, and so on) rather than placeholder wording. Generic list navigation keys that already behave the same across ordinary chooser/list surfaces (`Up`, `Down`, `Home`, `End`, `PgUp`, `PgDn`) must be omitted from chooser-style footer strips unless that navigation is itself the distinctive local behavior or the surface needs the hint to explain a nonstandard rule.
*   **Chooser Chrome Rule:** Standalone modal choosers get box lines. Embedded transient mini-pickers do not.
*   **Mini-Chooser Alignment Rule:** Embedded chooser/list command strips that live inside a local popup or mini-menu box (such as history or the `F2` picker) must be left-aligned to the local surface's inner padding instead of centered. Use the same local inset consistently so the surface reads like one coherent control surface.
*   **Help-Family Rule:** Keep three authored help families distinct. **Contextual pages** belong to one active runtime surface (mode, prompt, dialog, picker, preview, split, or future equivalent). **Command explainers** belong to one command or concept family such as Copy, Filter, Compare, Output, Jump, or wildcard rename semantics. **Shared topics** belong to cross-cutting behavior such as help navigation, tagged workflow, command-line editing, VI keys, theming/customization, search semantics, or command catalogs.
*   **Command-Ownership Rule:** Apply the same ownership test to every command family. The local contextual page owns only the surface-specific consequence of the command here; any reused semantics belong to the owning shared topic; and a command-family explainer must exist only when there is a genuinely shared conceptual model across multiple surfaces. Matching command labels alone are not enough reason to force one shared explainer page. The common path must be one short local summary plus one owning explanation, not a stack of partial explanations spread across local detail pages and shared topics.
*   **Shared-Explainer Coherence Rule:** When a shared topic or command-family explainer owns a reusable rule, it must present that rule as one coherent explanation in the most natural and consistent place. Do not split the core explanation awkwardly across multiple local pages or sibling explainers so that users must stitch the meaning together themselves. Local pages may summarize the local consequence and link outward, but the owning shared explainer must still read cleanly as the complete reusable explanation. Avoid a three-step chain of “surface sentence -> intermediate detail page -> real shared explanation” unless the intermediate page owns a clearly different concept rather than the missing half of the same rule.
*   **Separate Source Rule:** Authored contextual-help prose belongs to `etc/help/f1.en.md`. Authored terse reference prose for the manpage and `docs/USAGE.md` belongs to `etc/help/man.en.md`. One prose file MUST NOT be forced to do both jobs.
*   **Minimal Cross-Reference Rule:** The two help families may acknowledge each other, but only briefly. `F1` help does not retell the manual, and the man/USAGE reference does not explain the popup workflow beyond noting that `F1` exists.
*   **Translator Editability Rule:** Both help sources MUST remain directly human-editable and translator-editable. Optional shared fragments may exist only when they happen to be naturally reusable; neither source may be distorted to suit the other or to satisfy an enforced DRY scheme.
*   **Authored-Source Intelligibility Rule:** `etc/help/f1.en.md` and `etc/help/man.en.md` are authored sources, not opaque intermediate artifacts. They must stay readable and understandable to maintainers in the same practical sense as good source code: stable structure, obvious ownership, locally comprehensible topic blocks, and no clever authoring scheme that makes ordinary manual editing feel like reverse-engineering generated data.
*   **Topic Block Rule:** Each help topic starts with an exact `## topic:<id>` heading, followed immediately by a `ytnova-help-meta` fence that declares `title:` and `contexts:`. `contexts:` is a comma-separated list of stable runtime context/prompt IDs or the literal `none` for link-only explainer pages.
*   **F1 Section Rule:** Every F1 topic block contains required `### Contextual F1` and `### Long form` sections, and may additionally contain `### Explainer links` when cross-topic links are useful. The contextual section owns the local plain-English summary, the long-form section owns the popup rows, optional explainer links own followable topic links, and the `Contents` return target MUST remain reachable from every help page via the help-popup hint line.
*   **Reference Section Rule:** Every man/USAGE topic block contains ordered `####` reference subsections written in terse reference tone. The reference source is markdown-editable for translators and maintainers and does not inherit F1 teaching prose by generation trickery.
*   **Generator Rule:** `scripts/generate_help_assets.py` is the sole correctness path for generating `etc/ytnova.1.md`, `docs/USAGE.md`, the build manpage output, and the generated runtime F1 help asset from the two authored sources.
*   **Explainer Link Rule:** Shared explainers are declared only with Markdown links of the form `[Label](topic:<id>)`. F1 reuse must link to shared topics rather than duplicate the same prose into multiple contextual pages, and link depth must remain shallow (one or two hops maximum from the starting page).
*   **Shallow-Path Rule:** Help links must keep the user oriented. Contextual pages should usually link upward to `Contents`, sideways to one owning command/concept explainer, and only sparingly to one shared topic. Command explainers must absorb their own common caveats instead of forcing a branching maze of follow-on links unless a subtopic is truly reused across multiple command families.
*   **First-Pass Topic Set Rule:** The topic inventory must include at least `intro`, `navigation`, `shared-commands`, `tagged`, `command-line-editing`, `vi-keys`, `f10`, `theming`, `dir`, `file`, `archive-dir`, `archive-file`, `showall`, `global`, `f7`, `f8`, `f8-dir`, `f8-file`, `filter`, `compare`, `output`, `history-dialog`, `volume-menu`, `applications-menu`, and `f2-picker`.
*   **Coverage-Matrix Rule:** Topic planning and audits MUST be driven by a maintained user-question matrix, not by author memory alone. Recurrent question families such as filters, jump/list-jump, wildcard/rename-pattern behavior, search/fuzzy semantics, command-line editing, tagged-flow variants, split ownership, preview caveats, theming/customization, and prompt syntax are explicit coverage items rather than optional prose extras.
*   **Visual Rule:** Command-strip words stay readable: the live UI renders the full word and highlights the bound letter in place. Literal key tokens such as `Esc`, `Enter`, `Up`, `Down`, and function keys render as key tokens, not as synthetic words. Live UI never shows bracket notation for mnemonics.
*   **Help Mnemonic Role Rule:** When a theme styles help-popup mnemonics separately from the global/footer mnemonic role, it MUST use `help_keybind`. When `help_keybind` is omitted, runtime falls back to `keybind` while keeping the `help_footer` background unless an explicit `on ...` background is given.
*   **Localized Label Rule:** In localized help and footer text, the visible command term is the localized command word, not a duplicated standalone key prefix. Runtime highlights the active bound letter in-place inside that localized term where possible; plain-text docs, tests, and code comments may use `(K)eyword` notation when a highlight must be spelled out without color.
*   **Runtime Resolution Rule:** Runtime help loads generated F1 topic content by stable context/prompt ID from the generated runtime asset. When one shared explainer fans out to multiple runtime pages, those runtime pages are separate topic blocks that link back to the shared explainer instead of re-embedding prose in C code. Split help is explicitly allowed to fan out into distinct directory- and file-focused runtime pages while keeping one shared split overview topic.
*   **Active-Surface Precedence Rule:** `F1` resolves against the innermost active runtime surface, not against a generic base mode. Prompt/dialog/picker surfaces that explicitly advertise prompt-local help own `F1` first; otherwise preview-specific help wins while `F7` preview is active; otherwise split/layout-specific help wins when that surface is the active differentiator; otherwise runtime falls back to the underlying directory/file/archive/Showall/Global context page or the owning shared explainer for that workflow. Once the help popup is open, its own keys consume input before the suspended underlying surface resumes dispatch.
*   **Geometry Rule:** The help popup uses the same fixed frame footprint as the history/up-arrow surfaces for the current layout topology: anchored at the main-content origin, height-stable, and sized from the active layout geometry rather than from per-topic text length. A split-layout help popup can therefore widen or narrow only when the underlying main-content footprint changes.
*   **Help-Popup Hint Line Rule:** The help-popup hint line is a low-noise action row for cross-topic follow hints plus back/close cues; it MUST NOT duplicate one-line local footer commands there. The default contextual hint line may start from `Contents  Navigation  Esc/Quit`, but runtime/spec content may expand it when needed to keep the help IA discoverable (for example Back or list/detail switching). If a cross-topic move is part of the intended help workflow, the hint line MUST advertise enough of that workflow that the user is not expected to guess it.
*   **Local Navigation Scope Rule:** Shared popup navigation belongs on the linked **Navigation** page. Generic movement keys such as `Enter`, `Left`, `Right`, `Plus`, `Asterisk`, paging keys, and similar list/tree traversal belong there rather than on ordinary mode pages. Contextual pages may still document truly mode-specific non-footer rules such as split ownership, aggregate-view scope, owner-jump behavior, or preview-only restrictions.
*   **Contextual List Rule:** Mode pages such as Directory, File, Archive-Dir, Archive-File, Showall, Global, `F7`, split dir/file, and future runtime-surface pages are concise command lists built from that surface's live footer commands, plus only the small number of additional surface-specific rules that cannot live elsewhere. Each row uses one short plain-English definition; do not pad contextual lists with filler prose such as “See Copy overview”. The opening page must answer “where am I, what can I do here, what caveat matters here, and where is the deeper shared rule?” in that order.
*   **Variant Behavior Rule:** Commands with the same visible name may still be distinct per surface. Directory Copy, File Copy, archive-side Copy, tagged variants, and similar cross-surface families MUST be documented as the behavior the user gets in that current surface, while their deeper explainer page clarifies the shared model and the important differences.
*   **Prompt/Dialog Rule:** Prompt, dialog, menu, and picker help owns only the active interaction: what this input expects now, what defaults/history/browse aids apply now, and what special keys matter now. Broader semantics belong to the owning command explainer or shared topic. Prompt-local `F1` must be selective rather than universal: use it for syntax-heavy, semantics-heavy, or otherwise non-obvious prompts, and do not force dedicated prompt help onto trivial confirmations or self-evident inputs. When a prompt has its own `F1`, the prompt footer must advertise it explicitly. When it does not, runtime falls back to the owning surface/shared explanation, but must not rely on hidden prompt-only help that the footer never hints exists. This includes prompt-local help for syntax-bearing commands plus dialog/menu help such as history, volume, and applications surfaces.
*   **Tagged Workflow Rule:** Tagged behavior is a first-class shared explainer because it is central to the ytnova workflow model. Commands that change behavior when tags exist MUST summarize that tagged variant inline on the local command page or base command row for that surface; the shared tagged explainer exists for the broader workflow model, not as an excuse to omit the inline command behavior or to force the user to leave the command page just to learn what the tagged variant does here.
*   **Help-Popup Link Rule:** Only commands that genuinely need more than one or two lines of explanation must become help popup links. Simple rows stay plain text and do not open redundant detail pages, while complex rows such as Filter, compare flows, `1..9` view meanings, aggregate-view scope, and similar option-heavy commands may open brief plain-English explainers.
*   **Popup Page-Key Rule:** `Home`, `End`, `PgUp`, and `PgDn` MUST keep their normal popup-scrolling behavior on contextual command pages. Contextual-row selection may coexist with those keys, but it MUST NOT steal or nullify page scrolling.
*   **Popup Navigation Rule:** If the chosen key already exists inside the linked word, runtime renders the full word and highlights that in-place mnemonic, for example `eXecute`; otherwise it renders `key space lowercase action`, for example `J compare` or `Esc close`. Close hints may advertise `Esc/Quit` when both keys close the popup.
*   **Mnemonic Emphasis Rule:** Mnemonic highlighting may be conveyed by color, underline, reverse, standout, or any combination the terminal supports. If no visual emphasis is available, runtime must still preserve the mnemonic shape by case alone.
*   **Text-Notation Rule:** In plain-text docs, tests, and code comments, `(K)eyword` notation is the durable way to describe that in-place highlight when color cannot be shown directly; runtime does not display the parentheses form to users.
*   **Direct-Voice Rule:** Rendered help talks to the user directly and does not waste space narrating its own mechanics. Avoid filler such as “This page explains...” or “Help for X...”; use that intent only in maintainer comments outside rendered help content.
*   **Formatting Rule:** Popup prose must wrap cleanly without blank spacer lines that split one sentence across multiple visual paragraphs. When popup width forces one authored help row to wrap, the continuation lines render as one visually continuous block with no synthetic blank line between them. Blank lines are reserved for real authored separators between bullets, items, or sections only, never mid-sentence or between wrapped fragments of the same item.
*   **Coverage Rule (Required):** Contract coverage includes filesystem and archive contexts (directory/file), `F7`, `F8`, `Showall`, `Global`, tagged workflows, and active picker/prompt/dialog surfaces such as history, volumes, applications, compare prompts, and syntax-bearing command prompts.
*   **Variant Rule:** Help rendering must stay correct for `VI_KEYS=1` variants and for prompt flows that document C-only tagged/search actions without requiring a modifier-state footer variant.
*   **i18n Readiness Rule:** Footer keybinding text, F1 text, and man/USAGE text must remain structured for gettext/po4a extraction while preserving direct human editability in both authored sources.
*   **Progress Coexistence Rule:** Long-operation progress rendering must coexist with footer keybinding/prompt/F1 guidance and must not seize ownership of those surfaces.

### 6.5 Modal/Dialog Color Taxonomy Contract
`ytnova` modal and dialog surfaces are split into two classes:
*   **Severity class (`info`, `warning`, `error`):** Outcome/diagnostic overlays that communicate informational notices, warnings, or errors and require acknowledgment.
*   **Neutral interaction class:** Selection/picker/help/history/volume/prompt-like interaction surfaces used to collect or browse input.

Routing contract:
*   Severity class MUST route through semantic severity roles only: `info`, `warning`, and `error`.
*   Severity modal headers, body text, frames, and prompts MUST retain the active severity role pair. They MUST NOT use raw reverse/blink styling that swaps foreground/background away from the configured severity colors.
*   Neutral interaction class MUST NOT use severity pairs. Neutral prompts/dialogs use `dialog`; the always-visible footer guidance strip uses `footer`; F1/context help surfaces use `help` for the reading body, `help_footer` for the popup command strip, `help_heading` for popup titles, `help_topic` for popup term/definition labels, `help_attention` for bounded authored callouts, `help_alert` for any future stronger help-side urgency tier, `help_keybind` for popup mnemonics, and `help_box_lines` for the popup frame when a theme wants explicit overrides; F2, history, completion, and volume selection surfaces use the `picker` role, with `picker_selection` for the active highlighted row/bar.
*   Tree status-marker columns use `margin`; tree guide glyphs use `tree_lines`; tree directory names and attributes use `dynamic_text`. File-type palette rules do not style directory tree rows.
*   Preview/search-hit highlighting uses `search_hit` only for the matched span, then resets to the surrounding content role.
*   Rationale: severity coloring encodes risk/outcome state, while neutral interaction coloring preserves low-stress, task-oriented input flow.

Current modal/dialog audit:

| Surface | Class | Routing |
| :--- | :--- | :--- |
| `src/ui/error.c` `UI_Message`, `UI_Notice`, `AboutBox` | Severity `info` | `MapModalWindow(... MODAL_SEVERITY_INFO)` -> `info` |
| `src/ui/error.c` `UI_Warning` | Severity `warning` | `MapModalWindow(... MODAL_SEVERITY_WARNING)` -> `warning` |
| `src/ui/error.c` `UI_Error` | Severity `error` | `MapModalWindow(... MODAL_SEVERITY_ERROR)` -> `error` |
| `src/ui/help_popup.c` `UI_ShowHelpPopup` | Neutral interaction (help popup) | `help` |
| `src/ui/volume_menu.c` `SelectLoadedVolume` window | Neutral interaction (volume picker) | `picker` |
| `src/ui/application_menu.c` `UI_OpenApplicationsMenu` window | Neutral interaction (applications picker) | `picker` |
| `src/ui/input_line.c` `UI_ReadStringInternal` prompt window | Neutral interaction (prompt/input) | `dialog` |
| `src/ui/history_dialog.c` `SelectHistoryEntry` | Neutral interaction (history browser) | `picker` |
| `src/ui/completion_dialog.c` completion list window | Neutral interaction (selection list) | `picker` |

---

## 7. Theme and Color Contract
Themes are plain-text user-editable files separate from the main configuration. The main config selects the active theme; theme files define semantic UI roles and optional file-type palette rules.

### 7.1 Theme and Config-Family Files
*   Packaged default sources are `etc/ytnova.conf`, `etc/ytnova.themes`, and `etc/ytnova.commands`; locale/layout command presets may additionally be shipped as separate packaged sources such as `etc/commands/<preset>.conf`. runtime binaries must not consult `etc/` directly.
*   Preferred config-family paths are `$XDG_CONFIG_HOME/ytnova/ytnova.conf`, `$XDG_CONFIG_HOME/ytnova/themes.conf`, `$XDG_CONFIG_HOME/ytnova/commands.conf`, and `$XDG_CONFIG_HOME/ytnova/applications.conf`; when `XDG_CONFIG_HOME` is unset, they fall back to `~/.config/ytnova/ytnova.conf`, `~/.config/ytnova/themes.conf`, `~/.config/ytnova/commands.conf`, and `~/.config/ytnova/applications.conf`.
*   Home-directory fallback user paths are `~/.ytnova`, `~/.ytnova.themes`, `~/.ytnova.commands`, and `~/.ytnova.applications` when the XDG target paths cannot be used.
*   If the user theme catalog is missing, runtime loads packaged or compiled-in default theme data without creating `~/.config/ytnova/themes.conf`.
*   If startup cannot load the selected theme from a user theme catalog, runtime falls back to packaged or compiled theme data before aborting startup; interactive reload still keeps the previous working theme and reports the load error.
*   If the user command catalog is missing, runtime loads packaged or compiled-in default command data without creating `~/.config/ytnova/commands.conf`.
*   `--init` creates missing user profile, commands, theme, and applications starter files at the active XDG or home-dotfile config targets without overwriting existing files.
*   Installed locale/layout preset catalogs live as read-only shared data (for example `/usr/share/ytnova/commands/<preset>.conf`); they are packaged defaults, not extra user-owned config files.
*   `etc/ytnova.commands` is the packaged default active command map. Upstream may ship it as English by default; a package may explicitly replace that default for a localized build, but runtime does not guess the active preset from locale at startup.
*   `commands.conf` is the canonical user-editable source for active command selection/overrides, line-1/line-2 command bindings, shown key tokens, plain labels, stable action IDs, and optional custom shell-command bindings. `ytnova.conf` must not remain the canonical home of `[MENU]`, `[DIRMAP]`, `[FILEMAP]`, `[DIRCMD]`, or `[FILECMD]`.
*   Command history is session state, not config: its preferred path is `$XDG_STATE_HOME/ytnova/ytnova.hst`, falling back to `~/.local/state/ytnova/ytnova.hst` when `XDG_STATE_HOME` is unset; legacy `~/.ytnova-hst` remains a compatibility path only when the state target cannot be used or when migrating old history forward.
*   Built-in theme names include `quiet-blue` and `bash-black`.
*   User-facing theme files use semantic role names only.
*   `etc/ytnova.themes` MUST expose the full starter-theme semantic-role surface that ships in the built-in catalog.
*   Each packaged starter-theme block MUST include every semantic role either as an active assignment or as a full-line commented fallback documentation entry.
*   Every commented fallback documentation entry MUST name the fallback source role or fallback behavior it documents.
*   `THEME=` selects one named theme block, role aliases stay within that theme, and omitted backgrounds inherit that theme's background unless explicitly pinned.

### 7.2 Semantic Roles
The starter-theme role surface is `background`, `box_lines`, `tree_lines`, `margin`, `static_text`, `dynamic_text`, `keybind`, `footer`, `selection`, `dialog`, `picker`, `picker_selection`, `help`, `help_footer`, `help_heading`, `help_topic`, `help_attention`, `help_alert`, `help_keybind`, `help_link`, `help_link_selection`, `help_box_lines`, `info`, `warning`, `error`, and `search_hit`.

Role meanings:
*   `background`: default application background.
*   `box_lines`: panel borders, separators, dialog boxes, and window frames.
*   `tree_lines`: tree guide glyphs.
*   `margin`: tree/file margins and status marker columns; inherits `dynamic_text` unless explicitly set.
*   `static_text`: fixed labels and captions.
*   `dynamic_text`: filenames, paths, counts, sizes, timestamps, current mode values, tree names, and file names.
*   `keybind`: preferred semantic role for mnemonic/key emphasis in footer, help, menu, and similar command-label surfaces.
*   `footer`: the always-visible footer guidance strip and other inline command-hint rows.
*   `selection`: active highlighted row/bar.
*   `dialog`: neutral prompt/dialog surfaces.
*   `picker`: selectable-list surfaces. The shipped starter themes keep picker-family surfaces on a different background so F2, history, volume, and applications menus stand out from the main content area.
*   `picker_selection`: picker-family highlighted row/bar override. When omitted, picker-family selection falls back to `selection`.
*   `help`: F1/context help reading surfaces.
*   `help_footer`: F1/context help popup command strip. When omitted, runtime inherits the `help` foreground and background.
*   `help_heading`: F1/context help popup titles. When omitted, runtime inherits the `help` foreground and background.
*   `help_topic`: F1/context help popup term/definition labels. When omitted, runtime inherits `help_heading`.
*   `help_attention`: bounded authored F1/context help callouts such as preserved `**...**` emphasis inside popup detail prose. When omitted, runtime inherits `help_topic`.
*   `help_alert`: reserved stronger F1/context help urgency tier. When omitted, runtime inherits `help_attention`.
*   `help_keybind`: F1/context help popup mnemonic highlight role. If omitted, runtime falls back to `keybind` on the `help_footer` background.
*   `help_link`: linked text inside F1/context help when hyperlink-capable help is enabled.
*   `help_link_selection`: the active linked target inside F1/context help when hyperlink-capable help is enabled.
*   `help_box_lines`: F1/context help popup frame lines. When omitted, runtime inherits the `help` foreground and background.
*   `info`, `warning`, `error`: severity road-sign roles.
*   `search_hit`: search/current-hit standout highlight.

### 7.3 Color Syntax
Theme styles accept named colors, numeric colors, `grey`/`gray`, and bright-prefix colors such as `+red`, `+yellow`, `+white`, and `+grey`/`+gray`. `grey`/`gray` names the dark-grey shade. `+grey`/`+gray` is accepted syntax but currently resolves to `white`, so examples must prefer `white` when they mean the rendered color. Preferred examples are `+white on blue`, `white on blue`, `cyan on blue`, `black on white`, `black on yellow`, and `+white on red`. User-facing docs and examples use `grey`/`gray` terminology for grey shades.

Every rendered style resolves internally to a complete foreground/background pair. If a role or file-type style omits a background, it inherits the active theme background appropriate for that surface. Shipped starter themes must use omitted backgrounds for ordinary content roles when they are meant to track the theme background, so changing `background` produces an intuitive full-surface repaint.

### 7.4 File-Type Palette Rules
File-type coloring is an optional content-decoration layer owned by the active theme. If a theme has no file-type rules, ordinary filenames use `dynamic_text`.

Palette rules use compact grouped lines:

```text
archives = red: tar,tgz,zip
scripts = +cyan: sh,bash,zsh,py,pl,rb
links = +cyan: LINK
executables = green: EXEC
```

Rules are first-match-wins. Selectors are extension names without `*.` by default; `LINK` and `EXEC` are special selectors. Directories in the tree use theme roles and are not styled by file-type palette rules. When a rule omits a background, it inherits the active filename/window background.

### 7.5 `commands.conf` Contract
`commands.conf` is a starter-commented plain-text file with zero or one optional preset selector line (for example `preset = en`) plus canonical per-context sections. Inside each section, rows use the canonical columns `binding | shown | label | action | command`.

Required contract:
*   Section headers name the stable runtime command surface that owns the following rows; they are not language names. Current canonical surface IDs include at least `[DIR]`, `[FILE]`, `[ARCHIVE_DIR]`, and `[ARCHIVE_FILE]`. Future command surfaces may add new stable section IDs without changing the row grammar.
*   `preset` names one packaged command-layout preset by stable untranslated ID. If present, runtime loads that preset first and then applies local section-row overrides from `commands.conf`. If absent, runtime uses the packaged default active command map from `etc/ytnova.commands`. A preset may represent a language-oriented mnemonic set, a keyboard-layout accommodation, or a mixed locale/layout variant, but it does not by itself switch all application text.
*   `binding` names the exact key inputs. Uppercase and lowercase letters may be bound separately. `Ctrl+letter` bindings are case-insensitive: `Ctrl+n` and `Ctrl+N` mean the same chord, so only one command may use a given `Ctrl+letter` chord. Alias bindings may be comma-separated only when they share the same section, shown token, label, action ID, and command payload.
*   `shown` names the token text rendered in footer keybinding/F1 surfaces. It is separate from the real binding so localized labels and display mnemonics do not need to mirror the raw input key exactly.
*   `label` stores plain user-visible text only. Users must not encode binding markup into the label column.
*   `action` stores the stable internal action ID (for example `copy`, `move`, `delete`, `compare`, `user-command`). Starter comments must state that users must not translate or rename action IDs.
*   `command` is blank for built-in actions. Custom shell-command bindings set `action` to `user-command` and store the shell command in `command`.
*   Packaged preset files use the same row model as `commands.conf`, but they are read-only shared data rather than a second user-editable config family.
*   Footer/help rendering must preserve separate theme roles for key tokens and labels.
*   If a shown token appears in the label, runtime must render the mnemonic inline by capitalizing/highlighting only the first matching occurrence, for example `Copy` -> `Copy` with the `C` highlighted, `execute` -> `eXecute`, or `movedir` -> `moVedir`. Runtime must not capitalize the leading letter just for title-case styling, so `commands` with shown token `M` renders `coMmands`, not `Commands`.
*   If a shown token does not appear in the label, runtime must render the highlighted token separately with a single space before the lowercase label, for example `J compare`.
*   If multiple shown tokens map to one visible entry, runtime must render highlighted tokens slash-separated with an unhighlighted slash, for example `M/^N move`.
*   The mnemonic span must use the `keybind` role when that produces a distinct emphasis that remains readable on the surrounding surface.
*   If `keybind` is unavailable or unsuitable for the current surrounding surface, the mnemonic span falls back to the surrounding role (`footer`, `help_link`, `help_link_selection`, or equivalent) rather than inventing a hard-coded color.
*   If the fallback role does not visually distinguish the mnemonic, runtime must still preserve the mnemonic shape by case alone.
*   Whole rendered footer/menu lines are not stored in `commands.conf`; they are assembled at runtime from `binding`, `shown`, `label`, `action`, and availability state.

Starter comments must include concise live examples such as:

```text
preset = en

[DIR]
binding | shown | label | action | command
C | C | Copy | copy |

[FILE]
binding | shown | label | action | command
X | X | eXecute | execute |

[ARCHIVE_DIR]
binding | shown | label | action | command
J | J | Compare | compare |

# Custom shell-command example:
# [FILE]
# g | G | gcc | user-command | gcc -O -c
```

### 7.5.1 Packaged Command Preset File Contract
Each packaged preset file uses the same section IDs and row grammar as `commands.conf`, but it is installed as read-only shared data and selected indirectly from `commands.conf`.

Required contract:
*   Every preset file begins with concise comment headers stating:
    *   the stable preset ID;
    *   the intended locale/layout or mnemonic audience;
    *   that the file is packaged read-only preset data selected from `commands.conf`; and
    *   that action IDs must remain untranslated.
*   Preset files may provide labels, shown tokens, bindings, and custom command payloads using the same `binding | shown | label | action | command` grammar as `commands.conf`.
*   Preset files must not require users to comment/uncomment language blocks inside the active user file.
*   Preset files do not own whole rendered footer keybinding/F1/menu lines; runtime still assembles visible command entries from the resolved active action table.

Example packaged preset header:

```text
# Preset: de
# Locale/layout: German mnemonic preset for PC QWERTZ-class keyboards.
# This is packaged read-only command-preset data selected from commands.conf.
# Action IDs are internal identifiers and must not be translated.
```

### 7.6 F10 Config Surface and Reload
`F10` opens the configuration command surface with entries in this order: `config`, `commands`, `themes`, `reload`, and `quit`. Reload is available only inside this surface. `F10` edits the active user file for that surface (XDG or home-dotfile fallback); if runtime is using built-in defaults for that surface, `F10` creates the XDG file for that surface and edits it. The `commands` path owns preset selection plus local command overrides, while packaged preset catalogs remain read-only shared data. Successful reload silently repaints. Failed reload keeps the previous working config/theme/commands state and reports the parse/load error in the footer/status area only.

---

## 8. The Virtual Filesystem (VFS)
*   **Archive Integration:** Archives are treated as directories. Entering an archive logs it as a Virtual Volume. `Left Arrow` at the root of an archive "Backs Out" to the parent physical volume.
*   **Stream Rewrite:** Modifications to archives use an atomic rewrite strategy to ensure data integrity.
*   **Live View:** Use `inotify` (where available) for automatic refreshes. If kernel limits are hit, the system falls back to manual refresh logic safely.

---

## 9. Filtering & Command Execution
*   **Filter Stack:** Cumulative logic applies: `Filespec AND Attribute Mask AND Date/Size AND Regex`.
*   **Grep Tagged (`^s`):** A non-destructive content filter applied to the currently tagged set.
*   **Targeting:** In Split-Screen, Copy/Move operations in the Active Panel use the Inactive Panel's current path as the default destination.
*   **Copy Contract:** `Copy` uses source-type semantics: file/tagged-file sources copy non-recursively; directory/tagged-directory sources copy recursively.
*   **Preserve Ancestor Paths Option:** `Copy` may preserve ancestor-relative path from source into destination when enabled. Base root for preserved segments is the operation base root (logged/selected source root), never filesystem `/`.
*   **Source Scope Rule:** Unlogged directories are excluded from copy source scope by default unless explicitly selected/logged.
*   **Copy/Move/PathCopy Multi-Input Exception:** Copy, move, and pathcopy remain an explicit two-prompt flow. The first prompt captures the replacement basename or wildcard rename pattern; the second `To Directory` prompt captures the destination directory and owns browse/history behavior.
    *   This exception exists because replacement name/pattern and destination directory are separate user decisions; collapsing them into one surface would hide meaning instead of removing bureaucracy.
*   **Copy/Move/PathCopy Confirmation Rule:** After the name prompt and destination prompt, only real safety confirmations may remain, such as overwrite/replace conflicts or creating a missing destination directory. Existing-destination overwrite/replace prompts SHOULD show factual source-versus-destination comparison details when available, including size, modify time, and whether the destination is newer/older or bigger/smaller. These flows MUST NOT be compressed into a combined name-plus-destination prompt, and they MUST NOT add a blanket post-target confirmation step.
*   **Tagged Delete Confirmation Rule:** Tagged delete keeps one explicit batch confirmation before deletion starts. The common path MUST NOT ask whether to confirm each tagged file after the operator already approved the batch; per-file follow-up prompts are allowed only for true safety exceptions such as read-only override.
*   **User Menu (`f9`):** Supports macro expansion: `%f` (file), `%d` (dir), `%t` (tagged list), `%p` (inactive panel path).

---

## 10. Safety & Integrity
*   **Signal Handling:** `SIGINT` and `SIGTERM` are trapped for graceful terminal restoration and VFS cleanup.
*   **Memory Management:** Recursive scans for the Tree View respect the `TREEDEPTH` safety limit to prevent stack overflows or OOM (Out of Memory) conditions on massive filesystems.
*   **Encapsulation:** Global state pointers are strictly forbidden. All logic must utilize the `ViewContext` structure passed explicitly through the call stack.
*   **Destructive-Action Confirmation Rule:** Before destructive mutations (delete, overwrite, replace), ytnova MUST show explicit confirmation with clear source/target context and a default-safe choice. Safe/non-destructive operations must remain confirmation-free.

---

## 11. Module Organization & Architecture

### 10.1 Directory Ownership
Every module (`.c`/`.h` pair) must reside in the directory corresponding to its architectural layer:
- **`src/core/`**: Application lifecycle, global state management (`ViewContext`, `Volume`), and session-level logic.
- **`src/fs/`**: Filesystem and archive I/O, VFS drivers, and low-level disk operations.
- **`src/cmd/`**: User command implementations (business logic). These modules coordinate between the FS model and the UI.
- **`src/ui/`**: Presentation layer, input loops (`ctrl_*.c`), rendering (`render_*.c`), and interaction widgets.
- **`src/util/`**: Stateless, non-business helpers (strings, memory_utils, path_utils, completion_utils).

### 10.2 Module Sizing & Cohesion
- **Target Size:** 100-800 Lines of Code (LOC).
- **Bloat Threshold:** Modules exceeding 1,000 LOC are candidates for decomposition.
- **Fragmentation Threshold:** Modules under 50 LOC must be merged into cohesive units.
- **Single Responsibility:** Each module must have one clear purpose.

### 10.3 Naming Conventions
- **`ctrl_` Prefix:** Reserved for modules containing the primary input/event loops for a view (Controller).
- **`render_` Prefix:** Reserved for modules dedicated to visual output via ncurses (View).
- **Generic Plural:** Use for stateless utility collections (e.g., `path_utils.c`).

### 10.4 Header Hygiene
- **Layered Access:** Communication between layers must occur through designated layer headers (`ytnova_fs.h`, `ytnova_ui.h`).
- **Decoupling:** Minimize cross-layer `#include` directives.
- **Encapsulation:** Internal module state and helper functions must remain `static`. Only the necessary API must be exposed in the header.
