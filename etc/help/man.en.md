# YtreeNova manpage and USAGE help source (English)

Edit this file to improve manpage and USAGE reference text. It is the authored source for the manpage and generated `docs/USAGE.md`, kept distinct from contextual `F1` help.

## Topic-block schema

Every topic block in this file follows the same parser-facing contract:

1. The block starts with a level-2 heading in the exact form `## topic:<topic-id>`.
2. The heading is followed immediately by a fenced metadata block labelled `ytnova-help-meta`.
3. The metadata block contains exactly these keys, in this order:
   * `title:` — plain-text topic title.
   * `contexts:` — comma-separated stable runtime context/prompt IDs, or the literal `none` for link-only explainer pages.
4. The block then contains these sections in order:
   * required `### Contextual F1`
   * optional `### Explainer links`
   * required `### Long form`
5. When `### Explainer links` is present, every item uses Markdown link syntax with a `topic:` target, for example `- [Navigation](topic:navigation)`.
6. `### Long form` contains one or more level-4 subsections (`#### ...`). Their order is preserved as-authored for later projection.

Keep cross-references sparse. This source is the fuller reference path; contextual `F1` help is the shorter in-task path.

## topic:index

```ytnova-help-meta
title: Contents
contexts: none
```

### Contextual F1

This manual is the fuller reference path for ytnova modes, commands, prompts, and support topics.
The in-app `F1` popup provides the shorter contextual version for the active surface.

### Long form

#### Purpose

This file is the fuller reference source for the manpage and generated `docs/USAGE.md`.
The in-app `F1` popup remains the shorter contextual path for the active screen, prompt, or dialog.

#### Contents

* **Modes and navigation**: `Directory`, `File`, `Archive-Dir`, `Archive-File`, `Showall`, `Global`, `F7 Preview`, and `F8 Split` explain what each runtime surface owns.
* **Shared operator rules**: `Navigation`, `Tagged`, `Shared Commands`, `Command-line Editing`, `Vi Keys`, `F10 Config`, and `Theming` collect cross-surface behavior once instead of repeating it in every mode page.
* **Prompt references**: `List Jump`, `Copy/Move Targets`, `Filter`, `Compare`, `Output`, `Execute`, `Create Archive`, and `Date Change` document syntax, scope, and decision points.
* **Chooser and support surfaces**: `History`, `Volume`, `Applications`, and the `F2 Picker` cover the reusable helper dialogs and menus.

## topic:navigation

```ytnova-help-meta
title: Navigation
contexts: none
```

### Contextual F1

The help popup uses list-style navigation.
`Up` and `Down` move, `Enter` or `Right` follow, `Left` goes back, and `Esc` or `Q` closes.

### Long form

#### Control-key notation

`C-<chr>` means hold the Control key while typing `<chr>`. For example, `C-f` means hold Control and type `f`.

#### Help popup keys

* **Up/Down**: Move between selectable rows or links.
* **Page Up/Page Down**: Scroll longer help pages.
* **Home/End**: Jump to the top or bottom of the current help page.
* **Enter/Right**: Open the selected help item or linked topic.
* **Left**: Go back one step.
* **Esc/Quit**: Close the popup.

#### Scope boundary

This topic owns help-popup movement only.
Use `List Jump` for runtime `/` name-jump behavior, and use the local mode page for ordinary tree/file selection commands.

## topic:list-jump

```ytnova-help-meta
title: List Jump
contexts: none
```

### Contextual F1

`/` is ytnova's in-list name jump.
It is distinct from help-popup navigation and remains scoped to the current runtime list.

### Long form

#### Jump model

`/` opens an incremental jump prompt for the current visible list only.
Tree/directory views jump among visible directory names, while file-oriented views jump among the visible file rows for that surface.

#### Acceptance and cancel

* **Type text**: Move immediately to the best current match as you type.
* **Enter**: Keep the current match and stay there.
* **Esc**: Cancel the jump and restore the original selection.
* **Scope changes**: Filtering, Showall/Global projection, archives, and split mode all change which visible list `/` searches, but they do not change the jump keys themselves.

## topic:shared-commands

```ytnova-help-meta
title: Shared Commands
contexts: none
```

### Contextual F1

These function keys keep their high-level meaning across modes.
Surface-specific details still belong to the relevant mode or prompt topic.

### Long form

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

## topic:tagged

```ytnova-help-meta
title: Tagged
contexts: none
```

### Contextual F1

Tagged files form a working set for bulk actions, narrowed views, searches, and archive/export flows.
Tag-driven behavior is central to ytnova command workflow.

### Long form

#### Tagged basics

Tags are a working set. They are not a second clipboard and not a saved search.
You can build a set, act on it, narrow it, then clear or invert it.

#### Common tagged flows

* **Tag** and **Untag**: Add or remove the current row from the working set.
* **Invert Tags**: Flip the tag state inside the current visible scope.
* **Filter**: Press `F`, then `Tab` to switch the current file-list scope between all rows and tagged-only rows without changing tag state.
* **Copy tagged** and **Move tagged**: Send the whole tagged set to one destination.
* **View tagged**: Open the tagged files one after another. In the internal viewer, `n`/`p` change file, `Space`/page keys scroll only the current file, and `/`/`?` move between tagged-search hits in that file. `TAGGEDVIEWER=external` keeps pager-native search and hit navigation.
* **Search tagged**: Search only the tagged files, then untag non-matches.
* **Archive**: Archive the tagged set first. When nothing is tagged, archive falls back to the current selection.

## topic:tagged-viewer

```ytnova-help-meta
title: Tagged Viewer
contexts: viewer.tagged
```

### Contextual F1

Use `n` and `p` for files, page keys and `Space` within the current file, and `/` or `?` for tagged-search hits. `C-s` searches the tagged list outside the viewer.

### Long form

#### Navigation scopes

The internal tagged viewer keeps file, page, and search-hit navigation separate. An external tagged viewer leaves search and hit navigation to the configured pager.

## topic:command-line-editing

```ytnova-help-meta
title: Command-line Editing
contexts: none
```

### Contextual F1

Most prompts share the same editing keys.
Prompt-specific syntax and scope rules belong to the relevant command topic.

### Long form

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

## topic:copy-move-targets

```ytnova-help-meta
title: Copy/Move Targets
contexts: none
```

### Contextual F1

Copy, move, and pathcopy use two explicit prompts.
First choose the replacement name or wildcard rename pattern.
Then choose the destination directory.
The split stays intentional because name/pattern and destination are separate decisions.
Merging them would hide meaning instead of removing friction.
Overwrite conflicts compare size/time so you can judge newer/older and bigger/smaller.

### Long form

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

## topic:vi-keys

```ytnova-help-meta
title: Vi Keys
contexts: none
```

### Contextual F1

When `VI_KEYS=1`, lowercase vi navigation is reserved.
Conflicting commands move to uppercase or another safe key.

### Long form

#### Navigation remap

With `VI_KEYS=1`, lowercase `h`, `j`, `k`, and `l` become `Left`, `Down`, `Up`, and `Right`.
`C-u` and `C-d` become page up and page down.

#### Command collisions

Commands that would steal those lowercase keys move out of the way.
Examples include `J compare`, `K volume`, `D delete tagged`, and `U untag all` where those actions exist.

## topic:f10

```ytnova-help-meta
title: F10 Config Help
contexts: none
```

### Contextual F1

F10 owns configuration-related actions, including profile editing, command editing, theme editing, and reload.
It is the setup surface rather than an ordinary file-management command.

### Long form

#### Config surface

Use `F10` when you want to change persistent behavior instead of doing one one-off file or directory action.
Profile settings, command labels, themes, and reload all live here.

#### Related files

`ytnova.conf` owns profile settings.
`commands.conf` owns user command labels and bindings.
`themes.conf` owns theme selection and theme-role overrides.

## topic:theming

```ytnova-help-meta
title: Theming
contexts: none
```

### Contextual F1

Themes style semantic UI roles and file-type palettes.
Theme edits belong in the config/theme files, not in per-screen hard-coded colors.

### Long form

#### Theme model

Themes set semantic roles such as `footer`, `help`, `help_footer`, `help_heading`, `help_topic`, `help_attention`, `help_alert`, `help_keybind`, `help_link`, `help_link_selection`, `selection`, `picker`, and `warning`.
`footer` owns the always-visible main-app footer, while `help` owns the F1 reading body, `help_footer` owns the popup strip, and `help_box_lines` owns the popup frame.
Headings/titles use `help_heading`, term-style labels use `help_topic`, bounded callouts use `help_attention`, and any future stronger urgency tier can use `help_alert`, so help pages stay readable without hard-coded colors.

#### Editing path

Use `F10` to open the theme or config editing path.
Keep high-frequency navigation surfaces readable first: selection, picker, footer, and help.

## topic:dir

```ytnova-help-meta
title: Directory Help
contexts: main.dir
```

### Contextual F1

Directory mode is the logged tree view.
It owns directory navigation, tree expansion, and directory-scoped commands.

### Long form

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

## topic:file

```ytnova-help-meta
title: File Help
contexts: main.file
```

### Contextual F1

File mode is the main file-list view.
It owns file navigation, file-scoped commands, tagged actions, and export entry points.

### Long form

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

## topic:archive-dir

```ytnova-help-meta
title: Archive Directory Help
contexts: main.archive-dir
```

### Contextual F1

Archive-Dir mode is the tree-style view inside a logged archive.
It mirrors directory work where the archive format permits it.

### Long form

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

## topic:archive-file

```ytnova-help-meta
title: Archive File Help
contexts: main.archive-file
```

### Contextual F1

Archive-File mode is the file-list view for archive-backed content.
Some filesystem commands are unavailable or become archive-aware here.

### Long form

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

## topic:filter

```ytnova-help-meta
title: Filter Help
contexts: prompt.filter,prompt.filter-tagged
```

### Contextual F1

Filters apply glob, exclusion, attribute, date, and size selectors to the current file-list family.
The prompt starts with `*`, which means all files.
Terms can be stacked by separating them with commas.

### Long form

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

## topic:compare

```ytnova-help-meta
title: Compare Help
contexts: none
```

### Contextual F1

Compare covers diff-style viewing, target selection, scope selection, basis selection, and result handling.
Use the related compare topics for the prompt-by-prompt details.

### Long form

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

## topic:compare-target

```ytnova-help-meta
title: Compare Target Help
contexts: prompt.compare-target
```

### Contextual F1

The compare target prompt selects the other file, directory, panel, or external viewer target.
Available choices depend on the active compare mode.

### Long form

#### Target rules

Enter one path.
The compare scope decides whether that one path is treated as a file target, a directory target, or a logged-tree target.

## topic:change-date

```ytnova-help-meta
title: Date Change Help
contexts: prompt.change-date
```

### Contextual F1

The date prompt accepts `YYYY-MM-DD` and optional `HH:MM[:SS]` time input for attribute edits.
`F3` cycles whether the entered value updates the modified time, accessed time, or both.

### Long form

#### Scope choices

Use `modified` to change only the last-modified timestamp.
Use `accessed` to change only the access timestamp.
Use `both` to write the entered value to both timestamps.

#### Format rules

If you omit the time portion, ytnova keeps the current hour, minute, and second.
Tagged date edits use the same prompt and scope cycle.

## topic:compare-scope

```ytnova-help-meta
title: Compare Scope Help
contexts: none
```

### Contextual F1

The compare scope prompt chooses single-item, tagged-set, current-directory, or wider list-family comparison scope.
The exact options depend on the active surface.

### Long form

#### Scope choices

Use `Directory` for one level.
Use `Logged tree` for the currently logged recursive tree.
Use `External viewer` when you want an external diff tool instead of tagged compare results inside ytnova.

## topic:compare-basis

```ytnova-help-meta
title: Compare Basis Help
contexts: none
```

### Contextual F1

The compare basis prompt chooses the matching criteria used for the current compare run.
Typical bases include name, size, time, and content-oriented comparisons.

### Long form

#### Basis choices

Choose the cheapest basis that answers the question you actually have.
Use `Hash` only when metadata is not trustworthy enough.

## topic:compare-results

```ytnova-help-meta
title: Compare Result Help
contexts: none
```

### Contextual F1

Compare results can be displayed, filtered, and converted into a tagged working set for follow-up commands.
This topic covers the result-handling side of compare.

### Long form

#### Result tagging

The compare command never rewrites files.
It marks the chosen result class on the active/source side so you can inspect, copy, move, or archive that subset next.

## topic:execute-file

```ytnova-help-meta
title: Execute File Help
contexts: prompt.execute-file
```

### Contextual F1

The file execute prompt starts with `{}` for the selected file path. Type the command before it and any following shell syntax after it.

### Long form

#### Placeholder rules

`{}` stands for one selected file path, such as `mv {} /tmp` or `wc {} > count`.
When you use the tagged rerun path, the same command is repeated once per tagged file.

## topic:execute-dir

```ytnova-help-meta
title: Execute Directory Help
contexts: prompt.execute-dir
```

### Contextual F1

The directory execute prompt starts with `{}` for the current directory path. Type the command before it and any following shell syntax after it.

### Long form

#### Placeholder rules

`{}` stands for the current directory path, such as `tar -cf archive.tar {}`.
The tagged rerun path still walks tagged files from the active list, not tagged directories from somewhere else.

## topic:search-tagged

```ytnova-help-meta
title: Search Tagged Help
contexts: prompt.search-tagged
```

### Contextual F1

Search tagged runs a text search over the tagged set and removes tags from non-matching files.
It is a narrowing operation on an existing working set.

### Long form

#### Tagged search rules

Start by tagging a working set.
Then search only that set. The result is another, narrower tagged set because files that do not match lose their tags.

## topic:create-archive

```ytnova-help-meta
title: Create Archive Help
contexts: prompt.create-archive
```

### Contextual F1

Create archive builds a new archive from the tagged set first, or from the current selection when nothing is tagged.
Archive format support depends on the chosen suffix.

### Long form

#### Archive creation rules

Directory selections are archived recursively.
Archive creation picks the tagged set first because tagging is the normal way to build a custom archive batch.

## topic:output

```ytnova-help-meta
title: Output Help
contexts: none
```

### Contextual F1

Output exports one or more files to a destination using raw, framed, or page-break formats.
The related output topics cover format, separator, and destination prompts.

### Long form

#### Output model

`Output` is a batch export flow, not a viewer.
It writes the current file or tagged set as `Raw`, `Framed`, or `Page break` text, or sends the same stream to a printer command.

#### Output order

Choose the destination class first: file path or Hardcopy.
When the destination is a file, `F3` cycles `Raw`, `Framed`, and `Page break` before the final path is accepted.
Framed and page-break output ask for the separator string before returning to the destination prompt.
Hardcopy asks only for the printer command because it always streams raw output.

## topic:output-format

```ytnova-help-meta
title: Output Format Help
contexts: none
```

### Contextual F1

Output format chooses how each exported file is framed in the batch.
Raw, framed, and page-break output serve different downstream readers.

### Long form

#### Format choices

Use `Raw` when another tool will parse the output.
Use `Framed` or `Page break` when a human will read the exported batch.

## topic:output-destination

```ytnova-help-meta
title: Output Destination Help
contexts: prompt.output-destination
```

### Contextual F1

Output destination chooses file output versus Hardcopy first, then collects the final destination value.
For file output, `CWD` is the current working directory for bare filenames.
Press `F3` only on the file destination prompt to cycle `Raw`, `Framed`, and `Page break`.
Framed and page-break output ask for the separator before returning to the file destination prompt.
Hardcopy sends raw output to a shell printer command such as `lpr`, `lp`, or `cat > /dev/lp1`.

### Long form

#### Destination choices

File output writes exported text to a path.
Hardcopy sends raw exported text to the chosen printer command.

## topic:output-separator

```ytnova-help-meta
title: Output Separator Help
contexts: prompt.output-separator
```

### Contextual F1

Output separator appears only when `F3` selects framed or page-break output.
Raw output bypasses this prompt.

### Long form

#### Separator rules

The separator is reused between files for the current framed or page-break export.
It is not appended after the last file.

## topic:showall

```ytnova-help-meta
title: Showall Help
contexts: main.showall
```

### Contextual F1

Showall lists every file inside the current logged volume in one aggregated file list.
It keeps single-volume scope while flattening directory boundaries.

### Long form

#### Showall behavior

* **Scope**: Showall flattens one logged volume into one file list. It never crosses into other logged volume roots.
* **Return path**: `Esc` returns to the directory you came from, and `\` jumps to the owner directory of the selected file within that same volume.
* **List control**: `Sort`, `Filter`, `Jump`, and `Dotfiles` apply to the aggregated Showall result set rather than to each directory separately. The filter prompt still provides the tagged-only toggle on `Tab`.
* **Command family**: Showall reuses the File Mode command surface: `Attributes`, `Copy`, `Delete`, `Edit`, `Filter`, `Hex`, `Invert Tags`, `Compare`, `Volume`, `Log`, `Move`, `New File`, `Pipe`, `Quit`, `Rename`, `Sort`, `Tag`, `Untag`, `View`, `Output`, `Execute`, `Pathcopy`, `Archive`, `Jump`, and `Dotfiles`. The difference is only the flattened single-volume scope.

## topic:global

```ytnova-help-meta
title: Global Help
contexts: main.global
```

### Contextual F1

Global lists files from every logged volume in one aggregated file list.
It keeps multi-volume scope while flattening directory boundaries.

### Long form

#### Global behavior

* **Scope**: Global flattens every logged volume into one file list.
* **Return path**: `Esc` returns to the prior directory surface, and `\` jumps to the owner directory even when it lives under another logged volume root.
* **List control**: `Filter`, `Jump`, `Dotfiles`, and `Sort` operate on the aggregated Global result set. Repeating `G` is a no-op because you are already in Global.
* **Command family**: Global reuses the File Mode command surface: `Attributes`, `Copy`, `Delete`, `Edit`, `Filter`, `Hex`, `Invert Tags`, `Compare`, `Volume`, `Log`, `Move`, `New File`, `Pipe`, `Quit`, `Rename`, `Sort`, `Tag`, `Untag`, `View`, `Output`, `Execute`, `Pathcopy`, `Archive`, `Jump`, and `Dotfiles`. The difference is the multi-volume scope and the cross-volume owner jump.

## topic:f7

```ytnova-help-meta
title: F7 Preview Help
contexts: overlay.f7-dir,overlay.f7-file
```

### Contextual F1

F7 preview overlays file preview controls on top of the underlying file-selection context.
The preview owns scrolling while the underlying selection still owns the file target.

### Long form

#### Preview navigation

* **Two scopes stay active**: the underlying file selection still moves with `Up`, `Down`, `PgUp`, `PgDn`, `Home`, and `End`, while the preview buffer scrolls with `Shift-Up/Shift-Down`, `C-p/C-n`, `Shift-PgUp/Shift-PgDn`, and `Shift-Home/Shift-End`.
* **Leaving preview**: `F7` or `Esc` returns to the suspended directory/file surface without discarding its current selection.
* **Blocked overlays**: `F8` split and `Tab` panel switching are disabled while preview is active.

#### Preview command families

* **File-mode command reuse**: Preview keeps the file-focused command family available: `Attributes`, `Copy`, `Delete`, `Edit`, `Filter`, `Invert Tags`, `Compare`, `Move`, `New File`, `Rename`, `Tag`, `Untag`, `View`, `Output`, `Execute`, `Pathcopy`, `Archive`, `Jump`, and `Dotfiles`.
* **Tagged and bulk behavior**: `C-k` still copies the tagged set, and `C-s` still runs Search Tagged without leaving preview.
* **Applications handoff**: `F9` opens the Applications menu from preview without closing preview first.

## topic:f8

```ytnova-help-meta
title: F8 Split Help
contexts: none
```

### Contextual F1

Split mode keeps two panels active at once, and runtime F1 opens the directory or file split page for the active panel.
Use the split page for the live footer command list and this page for the shared split model.

### Long form

#### Split controls

* **Panel ownership**: Each panel keeps its own selection, tags, logged volume, view bands, and restore state. Split mode changes only which panel is active for the next command.
* **Target defaults**: Copy, move, and compare prompts seed the inactive panel as the default destination or target, but you can still edit that default before the operation runs.
* **Leaving split**: `F8` returns to one-panel mode. `Tab` switches the active panel without merging state between them.

## topic:f8-dir

```ytnova-help-meta
title: F8 Split Help
contexts: overlay.f8-dir
```

### Contextual F1

The split-directory page combines split-only rules with the active directory-footer command list.
It is the runtime F1 page when the split focus is on the tree panel.

### Long form

#### Split controls

* **Panel ownership**: Each panel keeps its own selection, tags, logged volume, view bands, and restore state. Split mode changes only which panel is active for the next command.
* **Target defaults**: Copy, move, and compare prompts seed the inactive panel as the default destination or target, but you can still edit that default before the operation runs.
* **Leaving split**: `F8` returns to one-panel mode. `Tab` switches the active panel without merging state between them.

#### Split directory commands

The active split-directory panel uses the same command families as Directory Mode: `1..9 view`, `Attributes`, `Copy`, `Delete`, `Filter`, `Global`, `Invert Tags`, `Compare`, `Volume`, `Log`, `Makedir`, `New File`, `Pipe`, `Quit`, `Rename`, `Showall`, `Tag`, `Untag`, `MoveDir`, `Output`, `Execute`, `Archive`, `Jump`, and `Dotfiles`.
The difference from one-panel Directory Mode is only panel targeting: prompt defaults point at the inactive panel where that makes sense, and `Filter` still owns the tagged-only toggle on `Tab`.

## topic:f8-file

```ytnova-help-meta
title: F8 Split Help
contexts: overlay.f8-file
```

### Contextual F1

The split-file page combines split-only rules with the active file-footer command list.
It is the runtime F1 page when the split focus is on the file panel.

### Long form

#### Split controls

* **Panel ownership**: Each panel keeps its own selection, tags, logged volume, view bands, and restore state. Split mode changes only which panel is active for the next command.
* **Target defaults**: Copy, move, and compare prompts seed the inactive panel as the default destination or target, but you can still edit that default before the operation runs.
* **Leaving split**: `F8` returns to one-panel mode. `Tab` switches the active panel without merging state between them.

#### Split file commands

The active split-file panel uses the same command families as File Mode: `1..9 view`, `Attributes`, `Copy`, `Delete`, `Edit`, `Filter`, `Hex`, `Invert Tags`, `Compare`, `Volume`, `Log`, `Move`, `New File`, `Pipe`, `Quit`, `Rename`, `Sort`, `Tag`, `Untag`, `View`, `Output`, `Execute`, `Pathcopy`, `Archive`, `Jump`, and `Dotfiles`.
The difference from one-panel File Mode is only panel targeting: prompt defaults point at the inactive panel where that makes sense, and `Filter` still owns the tagged-only toggle on `Tab`.

## topic:history-dialog

```ytnova-help-meta
title: History Help
contexts: dialog.history
```

### Contextual F1

The history dialog reuses earlier prompt entries and supports pinning or deletion.
It is a shared helper surface for prompts that keep history.

### Long form

#### History actions

* **Select entry**: `Up` and `Down` move through the current history list.
* **Scroll long entry**: `Left` and `Right` shift a long history line horizontally.
* **Pin**: `P` keeps an important entry at the top of the current history list.
* **Delete**: `D` removes the selected entry from the current history list.
* **Accept**: `Enter` reuses the selected entry.
* **Cancel**: `Esc` closes the dialog without reusing an entry.

## topic:volume-menu

```ytnova-help-meta
title: Volume Help
contexts: dialog.volume-menu
```

### Contextual F1

The volume menu lists loaded volumes, lets you switch to one, and can release a volume.
Loaded volumes keep independent in-memory state until released or reloaded.

### Long form

#### Volume actions

* **Select volume**: `Up` and `Down` move through the loaded-volume list.
* **Switch volume**: `Enter` activates the selected volume.
* **Keep state**: Selecting the already active volume keeps its current in-memory state.
* **Release volume**: `D` unloads the selected volume unless it is the last remaining one.
* **Cancel**: `Esc` closes the menu.

## topic:applications-menu

```ytnova-help-meta
title: Applications Help
contexts: dialog.applications
```

### Contextual F1

The applications menu lists configured application presets.
Use `Enter` to select the highlighted preset, then ytnova returns immediately while the launched application continues on its own.
Use `E` to edit the applications catalog that backs application presets, and `Esc` to cancel.
Use `{}` for the file or folder currently selected in ytnova and `{input}` for the text you type when the preset asks for extra input.

### Long form

#### Applications actions

* **Select preset**: `Up` and `Down` move through the preset list.
* **Launcher role**: `F9` is the named-preset launcher for repeat-heavy external workflows. It is distinct from ad hoc `eXecute`, which remains the one-off shell prompt with history and terminal-style output.
* **Return rule**: After a preset starts, ytnova returns directly to the working view without a blocking `PRESS ENTER` acknowledgment step.
* **Edit presets**: `E` opens the dedicated applications catalog so application presets can be changed without leaving the chooser family.
* **Selection and working directory**: `{}` inserts the currently selected file or folder. Presets start in that selection's directory context, so scripts without `{}` still run from the place you selected.
* **Prompt text**: `{input}` inserts the extra text you typed for the preset prompt.
* **Starter presets**: The bundled catalog starts with `xdg-open` launchers and includes commented examples for tools such as `mpv` or local helper scripts.
* **Cancel menu**: `Esc` closes the chooser without selecting a preset.

## topic:f2-picker

```ytnova-help-meta
title: F2 Picker Help
contexts: dialog.f2-picker
```

### Contextual F1

The F2 picker browses for a path or preset supported by the active prompt.
It is a prompt helper, not a standalone mode, and it also exposes local volume cycling, path logging, and dotfile toggles without leaving the prompt.

### Long form

#### F2 picker actions

* **Move in the tree**: `Up`/`Down` move the selection, while `Left` and `Right` collapse, expand, or enter subtrees.
* **Cycle loaded volumes**: `<` and `>` rotate through logged volumes in the picker.
* **Log a new path**: `L` logs a new directory or volume without leaving the picker.
* **Toggle dotfiles**: `` ` `` reuses the invoking view's dotfile visibility control inside the picker.
* **Select or cancel**: `Enter` selects the highlighted directory and `Esc` cancels the picker.
