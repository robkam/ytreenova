# **Bugs and Defects Requiring Fixes**

This file tracks fix-required bugs, architectural violations, and naming inconsistencies that require remediation.
Buglist is forward-looking (`planned`/`in-progress`). Completed items will be removed after fixed.

Ordering policy (for all editors, including AI editors):
- Put bugs that are high-impact first after that order remaining bugs by ease of implementation.
- Insert new approved bugs at the correct priority position (do not append by default).
- Bug numbering is top-to-bottom ascending (`BUG-1` = highest priority).
- Bug IDs are unstable labels and are likely to change often due to reprioritization/renumbering.

## **Current Runtime Defects (Highest Priority First)**

### **BUG-1: `-` Collapses a Directory Before Releasing Its Files**
*   **Description**: In directory mode, `-` currently collapses the selected branch before releasing its files. This prevents a user from keeping the visible directory tree for navigation while removing the selected directory’s files from Showall and the statistics.
*   **Expected**: The first `-` releases the selected directory’s files while keeping its visible subdirectories. The next `-` collapses those subdirectories.
*   **Reproduction**: Read a directory with subdirectories and files, then press `-`. The branch collapses immediately instead of leaving its subdirectories visible while the selected directory’s files disappear from Showall and the statistics.
*   **Impact**: Users cannot narrow a logged tree to a wanted branch without also losing the visible path used to reach it.

### **BUG-2: Split-Panel Filter State Leaks Across Panels on Shared Volumes and Volume Cycle**
*   **Description**: In split mode, a filespec filter set in one panel can appear in the other panel when both panels share the same volume, and it can also leak after the target panel cycles back to the same logged volume, instead of preserving panel-local filter state.
*   **Repro (manual, 2026-05-22)**:
    *   In left panel on volume 1, set a filter.
    *   `Tab` to right panel on volume 2 (no filter change observed there).
    *   In right panel, cycle to volume 1.
*   **Extension (manual, 2026-07-24)**:
    *   Enter split mode with `F8` so both panels are on the same drive.
    *   Set a filter in one panel.
    *   Press `Tab` to the other panel without cycling volumes.
*   **Expected**: The target panel restores its own saved filter state for that panel/volume context (panel-local isolation), not the other panel’s filter.
*   **Actual**: The target panel shows the other panel’s filter even without a volume cycle.
*   **Spec Violations**:
    *   `docs/SPECIFICATION.md` §5.1 **Active-Only Mutation Rule**
    *   `docs/SPECIFICATION.md` §5.2 **Filter (Filespec): Independent search/filter strings**
*   **Impact**: Cross-panel state leakage can silently narrow file lists and increase wrong-target risk.
*   **Remediation**:
    *   Make filter ownership panel-local for split state (panel + volume context), not volume-global.
    *   On `F8`, `Tab`, and volume-cycle transitions, restore filter from the target panel snapshot only; do not import from the opposite panel.
    *   Remove shared-buffer/aliasing paths that let both panels read/write one filter instance.
    *   Enforce active-only mutation and inactive freeze semantics for filter state.
*   **Status**: Confirmed.

### **BUG-3: F7 Preview Over-Restricts Command Availability**
*   **Description**: `F7` mode is currently incomplete for inspect-and-act workflows. Too many common file actions are disabled, so users must leave preview to continue work.
*   **Expected Behavior**:
    *   `F7` must allow a practical command subset for in-context file work (for example attributes/copy/delete/edit/filter/compare/move/new-date/open/print/rename/tag/untag/view/execute/quit paths as applicable).
    *   Tagged/search workflow must operate in `F7`: `^T` (tag-all), `^S` (search), then `^V` (view tagged/search results) without leaving preview.
    *   In `F7` preview, tagged search hits/results must be visibly highlighted.
    *   `F8` and `Tab` must remain disabled in `F7` preview mode so split/layout switching cannot mutate preview state unexpectedly.
*   **Impact**: Makes `F7` feel unfinished and adds avoidable friction in routine review workflows.
*   **Remediation**: Finish `F7` as an in-place work mode: allow core actions (tag/search/view results/compare/copy/move/rename) without leaving preview, keep `F8`/`Tab` blocked for state safety, and add regression coverage for allowed actions and blocked keys.
*   **Related**: Existing regression intent for `F8`-in-`F7` state safety must remain preserved and extended to `Tab`.
*   **Status**: Confirmed.

### **BUG-4: `Write` Offers/Describes Actions That Are Not Context-Valid**
*   **Description**: `Write` format/options/prompt/help are not consistently aligned with context (`dir`/`file`/`archive`/`tagged`) and can imply workflows that are unavailable or misleading in the active mode.
*   **Impact**: Reduces discoverability and trust, and creates avoidable trial-and-error in critical output/export flows.
*   **Remediation**: Define and enforce a context-valid option matrix for `Write`, expose only valid options in each mode, and keep prompt/help text explicit and non-jargon (including destination examples such as file output and printer-command output). Keep `SPECIFICATION`, `F1` help, and manpage/USAGE text synchronized with the same destination semantics.
*   **Status**: Confirmed.

### **BUG-5: Footer/Help/Prompt Trust Family**
*   **Description**: BUG-5 is the root footer keybinding/F1/prompt trust family. BUG-5.1 through BUG-5.4 are visible effects of the same underlying discoverability problem.
*   **Family contract**: footer, F1 help, and prompt text must report the same available actions and context; help surfaces must not imply unavailable actions; cancel/exit paths must restore the normal context footer; archive-specific messages must report the actual attempted shortcut; archive tree rendering must stay structurally honest.
*   **Related**: contextual F1/footer parity and shared-explainer link behavior.

#### **BUG-5.1: Copy/Move Cancel (`Esc`) Can Leave Footer Blank**
*   **Description**: In `Copy`/`Move` flows, canceling with `Esc` can leave the footer keybinding lines blank instead of restoring the normal context footer.
*   **Findings**:
    *   Deterministic repro under sanitizer gate (`make qa-sanitize`) on 2026-05-16: `tests/test_display_layout.py::test_dir_copy_to_missing_destination_prompts_create_and_no_restores_footer` fails with `AssertionError: Header/path row disappeared after canceling create prompt`.
    *   The failing path is directory copy to a missing destination where the create-directory prompt is canceled with `No`.
*   **Impact**: Hides command discoverability immediately after a canceled mutation flow and makes the UI look partially broken.
*   **Remediation**: On all `Copy`/`Move` cancel/exit paths (`Esc` and equivalent cancel keys), restore footer keybinding/F1 ownership deterministically to the active view context and force a full footer redraw before accepting the next command.
*   **Related**: `BUG-15` (footer restore consistency during input flows) and the contextual F1/footer parity contract.
*   **Status**: Confirmed.

#### **BUG-5.2: Prompt Footer/F1 Parity Can Hide Available Prompt Actions**
*   **Description**: In prompt-driven workflows, footer/F1 coverage can omit active prompt actions and semantics (for example completion/browse controls and compare/archive prompt meanings), leaving available behavior under-discoverable.
*   **Impact**: Creates hidden-feature workflow confusion and high-friction issue reports during routine operations.
*   **Remediation**: Enforce a prompt-context parity contract: footer shows currently available prompt actions; F1 may add concise semantics/examples for those same actions, but must not advertise unavailable actions.
*   **Related**: the contextual F1/footer parity contract, `BUG-5.3` (archive unavailable-action messaging), and `BUG-4` (prompt/help context mismatch).
*   **Status**: Confirmed.

#### **BUG-5.3: Archive Unavailable-Action Message Reports Wrong Shortcut**
*   **Description**: In archive mode, triggering `^W` can show an error message for a different shortcut (`^P is not available in archive mode`).
*   **Impact**: Misleading feedback increases operator confusion and undermines trust in key/action hints.
*   **Remediation**: Ensure unavailable-action messaging reports the actual attempted action/shortcut in archive context.
*   **Status**: Confirmed.

#### **BUG-5.4: Single-Empty-Directory Archive Can Collapse Tree Rendering**
*   **Description**: In archive mode, when the archive contains only one empty directory, ytnova can skip normal tree-node rendering and show that directory identity as appended archive-name/path text instead.
*   **Impact**: Obscures archive structure and creates high-friction navigation confusion in a common edge case.
*   **Remediation**: Keep archive tree rendering consistent in this edge case: render a proper directory node in the tree and keep archive identity separate from child directory labels.
*   **Related**: `BUG-5.3` (name-text rendering contamination) and the path/message formatting contract.
*   **Status**: Confirmed.

### **BUG-6: Progress Spinner Can Overwrite Footer/Prompt Help Surfaces**
*   **Description**: During long-running operations, spinner/progress rendering can overwrite footer keybinding or prompt text instead of using a non-obtrusive status area.
*   **Impact**: Hides available actions and makes active workflows look unstable or hung.
*   **Remediation**: Preserve footer keybinding/prompt/F1 ownership during progress updates. Render progress in a dedicated non-obtrusive status surface, and degrade to a compact indicator when space is constrained rather than overwriting user guidance text.
*   **Related**: `ROADMAP` Task 5 (Progress Indicators for Copy/Move/Delete/Archive Workflows), Task 5.1 (Keep Progress Indicators from Clobbering Footer/Prompt/F1 Guidance), and the contextual F1/footer parity contract.
*   **Status**: Confirmed.

### **BUG-7: File-View Focus Leak After Parent Jump (`\\`)**
*   **Description**: After entering parent-directory context from file view using `\\`, navigation keys affect the directory pane before explicit mode switch.
*   **Findings**:
    *   Arrow keys can change adjacent directory selection while the user is still in file view.
    *   `Home`/`End` act on the directory window instead of the file list in this state.
*   **Impact**: Breaks view isolation and can cause accidental navigation outside the intended file-list scope.
*   **Remediation**: Keep navigation scope in the file list after `\\` parent jump and require explicit `Enter` transition before directory-pane navigation is allowed.
*   **Status**: Confirmed.

### **BUG-8: Zoom/Split State Corruption After Parent/View Toggles**
*   **Description**: After a sequence involving show-all, repeated view-mode toggles, parent jump (`\\`), and another view toggle, the UI exits the expected zoomed state and shows an empty small pane.
*   **Findings**:
    *   Layout unexpectedly switches back to tree + small window.
    *   Small window can become empty instead of preserving the current context.
*   **Impact**: Breaks predictable navigation flow and increases risk of accidental context loss.
*   **Remediation**: Preserve active zoom/view state across parent-jump and view-toggle transitions; prevent empty-pane state in this flow.
*   **Related**: `BUG-7` (same focus/state-transition family).
*   **Status**: Confirmed.

### **BUG-9: Long Lines Wrap Instead of Truncate in List Views**
*   **Description**: Long entries wrap to additional rows instead of truncating in single-row list rendering contexts (reported in `^f` small-window flow and dir/tree list views).
*   **Impact**: Breaks scanability, corrupts row alignment, and causes ambiguous cursor context in navigation-heavy views.
*   **Remediation**: Enforce truncate/clipping semantics for list-row rendering in these views and add regression tests that fail on wrapping behavior.
*   **Status**: Confirmed.

### **BUG-10: Intermittent Showall/Global Filter Can Hide Matching Files**
*   **Description**: In intermittent edge-case `Showall/Global` + filter combinations, directories can show no files even when matching files exist.
*   **Impact**: Breaks trust in filter correctness and can make users miss valid results.
*   **Remediation**: Capture a minimal deterministic repro and add regression coverage; verify file-list build/count/filter logic remains consistent in `Showall/Global` contexts with active filters.
*   **Status**: Confirmed.

### **BUG-11: Directory Copy Prompt Does Not Clearly State Recursive Behavior**
*   **Description**: Directory copy flow prompt text is not explicit enough that directory copy is recursive, so users cannot reliably predict whether descendants are included.
*   **Impact**: Creates avoidable trust/friction issues in backup-style workflows and increases accidental over-copy concern.
*   **Remediation**: Make directory-copy prompts/confirmation text explicit about recursive behavior and resulting destination semantics before execution.
*   **Status**: Confirmed.

### **BUG-12: Directory Copy Can Appear Successful While Producing No Effective Update**
*   **Description**: In some destination states (for example existing target or edge-case destination handling), directory-copy flow can look like it executed successfully while leaving destination contents unchanged or unclear to the user.
*   **Impact**: High wrong-assurance risk for repeat backup workflows where users expect an update on each run.
*   **Remediation**: Report explicit copy outcome (`updated`, `skipped`, `destination exists`, or error) and never leave no-op outcomes ambiguous. Add regression coverage for existing-destination and missing-parent edge paths.
*   **Status**: Confirmed.

### **BUG-13: Attributes Name Truncation Can Hide File Identity**
*   **Description**: In attributes/stat contexts, long file names can be truncated using a tail-only style (for example `...fy_xml_integrity.sh`) that hides too much distinguishing information and makes similarly named files harder to differentiate at a glance.
*   **Impact**: Increases wrong-target risk during metadata workflows and slows navigation in dense directories with similar filenames.
*   **Remediation**: Apply an identity-preserving truncation policy for filename-bearing attribute surfaces: keep static deterministic text (no marquee/auto-scroll), prefer `prefix…suffix` for plain filenames, and use suffix-focused clipping only where path-tail context is explicitly higher-value.
*   **Related**: `BUG-9` (no-wrap/truncate contract), `ROADMAP` Task 3 (manual file-column width controls).
*   **Status**: Confirmed.

### **BUG-14: Internal Preview Down-Scroll Can Pass EOF and Repeat Last Page**
*   **Description**: In internal preview paths (`F7` preview and internal `^V` tagged viewer), down-scroll/page-down can continue past EOF and keep redisplaying the last page. Up-scroll behavior does not show this defect. External viewer mode stops correctly at EOF.
*   **Impact**: Produces misleading navigation state and inconsistent behavior between internal and external viewing paths.
*   **Remediation**: Clamp downward preview offsets at the last valid page in shared internal preview rendering paths so bottom-of-file is a hard stop.
*   **Related**: `F7` preview and internal `^V` tagged viewer should be treated as one defect family.
*   **Status**: Confirmed.

### **BUG-15: `/` Jump Replaces Footer Keybinding Hints with Prompt UI**
*   **Description**: Pressing `/` currently switches footer content to a `Jump to:` prompt UI instead of keeping the normal footer keybinding text visible while incremental jump runs.
*   **Impact**: Breaks the expected inline jump flow and creates avoidable UI churn during frequent navigation.
*   **Remediation**: Keep footer keybinding text unchanged during `/` incremental jump and apply immediate selection movement as characters are typed (for example `/y` jumps to the first matching entry) without footer prompt takeover.
*   **Status**: Confirmed.

### **BUG-16: Recursive Scan Interrupt Responsiveness**
*   **Description**: Interrupting a recursive expansion (`*`) via `ESC` is supported but requires multiple keypresses (Prompt Y/N).
*   **Impact**: Users cannot instantly halt accidental large-branch scans.
*   **Remediation**: Evaluate if `ESC` during `ReadTree` should immediately halt the scan once instead of prompting, given that partial results are preserved.
*   **Status**: Confirmed.

## **Correctness, Consistency, and Naming Defects (Priority Ordered)**

### **BUG-17: Configuration Template Drift (`VI_KEYS`)**
*   **Description**: Discrepancy in default visibility and documentation for `VI_KEYS`.
*   **Findings**:
    *   `default_profile_template.h` uses `VI_KEYS=0`.
    *   `etc/ytnova.conf` uses `VI_KEYS=0`.
    *   User scratchpad reports `ytnova.conf` had `1`.
*   **Impact**: Users may experience "magic" behavior changes if the disk-based config diverges from the internal template.
*   **Remediation**: Ensure the `ytnova --init` generation path strictly matches the `etc/ytnova.conf` provided in the distribution.
*   **Status**: Confirmed.

### **BUG-18: VI Mode Key Ambiguity and Collisions**
*   **Description**: When `VI_KEYS=1` is enabled, lowercase navigation keys (`h/j/k/l`) collide with primary command keys without clear UI signaling.
*   **Findings**:
    *   `j` maps to both `ACTION_MOVE_DOWN` (via `VI_KEY_DOWN`) and historically to `ACTION_LOG_VOLUME` (though currently `l/L` is the log volume key, older documentation/muscle memory remains confused).
    *   `k/K` is used for `ACTION_VOL_MENU`. In VI mode, lowercase `k` is stolen for `Up`, making the volume menu reachable only via uppercase `K`.
*   **Impact**: Inconsistent UI accessibility for power users.
*   **Remediation**: Audit all `VI_KEY` remappings in `key_engine.c` and ensure the footer keybinding lines (`display.c`) dynamically update to show the uppercase variants when `VI_KEYS=1`.
*   **Status**: Confirmed.

### **BUG-19: Incremental Search Legacy Mapping (`F12`)**
*   **Description**: `F12` is used as an alias for `/` (Incremental Search/Jump), but its presence is inconsistent in help strings and documentation.
*   **Impact**: Confuses users about "hidden" keys.
*   **Remediation**: Explicitly document `F12` as a legacy alias or deprecate it in favor of standard `/`.
*   **Parity Principle:** Treat this as a documentation-parity defect class: no active keybinding may exist in runtime without consistent footer, `F1`, and manpage/USAGE coverage.
*   **Status**: Confirmed.

### **BUG-20: Misleading Tree Expansion Action Names**
*   **Description**: The internal `YtreeNovaAction` names for tree expansion are swapped relative to their behavior and documentation.
*   **Findings**:
    *   `+` key maps to `ACTION_TREE_EXPAND_ALL`, but only expands **one level**.
    *   `*` key maps to `ACTION_ASTERISK`, and expands **recursively**.
*   **Impact**: Developer confusion and maintenance risk.
*   **Remediation**:
    *   Rename `ACTION_TREE_EXPAND_ALL` -> `ACTION_TREE_EXPAND` (or `ACTION_TREE_EXPAND_LEVEL`).
    *   Rename `ACTION_ASTERISK` -> `ACTION_TREE_EXPAND_RECURSIVE`.
*   **Status**: Confirmed.

### **BUG-21: Intermittent Split-Brain Redraw Between Stats Box and Main Panes**
*   **Description**: Intermittently, the stats box redraw state can diverge from the main UI surfaces (`path`, `dir`, and `file` windows), leaving one surface fresh while the other appears stale/corrupted.
*   **Impact**: Creates a visibly broken UI state and undermines trust in navigation context during active workflows.
*   **Remediation**: Unify frame redraw ownership so stats and main panes are rendered from one layout snapshot in one update cycle, and force full-surface invalidation/redraw on resize/recovery/error paths.
*   **Related**: `ROADMAP` Task 6 (unified frame redraw contract).
*   **Status**: Confirmed (intermittent; no deterministic repro sequence yet).

### **BUG-22: Directory Copy/Move Can Blank or Partially Drop the Main Frame During In-Session Refresh**
*   **Description**: After accepting a directory copy or move target, the live view can blank or lose parts of the main frame while the tree updates. The tree content may remain or reappear, but header/path, border lines, stats, or footer can disappear temporarily, making the operation look visually broken even when the filesystem mutation succeeds.
*   **Repro (manual, 2026-08-11)**:
    *   Copy or move a directory in the logged tree.
    *   Representative recordings:
        *   `/home/rob/recordings/inbox/ytnova-20260811-154517-135295-pc3eF7.cast`
        *   `/home/rob/recordings/inbox/ytnova-20260811-155440-139983-Yoy25x.cast`
*   **Expected**: The existing view stays intact and the destination branch updates in-place with no blank screen, no partial frame loss, and no whole-screen redraw glitch.
*   **Actual**: The directory tree can update while the surrounding frame drops out or goes blank long enough to look broken.
*   **Impact**: High-trust workflow damage in copy/move paths because successful filesystem mutations still look like a rendering failure.
*   **Remediation**: Treat directory copy/move refresh as a deterministic redraw/invalidation bug, not as prompt-flow work. Rebuild one authoritative frame update path for header/path, tree, file pane, stats, and footer so in-session directory mutations cannot leave partial surfaces behind.
*   **Related**: `BUG-21` (split-brain redraw), `BUG-23` (stats owner split), `ROADMAP` Task 6 (unified frame redraw contract).
*   **Status**: Confirmed.

### **BUG-23: Stats Panel Lacks a Single Coherent State/Render Owner**
*   **Description**: The stats area does not behave like one consolidated UI component with one authoritative state/render path. In broken states, one subsection (for example `ATTRIBUTES`) can remain visible while the rest of the stats surface is blank or stale, which makes the panel look like multiple historical fragments glued together instead of one coherent unit.
*   **Example symptom**: During prompt-driven flows such as `Write`, the right-side panel can show only the lower `ATTRIBUTES` subsection while upper stats sections disappear, even though the outer frame and surrounding panes remain on screen.
*   **Expected**: Stats must be one complete component in the appstate architecture: one explicit owner for its projection, one layout contract, one redraw/invalidation path, and deterministic all-or-nothing rendering of its subsections.
*   **Impact**: Breaks trust in the appstate architecture, makes redraw bugs harder to reason about, and creates a visibly kludged UI impression because users can see that different stats subsections are not being treated as one unit.
*   **Remediation**: Define stats as a first-class appstate/render component with one authoritative projection boundary. Consolidate subsection visibility, titles, values, and borders under one render contract so prompt/modal flows cannot partially orphan or preserve only one subsection. Regression coverage should verify that stats either renders as one complete valid unit or is intentionally hidden as one unit.
*   **Related**: `BUG-21` (split-brain redraw), `BUG-22` (directory copy/move frame drop), `docs/ARCHITECTURE.md` §4.2.3 (`AppState` transition contract), `ROADMAP` Task 6.1 (unified stats + main-pane redraw contract).
*   **Status**: Confirmed.
