# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Scope policy: `docs/CHANGELOG.md` records shipped, already implemented, major user-visible or project-level milestones only.
Minor/trivial fixes are tracked in git history.

## [1.0.0-beta] - 2026-09-14

First public beta release.

## [1.0.0-alpha] - 2026-06-10

*YtreeNova release history starts here after the rebrand. The codebase began from Ytree v2.10 on 30 Oct 2025 and has since been developed as a separate Unix-like XTreeGold tribute with broader workflows, stronger QA, and contemporary POSIX-focused internals.*

### Lineage
- YtreeNova began from the [Ytree](https://www.han.de/~werner/ytree.html) v2.10 codebase by Werner Bregulla and others, but now has its own name and release history.

### Core Architecture & Runtime
- **Refactoring & Standardization**: Modernized the original C89-oriented codebase to C99/POSIX standards.
- **Context-Passing Architecture**: Replaced implicit cross-module state access with explicit context parameters and ownership boundaries.
- **Global State Removal**: Drove major mutable application state out of file-scope globals and into structured runtime state.
- **Volume Architecture**: Reworked logged roots, physical directories, and archive contexts into explicit volume contexts so multi-volume and archive workflows share a common model.
- **XDG Configuration Support**: Honors standard XDG configuration locations.
- **AppState Contracts**: Documented and enforced AppState transition boundaries so controller and UI code can preserve selected-panel, viewport, and active-volume invariants.
- **State-Machine Transition Core**: Reframed `AppState` around one explicit transition machine with validated state changes and rendering as projection only, reducing split/focus/restore regressions.
- **SRP & SoC Enforcement**: Deep-dive refactor to decouple the Filesystem Model (Model) from the UI (View), replacing implicit state access with explicit ownership and context passing.
- **Global State Encapsulation**: Refactored the core engine to encapsulate previously global variables, improving reentrancy and modularity.
- **Source Normalization**: Reorganized the source tree into semantic layers (`core`, `ui`, `fs`, `cmd`, `util`) with modular header decomposition.
- **Identifier Normalization**: Anglicised source identifiers for consistency in the YtreeNova fork.
- **Signal Safety & Precision**: Moved clock handling to event-loop integration and enforced strict signal-safety for application shutdown.
- **Clock & Date Localization**: Stabilized the real-time clock and date handlers for the YtreeNova runtime.
- **Dynamic Layout Engine**: Eliminated layout "magic numbers" in favor of a runtime geometry engine that supports responsive resizing.
- **UTF-8 Support**: Full wide-character support (`ncursesw`) for correct display of Unicode filenames.

### Multi-Volume & Archive Management
- **Multi-Volume Architecture**: Support for logging multiple drives, directories, or archives simultaneously.
- **Volume Navigation**: New Volume Menu (`K`) and cycling (`<`, `>`) to switch between loaded contexts (including integrated release/unlog via `D`).
- **Universal Archive Engine**: Fully integrated `libarchive` as the primary engine for robust browsing and extraction (ZIP, TAR, 7Z, ISO, etc.).
- **Full Archive Creation**: Added the `O` action to the TUI flow for creating new archives, with automatic format inference from extensions (ZIP, TAR, GZ, BZ2, XZ).
- **Atomic Archive Modification**: Implemented a "Stream Rewrite" engine allowing for atomic entry addition, deletion, and renaming within compressed containers.
- **Archive Filesystem Parity**: Made archive browsing and operations follow the same selection, tagging, navigation, progress, and mutation rules as filesystem content.
- **Transparent Navigation**: Seamlessly traverse nested archives and exit archive contexts back to the physical filesystem using intuitive navigation logic.
- **Archive Search & Execute**: Support for searching (`^S`) and executing (`X`) files directly within compressed archive containers.
- **UDF/ISO Bridge Support**: Enhanced detection logic to correctly handle multi-format bridge media (e.g., modern Windows ISOs).

### New Features & UI/UX Refinements
- **Color Theme Engine**: Added full **256-color support**, a semantic theme catalog, and theme-local file-type palettes. Users select a named theme in `ytnova.conf`, then define UI roles and file-type coloring in `themes.conf`.
- **Hidden File Visibility**: Dotfiles and hidden directories are now filtered by default. Use the backtick (`` ` ``) key to toggle their visibility globally.
- **Split-Screen (F8)**: Support for independent panels with separate context, cursors, and filters for efficient cross-volume operations.
- **Integrated Comparison Suite**: Added a dedicated Compare submenu supporting Directory, File, and Logged-Tree comparisons.
- **Progress & ETA Tracking**: Implemented a universal progress display for long-running operations, providing a real-time progress bar, transfer rates, and linear ETA projections.
- **Incremental "To" Jump**: Integrated high-speed list navigation (`/`) and "To Dir"/"To File" selection jumps with sticky-cursor logic.
- **Comprehensive Filtering**: Unified filter stack supporting complex Regex patterns, Date ranges, File Attributes, and Size suffixes.
- **Internal File Preview (F7)**: Integrated file inspection mode that seamlessly toggles the statistics panel.
- **Contextual F1 Help**: Reworked help into authored inline pages with direct contextual guidance and linked navigation.
- **Kitty Keyboard Protocol**: Distinguishes `Ctrl-M` from Enter for tagged move when supported, while preserving the `Ctrl-N` fallback.
- **Unified Numeric FileInfo Band**: Replaced legacy display-mode cycling with direct `1..9` panel display controls for Name, Attributes, Owner, Times, Compact, size units, Mini preview, File detail, Git.
- **Vi-Keys Profile**: Runtime toggle for `h/j/k/l` navigation with automatic remapping of conflicting legacy keybindings.
- **Attributes Menu Consolidation**: Unified separate Date, Mode, Owner, and Group modification commands under a single `A` (Attributes) action menu.
- **Integrated Window Stack**: Implemented a tiered Dialog Manager for flicker-free rendering of prompts, menus, and history pop-overs.
- **Activity Spinner**: Visual feedback in the menu bar during long operations.
- **Contextual History**: Separated command history into relevant categories with support for "favorite" entries.
- **Smart Directory Creation**: Automatic prompts to create missing parent directories during Copy and Move operations.
- **Standardized Tag Inversion**: Aligned the invert-selection keys to `i` (current) and `I` (all) for consistency.
- **UI Consistency**: Standardized input prompt padding and command line geometry across all application modules.

### Stability, Security & QA
- **Security Hardening**: Completed a comprehensive sweep to replace all unsafe `sprintf` calls with bounds-checked `snprintf` across the entire codebase.
- **Memory Safety & Hygiene**: Established a zero-leak baseline using Valgrind/ASan, implemented standardized `xmalloc` wrappers, and modernized user/group database handling.
- **Subsystem Header Hardening**: Decoupled core and UI subsystem headers to eliminate transitive exports and enforce strict include discipline.
- **Path Utility Standardization**: Migrated all command-layer path compositions to a centralized, bounds-safe `Path_Join` utility.
- **Internal Viewer Geometry Encapsulation**: Implemented an explicit viewer geometry contract and removed direct layout reads from `src/ui/view_internal.c`.
- **CI and QA Suite**: Introduced GitHub Actions CI and a comprehensive local QA gate with `clang-tidy`, `cppcheck`, `scan-build`, Valgrind (including automated interactive runs), `pytest`/`pexpect`, and a module-boundary guard (`make qa-all`).
- **Controller Hotspot Decomposition**: Broke up high-risk controller "god function" paths into smaller, cohesive units to reduce regression blast radius and improve maintainability.
- **Permanent Security Regression Gates**: Added durable security/file-operation integrity checks so previously fixed exploitability classes are continuously guarded in QA/CI.
- **QA Cadence Optimization**: Reorganized QA gate cadence and overlap to improve iteration feedback speed while preserving strict pre-merge and release assurance depth.
- **Build System**: Updated Makefile for dependency tracking and unified documentation sourcing (`docs/USAGE.md` and `ytnova.1.md` are now generated from a single `etc/ytnova.1.md` source).
- **Silent Refresh-Scan Handling**: Suppressed transient `stat` errors during directory refreshes to prevent non-fatal race conditions.
- **Overwrite-All Conflict Hardening**: Unified COPY/MOVE overwrite-all behavior so selecting `A` on the first conflict suppresses repeated prompts across remaining tagged-file conflicts.
- **AI Governance framework**: Established a formal AI orchestration system using persona routing and automation "skills" to maintain architectural consistency.

## [2.10]
- 7zip / iso support.
- Should compile with gcc 15.

## [2.09]
- Window resize support for hex-view and hex-edit.
- Display format improvements.

## [2.08]
- "Show tagged files only" in file windows (Shift-F4 or C-f4).
- Lots of UTF_8 improvements/bugfixes.
- Improve compatibility with old unix systems / some minor fixes.

## [2.07]
- Removed -O compiler switch due to problems with UTF_8 code.
- Added workaround when resizing ytnova window during string input.

## [2.06]
- Fixed compile error in input.c (UTF_8 only).

## [2.05]
- Uses **regcomp** instead of deprecated re_comp/re_exec (**Unix/POSIX**).
- Debug file sort issue for very large file sizes.
- Reduced compiler warnings.

## [2.04]
- Avoid security warning for mvprintw in input.c.

## [2.03]
- cleanup + added LDFLAG "-ltinfo" (needed for some newer ncurses versions).

## [2.02]
- Crash during startup (group.c).

## [2.01]
- Updated man page (Thanks to Micah Muer).
- Debug crash when using "tagged rename".

## [2.00]
- No new functionality, but changed the copyright to GPLv2 or (at your option) any later version.

## [1.99pl2]
- Filename extensions debug (Thanks to Frank).

## [1.99pl1]
- Debug Path display for non color mode (Thanks to Ali).

## [1.99]
- Debug crash when switching directory view.
- Support large disks.

## [1.98]
- Bugfix for crash during showall with empty subfolders.
- New function "ListJump" (F12).
- Removed some compiler warnings.

## [1.97]
- Bugfixing in pathcopy / allow cancel during tagged copy.

## [1.96]
- Bugfix for long usernames in /etc/passwd.

## [1.95]
- Improved UTF-8 support.

## [1.94]
- Adapted Makefile to respect environment CC and LDFLAGS.
- New login to parent node ('p') command.

## [1.93]
- Debug Crash during switch to file window (Fedora 11).

## [1.92]
- Compile on DJGPP.
- Debug archive read if clock is enabled.

## [1.91]
- Compiles on Mac OS 10.5.4.
- Debug crash for tagged delete.

## [1.90]
- Changes for NetBSD.

## [1.89]
- Changes for NetBSD.

## [1.88]
- Changes for OpenBSD.

## [1.87]
- Removed warning in util.c.

## [1.86]
- Fixed crash when deleting last file in a directory.

## [1.85]
- Hopefully compiles again on sun architecture.

## [1.84]
- Enhanced UTF-8 support + russian man-page.

## [1.83]
- Bugfixes in Hex-Editor.
- UTF-8 support.

## [1.82]
- Removed character mangling bug in "Path:" line on certain platforms (at least Solaris).
- Ported to Mac OS X.

## [1.81]
- Hex-Editor.

## [1.80]
- ^Search command to untag files using an external program.

## [1.79]
- Libreadline support optional. Changed license from GPL 1 to GPL 2 due to use of libreadline.

## [1.78-cbf]
- Securityfix: ~/.ytnova-hst now honors umask.
- Bugfix: some window sizes and layout adjustments.
- New builtin hexdump with some code to do the editing of hex files.
- Readline integration into InputString including tilde expansion.
- Cleanup dirwin.c and others to make it more readeable.

## [1.78]
- Bugfix: Copy to rel. nonexisting path does not work (Thanks to ZP Gu).

## [1.77]
- Builtin hexdump.

## [1.76]
- Added QuitTo feature.

## [1.75]
- Port to OpenBSD.

## [1.74]
- Sort by extension now with secondary key.
- Port to GNU Hurd.

## [1.73]
- Show all tagged files added.
- Makefile update.
- Layout adjustments.

## [1.72]
- Added fully user-configurable commands and menu text.
- New command-line arguments: -p (profile), -h (history).
- Added configuration items: INITIALDIR, [MENU], [DIRCMD], [DIRMAP], [FILECMD], [FILEMAP].

## [1.71]
- ia64 ready.
- port to QNX 4.25 using ncurses 4.2.
- native X11 port using PDCurses.
- added configuration items: NOSMALLWINDOW, NUMBERSEP, FILEMODE.

## [1.70]
- Large file support.
- New spanish man-page.
- Dynamic resize window support.
- Bugfixes...

## [1.67]
- rar support.
- No 4GB freespace limit.

## [1.66]
- User defined view.
- spm Support.

## [1.65]
- FreeBSD Port.
- DJGPP Port.
- "jar" Support.
- minor changes.

## [1.64]
- rpm Support.
- Bugfixes...

## [1.63]
- Bzip2 Support.
- 8bit file/directory-names.
- Bugfixes...

## [1.62]
- Better escaping of meta-characters.
- Some minor bugfixes.

## [1.61]
- Clock Support.
- Enhanced Sort.
- Display Directory Attributes.
- Viewer Configuration.
- Color support.
- Command line history.
- Visual directory selection.
- Helper-Applications now configurable in ~/.ytnova.
- Force removing of non-empty directories.
- Dynamic directory scanning.
