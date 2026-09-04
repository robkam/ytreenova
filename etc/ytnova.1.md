<!-- Auto-generated from etc/help/man.en.md by scripts/generate_help_assets.py; do not edit directly. -->

# NAME

ytnova - a file manager for Unix-like systems

# SYNOPSIS

`ytnova` [`--init`] [`-v`|`-V`|`--version`] [`-p` *config_file*] [`-h` *history_file*] [`-d` *depth*] [`-f` *filter*] [*directory*|*archive*]...

# DESCRIPTION

**ytnova** is a file manager for UNIX-like systems (Linux, BSD, etc.). It is inspired by the DOS file manager **XTree**, offering a text-based user interface (TUI) that is fast, lightweight, and keyboard-driven.

It began from Ytree v2.10 but now continues as a separate Unix-like XTreeGold tribute, using contemporary POSIX/C99 code and libraries such as `libarchive`.

If no command line arguments are provided, the current directory will be logged.

# OPTIONS

*   **-d** *depth*: Override the default scan depth (TREEDEPTH). Supports numeric values or keywords: **min**/**root** (0), **max**/**all** (100).
*   **-f** *filter*: Specify an initial file filter (filespec) on startup. Supports patterns (e.g., `*.c`), exclusions (`-*.o`), and combinations (e.g., `*.c,*.h`). Use quotes to prevent shell expansion (e.g., `ytnova -f "*.c"`).
*   **-h** *history_file*: Use *history_file* instead of the default `~/.ytnova-hst`.
*   **--init**: Create missing starter profile, commands, theme, and applications files and exit. By default this creates `~/.config/ytnova/ytnova.conf`, `~/.config/ytnova/commands.conf`, `~/.config/ytnova/themes.conf`, and `~/.config/ytnova/applications.conf` only if they do not already exist, and falls back to the home-dotfile paths only when the XDG target cannot be used. Use `-p` to target a different profile file.
*   **-p** *config_file*: Use *config_file* instead of the default `~/.config/ytnova/ytnova.conf`.
*   **-v**, **-V**, **--version**: Print ytnova version information and exit.
*   *directory*|*archive*: One or more directories or archive files to log on startup. If multiple paths are provided, they are all loaded as separate volumes. The first path specified becomes the active view.

# CONCEPTS

### The Display
The screen is divided into three panes plus a footer keybinding line: the **Directory Tree** (upper-left), the **File Window** (below the tree), and the **Statistics/Info** pane (right, spanning both left panes). The footer shows context-sensitive keybinding hints.

### Logging
Unlike file managers that rescan directories on demand, ytnova "logs" (scans) directory structures into memory. This allows instant navigation and searching without disk lag. Use the **l** command to log new paths or archives.

### Auto-Refresh
ytnova monitors the **currently selected directory** for changes (created/deleted/modified files) and updates the file list automatically.

**Note:** This monitoring is **active only for the current directory**. Changes occurring in parent or sibling directories while they are not selected will not be detected automatically. Use **C-l** (Reload) or **F5** to refresh the view when navigating back to previously modified areas. Additionally, auto-refresh relies on kernel notifications. It may not function on network shares (NFS, SMB) or non-native mounts (e.g., WSL Windows drives) where the operating system does not propagate change events.

# MODES AND NAVIGATION

### Help System

This manual is the fuller reference path for ytnova modes, commands, prompts, and support topics.
The in-app `F1` popup provides the shorter contextual version for the active surface.
### Navigation

The help popup uses list-style navigation.
`Up` and `Down` move, `Enter` or `Right` follow, `Left` goes back, and `Esc` or `Q` closes.
### Directory Mode

Directory mode is the logged tree view.
It owns directory navigation, tree expansion, and directory-scoped commands.
### File Mode

File mode is the main file-list view.
It owns file navigation, file-scoped commands, tagged actions, and export entry points.
### Archive-Dir Mode

Archive-Dir mode is the tree-style view inside a logged archive.
It mirrors directory work where the archive format permits it.
### Archive-File Mode

Archive-File mode is the file-list view for archive-backed content.
Some filesystem commands are unavailable or become archive-aware here.
### Showall Mode

Showall lists every file inside the current logged volume in one aggregated file list.
It keeps single-volume scope while flattening directory boundaries.
### Global Mode

Global lists files from every logged volume in one aggregated file list.
It keeps multi-volume scope while flattening directory boundaries.
### File Preview Mode

F7 preview overlays file preview controls on top of the underlying file-selection context.
The preview owns scrolling while the underlying selection still owns the file target.
### Split Screen Mode

Split mode keeps two panels active at once, and runtime F1 opens the directory or file split page for the active panel.
Use the split page for the live footer command list and this page for the shared split model.
# KEY BINDINGS

**Note:** All keys are case insensitive unless otherwise noted. `C-<key>` means hold the Control key while pressing `<key>`. For most commands, pressing **C-key** (indicated in footer menus only where different) applies the action to all **tagged** files in the current scope. The live footer stays low-noise: there is no held-Control footer variant, and control-only tagged/search semantics are explained in the active prompt/**F1** help instead of being shown all the time.

### Global Commands
These commands work in most modes:

*   **F1**: Help. Opens a context-sensitive popup for the active runtime surface: directory/file/archive views, Showall/Global lists, `F7` preview, split-panel targeting notes, picker dialogs, and prompt-specific syntax such as `{}` placeholders or tagged-flow semantics.
*   **F5**: Refresh (same as **C-l**).
*   **F6**: Toggle the stats panel itself on and off. This does not change the current file or directory view selection.
*   **F7**: Toggle File Preview Pane.
*   **F8**: Toggle Split Screen Mode.
*   **F9**: Open the Applications menu. This picker lists external app presets from `applications.conf` or the packaged defaults; use `{}` for the selected path and `{input}` for prompted text.
*   **F10**: Open configuration. Press **Enter** or **C** to edit the main config, **M** to edit `commands.conf`, **T** to edit themes, or **R** to reload the current config/theme/commands set. The commands path owns preset selection plus local command overrides; packaged command presets stay read-only shared data. The Applications catalog lives in `applications.conf` and is edited from `F9`. A successful reload silently repaints; a failed reload keeps the previous working config/theme/commands state and reports the error in the status/footer area.
*   **/**: **Incremental Jump** (List Jump). Start typing to jump to the first matching entry in the current list (directory names in the Directory Window, filenames in the File Window). The selection updates immediately as you type. Press **Enter** to accept the current match, or **Esc** to cancel and restore the original selection.
*   **\**: In **Showall**/**Global** file lists, exit that mode and jump to the selected file in its owner directory. In Archive-Dir mode, `\` jumps to archive root when used below root, and exits to the parent physical directory when used at archive root. In normal filesystem dir/file windows and Archive-File mode, `\` is a no-op.
*   **1 .. 9**: File or directory info band for the active panel (disabled in `F7` preview).
    *   In tree/directory focus the footer shows `1..9 dir view`.
    *   In file focus the footer shows `1..9 file view`.
    *   The current file/directory stats section shows the active view by name (for example `View: Name`, `View: Compact`, or `View: Git`).
    *   `1`: Name only. This is the plain default/baseline view, startup always begins here, and pressing `1` also resets temporary compact/overlay state back to Name.
    *   `2`: Attributes, including `name -> target` symlink rows in file projections.
    *   `3`: Owner.
    *   `4`: Times.
    *   By default, `1..4` are shared per panel, so changing the tree/directory view also changes the file-window view for that panel.
    *   Selecting `1..4` returns that file projection to its named base view and clears temporary extra view state there.
    *   Pressing the already-active `2`, `3`, or `4` again resets that context back to `1` / Name.
    *   Set `SEPARATE_DIR_FILE_VIEWS=1` to make tree/directory and file-window `1..4` views independent again.
    *   `5`: Toggle the compact Name/full-width file rendering variant when the current `1` / Name base view is active.
    *   `6`: Toggle binary vs human-readable size units for directory/file rows only. Stats stay human-readable.
    *   `7`: Toggle Mini preview detail (start of readable file contents on every visible file row). This leaves Compact so the detail is visible.
    *   `8`: Toggle File detail (`file`-style type-summary text on every visible file row). This leaves Compact so the detail is visible.
    *   `9`: Toggle the Git status band in filesystem file lists when the current directory is inside a Git worktree.
    *   `5` only works from the current `1` / Name base view; it always uses the Name file projection and is a silent no-op from `2`, `3`, or `4`.
    *   `5`, `7`, `8`, and `9` do not change tree rows; they change the panel's file projection instead, so in tree focus they update the small file window and in file focus they update the file window.
    *   Extra view states do not stack in the stats label; it names the one visible active state (`Compact`, `Mini preview`, `File`, or `Git`).
    *   `0`: Currently unused; silent no-op.
*   **C-l**: **Reload**. Re-read the contents of the current directory from disk and refresh the view.
*   **K**: **Volume Menu**. Show a list of all currently logged volumes (drives/paths). Select a volume to switch context instantly. Selecting the already-active volume preserves its current in-memory state (no implicit relog). Press `Delete` (or `D`) in the menu to release (unlog) a volume. *(With `VI_KEYS=1`, use uppercase `K`; lowercase `k` is navigation.)*
*   **<** / **>** (or **,** / **.**): **Cycle Volumes**. Switch to the previous or next logged volume instantly.
*   **C-q**: **Quit to Directory**. If you exit ytnova with C-q, the last selected directory becomes your current working directory. See shell wrapper function below.
*   **Q**: **Quit**. Exit ytnova.

### Vi Keys Mode (Profile Option)
When `VI_KEYS=1` in `[GLOBAL]`, ytnova reserves lowercase vi navigation keys:
`h/j/k/l` and `C-d/C-u` (page down/up). To avoid collisions:

*   Use **H/L/K/J** for **Hex/Log/Volume Menu/Compare**.
*   In file-view contexts, use **D** for **Delete Tagged** and **U** for
    **Untag All**.
*   Lowercase **d/u** keep the regular context action (single item / current
    scope untag).

### Shared Commands

#### Shared function keys
* **F1**: Open contextual help for the active surface.
* **F5**: Refresh the current view.
* **F6**: Change the stats/details presentation for the active view.
* **F7**: Toggle preview for the active file context.
* **F8**: Toggle split-screen mode.
* **F9**: Open the Applications menu.
* **F10**: Open the configuration command surface.
* **Esc**: Back out of the current overlay, prompt, or popup.

#### Footer space
The footer is authoritative for the active command map. `commands.conf` may change bindings and labels, so use the footer for the exact active keys.
When the footer is truncated in small terminal windows, use `C--` to reduce the terminal text size or `C-+` to increase it.
### Directory Mode

#### Directory navigation
* **Enter / Right / Left**: `Enter` opens the file window and finishes logging when the selected directory is not expanded yet. `Right` expands first and then descends. `Left` collapses the current node or climbs to its parent.
* **Logging controls**: `+` logs or reveals one level without moving. `*` expands recursively. `-` collapses the branch, and a second `-` on a collapsed logged node releases it back to an unlogged state.
* **Tree ownership**: Directory Mode owns branch shape and logged-tree coverage. File lists, Showall, and Global only project files from the tree you have already logged.

#### Directory command families
* **Presentation and scope**: `1..9 view` changes the panel presentation. `Filter`, `Showall`, `Global`, and `Jump` change which projected file set or visible subset you are inspecting.
* **Filesystem changes**: `Attributes`, `Rename`, `Delete`, `Makedir`, and `New File` change metadata or create/remove entries. `Log` adds or reloads a logged root.
* **Working-set control**: `Tag`, `Untag`, and `Invert Tags` define the set that later bulk commands consume.
* **Transfer and export**: `Copy`, `MoveDir`, `Output`, `Pipe`, and `Archive` act on the selected branch or on the tagged set, depending on the command.
* **Cross-surface actions**: `Compare` hands off to the compare flow, `Execute` runs a shell command with the current path, `Volume` switches logged volumes, `Dotfiles` toggles hidden entries, and `Quit` leaves ytnova.
### File Mode

#### File navigation
* **Presentation**: `1..9 view` stays in file mode and changes Name, Attributes, Owner, and Times plus Compact, size units, Mini preview, File detail, and the Git band where they apply.
* **Enter**: Switch between the embedded file window and full-screen file mode without leaving the same file list.
* **Columns**: `Left` and `Right` move across visible file columns. In single-column layouts they page backward and forward through the same list.

#### File command families
* **Inspection**: `View`, `Hex`, and `Edit` open the selected file through the configured pager, hex viewer, or editor.
* **Transfer**: `Copy`, `Move`, and `Pathcopy` operate on the selected file. `Copy tagged` and `Move tagged` apply the same target rules to the tagged set.
* **Working-set control**: `Tag`, `Untag`, `Tag all`, `Untag all`, and `Invert Tags` build or clear the set that later bulk commands consume.
* **List control**: `Filter`, `Sort`, `Jump`, and `Dotfiles` change how the current file list is projected. The filter prompt still owns the tagged-only scope toggle on `Tab`.
* **Metadata and creation**: `Attributes`, `Rename`, `Delete`, `New File`, and `Log` edit file state or add/reload content sources.
* **Output and shell handoff**: `Output`, `Pipe`, `Execute`, and `Archive` export the current file or tagged set. `Execute` expands the prefilled `{}` path, and `C-x` reruns the command once per tagged file.
* **Cross-surface actions**: `Compare` enters the compare flow, `Search tagged` narrows the tagged set by content, `Volume` switches logged volumes, and `Quit` exits ytnova.
### Archive-Dir Mode

#### Archive directory navigation
* **Enter / Left / Right**: Navigate the virtual tree the same way as ordinary Directory Mode, but only within the currently opened archive.
* **Root handling**: `\` jumps to archive root from deeper nodes, and leaves the archive entirely when you are already at that root.
* **Archive scope**: Expansion state is virtual. It reflects archive contents, not a live writable filesystem tree.

#### Archive directory command families
* **Presentation and scope**: `1..9 view` still selects the base directory/file presentation, except `9` stays inert in archives. `Filter`, `Showall`, `Global`, and `Jump` still operate on the archive-backed visible set.
* **Archive-aware edits**: `Copy`, `Pathcopy`, `Move`, `Delete`, `Rename`, and `Makedir` only work when the current archive format and access path support write-back semantics. Directory transfers are recursive and reject a destination inside the source subtree. Common writable formats include `.tar`, `.tar.gz`, `.tar.bz2`, `.tar.xz`, and `.zip`; actual availability depends on the installed libarchive and archive properties.
* **Working-set control**: `Tag` and `Untag` operate on the current virtual directory scope.
* **Transfers and export**: `Output`, `Pipe`, and `Compare` use archive-backed paths. `Log` and `Volume` switch away to other logged roots or volumes when needed.
* **Session controls**: `Dotfiles` toggles hidden archive entries where the format exposes them, and `Quit` exits ytnova.
### Archive-File Mode

#### Archive file navigation
* **Presentation**: `1..8` keeps the usual file-view bands, while `9` remains a no-op because archive entries do not expose the Git band surface.
* **Enter**: Return to Archive Directory Mode for the same archive.
* **List control**: `Jump`, `Filter`, and `Sort` still operate on the archive-backed visible file list.

#### Archive file command families
* **Inspection**: `View` and `Hex` open the selected archive entry without first moving you into an ordinary file-mode session.
* **Transfer**: `Copy`, `Move`, and `Pathcopy` use archive-aware extract/copy paths. `Copy tagged` and `Move tagged` apply the same rules to the tagged archive set.
* **Working-set control**: `Tag`, `Untag`, and `Invert Tags` manage the current archive-backed working set.
* **Mutation limits**: `Delete` and `Rename` exist only where the archive path supports write-back. `Execute` is not available in archive file mode.
* **Output and comparison**: `Output`, `Pipe`, `Compare`, `Search tagged`, and `View tagged` all stay scoped to the archive-backed list rather than a normal filesystem directory.
* **Session controls**: `Log`, `Volume`, `Dotfiles`, and `Quit` behave like their file-mode counterparts, but they may take you out of the current archive session.
# COMPARE

#### Compare flow
Choose the target first.
Then choose the compare scope when the source is a directory.
Then choose the compare basis when the runtime offers more than one basis.
Finally choose which result class to tag on the source side.

#### Compare rules
* Logged-tree compare uses logged content only. It does not auto-log unopened `+` subdirectories.
* `FILEDIFF` may use `%1` and `%2`. When those placeholders are missing, ytnova appends source and target paths to the helper command.
* External directory/tree compare launches `DIRDIFF` or `TREEDIFF` instead of tagging runtime results.
* There is no separate compare-tagged-files mode.

# COMMAND LINE EDITING

### Line Editing Keys

Input prompts support standard text-editing shortcuts:

*   **C-a / Home**: Start of line.
*   **C-e / End**: End of line.
*   **C-k**: Delete to end of line.
*   **C-u**: Delete to start of line.
*   **C-w**: Delete word left.
*   **C-d / Del**: Delete character.
*   **C-h / Backspace**: Backspace.

### Prompt Navigation Keys

These keys apply while prompt dialogs are active (for example: Log, Copy, Move).

*   **Up Arrow**: History (with `P` to Pin, `D` to Delete).
*   **F2**: Directory picker for path-entry prompts.
*   **Missing copy/move destinations**: If the resolved destination directory does not exist, ytnova shows `Create missing directory? (y/N)` with the full target path. `y` creates it before the operation continues; `N`/`Esc` returns to the destination prompt without mutating the filesystem.

### Filter Help

#### Syntax
* **Glob selectors**: `*` shows everything. `*.c` matches one pattern. `*.c,*.h` stacks multiple include terms.
* **Exclusions**: Prefix a term with `-`, for example `-*.o`, to subtract matching rows after the include terms.
* **Extended selectors**: Attribute tests such as `:r` or `:x`, date tests such as `>2023-01-01`, and size tests such as `>1M` can be mixed with glob terms.
* **Combinations**: `*.c,-*.tmp`, `*.c,*.h,>1M`, `:r,*.sh`, and `*.log,>2024-01-01,-debug*` are all valid compound filters.
Use normal glob-like patterns such as `*.c`, comma-separated unions such as `*.c,*.h`, exclusions such as `-*.o`, and extended selectors such as `:r`, `:x`, `>2023-01-01`, or `>1M`.
If your shell would expand the pattern before ytnova sees it, quote it at the shell prompt.

#### Scope
The filter always applies to the current file-list family: a normal file list, an archive file list, Showall, or Global.
`Tab` switches between all rows and tagged-only rows for that same visible family.
The tagged-only toggle is offered only when tags already exist in the current scope, and the prompt changes to `FILTER [tagged only]:` when it is active.
### Output Help

#### Output model
`Output` is a batch export flow, not a viewer.
It writes the current file or tagged set as `Raw`, `Framed`, or `Page break` text, or sends the same stream to a printer command.

#### Output order
Choose the destination class first: file path or Hardcopy.
When the destination is a file, `F3` cycles `Raw`, `Framed`, and `Page break` before the final path is accepted.
Framed and page-break output ask for the separator string before returning to the destination prompt.
Hardcopy asks only for the printer command because it always streams raw output.
# SUPPORT TOPICS

### Command-line Editing

#### Editing keys
* **Left/Right**: Move inside the current prompt text.
* **Home/End**: Jump to the start or end of the prompt text.
* **Backspace/Delete**: Delete the character to the left or right of the cursor.
* **Enter**: Accept the current value.
* **Esc**: Cancel without committing the prompt.

#### Shared helpers
* **Up**: Open or cycle prompt history when that prompt keeps history.
* **F2**: Open a browser or picker when the current prompt supports browsing.
* **F1**: Show syntax or scope rules that matter only to the current prompt.
### Copy/Move Targets

#### Target forms
Use a directory path when you want the original names preserved under another directory.
Use one full replacement name when you want one selected item to land under a new explicit name.
Use a wildcard pattern such as `*.bak` or `copy-*` when you want ytnova to rewrite each selected basename by pattern.

#### Shared rules
Tagged copy/move uses the same target syntax as single-item copy/move.
Pathcopy uses the same two-prompt target flow while preserving the selected file's path relative to the current volume root.
Split mode may seed the inactive-panel directory as the default target, but you can still replace that default before the operation starts.
Archive-backed copy/move keeps the same destination model even when extraction or archive-aware paths are involved.
Only real safety prompts may follow the name and destination prompts, such as overwrite/replace conflicts or creating a missing destination directory.
Overwrite/replace conflicts show source and destination size/time facts when available so you can see whether the destination is newer/older or bigger/smaller before answering.
Directory copy/move starts after the destination is accepted; there is no extra copy-now or move-now confirmation.
### List Jump

#### Jump model
`/` opens an incremental jump prompt for the current visible list only.
Tree/directory views jump among visible directory names, while file-oriented views jump among the visible file rows for that surface.

#### Acceptance and cancel
* **Type text**: Move immediately to the best current match as you type.
* **Enter**: Keep the current match and stay there.
* **Esc**: Cancel the jump and restore the original selection.
* **Scope changes**: Filtering, Showall/Global projection, archives, and split mode all change which visible list `/` searches, but they do not change the jump keys themselves.
### Vi Keys

#### Navigation remap
With `VI_KEYS=1`, lowercase `h`, `j`, `k`, and `l` become `Left`, `Down`, `Up`, and `Right`.
`C-u` and `C-d` become page up and page down.

#### Command collisions
Commands that would steal those lowercase keys move out of the way.
Examples include `J compare`, `K volume`, `D delete tagged`, and `U untag all` where those actions exist.
### F10 Config

#### Config surface
Use `F10` when you want to change persistent behavior instead of doing one one-off file or directory action.
Profile settings, command labels, themes, and reload all live here.

#### Related files
`ytnova.conf` owns profile settings.
`commands.conf` owns user command labels and bindings.
`themes.conf` owns theme selection and theme-role overrides.
### Theming

#### Theme model
Themes set semantic roles such as `footer`, `help`, `help_footer`, `help_heading`, `help_topic`, `help_attention`, `help_alert`, `help_keybind`, `help_link`, `help_link_selection`, `selection`, `picker`, and `warning`.
`footer` owns the always-visible main-app footer, while `help` owns the F1 reading body, `help_footer` owns the popup strip, and `help_box_lines` owns the popup frame.
Headings/titles use `help_heading`, term-style labels use `help_topic`, bounded callouts use `help_attention`, and any future stronger urgency tier can use `help_alert`, so help pages stay readable without hard-coded colors.

#### Editing path
Use `F10` to open the theme or config editing path.
Keep high-frequency navigation surfaces readable first: selection, picker, footer, and help.
# CONFIGURATION

ytnova reads its main configuration from `~/.config/ytnova/ytnova.conf` by
default. The home-directory fallback path is `~/.ytnova` when the XDG target
cannot be used. Passing `-p` *config_file*
uses that explicit main config path instead.

Use `ytnova --init` to create the preferred main config when it is missing.
Existing files are never overwritten by `--init`.
Example: `ytnova --init`

The file created by `--init` is a fully annotated profile template. It selects
the default semantic theme; role definitions and file-type palette rules live
in theme files, not in the main config.

Theme catalogs are plain text. ytnova loads user themes from
`$XDG_CONFIG_HOME/ytnova/themes.conf` or `~/.config/ytnova/themes.conf`, falls
back to `~/.ytnova.themes` only when the XDG-style target cannot be used, then
uses the installed packaged catalog or compiled-in defaults without creating a
user theme file. Run `ytnova --init` to bootstrap an editable starter catalog.

Theme roles use semantic names such as `dynamic_text`, `static_text`, `keybind`,
`footer`, `selection`, `dialog`, `picker`, `picker_selection`, `help`,
`help_footer`, `help_heading`, `help_topic`, `help_attention`, `help_alert`,
`help_keybind`, `help_link`, `help_link_selection`, `help_box_lines`,
`warning`, `error`, and
`search_hit`. `footer` owns the always-visible main-app keybinding strip, while
`help` owns the F1 reading surface. `help_footer` owns the F1 popup strip,
`help_heading` owns popup titles, `help_topic` owns term-style labels,
`help_attention` owns bounded authored callouts, and `help_alert` is the
reserved stronger urgency tier. `help_keybind` owns help-popup mnemonic emphasis;
when it is omitted, runtime falls back to `keybind` on the `help_footer`
background. `help_box_lines` owns the F1 popup frame; when it is omitted,
runtime inherits the `help` foreground and background. When
`picker_selection` is omitted it falls back to
`selection`, so existing themes keep the same picker highlight behavior. The
bundled starter themes keep `picker` on a different background so F2,
history, volume, and applications menus stand out from the main content
background. Color values accept names or numbers, `grey`/`gray`, bright
prefixes such as `+white`, and optional backgrounds such as `+white on
blue`. `+grey`/`+gray` is accepted syntax but currently renders as
`white`, so prefer `white` when you mean the rendered color.

Theme-local file-type palettes use compact grouped rules, for example
`archives = red: tar,tgz,zip` or `scripts = +cyan: sh,bash,py`. Rules are
evaluated top to bottom; the first matching extension or special selector wins.
Special selectors may include `LINK` and `EXEC`; directory tree rows use theme
roles rather than file-type palette rules. When a rule omits a background, it
inherits the active filename/window background. Starter themes should also omit
redundant backgrounds on ordinary content roles when they are meant to follow
the theme background, so changing `background = ...` repaints the shared
surface intuitively.

Command customization lives in `commands.conf`. ytnova loads user command
overrides from `$XDG_CONFIG_HOME/ytnova/commands.conf` or
`~/.config/ytnova/commands.conf`, falls back to `~/.ytnova.commands` only when
the XDG-style target cannot be used, then uses the installed packaged active
command map or compiled-in defaults without creating a user command file.
`commands.conf` may optionally start with `preset = <id>` to select one
packaged read-only command preset before local per-action overrides are
applied. Packaged preset catalogs live under the shared app-data commands
directory (for example `/usr/share/ytnova/commands/<preset>.conf`); `F10`
edits only `commands.conf`, not the packaged preset files.

Applications presets live in `applications.conf`. ytnova loads user
applications from `$XDG_CONFIG_HOME/ytnova/applications.conf` or
`~/.config/ytnova/applications.conf`, falls back to `~/.ytnova.applications`
only when the XDG-style target cannot be used, then uses the installed
packaged Applications catalog or compiled-in defaults without creating a user
file. Press `F9`, then `E`, to bootstrap an editable starter catalog. Use `{}`
for the selected path and `{input}` for prompted text.

# QUIT TO DIRECTORY

To allow `C-q` to change your shell's working directory, add this shell wrapper function to your `~/.bashrc`. It also gives you a short `yt` command:

```bash
yt() {
    ytnova "$@"
    local tmpfile="$HOME/.ytnova-$$.chdir"
    if [ -f "$tmpfile" ]; then
        source "$tmpfile"
        rm "$tmpfile"
    fi
}
```

# FILES

*   `$XDG_CONFIG_HOME/ytnova/ytnova.conf` or `~/.config/ytnova/ytnova.conf`: Preferred main configuration file.
*   `$XDG_CONFIG_HOME/ytnova/commands.conf` or `~/.config/ytnova/commands.conf`: Preferred user command map and preset-selection file.
*   `$XDG_CONFIG_HOME/ytnova/applications.conf` or `~/.config/ytnova/applications.conf`: Preferred user Applications catalog.
*   `$XDG_CONFIG_HOME/ytnova/themes.conf` or `~/.config/ytnova/themes.conf`: Preferred user theme catalog.
*   `$XDG_STATE_HOME/ytnova/ytnova.hst` or `~/.local/state/ytnova/ytnova.hst`: Preferred command history path.
*   `~/.ytnova`: Legacy fallback main configuration file.
*   `~/.ytnova.commands`: Legacy fallback user command map file.
*   `~/.ytnova.applications`: Legacy fallback user Applications catalog.
*   `~/.ytnova.themes`: Legacy fallback user theme catalog.
*   `~/.ytnova-hst`: Legacy fallback command history path.
*   `/usr/share/ytnova/ytnova.commands`: Installed packaged active command map.
*   `/usr/share/ytnova/ytnova.applications`: Installed packaged Applications catalog.
*   `/usr/share/ytnova/commands/*.conf`: Installed packaged read-only command presets.

### Reporting problems

If you find anything amiss, you can report it using [GitHub Issues](https://github.com/robkam/ytreenova/issues).

It will help us to address the issue if you include the following:

*   **OS & Configuration:** (Distro, Terminal type, etc.)
*   **YtreeNova Version:**
*   **Steps to Reproduce:**
*   **Expected Behavior:**
*   **Actual Behavior:**

# AUTHORS

Authors and contributors are listed in the AUTHORS.md file.

# SEE ALSO

**bash**(1), **glob**(7), **grep**(1), **less**(1), **regex**(7), **vi**(1)
