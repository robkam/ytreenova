#!/usr/bin/env python3

"""Generate or verify canonical help outputs from split F1/man sources."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import difflib
from pathlib import Path
import re
import sys
from typing import Iterable


def generated_banner(source_path: str) -> str:
    return (
        f"<!-- Auto-generated from {source_path} by "
        "scripts/generate_help_assets.py; do not edit directly. -->"
    )

MANPAGE_STATIC_HEAD = """# NAME

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
"""

MANPAGE_STATIC_GLOBAL_KEYS = """# KEY BINDINGS

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
*   **\\**: In **Showall**/**Global** file lists, exit that mode and jump to the selected file in its owner directory. In Archive-Dir mode, `\\` jumps to archive root when used below root, and exits to the parent physical directory when used at archive root. In normal filesystem dir/file windows and Archive-File mode, `\\` is a no-op.
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
    *   `0`: Do nothing on filesystem volumes; use `F6` for stats. In archive lists, toggle preloaded per-file Size, Packed, and Ratio details.
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
"""

MANPAGE_STATIC_COMMAND_LINE = """# COMMAND LINE EDITING

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
"""

MANPAGE_STATIC_TAIL = """# CONFIGURATION

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

{authors_line}

# SEE ALSO

**bash**(1), **glob**(7), **grep**(1), **less**(1), **regex**(7), **vi**(1)
"""

MODE_TOPIC_ORDER = [
    ("index", "Help System"),
    ("navigation", "Navigation"),
    ("dir", "Directory Mode"),
    ("file", "File Mode"),
    ("archive-dir", "Archive-Dir Mode"),
    ("archive-file", "Archive-File Mode"),
    ("showall", "Showall Mode"),
    ("global", "Global Mode"),
    ("f7", "File Preview Mode"),
    ("f8", "Split Screen Mode"),
]

KEYBIND_TOPIC_ORDER = [
    ("shared-commands", "Shared Commands"),
    ("dir", "Directory Mode"),
    ("file", "File Mode"),
    ("archive-dir", "Archive-Dir Mode"),
    ("archive-file", "Archive-File Mode"),
]

PROMPT_TOPIC_ORDER = [
    ("filter", "Filter Help"),
    ("output", "Output Help"),
]

SUPPORT_TOPIC_ORDER = [
    ("command-line-editing", "Command-line Editing"),
    ("copy-move-targets", "Copy/Move Targets"),
    ("list-jump", "List Jump"),
    ("vi-keys", "Vi Keys"),
    ("f10", "F10 Config"),
    ("theming", "Theming"),
]


@dataclass(frozen=True)
class HelpLink:
    label: str
    target_topic_id: str


@dataclass(frozen=True)
class LongFormSection:
    title: str
    body: str


@dataclass(frozen=True)
class HelpStrip:
    left_back_label: str
    index_label: str
    index_key: str
    navigation_label: str
    navigation_key: str
    follow_label: str
    quit_label: str


DEFAULT_HELP_STRIP = HelpStrip(
    left_back_label="Left back",
    index_label="Index",
    index_key="I",
    navigation_label="Navigation",
    navigation_key="N",
    follow_label="Right/Enter follow",
    quit_label="Esc/Q quit",
)


@dataclass(frozen=True)
class HelpTopic:
    topic_id: str
    title: str
    contexts: tuple[str, ...]
    contextual_f1: str
    explainer_links: tuple[HelpLink, ...]
    long_form_sections: tuple[LongFormSection, ...]
    help_strip: HelpStrip


class HelpSourceError(ValueError):
    pass


TOPIC_ID_RE = re.compile(r"^[a-z0-9-]+$")
CONTEXTS_RE = re.compile(r"^[a-z0-9.-]+(?:,[a-z0-9.-]+)*$")
LINK_RE = re.compile(r"^- \[([^\]]+)\]\(topic:([a-z0-9-]+)\)$")
HELP_STRIP_RE = re.compile(
    r"^```ytnova-help-strip\n(?P<body>.*?)^```$", re.MULTILINE | re.DOTALL
)
HELP_STRIP_FIELDS = (
    "left-back-label",
    "index-label",
    "index-key",
    "navigation-label",
    "navigation-key",
    "follow-label",
    "quit-label",
)


def parse_help_strip(source_text: str, *, required: bool) -> HelpStrip:
    matches = list(HELP_STRIP_RE.finditer(source_text))
    if not matches:
        if required:
            raise HelpSourceError("help source is missing top-level ytnova-help-strip metadata")
        return DEFAULT_HELP_STRIP
    if len(matches) != 1:
        raise HelpSourceError("help source must define exactly one top-level ytnova-help-strip metadata block")

    lines = matches[0].group("body").splitlines()
    fields = []
    for line in lines:
        if ": " not in line:
            raise HelpSourceError("ytnova-help-strip metadata entries must use key: value")
        field, value = line.split(": ", 1)
        if not value:
            raise HelpSourceError(f"ytnova-help-strip field {field!r} is empty")
        fields.append((field, value))
    if tuple(field for field, _ in fields) != HELP_STRIP_FIELDS:
        raise HelpSourceError("ytnova-help-strip metadata keys must be complete and in schema order")

    values = dict(fields)
    for field in ("index-key", "navigation-key"):
        key = values[field]
        if not re.fullmatch(r"[A-Za-z]", key):
            raise HelpSourceError(f"ytnova-help-strip {field} must be one ASCII letter")
    keys = {values["index-key"].upper(), values["navigation-key"].upper()}
    if len(keys) != 2:
        raise HelpSourceError("ytnova-help-strip Index and Navigation keys must be distinct")
    if "Q" in keys:
        raise HelpSourceError("ytnova-help-strip keys must not conflict with Q quit")

    return HelpStrip(
        left_back_label=values["left-back-label"],
        index_label=values["index-label"],
        index_key=values["index-key"],
        navigation_label=values["navigation-label"],
        navigation_key=values["navigation-key"],
        follow_label=values["follow-label"],
        quit_label=values["quit-label"],
    )


def parse_help_source(
    source_text: str, *, require_long_form: bool = False, require_help_strip: bool = False
) -> list[HelpTopic]:
    help_strip = parse_help_strip(source_text, required=require_help_strip)
    lines = source_text.splitlines()
    topic_lines = [idx for idx, line in enumerate(lines) if line.startswith("## topic:")]
    if not topic_lines:
        raise HelpSourceError("canonical help source does not define any topic blocks")

    topics: list[HelpTopic] = []
    seen_topics: set[str] = set()

    for index, start in enumerate(topic_lines):
        end = topic_lines[index + 1] if index + 1 < len(topic_lines) else len(lines)
        block = lines[start:end]
        topic_id = block[0][len("## topic:") :].strip()
        line_no = start + 1
        if not TOPIC_ID_RE.fullmatch(topic_id):
            raise HelpSourceError(f"line {line_no}: invalid topic id {topic_id!r}")
        if topic_id in seen_topics:
            raise HelpSourceError(f"line {line_no}: duplicate topic id {topic_id!r}")
        seen_topics.add(topic_id)

        if len(block) < 6:
            raise HelpSourceError(f"line {line_no}: topic {topic_id!r} is incomplete")

        metadata_start = 1
        while metadata_start < len(block) and not block[metadata_start]:
            metadata_start += 1
        if metadata_start + 4 >= len(block) or block[metadata_start] != "```ytnova-help-meta":
            raise HelpSourceError(
                f"line {line_no + metadata_start}: topic {topic_id!r} must start metadata with ```ytnova-help-meta"
            )
        if not block[metadata_start + 1].startswith("title: "):
            raise HelpSourceError(f"line {line_no + metadata_start + 1}: topic {topic_id!r} is missing title:")
        if not block[metadata_start + 2].startswith("contexts: "):
            raise HelpSourceError(f"line {line_no + metadata_start + 2}: topic {topic_id!r} is missing contexts:")
        if block[metadata_start + 3] != "```":
            raise HelpSourceError(f"line {line_no + metadata_start + 3}: topic {topic_id!r} metadata fence is not closed")

        contextual_heading = metadata_start + 4
        while contextual_heading < len(block) and not block[contextual_heading]:
            contextual_heading += 1
        if contextual_heading >= len(block) or block[contextual_heading] != "### Contextual F1":
            raise HelpSourceError(f"line {line_no + contextual_heading}: topic {topic_id!r} must declare ### Contextual F1")

        title = block[metadata_start + 1][len("title: ") :].strip()
        contexts_raw = block[metadata_start + 2][len("contexts: ") :].strip()
        if not title:
            raise HelpSourceError(f"line {line_no + metadata_start + 1}: topic {topic_id!r} has an empty title")
        if contexts_raw != "none" and not CONTEXTS_RE.fullmatch(contexts_raw):
            raise HelpSourceError(
                f"line {line_no + metadata_start + 2}: topic {topic_id!r} has invalid contexts list {contexts_raw!r}"
            )
        contexts = tuple() if contexts_raw == "none" else tuple(contexts_raw.split(","))
        if len(set(contexts)) != len(contexts):
            raise HelpSourceError(
                f"line {line_no + metadata_start + 2}: topic {topic_id!r} repeats a runtime context in contexts:"
            )

        pos = contextual_heading + 1
        contextual_lines: list[str] = []
        while pos < len(block) and block[pos] not in {"### Explainer links", "### Long form"}:
            contextual_lines.append(block[pos])
            pos += 1
        contextual_f1 = "\n".join(contextual_lines).strip()
        if not contextual_f1:
            raise HelpSourceError(f"line {line_no + contextual_heading}: topic {topic_id!r} has an empty Contextual F1 section")

        links: list[HelpLink] = []
        if pos < len(block) and block[pos] == "### Explainer links":
            pos += 1
            link_lines: list[str] = []
            while pos < len(block) and block[pos] != "### Long form":
                link_lines.append(block[pos])
                pos += 1
            for offset, link_line in enumerate(link_lines, start=1):
                stripped = link_line.strip()
                if not stripped:
                    continue
                match = LINK_RE.fullmatch(stripped)
                if not match:
                    raise HelpSourceError(
                        f"line {line_no + pos - len(link_lines) + offset - 1}: topic {topic_id!r} has invalid explainer link {stripped!r}"
                    )
                links.append(HelpLink(match.group(1), match.group(2)))
            if link_lines:
                contextual_lines.extend(link_lines)
                contextual_f1 = "\n".join(contextual_lines).strip()

        sections: list[LongFormSection] = []
        if pos < len(block) and block[pos] == "### Long form":
            pos += 1
            long_form_lines = block[pos:]
            sections = _parse_long_form_sections(
                topic_id, line_no + pos, long_form_lines
            )
        elif require_long_form:
            raise HelpSourceError(
                f"line {line_no}: topic {topic_id!r} is missing ### Long form"
            )
        topics.append(
            HelpTopic(
                topic_id=topic_id,
                title=title,
                contexts=contexts,
                contextual_f1=contextual_f1,
                explainer_links=tuple(links),
                long_form_sections=tuple(sections),
                help_strip=help_strip,
            )
        )

    known_topics = {topic.topic_id for topic in topics}
    for topic in topics:
        for link in topic.explainer_links:
            if link.target_topic_id not in known_topics:
                raise HelpSourceError(
                    f"topic {topic.topic_id!r} links to unknown topic {link.target_topic_id!r}"
                )

    return topics


def _parse_long_form_sections(topic_id: str, start_line: int, lines: list[str]) -> list[LongFormSection]:
    sections: list[LongFormSection] = []
    current_title: str | None = None
    current_lines: list[str] = []

    for offset, line in enumerate(lines):
        if line.startswith("#### "):
            if current_title is not None:
                body = "\n".join(current_lines).strip()
                if not body:
                    raise HelpSourceError(
                        f"line {start_line + offset - len(current_lines)}: topic {topic_id!r} subsection {current_title!r} is empty"
                    )
                sections.append(LongFormSection(current_title, body))
            current_title = line[len("#### ") :].strip()
            current_lines = []
            if not current_title:
                raise HelpSourceError(f"line {start_line + offset}: topic {topic_id!r} has an empty #### heading")
        else:
            current_lines.append(line)

    if current_title is None:
        raise HelpSourceError(
            f"line {start_line}: topic {topic_id!r} long-form section needs at least one #### subsection"
        )

    body = "\n".join(current_lines).strip()
    if not body:
        raise HelpSourceError(f"topic {topic_id!r} subsection {current_title!r} is empty")
    sections.append(LongFormSection(current_title, body))
    return sections


def render_manpage_markdown(
    topics: list[HelpTopic], *, usage_mode: bool, source_path: str
) -> str:
    topic_map = {topic.topic_id: topic for topic in topics}
    authors_line = (
        "Authors and contributors are listed in the [AUTHORS.md](AUTHORS.md) file."
        if usage_mode
        else "Authors and contributors are listed in the AUTHORS.md file."
    )
    parts = [
        generated_banner(source_path),
        "",
        MANPAGE_STATIC_HEAD.strip(),
        "",
        "# MODES AND NAVIGATION",
        "",
    ]
    for topic_id, heading in MODE_TOPIC_ORDER:
        parts.append(render_contextual_projection(topic_map[topic_id], heading))
    parts.extend([MANPAGE_STATIC_GLOBAL_KEYS.strip(), ""])
    for topic_id, heading in KEYBIND_TOPIC_ORDER:
        parts.append(render_long_form_projection(topic_map[topic_id], heading))
    parts.extend(["# COMPARE", "", render_long_form_projection(topic_map["compare"], topic_map["compare"].title, include_heading=False), ""])
    parts.extend([MANPAGE_STATIC_COMMAND_LINE.strip(), ""])
    for topic_id, heading in PROMPT_TOPIC_ORDER:
        parts.append(render_long_form_projection(topic_map[topic_id], heading))
    parts.extend(["# SUPPORT TOPICS", ""])
    for topic_id, heading in SUPPORT_TOPIC_ORDER:
        parts.append(render_long_form_projection(topic_map[topic_id], heading))
    parts.extend([MANPAGE_STATIC_TAIL.replace("{authors_line}", authors_line).strip(), ""])
    return "\n".join(parts).rstrip() + "\n"


def render_contextual_projection(topic: HelpTopic, heading: str) -> str:
    lines = [f"### {heading}", "", topic.contextual_f1.strip()]
    if topic.explainer_links:
        link_text = ", ".join(link.label for link in topic.explainer_links)
        lines.extend(["", f"See also: {link_text}."])
    return "\n".join(lines)


def render_long_form_projection(topic: HelpTopic, heading: str, *, include_heading: bool = True) -> str:
    lines: list[str] = []
    if include_heading:
        lines.append(f"### {heading}")
        lines.append("")
    for index, section in enumerate(topic.long_form_sections):
        if index:
            lines.append("")
        lines.append(f"#### {section.title}")
        lines.append(section.body.strip())
    return "\n".join(lines)


def render_runtime_header(
    topics: list[HelpTopic],
    *,
    source_path: str,
    locale_topics: list[tuple[str, str, list[HelpTopic]]] | None = None,
) -> str:
    catalogs = [("en", source_path, topics)]
    if locale_topics:
        catalogs.extend(locale_topics)

    banner_sources = [source_path] + [path for _, path, _ in catalogs[1:]]
    lines = [
        "/* Auto-generated from "
        + ", ".join(banner_sources)
        + " by scripts/generate_help_assets.py. */",
        "#include <stddef.h>",
        "",
        "typedef struct {",
        "    const char *label;",
        "    const char *target_topic_id;",
        "} GeneratedHelpLink;",
        "",
        "typedef struct {",
        "    const char *topic_id;",
        "    const char *title;",
        "    const char *contexts_csv;",
        "    const char *contextual_f1;",
        "    size_t explainer_link_count;",
        "    const GeneratedHelpLink *explainer_links;",
        "} GeneratedHelpTopic;",
        "",
        "typedef struct {",
        "    const char *left_back_label;",
        "    const char *index_label;",
        "    const char *index_key;",
        "    const char *navigation_label;",
        "    const char *navigation_key;",
        "    const char *follow_label;",
        "    const char *quit_label;",
        "} GeneratedHelpFooter;",
        "",
        "typedef struct {",
        "    const char *locale_id;",
        "    size_t topic_count;",
        "    const GeneratedHelpTopic *topics;",
        "    GeneratedHelpFooter footer;",
        "} GeneratedHelpCatalog;",
        "",
    ]

    for locale_id, _, locale_catalog_topics in catalogs:
        locale_suffix = locale_id.replace("-", "_")
        for topic in locale_catalog_topics:
            stem = f"{locale_suffix}_{topic.topic_id.replace('-', '_')}"
            if topic.explainer_links:
                lines.append(
                    f"static const GeneratedHelpLink generated_help_links_{stem}[] = {{"
                )
                for link in topic.explainer_links:
                    lines.append(
                        f"    {{{c_literal(link.label)}, {c_literal(link.target_topic_id)}}},"
                    )
                lines.append("};")
                lines.append("")
        lines.append(f"static const GeneratedHelpTopic generated_help_topics_{locale_suffix}[] = {{")
        for topic in locale_catalog_topics:
            stem = f"{locale_suffix}_{topic.topic_id.replace('-', '_')}"
            contexts_csv = ",".join(topic.contexts)
            link_array = (
                f"generated_help_links_{stem}" if topic.explainer_links else "NULL"
            )
            lines.append("    {")
            lines.append(f"        {c_literal(topic.topic_id)},")
            lines.append(f"        {c_literal(topic.title)},")
            lines.append(
                f"        {c_literal(contexts_csv) if contexts_csv else 'NULL'},"
            )
            lines.append(f"        {c_literal(topic.contextual_f1)},")
            lines.append(f"        {len(topic.explainer_links)},")
            lines.append(f"        {link_array},")
            lines.append("    },")
        lines.append("};")
        lines.append("")
        lines.append(
            f"static const size_t generated_help_topic_count_{locale_suffix} = {len(locale_catalog_topics)};"
        )
        lines.append("")

    lines.append("static const GeneratedHelpCatalog generated_help_catalogs[] = {")
    for locale_id, _, locale_catalog_topics in catalogs:
        locale_suffix = locale_id.replace("-", "_")
        help_strip = locale_catalog_topics[0].help_strip
        lines.append("    {")
        lines.append(f"        {c_literal(locale_id)},")
        lines.append(f"        {len(locale_catalog_topics)},")
        lines.append(f"        generated_help_topics_{locale_suffix},")
        lines.append("        {")
        lines.append(f"            {c_literal(help_strip.left_back_label)},")
        lines.append(f"            {c_literal(help_strip.index_label)},")
        lines.append(f"            {c_literal(help_strip.index_key)},")
        lines.append(f"            {c_literal(help_strip.navigation_label)},")
        lines.append(f"            {c_literal(help_strip.navigation_key)},")
        lines.append(f"            {c_literal(help_strip.follow_label)},")
        lines.append(f"            {c_literal(help_strip.quit_label)},")
        lines.append("        },")
        lines.append("    },")
    lines.append("};")
    lines.append("")
    lines.append(
        "static const size_t generated_help_catalog_count = "
        f"{len(catalogs)};"
    )
    lines.append("")
    return "\n".join(lines)


def render_roff_document(markdown: str, *, version: str, versiondate: str) -> str:
    lines = [f'.TH "YTNOVA" "1" "{escape_roff_text(versiondate)}" "ytnova {escape_roff_text(version)}" "User Commands"']
    paragraph: list[str] = []
    bullet: list[str] = []
    in_code = False

    def strip_bullet_marker(line: str) -> str | None:
        stripped = line.lstrip()
        if len(stripped) >= 2 and stripped[0] in {"*", "-"} and stripped[1].isspace():
            return stripped[2:].lstrip()
        return None

    def flush_paragraph() -> None:
        nonlocal paragraph
        if not paragraph:
            return
        text = " ".join(part.strip() for part in paragraph).strip()
        if text:
            lines.append(".PP")
            lines.append(format_roff_inline(text))
        paragraph = []

    def flush_bullet() -> None:
        nonlocal bullet
        if not bullet:
            return
        text = " ".join(part.strip() for part in bullet).strip()
        if text:
            lines.append('.IP "\\[bu]" 2')
            lines.append(format_roff_inline(text))
        bullet = []

    for raw_line in markdown.splitlines():
        line = raw_line.rstrip()
        if line.startswith("<!--"):
            continue
        if in_code:
            if line.startswith("```"):
                flush_bullet()
                lines.append(".fi")
                in_code = False
            else:
                literal = line.replace("\\", r"\\")
                if literal.startswith((".", "'")):
                    literal = r"\&" + literal
                lines.append(literal)
            continue
        if line.startswith("```"):
            flush_paragraph()
            flush_bullet()
            lines.append(".nf")
            in_code = True
            continue
        if not line.strip():
            flush_paragraph()
            flush_bullet()
            continue
        if line.startswith("# "):
            flush_paragraph()
            flush_bullet()
            lines.append(f'.SH "{escape_roff_text(line[2:].strip())}"')
            continue
        if line.startswith("### "):
            flush_paragraph()
            flush_bullet()
            lines.append(f'.SS "{escape_roff_text(line[4:].strip())}"')
            continue
        if line.startswith("#### "):
            flush_paragraph()
            flush_bullet()
            lines.append(f'.SS "{escape_roff_text(line[5:].strip())}"')
            continue
        stripped = line.lstrip()
        bullet_item = strip_bullet_marker(line)
        if bullet_item is not None:
            flush_paragraph()
            flush_bullet()
            bullet.append(bullet_item)
            continue
        if bullet and line[:1].isspace():
            bullet.append(stripped)
            continue
        flush_bullet()
        paragraph.append(line)

    flush_paragraph()
    flush_bullet()
    if in_code:
        lines.append(".fi")
    return "\n".join(lines).rstrip() + "\n"


def format_roff_inline(text: str) -> str:
    placeholders: list[str] = []

    def stash(replacement: str) -> str:
        token = f"\x00{len(placeholders)}\x00"
        placeholders.append(replacement)
        return token

    text = re.sub(r"\[([^\]]+)\]\(([^)]+)\)", lambda m: f"{m.group(1)} ({m.group(2)})", text)
    text = re.sub(
        r"`([^`]+)`",
        lambda m: stash(r"\fB" + escape_roff_text(m.group(1)) + r"\fR"),
        text,
    )
    text = re.sub(
        r"\*\*([^*]+)\*\*",
        lambda m: stash(r"\fB" + escape_roff_text(m.group(1)) + r"\fR"),
        text,
    )
    text = re.sub(
        r"\*([^*]+)\*",
        lambda m: r"\fI" + escape_roff_text(m.group(1)) + r"\fR",
        text,
    )
    for index, replacement in enumerate(placeholders):
        text = text.replace(f"\x00{index}\x00", replacement)
    text = escape_roff_leading(text)
    return text


def escape_roff_text(text: str) -> str:
    return text.replace("\\", r"\\")


def escape_roff_leading(text: str) -> str:
    return r"\&" + text if text.startswith((".", "'")) else text


def c_literal(text: str) -> str:
    escaped = (
        text.replace("\\", r"\\")
        .replace('"', r'\"')
        .replace("\n", r"\n")
    )
    return f'"{escaped}"'


def validate_topic_inventory(f1_topics: list[HelpTopic]) -> None:
    context_owners: dict[str, str] = {}

    for topic in f1_topics:
        for context_id in topic.contexts:
            owner = context_owners.get(context_id)
            if owner is not None and owner != topic.topic_id:
                raise HelpSourceError(
                    "runtime help context "
                    f"{context_id!r} is owned by multiple F1 topics: "
                    f"{owner!r}, {topic.topic_id!r}"
                )
            context_owners[context_id] = topic.topic_id


def help_locale_id_from_path(path: Path) -> str:
    match = re.search(r"\.([a-z][a-z](?:[-_][A-Za-z0-9]+)?)\.md$", path.name)
    if not match:
        raise HelpSourceError(f"cannot derive help locale id from {path}")
    return match.group(1).replace("_", "-").lower()


def validate_locale_topic_projection(
    master_topics: list[HelpTopic], locale_topics: list[HelpTopic], *, locale_id: str
) -> None:
    master_ids = {topic.topic_id for topic in master_topics}
    locale_ids = {topic.topic_id for topic in locale_topics}

    if master_ids != locale_ids:
        missing = sorted(master_ids - locale_ids)
        extra = sorted(locale_ids - master_ids)
        problems = []
        if missing:
            problems.append(f"missing from locale {locale_id}: {', '.join(missing)}")
        if extra:
            problems.append(f"extra in locale {locale_id}: {', '.join(extra)}")
        raise HelpSourceError(
            "locale help source diverged from canonical topic inventory: "
            + "; ".join(problems)
        )

    locale_map = {topic.topic_id: topic for topic in locale_topics}
    for master_topic in master_topics:
        locale_topic = locale_map[master_topic.topic_id]
        if locale_topic.contexts != master_topic.contexts:
            raise HelpSourceError(
                f"locale {locale_id} topic {master_topic.topic_id!r} changed contexts ownership"
            )
        master_targets = [link.target_topic_id for link in master_topic.explainer_links]
        locale_targets = [link.target_topic_id for link in locale_topic.explainer_links]
        if sorted(locale_targets) != sorted(master_targets):
            raise HelpSourceError(
                f"locale {locale_id} topic {master_topic.topic_id!r} changed explainer link targets"
            )


def build_outputs(
    *,
    f1_source_path: Path,
    f1_locale_source_paths: list[Path],
    man_source_path: Path,
    man_md: str | None,
    usage_md: str | None,
    runtime_header: str | None,
    man_roff: str | None,
    version: str,
    versiondate: str,
) -> dict[str, str]:
    f1_topics = parse_help_source(
        f1_source_path.read_text(encoding="utf-8"), require_help_strip=True
    )
    man_topics = parse_help_source(
        man_source_path.read_text(encoding="utf-8"), require_long_form=True
    )
    locale_f1_catalogs: list[tuple[str, str, list[HelpTopic]]] = []
    validate_topic_inventory(f1_topics)
    for locale_path in f1_locale_source_paths:
        locale_id = help_locale_id_from_path(locale_path)
        locale_topics = parse_help_source(
            locale_path.read_text(encoding="utf-8"), require_help_strip=True
        )
        validate_locale_topic_projection(f1_topics, locale_topics, locale_id=locale_id)
        locale_f1_catalogs.append((locale_id, str(locale_path), locale_topics))
    outputs: dict[str, str] = {}
    if man_md:
        outputs[man_md] = render_manpage_markdown(
            man_topics, usage_mode=False, source_path=str(man_source_path)
        )
    if usage_md:
        outputs[usage_md] = render_manpage_markdown(
            man_topics, usage_mode=True, source_path=str(man_source_path)
        )
    if runtime_header:
        outputs[runtime_header] = render_runtime_header(
            f1_topics,
            source_path=str(f1_source_path),
            locale_topics=locale_f1_catalogs,
        )
    if man_roff:
        man_markdown = outputs.get(man_md) or render_manpage_markdown(
            man_topics, usage_mode=False, source_path=str(man_source_path)
        )
        outputs[man_roff] = render_roff_document(
            man_markdown, version=version, versiondate=versiondate
        )
    return outputs


def verify_outputs(outputs: dict[str, str]) -> int:
    status = 0
    for output_path, generated in outputs.items():
        path = Path(output_path)
        current = path.read_text(encoding="utf-8") if path.exists() else ""
        if current == generated:
            continue
        diff = difflib.unified_diff(
            current.splitlines(),
            generated.splitlines(),
            fromfile=str(path),
            tofile="generated",
            lineterm="",
        )
        sys.stderr.write(f"help asset drift detected for {path}\n")
        sys.stderr.write("\n".join(diff))
        sys.stderr.write("\n")
        status = 1
    return status


def write_outputs(outputs: dict[str, str]) -> int:
    for output_path, generated in outputs.items():
        path = Path(output_path)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(generated, encoding="utf-8")
    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate or verify canonical help outputs from etc/help/f1.en.md and etc/help/man.en.md.",
    )
    parser.add_argument(
        "--f1-source",
        default="etc/help/f1.en.md",
        help="Authored contextual F1 help source to read.",
    )
    parser.add_argument(
        "--f1-locale-source",
        action="append",
        default=[],
        help="Additional localized contextual F1 help source to embed in the runtime header.",
    )
    parser.add_argument(
        "--man-source",
        default="etc/help/man.en.md",
        help="Authored man/USAGE reference source to read.",
    )
    parser.add_argument("--man-md", help="Tracked long-form markdown manpage projection path.")
    parser.add_argument("--usage-md", help="Tracked docs/USAGE.md projection path.")
    parser.add_argument("--runtime-header", help="Generated runtime help header path.")
    parser.add_argument("--man-roff", help="Generated roff manpage output path.")
    parser.add_argument("--version", default="", help="Version string for roff projection.")
    parser.add_argument("--versiondate", default="", help="Version date for roff projection.")
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--write", action="store_true", help="Write generated outputs.")
    mode.add_argument("--check", action="store_true", help="Verify generated outputs match the checked-in copies.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    outputs = build_outputs(
        f1_source_path=Path(args.f1_source),
        f1_locale_source_paths=[Path(path) for path in args.f1_locale_source],
        man_source_path=Path(args.man_source),
        man_md=args.man_md,
        usage_md=args.usage_md,
        runtime_header=args.runtime_header,
        man_roff=args.man_roff,
        version=args.version,
        versiondate=args.versiondate,
    )
    if not outputs:
        raise HelpSourceError("no output paths were provided")
    if args.check:
        return verify_outputs(outputs)
    return write_outputs(outputs)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except HelpSourceError as exc:
        sys.stderr.write(f"help generation failed: {exc}\n")
        raise SystemExit(1)
