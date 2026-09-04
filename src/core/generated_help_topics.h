/* Auto-generated from etc/help/f1.en.md, etc/help/f1.de.md by scripts/generate_help_assets.py. */
#include <stddef.h>

typedef struct {
    const char *label;
    const char *target_topic_id;
} GeneratedHelpLink;

typedef struct {
    const char *topic_id;
    const char *title;
    const char *contexts_csv;
    const char *contextual_f1;
    size_t explainer_link_count;
    const GeneratedHelpLink *explainer_links;
} GeneratedHelpTopic;

typedef struct {
    const char *left_back_label;
    const char *index_label;
    const char *index_key;
    const char *navigation_label;
    const char *navigation_key;
    const char *follow_label;
    const char *quit_label;
} GeneratedHelpFooter;

typedef struct {
    const char *locale_id;
    size_t topic_count;
    const GeneratedHelpTopic *topics;
    GeneratedHelpFooter footer;
} GeneratedHelpCatalog;

static const GeneratedHelpLink generated_help_links_en_index[] = {
    {"Applications", "applications-menu"},
    {"Archive Directory", "archive-dir"},
    {"Archive File", "archive-file"},
    {"Command-line Editing", "command-line-editing"},
    {"Command-line Parameters", "command-line-parameters"},
    {"Compare", "compare"},
    {"Compare Basis", "compare-basis"},
    {"Compare Result", "compare-results"},
    {"Compare Scope", "compare-scope"},
    {"Compare Target", "compare-target"},
    {"Configuration Files", "configuration-files"},
    {"Copy/Move Targets", "copy-move-targets"},
    {"Create Archive", "create-archive"},
    {"Date Change", "change-date"},
    {"Directory", "directory"},
    {"F10 Config", "f10"},
    {"F2 Picker", "f2-picker"},
    {"F7 Preview", "f7"},
    {"F8 Split", "f8"},
    {"F8 Split Directory", "f8-dir"},
    {"F8 Split File", "f8-file"},
    {"File", "file"},
    {"Filter", "filter"},
    {"Global", "global"},
    {"History", "history-dialog"},
    {"List Jump", "list-jump"},
    {"Navigation", "ytnova-navigation"},
    {"Output", "output"},
    {"Output Destination", "output-destination"},
    {"Output Format", "output-format"},
    {"Output Separator", "output-separator"},
    {"Search Tagged", "search-tagged"},
    {"Shared Commands", "shared-commands"},
    {"Showall", "showall"},
    {"Tagged", "tagged"},
    {"Tagged Viewer", "tagged-viewer"},
    {"Theming", "theming"},
    {"Vi Keys", "vi-keys"},
    {"Volume", "volume-menu"},
};

static const GeneratedHelpLink generated_help_links_en_list_jump[] = {
    {"Directory mode", "directory"},
    {"File mode", "file"},
    {"Showall", "showall"},
    {"Global", "global"},
};

static const GeneratedHelpLink generated_help_links_en_shared_commands[] = {
    {"F7 preview", "f7"},
    {"F8 split", "f8"},
    {"Applications menu", "applications-menu"},
    {"F10 config", "f10"},
};

static const GeneratedHelpLink generated_help_links_en_command_line_editing[] = {
    {"Vi Keys", "vi-keys"},
    {"History", "history-dialog"},
    {"F2 Picker", "f2-picker"},
};

static const GeneratedHelpLink generated_help_links_en_command_line_parameters[] = {
    {"Configuration Files", "configuration-files"},
    {"Filter", "filter"},
};

static const GeneratedHelpLink generated_help_links_en_configuration_files[] = {
    {"F10 Config", "f10"},
    {"Theming", "theming"},
    {"Command-line Parameters", "command-line-parameters"},
};

static const GeneratedHelpLink generated_help_links_en_vi_keys[] = {
    {"Navigation", "ytnova-navigation"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLink generated_help_links_en_f10[] = {
    {"Theming", "theming"},
    {"Shared commands", "shared-commands"},
};

static const GeneratedHelpLink generated_help_links_en_theming[] = {
    {"F10 config", "f10"},
};

static const GeneratedHelpLink generated_help_links_en_archive_dir[] = {
    {"Navigation", "ytnova-navigation"},
    {"Filter", "filter"},
    {"Compare", "compare"},
};

static const GeneratedHelpLink generated_help_links_en_archive_file[] = {
    {"Navigation", "ytnova-navigation"},
    {"Tagged", "tagged"},
    {"Copy/Move Targets", "copy-move-targets"},
    {"Filter", "filter"},
    {"Compare", "compare"},
    {"Output", "output"},
};

static const GeneratedHelpLink generated_help_links_en_filter[] = {
    {"Tagged", "tagged"},
    {"Showall", "showall"},
    {"Global", "global"},
    {"Command-line Editing", "command-line-editing"},
};

static const GeneratedHelpLink generated_help_links_en_compare[] = {
    {"File", "file"},
    {"Directory", "directory"},
    {"Navigation", "ytnova-navigation"},
};

static const GeneratedHelpLink generated_help_links_en_compare_target[] = {
    {"Compare", "compare"},
    {"Command-line Editing", "command-line-editing"},
};

static const GeneratedHelpLink generated_help_links_en_change_date[] = {
    {"File mode", "file"},
    {"Directory mode", "directory"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLink generated_help_links_en_compare_scope[] = {
    {"Compare", "compare"},
};

static const GeneratedHelpLink generated_help_links_en_compare_basis[] = {
    {"Compare", "compare"},
};

static const GeneratedHelpLink generated_help_links_en_compare_results[] = {
    {"Compare", "compare"},
};

static const GeneratedHelpLink generated_help_links_en_execute_file[] = {
    {"File mode", "file"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLink generated_help_links_en_execute_dir[] = {
    {"Directory mode", "directory"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLink generated_help_links_en_search_tagged[] = {
    {"Tagged", "tagged"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLink generated_help_links_en_create_archive[] = {
    {"Tagged", "tagged"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLink generated_help_links_en_output[] = {
    {"File mode", "file"},
    {"Archive file", "archive-file"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLink generated_help_links_en_output_format[] = {
    {"Output Help", "output"},
};

static const GeneratedHelpLink generated_help_links_en_output_destination[] = {
    {"Output Help", "output"},
    {"Output Format Help", "output-format"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLink generated_help_links_en_output_separator[] = {
    {"Output Help", "output"},
};

static const GeneratedHelpLink generated_help_links_en_showall[] = {
    {"Navigation", "ytnova-navigation"},
    {"Tagged", "tagged"},
    {"Copy", "copy-move-targets"},
    {"Filter", "filter"},
    {"Compare", "compare"},
    {"Move", "copy-move-targets"},
    {"Output", "output"},
};

static const GeneratedHelpLink generated_help_links_en_global[] = {
    {"Navigation", "ytnova-navigation"},
    {"Tagged", "tagged"},
    {"Copy", "copy-move-targets"},
    {"Filter", "filter"},
    {"Compare", "compare"},
    {"Move", "copy-move-targets"},
    {"Output", "output"},
};

static const GeneratedHelpLink generated_help_links_en_f7[] = {
    {"Navigation", "ytnova-navigation"},
    {"Tagged", "tagged"},
    {"Copy/Move Targets", "copy-move-targets"},
    {"Filter", "filter"},
    {"Compare", "compare"},
    {"Applications", "applications-menu"},
    {"Output", "output"},
};

static const GeneratedHelpLink generated_help_links_en_f8[] = {
    {"Navigation", "ytnova-navigation"},
    {"F8 Split Directory", "f8-dir"},
    {"F8 Split File", "f8-file"},
};

static const GeneratedHelpLink generated_help_links_en_f8_dir[] = {
    {"Navigation", "ytnova-navigation"},
    {"F8 Split", "f8"},
    {"Copy", "copy-move-targets"},
    {"Filter", "filter"},
    {"J compare", "compare"},
    {"moVedir", "copy-move-targets"},
    {"Output", "output"},
};

static const GeneratedHelpLink generated_help_links_en_f8_file[] = {
    {"Navigation", "ytnova-navigation"},
    {"F8 Split", "f8"},
    {"Tagged", "tagged"},
    {"C/^Copy", "copy-move-targets"},
    {"Filter", "filter"},
    {"J compare", "compare"},
    {"M/^N move", "copy-move-targets"},
    {"Output", "output"},
};

static const GeneratedHelpLink generated_help_links_en_history_dialog[] = {
    {"Navigation", "ytnova-navigation"},
};

static const GeneratedHelpLink generated_help_links_en_volume_menu[] = {
    {"Navigation", "ytnova-navigation"},
};

static const GeneratedHelpLink generated_help_links_en_applications_menu[] = {
    {"Navigation", "ytnova-navigation"},
};

static const GeneratedHelpLink generated_help_links_en_f2_picker[] = {
    {"Navigation", "ytnova-navigation"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpTopic generated_help_topics_en[] = {
    {
        "index",
        "Help Index",
        NULL,
        "Browse this index when you know the question but not the page.\nPress `Enter` or `Right` on a topic to open it.\nUse `Left` to come back here.\nUse `Esc` to leave help.\n\n`Purpose`\nUse `F1` for the task in front of you, not as one giant manual.\nLocal pages answer the active question first.\nShared topics hold the repeated rules.\n\n`Index use`\nThis list stays alphabetical so you can scan it quickly.\nStart with the current screen when you know it; otherwise pick the topic that matches your question.\nOpen a topic, read the short answer, then use `Left` to come back without losing your place.\n\n\n- [Applications](topic:applications-menu)\n- [Archive Directory](topic:archive-dir)\n- [Archive File](topic:archive-file)\n- [Command-line Editing](topic:command-line-editing)\n- [Command-line Parameters](topic:command-line-parameters)\n- [Compare](topic:compare)\n- [Compare Basis](topic:compare-basis)\n- [Compare Result](topic:compare-results)\n- [Compare Scope](topic:compare-scope)\n- [Compare Target](topic:compare-target)\n- [Configuration Files](topic:configuration-files)\n- [Copy/Move Targets](topic:copy-move-targets)\n- [Create Archive](topic:create-archive)\n- [Date Change](topic:change-date)\n- [Directory](topic:directory)\n- [F10 Config](topic:f10)\n- [F2 Picker](topic:f2-picker)\n- [F7 Preview](topic:f7)\n- [F8 Split](topic:f8)\n- [F8 Split Directory](topic:f8-dir)\n- [F8 Split File](topic:f8-file)\n- [File](topic:file)\n- [Filter](topic:filter)\n- [Global](topic:global)\n- [History](topic:history-dialog)\n- [List Jump](topic:list-jump)\n- [Navigation](topic:ytnova-navigation)\n- [Output](topic:output)\n- [Output Destination](topic:output-destination)\n- [Output Format](topic:output-format)\n- [Output Separator](topic:output-separator)\n- [Search Tagged](topic:search-tagged)\n- [Shared Commands](topic:shared-commands)\n- [Showall](topic:showall)\n- [Tagged](topic:tagged)\n- [Tagged Viewer](topic:tagged-viewer)\n- [Theming](topic:theming)\n- [Vi Keys](topic:vi-keys)\n- [Volume](topic:volume-menu)",
        39,
        generated_help_links_en_index,
    },
    {
        "f1-navigation",
        "Help Navigation",
        NULL,
        "Use the `Up` and `Down` arrow keys to scroll help one line at a time.\nUse the `Up` and `Down` arrow keys to move between links.\n`Page Up` and `Page Down` move one help screen at a time.\n`Home` and `End` go to the top and bottom.\n`Enter` or the `Right` arrow key opens the selected link.\nThe `Left` arrow key returns to the previous help page.\nPress `I` to go to `Help Index`.\n`Esc` or `Q` closes help.",
        0,
        NULL,
    },
    {
        "ytnova-navigation",
        "YtreeNova Navigation",
        NULL,
        "YtreeNova is built for keyboard use. Mouse effects may occur, but they are incidental rather than designed controls.\nUse the `Up` and `Down` arrow keys to move one row at a time.\n`Page Up` and `Page Down` move one screen at a time.\n`Home` and `End` go to the first and last visible rows.\n`Enter` opens the selected item.\n`Right` opens or expands the selected item where that view supports it.\n`Left` goes back or collapses the selected item where that view supports it.\n[/ jump](topic:list-jump) moves to a matching name as you type in the current list. `Enter` selects it; `Esc` cancels.\n`Tab` moves to the next visible sibling in the tree and wraps at the end; `Shift-Tab` reverses it.\n[F8 split](topic:f8) mode: `Tab` switches active panels; some prompts give `Tab` a local meaning.\n[F7 preview](topic:f7) mode: ordinary navigation keys move the file selection; their shifted forms scroll the preview.\n\n`Terminal input limits`\n`Alt` is deliberately unsupported because terminals handle it inconsistently.\n`C-m` and `C-j` are Enter.\n`C-i` is Tab.\n`C-[` is Esc.",
        0,
        NULL,
    },
    {
        "list-jump",
        "List Jump",
        NULL,
        "Press `/`, type letters, and press `Enter` to land on the best visible match in the current list.\nIn tree views, repeat the jump in the next directory if you want to go deeper.\n\n`Jump model`\n`/` opens a live name-jump prompt for the current list only.\nTree and directory views jump among visible directory names.\nFile-oriented views jump among the visible file rows for that surface.\n\n`Accept or cancel`\n`Type letters`: Move to the best current match as you type.\n`Enter`: Land on the current match and stay there.\n`Esc`: Cancel the jump and restore the original selection.\n`Scope changes`: Filtering, Showall, Global, archives, and split mode all change which visible list `/` searches.\n\n\n- [Directory mode](topic:directory)\n- [File mode](topic:file)\n- [Showall](topic:showall)\n- [Global](topic:global)",
        4,
        generated_help_links_en_list_jump,
    },
    {
        "shared-commands",
        "Shared Commands",
        NULL,
        "These keys keep the same high-level meaning across more than one mode.\nUse the mode page for local commands, and use this page for the shared function-key family.\n\n`Shared function keys`\n`F1`: Open contextual help for the active surface.\n`F5`: Refresh the current view.\n`F6`: Toggle the statistics strip for the active panel.\nSplit panels keep independent visibility.\n`F7`: Toggle preview for the active file context.\n`F8`: Toggle split-screen mode.\n`F9`: Open the Applications menu.\n`F10`: Open the configuration command surface.\n`Esc`: Back out of the current overlay, prompt, or popup.\n\n\n- [F7 preview](topic:f7)\n- [F8 split](topic:f8)\n- [Applications menu](topic:applications-menu)\n- [F10 config](topic:f10)",
        4,
        generated_help_links_en_shared_commands,
    },
    {
        "tagged",
        "Tagged",
        NULL,
        "Tags select several files, then apply one operation to the group.\nTagged files are indicated by `*` in the margin.\n\nIn a file list, press `T` to tag the selected file or `U` to untag it. The selection moves down to the next file.\nIn a directory tree, `T` and `U` affect files that the current filter allows in the selected directory, then move down to the next directory.\n\n`i` reverses tags on files that the current filter allows in the current scope.\n\nWhen statistics are shown, tagged-file totals appear there.\n\nHold the Control key with the letter after `C-` to run the matching operation on all tagged files. `^` in a footer label means the same tagged operation, as in `C/^Copy`.\n\n`C-a` opens the attributes prompt.\n`C-c` opens the [copy](topic:copy-move-targets) prompt.\n`C-d` asks before deleting tagged files.\n`C-n` opens the [move](topic:copy-move-targets) prompt because `C-m` is Enter in terminals.\n`C-o` opens the [output](topic:output) prompt.\n`C-p` opens the pipe prompt.\n`C-r` opens the rename prompt.\n`C-s` [searches tagged files](topic:search-tagged) and untags files without a hit.\n`C-t` tags all files in the active file list. In a directory tree, it tags files allowed by the current filter in all logged directories.\n`C-u` untags all files in the active file list. In a directory tree, it untags files allowed by the current filter in all logged directories.\n`C-v` views tagged files one after another.\n`C-x` opens a [command prompt](topic:execute-file) and runs the operation once per tagged file.\n`C-y` [pathcopies](topic:copy-move-targets) the tagged files.\n`C-z` depending on where the selection is opens the [archive Directory](topic:archive-dir) or [archive file](topic:archive-file) prompt\n\nFor a tagged-only [filter](topic:filter) scope, press `F`, then `Tab`. This does not change tags.",
        0,
        NULL,
    },
    {
        "tagged-viewer",
        "Tagged Viewer",
        "viewer.tagged",
        "`View tagged` opens the tagged search results in the internal viewer.\nUse `n` and `p` to move to the next or previous tagged file.\nUse `Space`, `PgDn`, and `PgUp` only to move by pages in the current file.\nAfter `C-s` searches the tagged set, use `/` for the next hit and `?` for the previous hit in the current file.\n`C-s` remains the tagged-list search action outside this viewer.\nWith `TAGGEDVIEWER=external`, the configured pager owns search and hit navigation.",
        0,
        NULL,
    },
    {
        "command-line-editing",
        "Command-line Editing",
        NULL,
        "Most prompts share the same editing keys.\nUse this page for cursor movement, delete keys, history, and picker shortcuts.\n\n`Editing keys`\n`Left/Right`: Move one character.\n`Home/End`: Jump to the start or end.\n`C-a/C-e`: Same as `Home` and `End`.\n`Backspace/C-h`: Delete the character to the left.\n`Delete/C-d`: Delete the character under the cursor.\n`C-w`: Delete the word to the left.\n`C-u`: Delete from the cursor back to the start.\n`C-k`: Delete from the cursor to the end.\n`Enter`: Accept the current value.\n`Esc`: Cancel without committing the prompt.\n\n`Prompt helpers`\n`Up`: Open or cycle prompt history when that prompt keeps history.\n`History dialog`: Use `P` to pin, `D` to delete, `Enter` to reuse, and `Esc` to cancel.\n`F2`: Open a browser or picker when the current prompt supports browsing.\n`F1`: Show the syntax or local rules for the current prompt.\n\n\n- [Vi Keys](topic:vi-keys)\n- [History](topic:history-dialog)\n- [F2 Picker](topic:f2-picker)",
        3,
        generated_help_links_en_command_line_editing,
    },
    {
        "command-line-parameters",
        "Command-line Parameters",
        NULL,
        "Start ytnova with paths to log, or use an option to change startup behaviour.\nUse `ytnova --init` once to create missing configuration files.\nUse `ytnova --version` to print the version and exit.\n\n`Startup options`\n`-d depth`: Set the startup scan depth.\nUse a number, `min` or `root` for 0, or `max` or `all` for 100.\n`-f filter`: Start with a file filter.\nQuote shell patterns such as `\"*.c\"` so the shell does not expand them first.\n`-h history_file`: Use a different command-history file.\n`-p config_file`: Use a different main configuration file instead of `~/.config/ytnova/ytnova.conf`.\n`--init`: Create missing starter configuration files and exit.\n`-v`, `-V`, `--version`: Print version information and exit.\n`Paths`: Give one or more directory or archive paths to log them as startup volumes.\nWith no path, ytnova logs the current directory.\n\n\n- [Configuration Files](topic:configuration-files)\n- [Filter](topic:filter)",
        2,
        generated_help_links_en_command_line_parameters,
    },
    {
        "configuration-files",
        "Configuration Files",
        NULL,
        "Editable user configuration files are normally created under `~/.config/ytnova`.\nAn explicit profile, an existing legacy file, or packaged defaults can be used instead.\n\n`Configuration directory`\n`ytnova.conf`: Main profile settings, including startup scan depth, `VI_KEYS`, and `SEPARATE_DIR_FILE_VIEWS`.\n`commands.conf`: Command labels, key bindings, and command-preset selection.\n`themes.conf`: Theme selection and theme-role overrides.\n`applications.conf`: Application presets opened from `F9`.\n`History`: Command history is separate from these configuration files.\nUse `-h` to choose a different history file.\n\nUse `-p` to choose a different `ytnova.conf`.\nExisting legacy home-dotfiles and packaged defaults remain available when a preferred user file is not used.\n\n\n- [F10 Config](topic:f10)\n- [Theming](topic:theming)\n- [Command-line Parameters](topic:command-line-parameters)",
        3,
        generated_help_links_en_configuration_files,
    },
    {
        "copy-move-targets",
        "Copy/Move Targets",
        NULL,
        "`Copy`, `move`, and `pathcopy` first ask for a name or rename pattern, then for the destination directory.\n\nWith one file, leave its autofilled name to keep it, or edit it to rename it. You can use wildcards in either case.\n\nFor [Tagged](topic:tagged) files, leave the autofilled `*` to keep their names, or enter a rename pattern:\n`*` keeps the remaining original name. Use `copy-*` to add a prefix or `*.bak` to change the extension.\n`?` keeps one character from the original name. For example, `??-*` keeps the first two characters, adds `-`, then keeps the rest.\nOther characters are used as written.\n\nUse `Up` to reuse a previous name or pattern.\n\nEnter the destination directory in `To Directory`. In [F8 split](topic:f8) mode, it starts as the other panel’s currently selected directory.\n\nUse `Up` to reuse a previous destination, or press [F2](topic:f2-picker) to choose one.\n\nThe same prompts apply to [archive files](topic:archive-file) and [archive directories](topic:archive-dir). Copying an archive file extracts it as needed; moving it copies it, then removes the original. You can also copy into a logged archive.\n\n`pathcopy` recreates the selected file’s existing path below the destination directory.\n\nIf the destination directory is missing, ytnova asks whether to create it. If a destination file already exists, ytnova asks before replacing it.",
        0,
        NULL,
    },
    {
        "vi-keys",
        "Vi Keys",
        NULL,
        "With `VI_KEYS=1`, lowercase vi movement keys stay active.\nCommands that would collide move to uppercase or another safe key.\n\n`Navigation remap`\nWith `VI_KEYS=1`, lowercase `h`, `j`, `k`, and `l` become `Left`, `Down`, `Up`, and `Right`.\n`C-u` and `C-d` become page up and page down.\n\n`Command collisions`\nCommands that would steal those lowercase keys move out of the way.\nExamples include `J compare`, `K volume`, `D delete tagged`, and `U untag all` where those actions exist.\n\n\n- [Navigation](topic:ytnova-navigation)\n- [Command-line editing](topic:command-line-editing)",
        2,
        generated_help_links_en_vi_keys,
    },
    {
        "f10",
        "F10 Config Help",
        NULL,
        "Use `F10` for configuration work, not one-off file actions.\nThat is where profile, commands, themes, reload, and similar setup actions belong.\n\n`Config surface`\nUse `F10` when you want to change persistent behavior instead of changing only the current selection.\nProfile settings, command labels, themes, and reload all live here.\n\n`Related files`\n`ytnova.conf` owns profile settings.\n`commands.conf` owns user command labels and bindings.\n`themes.conf` owns theme selection and theme-role overrides.\n\n\n- [Theming](topic:theming)\n- [Shared commands](topic:shared-commands)",
        2,
        generated_help_links_en_f10,
    },
    {
        "theming",
        "Theming",
        NULL,
        "Themes style semantic roles, not one-off screen positions.\nThat keeps help, picker, selection, footer, and warning surfaces readable as one system.\n\n`Theme model`\nThemes set semantic roles such as `footer`, `help`, `help_keybind`, `help_link`, `help_link_selection`, `selection`, `picker`, and `warning`.\n`footer` owns footer-style command strips, while `help` owns the F1 reading body and `help_box_lines` owns the popup frame.\nLinked explainers use `help_link`, and the active linked target uses `help_link_selection`, so help pages stay readable without hard-coded colors.\n\n`Editing path`\nUse `F10` to open the theme or config editing path.\nKeep high-frequency navigation surfaces readable first: selection, picker, footer, and help.\n\n\n- [F10 config](topic:f10)",
        1,
        generated_help_links_en_theming,
    },
    {
        "directory",
        "Directory Help",
        "main.dir",
        "This is the `directory window`. It shows the hierarchical directory tree that ytnova has read in the current `volume`.\nThe selected directory’s files appear in the `small window`.\n\n`+` means ytnova has not yet expanded or logged that directory’s subdirectory branch in the tree.\n\nThe `footer` shows the commands available here.\nSome commands have their own `F1` help.\n\n`Left` collapses the selected branch. If that branch is already hidden, it moves to the directory above.\n`Right` moves into the first shown directory below the selected one. If ytnova has not read the selected directory yet, it reads it first.\n`+` reads the selected directory and adds its branch to the tree without moving the selection.\n`*` reads every directory below the selected directory without moving the selection.\n`-` collapses the selected directory and unlogs its files from the logged tree. They no longer appear in Showall or the statistics.\nPress `Enter` to switch into the selected directory’s `file window`. If its tree branch is not logged yet, ytnova reads it first.\n\nThe number keys change what you see.\n`1` shows names, the default.\n`2` shows attributes.\n`3` shows the owner.\n`4` shows times.\nPress an active `2`, `3`, or `4` again to return to names.\nThese views normally change both this panel’s tree and its `file window`.\nSet `SEPARATE_DIR_FILE_VIEWS=1` in `ytnova.conf` to keep them separate.\n\n`5` turns compact names on or off in the `file window`.\n`6` switches size units in both tree and file rows.\n`7` shows a small text preview for each displayed file.\n`8` shows file details for each displayed file.\n`9` shows Git information for files in a Git project.\n`5`, `7`, `8`, and `9` change only the `file window`.\n\n`Attributes` opens a submenu to alter the selected directory’s attributes.\n[Copy](topic:copy-move-targets) copies the selected directory and its contents.\n`Delete` deletes the selected directory.\n[Filter](topic:filter) changes the files shown in the current view.\n`filter-matching` means files allowed by the current Filter.\n`Global` shows files from every logged volume in one list.\n`Invert` reverses tags on matching files in the selected directory.\n`J` [compare](topic:compare) compares this directory with another.\n`K volume` opens the volume menu.\n`Log` reads the selected directory or archive, or reads a logged directory again.\n`Makedir` creates a directory below the selected directory.\n`Newfile` creates an empty file inside the selected directory.\n[Output](topic:output) exports the current selection.\n`Pipe` runs a command in the selected directory and sends it the displayed file names.\n`Quit` exits ytnova.\n`Rename` renames the selected directory.\n`Showall` shows all files in the current volume in one list.\n`T` and `U` tag and untag all matching files in the selected directory, then move to the next directory.\n`C-t` and `C-u` [tag and untag](topic:tagged) all matching files in every logged directory in the current volume.\n[moVedir](topic:copy-move-targets) moves the selected directory and its contents.\n`eXecute` runs a command for the selected directory.\n`Z archive` creates an archive from the current selection.\n`/ jump` moves to a displayed name as you type.\n`\\` dotfiles` shows or hides hidden names.\n\n`F5` refreshes the active view.\n`F6` shows or hides statistics.\n`F7` turns autoview on or off.\n`F8` turns split-screen on or off.\n`F9` opens the Applications menu.\n`F10` opens configuration.\n`Esc` cancels the current operation.",
        0,
        NULL,
    },
    {
        "file",
        "File Help",
        "main.file",
        "This is the `file window`. The selected row is a `file`.\nCommands in this `footer` act on it unless the line says it uses tagged files.\n\nThe `footer` shows the commands available here.\nSome commands have their own `F1` help.\n\nThe number keys change what you see.\n`1`: Show names.\n`2`: Show attributes, including `name -> target` for symlinks.\n`3`: Show the owner.\n`4`: Show times.\nPress the active `2`, `3`, or `4` again to return to names.\n`5`: Turn `Compact` on or off from the names view.\n`6`: Switch visible file sizes between readable and raw units.\n`7`: Show a small text preview on each visible file row.\n`8`: Show file details on each visible file row.\n`9`: Show the `Git band` when this directory is in a Git worktree.\n\n`A`: Open attributes for the selected file.\n`C-a`: Open attributes for matching [tagged files](topic:tagged).\n`C`: Open [Copy](topic:copy-move-targets) for the selected file.\n`C-c`: Copy matching tagged files.\n`D`: Delete the selected file.\n`C-d`: Delete matching tagged files.\n\n`E`: Edit the selected file in the configured editor.\n`F`: Open [Filter](topic:filter) for this list.\n`H`: Open the selected file in hex view.\n`I`: Reverse tags in the visible list.\n\n`J`: [Compare](topic:compare) the selected file with another file.\n`K`: Open the volume menu.\n`L`: Log a directory or archive without leaving this list.\n`M`: Open [Move](topic:copy-move-targets) for the selected file.\n`C-n`: Move matching tagged files.\n`C-m` is Enter in terminals, so it cannot mean move tagged files.\n`N`: Create a new empty file.\n\n`O`: Open [Output](topic:output) for the selected file.\n`C-o`: Output matching tagged files.\n`P`: Run a command with the selected file’s contents as input.\n`C-p`: Run that command for matching tagged files.\n`Q`: Quit ytnova.\n`R`: Rename the selected file.\n`C-r`: Rename matching tagged files.\n\n`S`: Choose the file-list sort order.\n`C-s`: Search tagged files.\n`T`: [Tag](topic:tagged) the selected file, then move to the next file.\n`C-t`: Tag every visible file.\n`U`: Remove the selected file’s tag, then move to the next file.\n`C-u`: Remove tags from every visible file.\n\n`V`: View the selected file.\n`C-v`: View matching tagged files one after another.\n`X`: Type a shell command. `{}` is the selected file’s path.\n`C-x`: Run that command once for each matching tagged file.\n`Y`: [Pathcopy](topic:copy-move-targets) the selected file and keep its path relative to the current volume root.\n`C-y`: Pathcopy matching tagged files.\n`Z`: Archive tagged files, or the selected file when none are tagged.\n`C-z`: Archive matching tagged files.\n\n`/ jump` moves to a displayed name as you type.\n`\\` dotfiles` shows or hides hidden names.\n\n`F5`: Refresh the active view.\n`F6`: Show or hide the statistics strip.\n`F7`: Turn autoview on or off.\n`F8`: Turn split-screen on or off.\n`F9`: Open the Applications menu.\n`F10`: Open configuration.\n`Esc`: Cancel the current operation.",
        0,
        NULL,
    },
    {
        "archive-dir",
        "Archive Directory Help",
        "main.archive-dir",
        "The archive-directory footer acts inside the current archive location.\nArchive commands follow the usual list keys unless a command says otherwise. Mutating commands appear only when the opened archive and installed libarchive support them; unavailable commands reject clearly if invoked. Common writable formats include `.tar`, `.tar.gz`, `.tar.bz2`, `.tar.xz`, and `.zip`, but availability depends on this archive and the installed libarchive.\n\n`View and scope`\n`1`: Name only.\nThis is the plain default view.\n`2`: Attributes.\n`3`: Owner.\n`4`: Times.\n`Reset`: `1` always returns to the plain Name view.\nIf `2`, `3`, or `4` is already active, pressing that same key again also returns to Name.\n`5`: Turn Compact on or off from the current `1` / Name view only.\n`6`: Switch archive rows between readable and raw size units.\nStats stay readable.\n`7`: Show a small text preview on each visible archive file row.\nIt leaves Compact so you can see the text.\n`8`: Show file detail text on each visible archive file row.\nIt leaves Compact so you can see the summary.\n`9`: Unused in archive lists.\n`0`: Currently unused; does nothing.\n`Delete`: Delete the selected archive directory entry.\n`Filter`: Filter this archive-backed file list.\n`Global`: Mix archive results into the cross-volume file list.\n`J compare`: Compare this archive directory or its logged tree.\n`K volume`: Open the volume menu.\n`Log`: Log another directory or archive.\n`Makedir`: Create a directory where the archive format supports it.\n`Output`: Export the current archive-backed selection.\n`Pipe`: Type a shell command. ytnova sends the visible matching names from the selected archive directory to its standard input, one per line.\n`Quit`: Quit ytnova.\n`Rename`: Rename the selected archive directory entry.\n`Showall`: Open the current archive-wide file list.\n`Tag`: Tag the files in the current virtual directory.\n`Untag`: Remove those tags.\n`\\ root/exit`: Jump to archive root, or leave the archive when you are already there.\n`/ jump`: Press `/`, type letters, and press `Enter` to land on the best visible match.\n`Dotfiles`: Show or hide hidden archive entries when this view exposes them.\n`F1`: Open this help.\n`F5`: Refresh.\n`F6`: Toggle the statistics strip for the active panel.\nSplit panels keep independent visibility.\n`F7`: Toggle preview.\n`F8`: Toggle split.\n`F9`: Open Applications.\n`F10`: Open configuration.\n\n\n- [Navigation](topic:ytnova-navigation)\n- [Filter](topic:filter)\n- [Compare](topic:compare)",
        3,
        generated_help_links_en_archive_dir,
    },
    {
        "archive-file",
        "Archive File Help",
        "main.archive-file",
        "The archive-file footer acts on the selected archive entry or tagged set.\nArchive commands follow the usual list keys unless a command says otherwise. Mutating commands appear only when the opened archive and installed libarchive support them; unavailable commands reject clearly if invoked. Common writable formats include `.tar`, `.tar.gz`, `.tar.bz2`, `.tar.xz`, and `.zip`, but availability depends on this archive and the installed libarchive.\n\n`View and scope`\n`1`: Name only.\nThis is the plain default view.\n`2`: Attributes.\n`3`: Owner.\n`4`: Times.\n`Reset`: `1` always returns to the plain Name view.\nIf `2`, `3`, or `4` is already active, pressing that same key again also returns to Name.\n`5`: Turn Compact on or off from the current `1` / Name view only.\n`6`: Switch archive rows between readable and raw size units.\nStats stay readable.\n`7`: Show a small text preview on each visible archive file row.\nIt leaves Compact so you can see the text.\n`8`: Show file detail text on each visible archive file row.\nIt leaves Compact so you can see the summary.\n`9`: Unused in archive lists.\n`0`: Currently unused; does nothing.\n`C/^Copy`: `C` copies the selected archive entry through archive-aware extract/copy paths.\n`C-c` copies the tagged archive entries.\n`Delete`: Delete the selected archive entry.\n`Filter`: Filter this archive-backed list.\n`C-s` searches only tagged entries and untags non-matches.\n`Hex`: Open the selected archive entry in hex view.\n`Invert`: Flip tags in the visible scope.\n`J compare`: Compare the selected archive entry with another file.\n`K volume`: Open the volume menu.\n`Log`: Log another directory or archive.\n`M/^N move`: `M` moves the selected archive entry.\n`C-n` moves the tagged archive entries.\n`Output`: Export the selected archive entry.\n`Pipe`: Type a shell command and feed it the contents of the selected archive entry on standard input.\n`Quit`: Quit ytnova.\n`Rename`: Rename the selected archive entry.\n`Sort`: Change the archive file-list sort order.\n`Tag`: Tag the selected archive entry.\n`C-t` tags every visible archive row.\n`Untag`: Remove the selected tag.\n`C-u` clears archive tags in this scope.\n`View`: View the selected archive entry.\n`C-v` views the tagged archive entries one after another.\n`eXecute`: Not available in archive file mode.\n`pathcopY`: Copy the selected archive entry while keeping its relative path.\n`/ jump`: Press `/`, type letters, and press `Enter` to land on the best visible match.\n`Dotfiles`: Show or hide hidden archive entries when this view exposes them.\n`F1`: Open this help.\n`F5`: Refresh.\n`F6`: Toggle the statistics strip for the active panel.\nSplit panels keep independent visibility.\n`F7`: Toggle preview.\n`F8`: Toggle split.\n`F9`: Open Applications.\n`F10`: Open configuration.\n\n\n- [Navigation](topic:ytnova-navigation)\n- [Tagged](topic:tagged)\n- [Copy/Move Targets](topic:copy-move-targets)\n- [Filter](topic:filter)\n- [Compare](topic:compare)\n- [Output](topic:output)",
        6,
        generated_help_links_en_archive_file,
    },
    {
        "filter",
        "Filter Help",
        "prompt.filter,prompt.filter-tagged",
        "Type one or more filter terms for the current file list.\n`*` means show everything, `*.c` matches by name, `-*.o` excludes, `:r` and `:x` test readable or executable, and `>2023-01-01` or `>1M` test date or size.\nSeparate terms with commas so they all apply together.\nPress `Tab` to switch between all files and tagged files when that extra scope is available.\n\n`Syntax`\n* `*` — show all files\n* `*.c` — glob match\n* `*.c,*.h` — more than one glob term\n* `-*.o` — exclude matches\n* `:r` — readable files\n* `:x` — executable files\n* `>2023-01-01` — newer than a date\n* `>1M` — larger than a size\n* Combine them, for example `*.c,-*.tmp` or `*.log,>2024-01-01,-debug*`.\n\n`Scope`\nThe filter always applies to the current file-list family.\nThat may be a normal file list, an archive file list, Showall, or Global.\nWhen tagged scope is active, the prompt changes to `FILTER [tagged only]:`.\n\n\n- [Tagged](topic:tagged)\n- [Showall](topic:showall)\n- [Global](topic:global)\n- [Command-line Editing](topic:command-line-editing)",
        4,
        generated_help_links_en_filter,
    },
    {
        "compare",
        "Compare Help",
        NULL,
        "File compare checks the selected file against one target file.\nDirectory compare checks the selected directory against another directory, the logged tree under it, or an external diff tool.\nThe built-in compare does not change files.\nIt tags the results you asked for so you can act on them next.\n\n`Compare flow`\nChoose the target on the compare prompt.\nUse `F3` to choose file, directory, tree, or external compare.\nUse `F4` to choose how ytnova decides whether two files match.\nUse `F5` to choose which result should be tagged after the compare.\nPress `Enter` when the prompt shows the compare plan you want.\n\n`Compare rules`\n`J compare`: The `J` key keeps the old XTree-family compare key.\n`Logged tree`: Uses the part of the tree that is already logged.\nIt does not auto-log unopened `+` subdirectories.\n`FILEDIFF`: May use `%1` and `%2`.\nWhen those placeholders are missing, ytnova appends source and target paths.\n`External compare`: Launches `DIRDIFF` or `TREEDIFF` instead of tagging results inside ytnova.\n`Tagged compare`: There is no separate compare-tagged-files mode.\n\n\n- [File](topic:file)\n- [Directory](topic:directory)\n- [Navigation](topic:ytnova-navigation)",
        3,
        generated_help_links_en_compare,
    },
    {
        "compare-target",
        "Compare Target Help",
        "prompt.compare-target",
        "The current file, directory, or logged tree is the compare source.\nEnter one target path directly.\nUse `F2` to browse.\nUse `Up` for history.\nPress `F3` for Compare Scope: file, directory, tree, or external compare.\nPress `F4` for Compare Basis: `size`, `date`, `size+date`, or `hash`.\nPress `F5` to choose which result gets tagged after the compare.\nIn split view, the inactive panel seeds the default compare target.\n\n`Target rules`\nEnter one path.\n`F3` cycles directory -> logged tree -> external directory -> external tree.\n`F4` cycles `size+date` -> `size` -> `date` -> `hash`.\n`F5` cycles `Different` -> `Match` -> `Newer` -> `Older` -> `Unique` -> `Type mismatch` -> `Error`.\nExternal compare still shows the saved internal choices so you can switch back without losing them.\n\n\n- [Compare](topic:compare)\n- [Command-line Editing](topic:command-line-editing)",
        2,
        generated_help_links_en_compare_target,
    },
    {
        "change-date",
        "Date Change Help",
        "prompt.change-date",
        "Enter the new date as `YYYY-MM-DD` or add a time as `YYYY-MM-DD HH:MM[:SS]`.\nPress `F3` to cycle whether the entered value updates the modified time, accessed time, or both.\nTagged date edits use the same prompt and scope cycle.\n\n`Scope choices`\n`modified` changes only the last-modified timestamp.\n`accessed` changes only the access timestamp.\n`both` writes the entered value to both timestamps.\n\n`Format rules`\nIf you omit the time portion, ytnova keeps the existing hour, minute, and second from the current value.\nUse `Up` for prompt history and `Esc` to cancel without changing either timestamp.\n\n\n- [File mode](topic:file)\n- [Directory mode](topic:directory)\n- [Command-line editing](topic:command-line-editing)",
        3,
        generated_help_links_en_change_date,
    },
    {
        "compare-scope",
        "Compare Scope Help",
        NULL,
        "Directory compares only the current directory.\nLogged tree compares everything already logged under the current directory and never auto-logs unopened branches.\nExternal viewer hands the paths to your configured diff tool instead of tagging results inside ytnova.\n\n`Scope choices`\nUse `Directory` for one level.\nUse `Logged tree` for the currently logged recursive tree.\nUse `External viewer` when you want an external diff tool instead of tagged compare results inside ytnova.\n\n\n- [Compare](topic:compare)",
        1,
        generated_help_links_en_compare_scope,
    },
    {
        "compare-basis",
        "Compare Basis Help",
        NULL,
        "`Size` is the quickest rough check.\n`size+date` is usually better because it also compares last-modified time.\n`Hash` is the strongest check: ytnova reads both files and compares their actual content, so it is slower.\n\n`Basis choices`\nUse `Size` when you only need a quick rough pass.\nUse `size+date` for the normal “are these probably the same?” check.\nUse `Hash` when you need the strongest answer.\nA hash is a fingerprint made from the file contents, so matching hashes mean the content matches exactly.\n\n\n- [Compare](topic:compare)",
        1,
        generated_help_links_en_compare_basis,
    },
    {
        "compare-results",
        "Compare Result Help",
        NULL,
        "Choose which compare result should be tagged after the compare.\n`diFferent` tags mismatches, `Unique` tags files that exist only on the selected side, and the other choices tag only that one result.\n\n`Result tagging`\nThe compare command never rewrites files.\nIt tags the chosen result on the selected side so you can inspect, copy, move, or archive that subset next.\n\n\n- [Compare](topic:compare)",
        1,
        generated_help_links_en_compare_results,
    },
    {
        "execute-file",
        "Execute File Help",
        "prompt.execute-file",
        "The prompt starts with `{}` for the selected file path. Type your command before it.\nKeep `{}` where the selected path belongs; type a destination, redirect, pipe, or other shell syntax after it.\nUse `C-x` to repeat the same command once per tagged file.\n\n`Placeholder rules`\n`{}` stands for one selected file path. For example, use `mv {} /tmp` or `wc {} > count`.\nWhen you use the tagged rerun path, the same command is repeated once per tagged file.\n\n\n- [File mode](topic:file)\n- [Command-line editing](topic:command-line-editing)",
        2,
        generated_help_links_en_execute_file,
    },
    {
        "execute-dir",
        "Execute Directory Help",
        "prompt.execute-dir",
        "The prompt starts with `{}` for the current directory path. Type your command before it.\nKeep `{}` where the selected path belongs; type a destination, redirect, pipe, or other shell syntax after it.\nUse `C-x` to repeat the same command once per tagged file in the active list.\n\n`Placeholder rules`\n`{}` stands for the current directory path. For example, use `tar -cf archive.tar {}`.\nThe tagged rerun path still walks tagged files from the active list, not tagged directories from somewhere else.\n\n\n- [Directory mode](topic:directory)\n- [Command-line editing](topic:command-line-editing)",
        2,
        generated_help_links_en_execute_dir,
    },
    {
        "search-tagged",
        "Search Tagged Help",
        "prompt.search-tagged",
        "Enter plain search text only.\nytnova builds `grep -i -- PATTERN {}` for you.\nOnly tagged files are searched, and non-matches are untagged.\n\n`Tagged search rules`\nStart by tagging a working set.\nThen search only that set.\nThe result is another, narrower tagged set because files that do not match lose their tags.\n\n\n- [Tagged](topic:tagged)\n- [Command-line editing](topic:command-line-editing)",
        2,
        generated_help_links_en_search_tagged,
    },
    {
        "create-archive",
        "Create Archive Help",
        "prompt.create-archive",
        "Use `.tar`, `.tar.gz` or `.tgz`, `.tar.bz2` or `.tbz2`, `.tar.xz` or `.txz`, or `.zip`.\nWhen tags exist, the tagged set wins.\nWhen nothing is tagged, ytnova archives the current file or directory selection.\n\n`Archive creation rules`\nDirectory selections are archived recursively.\nArchive creation picks the tagged set first because tagging is the normal way to build a custom archive batch.\n\n\n- [Tagged](topic:tagged)\n- [Command-line editing](topic:command-line-editing)",
        2,
        generated_help_links_en_create_archive,
    },
    {
        "output",
        "Output Help",
        NULL,
        "Output exports file content to a file path or a printer command.\nChoose file or hardcopy first, then give the final destination.\nOn file output, `F3` cycles `Raw`, `Framed`, and `Page break`.\n`Framed` and `Page break` ask for a separator before the final file path.\n\n`Output model`\n`Output` is an export flow, not an editor.\nIt can write plain content, framed content, or page-break-separated content.\nIt can also send that export to a printer command instead of a file path.\n\n`Prompt order`\nChoose file or hardcopy first.\nOn the file destination prompt, `F3` cycles `Raw`, `Framed`, and `Page break`.\nWhen `Framed` or `Page break` is active, choose the separator before entering the final file path.\nHardcopy asks only for the printer command.\n\n\n- [File mode](topic:file)\n- [Archive file](topic:archive-file)\n- [Command-line editing](topic:command-line-editing)",
        3,
        generated_help_links_en_output,
    },
    {
        "output-format",
        "Output Format Help",
        NULL,
        "`Raw` writes content with no extra framing.\n`Framed` adds per-file headings or footers.\n`Page break` inserts a separator between files and skips a trailing separator at the end.\n\n`Format choices`\nUse `Raw` when another tool will parse the output.\nUse `Framed` or `Page break` when a human will read the exported batch.\n\n\n- [Output Help](topic:output)",
        1,
        generated_help_links_en_output_format,
    },
    {
        "output-destination",
        "Output Destination Help",
        "prompt.output-destination",
        "Choose file or hardcopy first, then enter that destination exactly as ytnova should use it.\nFile output writes exported text to a path.\nBare filenames go to `CWD`, the current working directory.\nHardcopy sends raw exported text to a printer command.\nUse helpers such as `lpr`, `lp`, or `cat > /dev/lp1`.\nPress `F3` on the file destination prompt to cycle `Raw`, `Framed`, and `Page break`.\n`Framed` and `Page break` later ask for a separator.\n\n`Destination choices`\n`File output`: Write exported text to a path.\n`CWD`: Use the current working directory for bare filenames.\n`Hardcopy`: Send raw exported text to a shell printer command such as `lpr`, `lp`, or `cat > /dev/lp1`.\n\n`Format cycle`\n`F3` is available only on the file destination prompt.\nWhen it selects `Framed` or `Page break`, ytnova asks for the separator before returning to the file path prompt.\n\n\n- [Output Help](topic:output)\n- [Output Format Help](topic:output-format)\n- [Command-line editing](topic:command-line-editing)",
        3,
        generated_help_links_en_output_destination,
    },
    {
        "output-separator",
        "Output Separator Help",
        "prompt.output-separator",
        "This prompt appears only when `F3` selects `Framed` or `Page break`.\nLeave it blank to accept the default triple-backtick fence.\nRaw output skips this prompt.\n\n`Separator rules`\nThe separator is reused between files for the current framed or page-break export.\nIt is not appended after the last file.\n\n\n- [Output Help](topic:output)",
        1,
        generated_help_links_en_output_separator,
    },
    {
        "showall",
        "Showall Help",
        "main.showall",
        "Showall gathers every file in the current logged volume.\nIts footer acts on the selected result or tagged set.\n\n`View and scope`\n`Scope`: Showall lists every file inside the current logged volume only.\nIt does not cross into other loaded volumes.\n`Return`: Return to the previously selected directory.\n`Open owner`: Jump to the owner directory of the selected file inside the current logged volume.\n`1`: Name only.\nThis is the plain default view.\n`2`: Attributes.\nIn file lists this also shows `name -> target` for symlinks.\n`3`: Owner.\n`4`: Times.\n`Reset rule`: `1` returns to plain Name.\nPressing the already-active `2`, `3`, or `4` again also drops back to Name.\n`Shared-per-panel rule`: By default, `1..4` are shared inside one panel.\nSet `SEPARATE_DIR_FILE_VIEWS=1` to make Showall/file-window and tree-directory base views independent again.\n`5`: Toggle Compact from the current `1` / Name view only.\n`6`: Switch file rows between human-readable and raw size units.\nStats stay human-readable.\n`7`: Show Mini preview text on each visible file row, and leave Compact so you can see it.\n`8`: Show File detail text on each visible file row, and leave Compact so you can see it.\n`9`: Show the Git band when the current directory is inside a Git worktree.\n`0`: Currently unused; it does nothing.\n`Sort`: Repeating `S` changes sort without leaving Showall.\n`Jump`: Press `/`, type letters, and press `Enter` to land on the best visible match.\n`Dotfiles`: Show or hide hidden files in the current Showall result set.\n\n`Working set`\n`Filter`: Filter the current Showall result set.\n`C-s` searches only tagged files there.\nInside the prompt, `Tab` narrows the same result set to tagged-only.\n`Tag`: Tag the selected file, and `C-t` tags every visible file in the current Showall result set.\n`Untag`: Remove the tag from the selected file, and `C-u` clears tags in the current Showall result set.\n`Invert Tags`: Flip tag state inside the visible Showall result set.\n`Archive`: Archive the tagged set first, or the current selection when nothing is tagged.\n\n`File actions`\n`Attributes`: Open the attributes submenu for the selected file.\n`Copy`: `C` copies the selected file, and `C-c` copies the tagged set through the same prompt.\n`Move`: `M` moves the selected file, and `C-n` moves the tagged set through the same prompt.\n`View`: View the selected file, and `C-v` views the tagged files one after another.\n`Edit`: Open the selected file in the configured editor.\n`Hex`: View the selected file in hex mode.\n`Compare`: Compare the selected file against another file.\n`Output`: Export the selection.\n`C-o` reuses the prompts for the tagged set, and `C-w` stays as a legacy alias.\n`Execute`: Type a shell command.\nUse `{}` where the selected file path should go, leave `{}` unquoted so ytnova can quote it safely, and use `C-x` to repeat the command once per tagged file.\n`Pathcopy`: Copy the selected file while preserving its path relative to the current volume root.\n`Pipe`: Type a shell command and feed it the contents of the selected file on standard input.\n`New File`: Create a new empty file.\n`Rename`: Rename the selected file.\n`Delete`: Delete the selected file.\n`Log`: Log a new directory or archive file without leaving Showall.\n`Volume`: Open the volume picker.\n`Quit`: Quit ytnova.\n\n`Showall function keys`\n`F1`: Open contextual help for the current Showall surface.\n`F5`: Refresh the active panel.\n`F6`: Toggle the statistics strip for the active panel.\nSplit panels keep independent visibility.\n`F7`: Toggle preview for the selected file context.\n`F8`: Toggle split-screen mode.\n`F9`: Open the Applications menu.\n`F10`: Open the configuration command surface.\n\n\n- [Navigation](topic:ytnova-navigation)\n- [Tagged](topic:tagged)\n- [Copy](topic:copy-move-targets)\n- [Filter](topic:filter)\n- [Compare](topic:compare)\n- [Move](topic:copy-move-targets)\n- [Output](topic:output)",
        7,
        generated_help_links_en_showall,
    },
    {
        "global",
        "Global Help",
        "main.global",
        "Global gathers files from every logged volume.\nIts footer acts on the selected result or tagged set.\n\n`View and scope`\n`Scope`: Global lists files from every logged volume.\n`Return`: Return to the previously selected directory.\n`Open owner`: Jump to the owner directory of the selected file even when it lives under another logged volume root.\n`1`: Name only.\nThis is the plain default view.\n`2`: Attributes.\nIn file lists this also shows `name -> target` for symlinks.\n`3`: Owner.\n`4`: Times.\n`Reset rule`: `1` returns to plain Name.\nPressing the already-active `2`, `3`, or `4` again also drops back to Name.\n`Shared-per-panel rule`: By default, `1..4` are shared inside one panel.\nSet `SEPARATE_DIR_FILE_VIEWS=1` to make Global/file-window and tree-directory base views independent again.\n`5`: Toggle Compact from the current `1` / Name view only.\n`6`: Switch file rows between human-readable and raw size units.\nStats stay human-readable.\n`7`: Show Mini preview text on each visible file row, and leave Compact so you can see it.\n`8`: Show File detail text on each visible file row, and leave Compact so you can see it.\n`9`: Show the Git band when the current directory is inside a Git worktree.\n`0`: Currently unused; it does nothing.\n`Sort`: `S` changes sort without leaving Global.\n`Jump`: Press `/`, type letters, and press `Enter` to land on the best visible match.\n`Dotfiles`: Show or hide hidden files in the current Global result set.\n\n`Working set`\n`Filter`: Filter the current Global result set.\n`C-s` searches only tagged files there.\nInside the prompt, `Tab` narrows the same result set to tagged-only.\n`Tag`: Tag the selected file, and `C-t` tags every visible file in the current Global result set.\n`Untag`: Remove the tag from the selected file, and `C-u` clears tags in the current Global result set.\n`Invert Tags`: Flip tag state inside the visible Global result set.\n`Archive`: Archive the tagged set first, or the current selection when nothing is tagged.\n\n`File actions`\n`Attributes`: Open the attributes submenu for the selected file.\n`Copy`: `C` copies the selected file, and `C-c` copies the tagged set through the same prompt.\n`Move`: `M` moves the selected file, and `C-n` moves the tagged set through the same prompt.\n`View`: View the selected file, and `C-v` views the tagged files one after another.\n`Edit`: Open the selected file in the configured editor.\n`Hex`: View the selected file in hex mode.\n`Compare`: Compare the selected file against another file.\n`Output`: Export the selection.\n`C-o` reuses the prompts for the tagged set, and `C-w` stays as a legacy alias.\n`Execute`: Type a shell command.\nUse `{}` where the selected file path should go, leave `{}` unquoted so ytnova can quote it safely, and use `C-x` to repeat the command once per tagged file.\n`Pathcopy`: Copy the selected file while preserving its path relative to the owning volume root.\n`Pipe`: Type a shell command and feed it the contents of the selected file on standard input.\n`New File`: Create a new empty file.\n`Rename`: Rename the selected file.\n`Delete`: Delete the selected file.\n`Log`: Log a new directory or archive file without leaving Global.\n`Volume`: Open the volume picker.\n`Quit`: Quit ytnova.\n\n`Global function keys`\n`F1`: Open contextual help for the current Global surface.\n`F5`: Refresh the active panel.\n`F6`: Toggle the statistics strip for the active panel.\nSplit panels keep independent visibility.\n`F7`: Toggle preview for the selected file context.\n`F8`: Toggle split-screen mode.\n`F9`: Open the Applications menu.\n`F10`: Open the configuration command surface.\n\n\n- [Navigation](topic:ytnova-navigation)\n- [Tagged](topic:tagged)\n- [Copy](topic:copy-move-targets)\n- [Filter](topic:filter)\n- [Compare](topic:compare)\n- [Move](topic:copy-move-targets)\n- [Output](topic:output)",
        7,
        generated_help_links_en_global,
    },
    {
        "f7",
        "F7 Preview Help",
        "overlay.f7-dir,overlay.f7-file",
        "Preview keeps the selected file visible while its footer commands remain available.\nThe preview-only rules below call out the exceptions.\n\n`Preview navigation`\n`Up/Down, PgUp/PgDn, Home/End`: Keep moving the selected file.\n`Shift-Up/Down` or `C-p/C-n`: Scroll preview lines.\n`Shift-PgUp/PgDn`: Scroll preview by pages.\n`Shift-Home/End`: Jump to the start or end of the preview.\n`F7`: Return to the underlying directory or file view.\n`F8`: Split does nothing while preview is active.\n`F9`: Open Applications without leaving preview.\n`Tab`: Do not switch panels while preview is active.\n`Esc`: Leave preview immediately.\n\n`Live commands`\n`Attributes`: Open file attributes.\n`C/^Copy`: `C` copies the selected file.\n`C-c` copies the tagged set.\n`Delete`: Delete the selected file without leaving preview.\n`Edit`: Open the selected file in the configured editor.\n`Filter`: Filter this preview list.\n`C-s` searches only tagged files.\n`Invert`: Flip tags in the visible scope.\n`J compare`: Compare the selected file with another file.\n`M/^N move`: `M` moves the selected file.\n`C-n` moves the tagged set.\n`Newfile`: Create an empty file without leaving preview.\n`Rename`: Rename the selected file without leaving preview.\n`Tag`: Tag the selected file.\n`Untag`: Remove the selected tag.\n`View`: View the selected file.\n`C-v` views the tagged files one after another.\n`Output`: Export the selection.\n`C-o` reuses the prompts for the tagged set.\n`eXecute`: Type a shell command.\nUse `{}` where the selected file path should go, leave `{}` unquoted so ytnova can quote it safely, and use `C-x` to repeat the command once per tagged file.\n`pathcopY`: Copy the selected file while keeping its path relative to the current volume root.\n`Z archive`: Archive the tagged set first, or the current selection when nothing is tagged.\n`/ jump`: Press `/`, type letters, and press `Enter` to land on the best visible match.\n`\\` dotfiles`: Show or hide hidden files in this preview-backed list.\n\n\n- [Navigation](topic:ytnova-navigation)\n- [Tagged](topic:tagged)\n- [Copy/Move Targets](topic:copy-move-targets)\n- [Filter](topic:filter)\n- [Compare](topic:compare)\n- [Applications](topic:applications-menu)\n- [Output](topic:output)",
        7,
        generated_help_links_en_f7,
    },
    {
        "f8",
        "F8 Split Help",
        NULL,
        "Split mode keeps both panels live.\nThe split-only rules below call out the exceptions.\n\n`Split rules`\n`F8`: Return to single-panel mode.\n`Tab`: Switch the active panel and keep the other panel's state intact.\nThis panel switch is only available in F8 split mode; some prompts use Tab for their own choices.\n`Target defaults`: Copy, move, and compare prompts start with the other panel as the default destination or target.\n`Panel independence`: Each panel keeps its own selection, view, tags, volume, and restore state.\n\n\n- [Navigation](topic:ytnova-navigation)\n- [F8 Split Directory](topic:f8-dir)\n- [F8 Split File](topic:f8-file)",
        3,
        generated_help_links_en_f8,
    },
    {
        "f8-dir",
        "F8 Split Directory Help",
        "overlay.f8-dir",
        "This page explains the live split-directory footer commands.\nUse `Tab` to change the active panel, and remember that copy, move, and compare default to the inactive panel.\nEach panel keeps its own selection, view, tags, volume, and restore state.\n\n`Live commands`\n`F8`: Return to single-panel mode.\n`Tab`: Change the active panel.\n`Target defaults`: Copy, move, and compare default to the inactive panel.\n\n`View and scope`\n`1`: Name only.\nThis is the plain default view.\n`2`: Attributes.\nIn file lists this also shows `name -> target` for symlinks.\n`3`: Owner.\n`4`: Times.\n`Reset`: `1` always returns to the plain Name view.\nIf `2`, `3`, or `4` is already active, pressing that same key again also returns to Name.\n`Shared per panel`: By default, `1..4` stay linked inside one panel.\nChanging the tree view also changes that panel's file window.\nSet `SEPARATE_DIR_FILE_VIEWS=1` to split them again.\n`Tree versus file window`: In split directory focus, `5`, `7`, `8`, and `9` do not change the tree rows.\nThey change the active panel's file window.\n`5`: Turn Compact on or off from the current `1` / Name view only.\n`6`: Switch file and directory rows between readable and raw size units.\nStats stay readable.\n`7`: Show a small text preview on each visible file row.\nIt leaves Compact so you can see the text.\n`8`: Show file detail text on each visible file row.\nIt leaves Compact so you can see the summary.\n`9`: Show the Git band when the current directory is inside a Git worktree.\n`0`: Currently unused; does nothing.\n`Attributes`: Open directory attributes.\n`Copy`: Copy the selected directory branch.\nIn split mode the other panel is the default destination.\n`Delete`: Delete the selected directory.\n`Filter`: Filter this file list.\n`Tab` switches between all files and tagged files when tags exist.\n`Global`: Open the cross-volume file list for the active panel.\n`Invert`: Flip tags in the visible scope.\n`J compare`: Compare this directory, its logged tree, or another target.\n`K volume`: Open the volume menu.\n`Log`: Log a directory or archive, or reload a logged path from the top.\n`Makedir`: Create a directory.\n`Newfile`: Create an empty file here.\n`Output`: Export the current selection.\n`Pipe`: Type a shell command. ytnova runs it in the selected directory and sends the visible matching names to its standard input, one per line.\n`Quit`: Quit ytnova.\n`Rename`: Rename the selected directory.\n`Showall`: Open the current-volume file list for the active panel.\n`Tag`: Tag the files under the selected directory.\n`Untag`: Remove those tags.\n`moVedir`: Move the selected directory branch.\nIn split mode the other panel is the default destination.\n`eXecute`: Type a shell command.\nUse `{}` where the selected directory path should go, and leave `{}` unquoted so ytnova can quote the path safely.\n`Z archive`: Archive the tagged set first, or the current selection when nothing is tagged.\n`/ jump`: Press `/`, type letters, and press `Enter` to land on the best visible match in this tree.\n`\\` dotfiles`: Show or hide hidden names.\n`F10`: Open configuration.\n\n\n- [Navigation](topic:ytnova-navigation)\n- [F8 Split](topic:f8)\n- [Copy](topic:copy-move-targets)\n- [Filter](topic:filter)\n- [J compare](topic:compare)\n- [moVedir](topic:copy-move-targets)\n- [Output](topic:output)",
        7,
        generated_help_links_en_f8_dir,
    },
    {
        "f8-file",
        "F8 Split File Help",
        "overlay.f8-file",
        "This page explains the live split-file footer commands.\nUse `Tab` to change the active panel, and remember that copy, move, and compare default to the inactive panel.\nEach panel keeps its own selection, view, tags, volume, and restore state.\n\n`Live commands`\n`F8`: Return to single-panel mode.\n`Tab`: Change the active panel.\n`Target defaults`: Copy, move, and compare default to the inactive panel.\n\n`View and scope`\n`1`: Name only.\nThis is the plain default view.\n`2`: Attributes.\nIn file lists this also shows `name -> target` for symlinks.\n`3`: Owner.\n`4`: Times.\n`Reset`: `1` always returns to the plain Name view.\nIf `2`, `3`, or `4` is already active, pressing that same key again also returns to Name.\n`Shared per panel`: By default, `1..4` stay linked inside one panel.\nChanging the tree view also changes that panel's file window.\nSet `SEPARATE_DIR_FILE_VIEWS=1` to split them again.\n`5`: Turn Compact on or off from the current `1` / Name view only.\n`6`: Switch file and directory rows between readable and raw size units.\nStats stay readable.\n`7`: Show a small text preview on each visible file row.\nIt leaves Compact so you can see the text.\n`8`: Show file detail text on each visible file row.\nIt leaves Compact so you can see the summary.\n`9`: Show the Git band when the current directory is inside a Git worktree.\n`Extra state label`: `5`, `7`, `8`, and `9` do not stack in the stats label.\nIt shows only the one extra state you can currently see.\n`0`: Currently unused; does nothing.\n`Attributes`: Open file attributes.\n`C/^Copy`: `C` copies the selected file.\n`C-c` copies the tagged set.\nIn split mode the other panel is the default destination.\n`Delete`: Delete the selected file.\n`Edit`: Open the selected file in the configured editor.\n`Filter`: Filter this list.\n`C-s` searches only tagged files.\n`Tab` switches between all files and tagged files when tags exist.\n`Hex`: Open the selected file in hex view.\n`Invert`: Flip tags in the visible scope.\n`J compare`: Compare the selected file with another file.\n`K volume`: Open the volume menu.\n`Log`: Log a directory or archive without leaving split file mode.\n`M/^N move`: `M` moves the selected file.\n`C-n` moves the tagged set.\nIn split mode the other panel is the default destination.\n`Newfile`: Create an empty file.\n`Output`: Export the selection.\n`C-o` reuses the prompts for the tagged set.\n`Pipe`: Type a shell command and feed it the contents of the selected file on standard input.\n`Quit`: Quit ytnova.\n`Rename`: Rename the selected file.\n`Sort`: Change the file-list sort order.\n`Tag`: Tag the selected file.\n`C-t` tags every visible file.\n`Untag`: Remove the selected tag.\n`C-u` clears tags in this scope.\n`View`: View the selected file.\n`C-v` views the tagged files one after another.\n`eXecute`: Type a shell command.\nUse `{}` where the selected file path should go, leave `{}` unquoted so ytnova can quote it safely, and use `C-x` to repeat the command once per tagged file.\n`pathcopY`: Copy the selected file while keeping its path relative to the current volume root.\n`Z archive`: Archive the tagged set first, or the current selection when nothing is tagged.\n`/ jump`: Press `/`, type letters, and press `Enter` to land on the best visible match.\n`\\` dotfiles`: Show or hide hidden files.\n`F10`: Open configuration.\n\n\n- [Navigation](topic:ytnova-navigation)\n- [F8 Split](topic:f8)\n- [Tagged](topic:tagged)\n- [C/^Copy](topic:copy-move-targets)\n- [Filter](topic:filter)\n- [J compare](topic:compare)\n- [M/^N move](topic:copy-move-targets)\n- [Output](topic:output)",
        8,
        generated_help_links_en_f8_file,
    },
    {
        "history-dialog",
        "History Help",
        "dialog.history",
        "Use `Up` and `Down` to choose an entry.\nUse `Left` and `Right` to scroll a long entry.\nUse `P` to pin or unpin.\nUse `D` to delete.\nUse `Enter` to accept.\nUse `Esc` to cancel.\n\n`History actions`\n`Select entry`: `Up` and `Down` move through the current history list.\n`Scroll long entry`: `Left` and `Right` shift a long history line horizontally.\n`Pin`: `P` keeps an important entry at the top of the current history list.\n`Delete`: `D` removes the selected entry from the current history list.\n`Accept`: `Enter` reuses the selected entry.\n`Cancel`: `Esc` closes the dialog without reusing an entry.\n\n\n- [Navigation](topic:ytnova-navigation)",
        1,
        generated_help_links_en_history_dialog,
    },
    {
        "volume-menu",
        "Volume Help",
        "dialog.volume-menu",
        "Use `Up` and `Down` to choose a loaded volume.\nUse `Enter` to switch to it.\nUse `D` to release it, unless it is the last one.\nUse `Esc` to leave the menu.\n\n`Volume actions`\n`Select volume`: `Up` and `Down` move through the loaded-volume list.\n`Switch volume`: `Enter` activates the selected volume.\n`Keep state`: Selecting the already active volume keeps its current in-memory state.\n`Release volume`: `D` unloads the selected volume unless it is the last remaining one.\n`Cancel`: `Esc` closes the menu.\n\n\n- [Navigation](topic:ytnova-navigation)",
        1,
        generated_help_links_en_volume_menu,
    },
    {
        "applications-menu",
        "Applications Help",
        "dialog.applications",
        "Use Enter to select the highlighted preset.\nAfter launch, ytnova keeps running and the application continues on its own.\nUse E to edit the applications catalog that backs application presets.\nUse Esc to cancel the menu.\nUse {} for the file or folder currently selected in ytnova.\nUse {input} for the text you type when the preset asks for extra input.\n\n`Applications actions`\n`Select preset`: `Up` and `Down` move through the preset list.\n`Launch behavior`: `F9` starts the selected preset and returns straight to the TUI.\nUse it for repeat-heavy external workflows, not for one-off shell typing.\n`Use `eXecute` for one-offs`: The `X` command prompt stays the ad hoc shell surface with history and terminal-style output.\nUse it when you need a one-off command.\n`Edit presets`: `E` opens the dedicated applications catalog so presets can be changed without leaving the chooser family.\n`Selection and working directory`: `{}` inserts the current file or folder.\nPresets also start in that directory, so scripts without `{}` still run from the place you selected.\n`Prompt text`: `{input}` inserts the extra text you typed for the preset prompt.\n`Starter presets`: The bundled catalog starts with `xdg-open` launchers and includes commented examples for tools such as `mpv` or local helper scripts.\n`Cancel menu`: `Esc` closes the chooser without selecting a preset.\n\n\n- [Navigation](topic:ytnova-navigation)",
        1,
        generated_help_links_en_applications_menu,
    },
    {
        "f2-picker",
        "F2 Picker Help",
        "dialog.f2-picker",
        "Use `Up` and `Down` to move.\nUse `Right` to expand or enter the first child.\nUse `Left` to collapse or go to the parent.\nUse `<` and `>` to cycle loaded volumes.\nUse `L` to log a new path.\nUse `` ` `` to toggle dotfiles.\nUse `Enter` to select the highlighted directory.\nUse `Esc` to cancel.\n\n`Picker actions`\n`Move`: `Up` and `Down` move through the visible directory rows.\n`Expand`: `Right` expands the current directory one level, then moves into the first child when that level is already open.\n`Collapse`: `Left` collapses the current directory, or moves to its parent when the current row is already closed.\n`Select`: `Enter` uses the highlighted directory for the calling prompt.\n`Cancel`: `Esc` closes the picker without changing the prompt.\n\n\n- [Navigation](topic:ytnova-navigation)\n- [Command-line editing](topic:command-line-editing)",
        2,
        generated_help_links_en_f2_picker,
    },
};

static const size_t generated_help_topic_count_en = 43;

static const GeneratedHelpLink generated_help_links_de_index[] = {
    {"Applications", "applications-menu"},
    {"Archive Directory", "archive-dir"},
    {"Archive File", "archive-file"},
    {"Command-line Editing", "command-line-editing"},
    {"Kommandozeilenparameter", "command-line-parameters"},
    {"Compare", "compare"},
    {"Compare Basis", "compare-basis"},
    {"Compare Result", "compare-results"},
    {"Compare Scope", "compare-scope"},
    {"Compare Target", "compare-target"},
    {"Konfigurationsdateien", "configuration-files"},
    {"Copy/Move Targets", "copy-move-targets"},
    {"Create Archive", "create-archive"},
    {"Date Change", "change-date"},
    {"Directory", "directory"},
    {"F10 Config", "f10"},
    {"F2 Picker", "f2-picker"},
    {"F7 Preview", "f7"},
    {"F8 Split", "f8"},
    {"F8 Split Directory", "f8-dir"},
    {"F8 Split File", "f8-file"},
    {"File", "file"},
    {"Filter", "filter"},
    {"Global", "global"},
    {"History", "history-dialog"},
    {"List Jump", "list-jump"},
    {"Navigation", "ytnova-navigation"},
    {"Output", "output"},
    {"Output Destination", "output-destination"},
    {"Output Format", "output-format"},
    {"Output Separator", "output-separator"},
    {"Search Tagged", "search-tagged"},
    {"Shared Commands", "shared-commands"},
    {"Showall", "showall"},
    {"Tagged", "tagged"},
    {"Markierungsanzeige", "tagged-viewer"},
    {"Theming", "theming"},
    {"Vi Keys", "vi-keys"},
    {"Volume", "volume-menu"},
};

static const GeneratedHelpLink generated_help_links_de_list_jump[] = {
    {"Directory", "directory"},
    {"File", "file"},
    {"Showall", "showall"},
    {"Global", "global"},
};

static const GeneratedHelpLink generated_help_links_de_shared_commands[] = {
    {"F7 preview", "f7"},
    {"F8 split", "f8"},
    {"Applications menu", "applications-menu"},
    {"F10 config", "f10"},
};

static const GeneratedHelpLink generated_help_links_de_command_line_editing[] = {
    {"VI-Tasten", "vi-keys"},
    {"Historie", "history-dialog"},
    {"F2-Auswahl", "f2-picker"},
};

static const GeneratedHelpLink generated_help_links_de_command_line_parameters[] = {
    {"Konfigurationsdateien", "configuration-files"},
    {"Filter", "filter"},
};

static const GeneratedHelpLink generated_help_links_de_configuration_files[] = {
    {"F10 Konfiguration", "f10"},
    {"Theming", "theming"},
    {"Kommandozeilenparameter", "command-line-parameters"},
};

static const GeneratedHelpLink generated_help_links_de_vi_keys[] = {
    {"Navigation", "ytnova-navigation"},
    {"Bearbeitung in Eingabezeilen", "command-line-editing"},
};

static const GeneratedHelpLink generated_help_links_de_f10[] = {
    {"Theming", "theming"},
    {"Gemeinsame Befehle", "shared-commands"},
};

static const GeneratedHelpLink generated_help_links_de_theming[] = {
    {"F10 config", "f10"},
};

static const GeneratedHelpLink generated_help_links_de_archive_dir[] = {
    {"Navigation", "ytnova-navigation"},
    {"Filter", "filter"},
    {"Compare", "compare"},
};

static const GeneratedHelpLink generated_help_links_de_archive_file[] = {
    {"Navigation", "ytnova-navigation"},
    {"Markiert", "tagged"},
    {"Kopier-/Verschiebeziele", "copy-move-targets"},
    {"Filter", "filter"},
    {"Vergleich", "compare"},
    {"Output", "output"},
};

static const GeneratedHelpLink generated_help_links_de_filter[] = {
    {"Tagged", "tagged"},
    {"Showall", "showall"},
    {"Global", "global"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLink generated_help_links_de_compare[] = {
    {"File mode", "file"},
    {"Directory mode", "directory"},
    {"Navigation", "ytnova-navigation"},
};

static const GeneratedHelpLink generated_help_links_de_compare_target[] = {
    {"Compare Help", "compare"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLink generated_help_links_de_compare_scope[] = {
    {"Compare Help", "compare"},
};

static const GeneratedHelpLink generated_help_links_de_change_date[] = {
    {"File mode", "file"},
    {"Directory mode", "directory"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLink generated_help_links_de_compare_basis[] = {
    {"Compare Help", "compare"},
};

static const GeneratedHelpLink generated_help_links_de_compare_results[] = {
    {"Compare Help", "compare"},
};

static const GeneratedHelpLink generated_help_links_de_execute_file[] = {
    {"File mode", "file"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLink generated_help_links_de_execute_dir[] = {
    {"Directory mode", "directory"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLink generated_help_links_de_search_tagged[] = {
    {"Tagged", "tagged"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLink generated_help_links_de_create_archive[] = {
    {"Tagged", "tagged"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLink generated_help_links_de_output[] = {
    {"File mode", "file"},
    {"Archive file", "archive-file"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLink generated_help_links_de_output_format[] = {
    {"Output Help", "output"},
};

static const GeneratedHelpLink generated_help_links_de_output_destination[] = {
    {"Output Help", "output"},
    {"Output Format Help", "output-format"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpLink generated_help_links_de_output_separator[] = {
    {"Output Help", "output"},
};

static const GeneratedHelpLink generated_help_links_de_showall[] = {
    {"Navigation", "ytnova-navigation"},
    {"Markiert", "tagged"},
    {"Kopierziele", "copy-move-targets"},
    {"Filter", "filter"},
    {"Vergleich", "compare"},
    {"Verschiebeziele", "copy-move-targets"},
    {"Output", "output"},
};

static const GeneratedHelpLink generated_help_links_de_global[] = {
    {"Navigation", "ytnova-navigation"},
    {"Markiert", "tagged"},
    {"Kopierziele", "copy-move-targets"},
    {"Filter", "filter"},
    {"Vergleich", "compare"},
    {"Verschiebeziele", "copy-move-targets"},
    {"Output", "output"},
};

static const GeneratedHelpLink generated_help_links_de_f7[] = {
    {"Navigation", "ytnova-navigation"},
    {"Markiert", "tagged"},
    {"Kopier-/Verschiebeziele", "copy-move-targets"},
    {"Filter", "filter"},
    {"Vergleich", "compare"},
    {"Anwendungsmenue", "applications-menu"},
    {"Output", "output"},
};

static const GeneratedHelpLink generated_help_links_de_f8[] = {
    {"Navigation", "ytnova-navigation"},
    {"Directory split page", "f8-dir"},
    {"File split page", "f8-file"},
};

static const GeneratedHelpLink generated_help_links_de_f8_dir[] = {
    {"Navigation", "ytnova-navigation"},
    {"F8-Split", "f8"},
    {"Kopierziele", "copy-move-targets"},
    {"Filter", "filter"},
    {"Vergleich", "compare"},
    {"Verschiebeziele", "copy-move-targets"},
    {"Output", "output"},
};

static const GeneratedHelpLink generated_help_links_de_f8_file[] = {
    {"Navigation", "ytnova-navigation"},
    {"F8-Split", "f8"},
    {"Markiert", "tagged"},
    {"Kopierziele", "copy-move-targets"},
    {"Filter", "filter"},
    {"Vergleich", "compare"},
    {"Verschiebeziele", "copy-move-targets"},
    {"Output", "output"},
};

static const GeneratedHelpLink generated_help_links_de_history_dialog[] = {
    {"Navigation", "ytnova-navigation"},
};

static const GeneratedHelpLink generated_help_links_de_volume_menu[] = {
    {"Navigation", "ytnova-navigation"},
};

static const GeneratedHelpLink generated_help_links_de_applications_menu[] = {
    {"Navigation", "ytnova-navigation"},
};

static const GeneratedHelpLink generated_help_links_de_f2_picker[] = {
    {"Navigation", "ytnova-navigation"},
    {"Command-line editing", "command-line-editing"},
};

static const GeneratedHelpTopic generated_help_topics_de[] = {
    {
        "index",
        "Hilfeindex",
        NULL,
        "Benutze diesen Index, wenn du die Frage kennst, aber nicht weißt, welche Seite sie beantwortet.\nDrücke auf einem Thema `Enter` oder `Right`, um es zu öffnen.\nTippe `/` und einen Themennamen, um ihn auszuwählen, und drücke dann `Enter` oder `Right`, um ihn zu öffnen.\nGehe mit `Left` zurück hierher.\nVerlasse die Hilfe mit `Esc`.\n\n`Zweck`\nBenutze `F1` für die Aufgabe vor dir und nicht als ein riesiges Handbuch.\nLokale Seiten beantworten zuerst die aktuelle Frage.\nGemeinsame Themen halten die Regeln, die sich wiederholen.\n\n`Benutzung des Index`\nDiese Liste bleibt alphabetisch, damit du sie schnell überfliegen kannst.\nWenn du den aktuellen Bildschirm kennst, starte dort.\nSonst nimm das Thema, das zu deiner Frage passt.\nÖffne ein Thema, lies die kurze Antwort und gehe dann mit `Left` zurück, ohne deinen Platz zu verlieren.\n\n\n- [Applications](topic:applications-menu)\n- [Archive Directory](topic:archive-dir)\n- [Archive File](topic:archive-file)\n- [Command-line Editing](topic:command-line-editing)\n- [Kommandozeilenparameter](topic:command-line-parameters)\n- [Compare](topic:compare)\n- [Compare Basis](topic:compare-basis)\n- [Compare Result](topic:compare-results)\n- [Compare Scope](topic:compare-scope)\n- [Compare Target](topic:compare-target)\n- [Konfigurationsdateien](topic:configuration-files)\n- [Copy/Move Targets](topic:copy-move-targets)\n- [Create Archive](topic:create-archive)\n- [Date Change](topic:change-date)\n- [Directory](topic:directory)\n- [F10 Config](topic:f10)\n- [F2 Picker](topic:f2-picker)\n- [F7 Preview](topic:f7)\n- [F8 Split](topic:f8)\n- [F8 Split Directory](topic:f8-dir)\n- [F8 Split File](topic:f8-file)\n- [File](topic:file)\n- [Filter](topic:filter)\n- [Global](topic:global)\n- [History](topic:history-dialog)\n- [List Jump](topic:list-jump)\n- [Navigation](topic:ytnova-navigation)\n- [Output](topic:output)\n- [Output Destination](topic:output-destination)\n- [Output Format](topic:output-format)\n- [Output Separator](topic:output-separator)\n- [Search Tagged](topic:search-tagged)\n- [Shared Commands](topic:shared-commands)\n- [Showall](topic:showall)\n- [Tagged](topic:tagged)\n- [Markierungsanzeige](topic:tagged-viewer)\n- [Theming](topic:theming)\n- [Vi Keys](topic:vi-keys)\n- [Volume](topic:volume-menu)",
        39,
        generated_help_links_de_index,
    },
    {
        "f1-navigation",
        "Hilfe-Navigation",
        NULL,
        "Mit den Pfeiltasten `Up` und `Down` scrollst du die Hilfe zeilenweise.\nMit den Pfeiltasten `Up` und `Down` wechselst du zwischen Links.\n`Page Up` und `Page Down` bewegen eine Hilfeseite.\n`Home` und `End` gehen zum Anfang und Ende.\n`Enter` oder die Pfeiltaste `Right` öffnet den gewählten Link.\nDie Pfeiltaste `Left` kehrt zur vorherigen Hilfeseite zurück.\nDrücke `H`, um zum `Hilfeindex` zu gehen.\n`Esc` oder `Q` schließt die Hilfe.",
        0,
        NULL,
    },
    {
        "ytnova-navigation",
        "YtreeNova-Navigation",
        NULL,
        "YtreeNova ist für die Tastatur gemacht.\nMauseffekte können vorkommen, sind aber keine vorgesehenen Steuerelemente.\nMit den Pfeiltasten `Up` und `Down` bewegst du dich jeweils eine Zeile.\n`Page Up` und `Page Down` bewegen jeweils eine Bildschirmseite.\n`Home` und `End` gehen zur ersten oder letzten sichtbaren Zeile.\n`Enter` öffnet das gewählte Element.\n`Right` öffnet oder erweitert das gewählte Element, wenn die Ansicht das unterstützt.\n`Left` geht zurück oder klappt das gewählte Element ein, wenn die Ansicht das unterstützt.\n[/ Sprung](topic:list-jump) geht beim Tippen zu einem passenden Namen in der aktuellen Liste. `Enter` wählt ihn aus; `Esc` bricht ab.\n`Tab` geht zum nächsten sichtbaren Geschwisterelement im Baum und springt am Ende zum Anfang; `Shift-Tab` kehrt dies um.\n[F8 Split](topic:f8)-Modus: `Tab` wechselt das aktive Panel; manche Prompts geben `Tab` eine eigene lokale Bedeutung.\n[F7 Vorschau](topic:f7): Gewöhnliche Navigationstasten bewegen die Dateiauswahl; ihre Umschaltvarianten scrollen die Vorschau.\n\n`Terminal-Eingabegrenzen`\n`Alt` wird absichtlich nicht unterstützt, weil Terminals es uneinheitlich behandeln.\n`C-m` und `C-j` sind Enter.\n`C-i` ist Tab.\n`C-[` ist Esc.",
        0,
        NULL,
    },
    {
        "list-jump",
        "List Jump",
        NULL,
        "Drücke `/`, tippe Buchstaben und drücke `Enter`, um auf dem besten sichtbaren Treffer in der aktuellen Liste zu landen.\nIn Baumansichten wiederholst du den Sprung im nächsten Verzeichnis, wenn du tiefer gehen willst.\n\n`Sprungmodell`\n`/` öffnet einen Live-Sprung-Prompt nur für die aktuelle Liste.\nBaum- und Verzeichnisansichten springen zwischen sichtbaren Verzeichnisnamen.\nDateiorientierte Ansichten springen zwischen den sichtbaren Dateizeilen dieser Oberfläche.\n\n`Übernehmen oder abbrechen`\n`Buchstaben tippen`: Zum besten aktuellen Treffer springen, während du tippst.\n`Enter`: Auf dem aktuellen Treffer landen und dort bleiben.\n`Esc`: Den Sprung abbrechen und die ursprüngliche Auswahl wiederherstellen.\n`Bereichswechsel`: Filter, Showall, Global, Archive und Split-Modus ändern, welche sichtbare Liste `/` durchsucht.\n\n\n- [Directory](topic:directory)\n- [File](topic:file)\n- [Showall](topic:showall)\n- [Global](topic:global)",
        4,
        generated_help_links_de_list_jump,
    },
    {
        "shared-commands",
        "Gemeinsame Befehle",
        NULL,
        "Diese Tasten behalten in mehr als einem Modus dieselbe Grundbedeutung.\nBenutze die Modusseite für lokale Befehle und diese Seite für die gemeinsame Funktionstastenfamilie.\n\n`Gemeinsame Funktionstasten`\n`F1`: Kontextuelle Hilfe für die aktive Oberfläche öffnen.\n`F5`: Die aktuelle Ansicht aktualisieren.\n`F6`: Die Statistik- oder Detaildarstellung der aktiven Ansicht ändern.\n`F7`: Die Vorschau für den aktiven Dateikontext umschalten.\n`F8`: Den Split-Screen-Modus umschalten.\n`F9`: Das Anwendungsmenü öffnen.\n`F10`: Die Konfigurationsoberfläche öffnen.\n`Esc`: Das aktuelle Overlay, den Prompt oder das Popup verlassen.\n\n\n- [F7 preview](topic:f7)\n- [F8 split](topic:f8)\n- [Applications menu](topic:applications-menu)\n- [F10 config](topic:f10)",
        4,
        generated_help_links_de_shared_commands,
    },
    {
        "tagged",
        "Markierungen",
        NULL,
        "Markierungen wählen mehrere Dateien aus, auf die dann ein Vorgang gemeinsam angewendet wird.\nMarkierte Dateien sind am `*` im Rand zu erkennen.\n\nDrücke in einer Dateiliste `T`, um die gewählte Datei zu markieren, oder `U`, um die Markierung zu entfernen. Die Auswahl geht zur nächsten Datei.\nIn einem Verzeichnisbaum wirken `T` und `U` auf die Dateien, die der aktuelle Filter im gewählten Verzeichnis zulässt, und gehen dann zum nächsten Verzeichnis nach unten.\n\n`i` kehrt die Markierungen der Dateien um, die der aktuelle Filter im aktuellen Bereich zulässt.\n\nWenn Statistiken angezeigt werden, erscheinen dort auch die Gesamtwerte der markierten Dateien.\n\nHalte die Steuerungstaste mit dem Buchstaben nach `C-` gedrückt, um den passenden Vorgang für alle markierten Dateien auszuführen. `^` in einem Footer-Label bedeutet denselben Markierungsvorgang, zum Beispiel `C/^Copy`.\n\n`C-a` öffnet den Attribute-Prompt für markierte Dateien.\n`C-c` öffnet den [Kopier-](topic:copy-move-targets)Prompt für markierte Dateien.\n`C-d` fragt vor dem Löschen markierter Dateien nach.\n`C-n` öffnet den [Verschiebe-](topic:copy-move-targets)Prompt, weil `C-m` in Terminals Enter ist.\n`C-o` öffnet den [Ausgabe-](topic:output)Prompt für markierte Dateien.\n`C-p` öffnet den Pipe-Prompt für markierte Dateien.\n`C-r` öffnet den Umbenennen-Prompt für markierte Dateien.\n`C-s` [durchsucht markierte Dateien](topic:search-tagged) und entfernt die Markierung von Dateien ohne Treffer.\n`C-t` markiert alle Dateien in der aktiven Dateiliste. In einem Verzeichnisbaum markiert es die Dateien, die der aktuelle Filter in allen geloggten Verzeichnissen zulässt.\n`C-u` entfernt die Markierungen aller Dateien in der aktiven Dateiliste. In einem Verzeichnisbaum entfernt es die Markierungen der Dateien, die der aktuelle Filter in allen geloggten Verzeichnissen zulässt.\n`C-v` zeigt markierte Dateien nacheinander an.\n`C-x` öffnet einen [Befehlsprompt](topic:execute-file) und führt den Vorgang einmal für jede markierte Datei aus.\n`C-y` [Kopiert Pfade](topic:copy-move-targets) der markierten Dateien.\n`C-z` öffnet den [Archiv-](topic:archive)Prompt für markierte Dateien.\n\nDrücke für einen [Filter](topic:filter)-Bereich nur mit markierten Dateien `F` und dann `Tab`. Das ändert die Markierungen nicht.",
        0,
        NULL,
    },
    {
        "tagged-viewer",
        "Markierungsanzeige",
        "viewer.tagged",
        "`Markierte anzeigen` öffnet die markierten Suchergebnisse in der internen Anzeige.\nMit `n` und `p` wechseln Sie zur nächsten oder vorherigen markierten Datei.\n`Leertaste`, `BildAb`, und `BildAuf` bewegen nur seitenweise in der aktuellen Datei.\nNach `C-s` für die Suche in der Markierung wechselt `/` zum nächsten und `?` zum vorherigen Treffer in der aktuellen Datei.\nAußerhalb dieser Anzeige bleibt `C-s` die Suchaktion für die Markierungsliste.\nMit `TAGGEDVIEWER=external` übernimmt das konfigurierte Anzeigeprogramm Suche und Treffernavigation.",
        0,
        NULL,
    },
    {
        "command-line-editing",
        "Bearbeitung in Eingabezeilen",
        NULL,
        "Die meisten Prompts teilen sich dieselben Bearbeitungstasten.\nLerne sie einmal hier und benutze die Prompt-Seite nur noch für Syntax, Vorgaben und Geltungsbereich.\n\n`Bearbeitungstasten`\n`Left/Right`: Innerhalb des aktuellen Prompt-Textes bewegen.\n`Home/End`: Zum Anfang oder Ende des Prompt-Textes springen.\n`Backspace/Delete`: Das Zeichen links oder rechts vom Cursor löschen.\n`Enter`: Den aktuellen Wert übernehmen.\n`Esc`: Abbrechen, ohne den Prompt zu übernehmen.\n\n`Gemeinsame Hilfen`\n`Up`: Die Prompt-Historie öffnen oder darin weitergehen, wenn dieser Prompt eine Historie hat.\n`F2`: Einen Browser oder eine Auswahl öffnen, wenn der aktuelle Prompt Browsing unterstützt.\n`F1`: Syntax- oder Bereichsregeln zeigen, die nur für diesen Prompt gelten.\n\n\n- [VI-Tasten](topic:vi-keys)\n- [Historie](topic:history-dialog)\n- [F2-Auswahl](topic:f2-picker)",
        3,
        generated_help_links_de_command_line_editing,
    },
    {
        "command-line-parameters",
        "Kommandozeilenparameter",
        NULL,
        "Starten Sie ytnova mit zu protokollierenden Pfaden oder einer Option für das Startverhalten.\nMit `ytnova --init` werden fehlende Konfigurationsdateien erstellt.\n`ytnova --version` gibt die Version aus und beendet das Programm.\n\n`Startoptionen`\n`-d depth`: Setzt die Start-Scan-Tiefe.\nZahlen sowie `min` oder `root` für 0 und `max` oder `all` für 100 sind möglich.\n`-f filter`: Startet mit einem Dateifilter.\nShell-Muster wie `\"*.c\"` bitte quoten, damit die Shell sie nicht vorher erweitert.\n`-h history_file`: Verwendet eine andere Befehlsverlaufsdatei.\n`-p config_file`: Verwendet statt `~/.config/ytnova/ytnova.conf` eine andere Hauptkonfiguration.\n`--init`: Erstellt fehlende Startkonfigurationen und beendet das Programm.\n`-v`, `-V`, `--version`: Gibt Versionsinformationen aus und beendet das Programm.\n`Pfade`: Ein oder mehrere Verzeichnis- oder Archivpfade werden als Start-Volumes protokolliert.\nOhne Pfad protokolliert ytnova das aktuelle Verzeichnis.\n\n\n- [Konfigurationsdateien](topic:configuration-files)\n- [Filter](topic:filter)",
        2,
        generated_help_links_de_command_line_parameters,
    },
    {
        "configuration-files",
        "Konfigurationsdateien",
        NULL,
        "Bearbeitbare Benutzerkonfigurationen werden normalerweise unter `~/.config/ytnova` erstellt.\nStattdessen können ein explizites Profil, eine vorhandene alte Datei oder mitgelieferte Vorgaben verwendet werden.\n\n`Konfigurationsverzeichnis`\n`ytnova.conf`: Hauptprofil mit Einstellungen wie Start-Scan-Tiefe, `VI_KEYS` und `SEPARATE_DIR_FILE_VIEWS`.\n`commands.conf`: Befehlsnamen, Tastenzuordnungen und Auswahl der Befehlsvorgaben.\n`themes.conf`: Themenauswahl und Überschreibungen für Themenrollen.\n`applications.conf`: Anwendungsvorgaben für `F9`.\n`Verlauf`: Der Befehlsverlauf liegt getrennt von diesen Konfigurationsdateien.\nMit `-h` wählen Sie eine andere Verlaufsdatei.\n\nMit `-p` wählen Sie eine andere `ytnova.conf`.\nVorhandene alte Home-Dotfiles und mitgelieferte Vorgaben bleiben verfügbar, wenn keine bevorzugte Benutzerdatei verwendet wird.\n\n\n- [F10 Konfiguration](topic:f10)\n- [Theming](topic:theming)\n- [Kommandozeilenparameter](topic:command-line-parameters)",
        3,
        generated_help_links_de_configuration_files,
    },
    {
        "copy-move-targets",
        "Copy/Move Targets",
        NULL,
        "`Copy`, `move` und `pathcopy` fragen zuerst nach einem Namen oder Umbenennungsmuster, dann nach dem Zielverzeichnis.\n\nBei einer Datei lässt du ihren vorbelegten Namen stehen, um ihn beizubehalten, oder änderst ihn zum Umbenennen. Du kannst in beiden Fällen Wildcards verwenden.\n\nBei [markierten Dateien](topic:tagged) lässt du das vorbelegte `*` stehen, um ihre Namen beizubehalten, oder gibst ein Umbenennungsmuster ein:\n`*` übernimmt den restlichen ursprünglichen Namen. Nutze `copy-*`, um ein Präfix hinzuzufügen, oder `*.bak`, um die Erweiterung zu ändern.\n`?` übernimmt ein Zeichen des ursprünglichen Namens. Zum Beispiel übernimmt `??-*` die ersten beiden Zeichen, fügt `-` hinzu und übernimmt dann den Rest.\nAndere Zeichen werden wie geschrieben verwendet.\n\nNutze `Up`, um einen früheren Namen oder ein früheres Muster wiederzuverwenden.\n\nGib das Zielverzeichnis in `To Directory` ein. Im [F8-Split](topic:f8)-Modus beginnt es mit dem derzeit ausgewählten Verzeichnis des anderen Panels.\n\nNutze `Up`, um ein früheres Ziel wiederzuverwenden, oder drücke [F2](topic:f2-picker), um eines zu wählen.\n\nDieselben Prompts gelten für [Archivdateien](topic:archive-file) und [Archivverzeichnisse](topic:archive-dir). Beim Kopieren einer Archivdatei extrahiert ytnova sie bei Bedarf; beim Verschieben kopiert ytnova sie und entfernt dann das Original. Du kannst auch in ein geloggtes Archiv kopieren.\n\n`pathcopy` stellt den bestehenden Pfad der ausgewählten Datei unterhalb des Zielverzeichnisses wieder her.\n\nWenn das Zielverzeichnis fehlt, fragt ytnova, ob es erstellt werden soll. Existiert bereits eine Zieldatei, fragt ytnova vor dem Ersetzen.",
        0,
        NULL,
    },
    {
        "vi-keys",
        "VI-Tasten",
        NULL,
        "Mit `VI_KEYS=1` bleiben die vi-Bewegungstasten in Kleinbuchstaben aktiv.\nBefehle, die kollidieren würden, wandern auf Großbuchstaben oder eine andere sichere Taste.\n\n`Navigations-Umlegung`\nMit `VI_KEYS=1` werden `h`, `j`, `k` und `l` zu `Left`, `Down`, `Up` und `Right`.\n`C-u` und `C-d` werden zu Seite hoch und Seite runter.\n\n`Befehlskollisionen`\nBefehle, die diese Kleinbuchstaben stehlen würden, gehen aus dem Weg.\nBeispiele sind `J compare`, `K volume`, `D delete tagged` und `U untag all`, wo diese Aktionen existieren.\n\n\n- [Navigation](topic:ytnova-navigation)\n- [Bearbeitung in Eingabezeilen](topic:command-line-editing)",
        2,
        generated_help_links_de_vi_keys,
    },
    {
        "f10",
        "F10-Konfiguration",
        NULL,
        "Benutze `F10` für Konfigurationsarbeit, nicht für einmalige Dateiaktionen.\nDort gehören Profil, Befehle, Themes, Reload und ähnliche dauerhafte Einstellungen hin.\n\n`Konfigurationsoberfläche`\nBenutze `F10`, wenn du dauerhaftes Verhalten ändern willst, statt nur die aktuelle Auswahl zu beeinflussen.\nProfileinstellungen, Befehlsbeschriftungen, Themes und Reload leben hier.\n\n`Zugehörige Dateien`\n`ytnova.conf` enthält Profileinstellungen.\n`commands.conf` enthält Benutzerlabels und Bindings für Befehle.\n`themes.conf` enthält Theme-Auswahl und Theme-Overrides.\n\n\n- [Theming](topic:theming)\n- [Gemeinsame Befehle](topic:shared-commands)",
        2,
        generated_help_links_de_f10,
    },
    {
        "theming",
        "Theming",
        NULL,
        "Themes stylen semantische Rollen, nicht einzelne Bildschirmpositionen.\nDadurch bleiben Hilfe, Auswahlfenster, Auswahlmarkierung, Footer und Warnflächen als ein System lesbar.\n\n`Theme-Modell`\nThemes setzen semantische Rollen wie `footer`, `help`, `help_link`, `selection`, `picker` und `warning`.\nSo bleibt eine Theme-Änderung im ganzen UI konsistent.\n\n`Bearbeitungspfad`\nBenutze `F10`, um den Bearbeitungspfad für Theme oder Konfiguration zu öffnen.\nAchte zuerst auf lesbare Flächen mit hoher Nutzung: Auswahl, Picker, Footer und Hilfe.\n\n\n- [F10 config](topic:f10)",
        1,
        generated_help_links_de_theming,
    },
    {
        "directory",
        "Directory Help",
        "main.dir",
        "Dies ist das `Verzeichnisfenster`. Es zeigt den hierarchischen Verzeichnisbaum, den ytnova im aktuellen `Volume` gelesen hat.\nDie Dateien des gewählten Verzeichnisses erscheinen im `kleinen Fenster`.\n\n`+` bedeutet, dass ytnova den Unterverzeichniszweig dieses Verzeichnisses im Baum noch nicht erweitert oder geloggt hat.\n\n`Left` klappt den ausgewählten Zweig ein. Wenn dieser Zweig bereits verborgen ist, geht die Auswahl zum übergeordneten Verzeichnis.\n`Right` geht zum ersten angezeigten Verzeichnis unter dem ausgewählten Verzeichnis. Wenn ytnova es noch nicht gelesen hat, liest es dieses zuerst ein.\n`+` liest das ausgewählte Verzeichnis ein und fügt seinen Zweig zum Baum hinzu, ohne die Auswahl zu bewegen.\n`*` liest jedes Verzeichnis unter dem ausgewählten Verzeichnis ein, ohne die Auswahl zu bewegen.\n`-` klappt das ausgewählte Verzeichnis ein und entfernt seine Dateien aus dem eingelesenen Baum. Sie erscheinen nicht mehr in Showall oder den Statistiken.\nDrücke `Enter`, um in das `Dateifenster` des ausgewählten Verzeichnisses zu wechseln. Wenn dessen Baumzweig noch nicht geloggt ist, liest ytnova ihn zuerst ein.\n\nDer `Footer` zeigt die hier verfügbaren Befehle.\nEinige Befehle haben ihre eigene `F1`-Hilfe.\n\nDie Zahlentasten ändern, was du siehst.\n`1` zeigt Namen, die Standardansicht.\n`2` zeigt Attribute.\n`3` zeigt den Eigentümer.\n`4` zeigt Zeiten.\nDrücke eine aktive `2`, `3` oder `4` erneut, um zu den Namen zurückzukehren.\nDiese Ansichten ändern normalerweise sowohl den Baum als auch das `Dateifenster` dieses Panels.\nSetze `SEPARATE_DIR_FILE_VIEWS=1` in `ytnova.conf`, um sie getrennt zu halten.\n\n`5` schaltet kompakte Namen im `Dateifenster` ein oder aus.\n`6` schaltet die Größeneinheiten in Baum- und Dateizeilen um.\n`7` zeigt eine kleine Textvorschau für jede angezeigte Datei.\n`8` zeigt Dateidetails für jede angezeigte Datei.\n`9` zeigt Git-Informationen für Dateien in einem Git-Projekt.\n`5`, `7`, `8` und `9` ändern nur das `Dateifenster`.\n\n`Attributes` öffnet ein Untermenü, um die Attribute des gewählten Verzeichnisses zu ändern.\n[Copy](topic:copy-move-targets) kopiert das gewählte Verzeichnis mit seinem Inhalt.\n`Delete` löscht das gewählte Verzeichnis.\n[Filter](topic:filter) ändert die Dateien in der aktuellen Ansicht.\n`filter-passend` bedeutet: Dateien, die der aktuelle Filter zulässt.\n`Global` zeigt Dateien aus allen geloggten Volumes in einer Liste.\n`Invert` kehrt die Markierungen passender Dateien im gewählten Verzeichnis um.\n`J` [compare](topic:compare) vergleicht dieses Verzeichnis mit einem anderen.\n`K volume` öffnet das Volume-Menü.\n`Log` liest das gewählte Verzeichnis oder Archiv oder liest ein geloggtes Verzeichnis erneut.\n`Makedir` erstellt ein Verzeichnis unter dem gewählten Verzeichnis.\n`Newfile` erstellt eine leere Datei im gewählten Verzeichnis.\n[Output](topic:output) exportiert die aktuelle Auswahl.\n`Pipe` führt einen Befehl im gewählten Verzeichnis aus und gibt ihm die angezeigten Dateinamen.\n`Quit` beendet ytnova.\n`Rename` benennt das gewählte Verzeichnis um.\n`Showall` zeigt alle Dateien im aktuellen Volume in einer Liste.\n`T` und `U` markieren oder entmarkieren alle passenden Dateien im gewählten Verzeichnis und gehen dann zum nächsten Verzeichnis.\n`C-t` und `C-u` [markieren oder entmarkieren](topic:tagged) alle passenden Dateien in jedem geloggten Verzeichnis des aktuellen Volumes.\n[moVedir](topic:copy-move-targets) verschiebt das gewählte Verzeichnis mit seinem Inhalt.\n`eXecute` führt einen Befehl für das gewählte Verzeichnis aus.\n`Z archive` erstellt ein Archiv aus der aktuellen Auswahl.\n`/ jump` springt beim Tippen zu einem angezeigten Namen.\n`\\` dotfiles` zeigt oder versteckt versteckte Namen.\n\n`F5` aktualisiert die aktive Ansicht.\n`F6` zeigt oder versteckt Statistiken.\n`F7` schaltet Autoview ein oder aus.\n`F8` schaltet den Split-Screen ein oder aus.\n`F9` öffnet das Anwendungsmenü.\n`F10` öffnet die Konfiguration.\n`Esc` bricht den aktuellen Vorgang ab.",
        0,
        NULL,
    },
    {
        "file",
        "File Help",
        "main.file",
        "This is the `file window`. The selected row is a `file`.\nCommands in this `footer` act on it unless the line says it uses tagged files.\n\nThe `footer` shows the commands available here.\nSome commands have their own `F1` help.\n\nThe number keys change what you see.\n`1`: Show names.\n`2`: Show attributes, including `name -> target` for symlinks.\n`3`: Show the owner.\n`4`: Show times.\nPress the active `2`, `3`, or `4` again to return to names.\n`5`: Turn `Compact` on or off from the names view.\n`6`: Switch visible file sizes between readable and raw units.\n`7`: Show a small text preview on each visible file row.\n`8`: Show file details on each visible file row.\n`9`: Show the `Git band` when this directory is in a Git worktree.\n\n`A`: Open attributes for the selected file.\n`C-a`: Open attributes for matching [tagged files](topic:tagged).\n`C`: Open [Copy](topic:copy-move-targets) for the selected file.\n`C-c`: Copy matching tagged files.\n`D`: Delete the selected file.\n`C-d`: Delete matching tagged files.\n\n`E`: Edit the selected file in the configured editor.\n`F`: Open [Filter](topic:filter) for this list.\n`H`: Open the selected file in hex view.\n`I`: Reverse tags in the visible list.\n\n`J`: [Compare](topic:compare) the selected file with another file.\n`K`: Open the volume menu.\n`L`: Log a directory or archive without leaving this list.\n`M`: Open [Move](topic:copy-move-targets) for the selected file.\n`C-n`: Move matching tagged files.\n`C-m` is Enter in terminals, so it cannot mean move tagged files.\n`N`: Create a new empty file.\n\n`O`: Open [Output](topic:output) for the selected file.\n`C-o`: Output matching tagged files.\n`P`: Run a command with the selected file’s contents as input.\n`C-p`: Run that command for matching tagged files.\n`Q`: Quit ytnova.\n`R`: Rename the selected file.\n`C-r`: Rename matching tagged files.\n\n`S`: Choose the file-list sort order.\n`C-s`: Search tagged files.\n`T`: [Tag](topic:tagged) the selected file, then move to the next file.\n`C-t`: Tag every visible file.\n`U`: Remove the selected file’s tag, then move to the next file.\n`C-u`: Remove tags from every visible file.\n\n`V`: View the selected file.\n`C-v`: View matching tagged files one after another.\n`X`: Type a shell command. `{}` is the selected file’s path.\n`C-x`: Run that command once for each matching tagged file.\n`Y`: [Pathcopy](topic:copy-move-targets) the selected file and keep its path relative to the current volume root.\n`C-y`: Pathcopy matching tagged files.\n`Z`: Archive tagged files, or the selected file when none are tagged.\n`C-z`: Archive matching tagged files.\n\n`/ jump` moves to a displayed name as you type.\n`\\` dotfiles` shows or hides hidden names.\n\n`F5`: Refresh the active view.\n`F6`: Show or hide the statistics strip.\n`F7`: Turn autoview on or off.\n`F8`: Turn split-screen on or off.\n`F9`: Open the Applications menu.\n`F10`: Open configuration.\n`Esc`: Cancel the current operation.",
        0,
        NULL,
    },
    {
        "archive-dir",
        "Archive Directory Help",
        "main.archive-dir",
        "Benutze diese Seite für Befehle im Archivbaum und für Regeln, die nur in Archiven gelten.\nGemeinsame Zeilen öffnen ihr erklärendes Thema.\nEinzeilige Zeilen sagen schon genug zum Handeln.\n\n`Ansicht und Bereich`\n`1`: Nur Name.\nDas ist die schlichte Standardansicht.\n`2`: Attribute.\nIn Dateilisten zeigt das auch `Name -> Ziel` bei Symlinks.\n`3`: Eigentümer.\n`4`: Zeiten.\n`Zurücksetzen`: `1` bringt zur schlichten Namensansicht zurück.\nWenn `2`, `3` oder `4` schon aktiv ist, springt dieselbe Taste ebenfalls zurück zu Name.\n`Archivbaum gegen Dateifenster`: Im Archiv-Verzeichnisfokus ändern `5`, `7` und `8` nicht die Baumzeilen.\nSie ändern das Archiv-Dateifenster dieses Panels.\n`5`: Compact nur aus der aktuellen `1`-Namensansicht umschalten.\n`6`: Archivzeilen zwischen lesbaren und rohen Größeneinheiten umschalten.\nDie Statistik bleibt lesbar.\n`7`: Mini-Vorschautext auf jeder sichtbaren Archiv-Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n`8`: Datei-Detailtext auf jeder sichtbaren Archiv-Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n`9`: In Archiven unbenutzt.\n`0`: Derzeit unbenutzt; macht nichts.\n`Showall`: Die Dateiliste für das aktuelle Archiv öffnen.\n`Global`: Archiv-Ergebnisse in die Sammelliste über mehrere Volumes mischen.\n`Root/Exit`: `\\` springt zur Archivwurzel oder verlässt das Archiv, wenn du dort schon bist.\n`Jump`: Drücke `/`, tippe Buchstaben und drücke `Enter`, um auf dem besten sichtbaren Treffer zu landen.\n`Dotfiles`: Versteckte Archiveinträge ein- oder ausblenden, wenn diese Ansicht sie zeigt.\n\n`Arbeitsmenge`\n`Filter`: Den aktuellen archivgestützten Dateibereich filtern.\n`Tag`: Dateien im aktuellen virtuellen Verzeichnisbereich markieren.\n`Untag`: Markierungen im aktuellen virtuellen Verzeichnisbereich entfernen.\n\n`Archiv-Verzeichnisaktionen`\n`Compare`: Das gewählte Archivverzeichnis oder den aktuellen Archivbaum vergleichen.\n`Output`: Die aktuelle archivgestützte Auswahl über den Output-Ablauf exportieren.\n`Pipe`: Einen Shell-Befehl eingeben. ytnova schickt die sichtbaren passenden Namen aus dem gewählten Archivverzeichnis zeilenweise an seine Standard-Eingabe.\n`Log`: Ein weiteres Verzeichnis oder eine Archivdatei loggen.\n`Volume`: Die Volume-Auswahl öffnen.\n`Makedir`: Ein Verzeichnis anlegen, sofern das Archivformat es unterstützt.\n`Rename`: Den gewählten Archiv-Verzeichniseintrag umbenennen.\n`Delete`: Den gewählten Archiv-Verzeichniseintrag löschen.\n`Quit`: ytnova beenden.\n\n`Archiv-Verzeichnis-Funktionstasten`\n`F1`: Kontextuelle Hilfe für die aktive Archiv-Verzeichnisoberfläche öffnen.\n`F5`: Das aktive Panel aktualisieren.\n`F6`: Die Statistik- oder Detaildarstellung des aktiven Panels ändern.\n`F7`: Die Vorschau für den aktuellen Dateikontext umschalten.\n`F8`: Den Split-Screen-Modus umschalten.\n`F9`: Das Anwendungsmenü öffnen.\n`F10`: Die Konfigurationsoberfläche öffnen.\n\n\n- [Navigation](topic:ytnova-navigation)\n- [Filter](topic:filter)\n- [Compare](topic:compare)",
        3,
        generated_help_links_de_archive_dir,
    },
    {
        "archive-file",
        "Archive File Help",
        "main.archive-file",
        "Benutze diese Seite für Befehle in Archiv-Dateilisten und für dateispezifische Archivregeln.\nGemeinsame Zeilen öffnen ihr erklärendes Thema.\nEinzeilige Zeilen sagen schon genug zum Handeln.\n\n`Ansicht und Bereich`\n`1`: Nur Name.\nDas ist die schlichte Standardansicht.\n`2`: Attribute.\nIn Dateilisten zeigt das auch `Name -> Ziel` bei Symlinks.\n`3`: Eigentümer.\n`4`: Zeiten.\n`Zurücksetzen`: `1` bringt zur schlichten Namensansicht zurück.\nWenn `2`, `3` oder `4` schon aktiv ist, springt dieselbe Taste ebenfalls zurück zu Name.\n`5`: Compact nur aus der aktuellen `1`-Namensansicht umschalten.\n`6`: Archivzeilen zwischen lesbaren und rohen Größeneinheiten umschalten.\nDie Statistik bleibt lesbar.\n`7`: Mini-Vorschautext auf jeder sichtbaren Archiv-Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n`8`: Datei-Detailtext auf jeder sichtbaren Archiv-Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n`9`: In Archiven unbenutzt.\n`Zusatzstatus`: `5`, `7` und `8` ersetzen einander im Statistiklabel, statt sich zu stapeln.\n`0`: Derzeit unbenutzt; macht nichts.\n`Sort`: Die Sortierung der aktuellen Archiv-Dateiliste ändern.\n`Jump`: Drücke `/`, tippe Buchstaben und drücke `Enter`, um auf dem besten sichtbaren Treffer zu landen.\n`Dotfiles`: Versteckte Archiveinträge ein- oder ausblenden, wenn diese Ansicht sie zeigt.\n\n`Arbeitsmenge`\n`Filter`: Die aktuelle archivgestützte Liste filtern.\n`C-s` durchsucht nur markierte Archiveinträge und entmarkiert Nicht-Treffer.\n`Tag`: Den gewählten Archiveintrag markieren, und `C-t` markiert alle sichtbaren Zeilen im aktuellen Bereich.\n`Untag`: Die Markierung des gewählten Archiveintrags entfernen, und `C-u` löscht Archiv-Markierungen im aktuellen Bereich.\n`Invert Tags`: Den Markierungszustand im sichtbaren Bereich umkehren.\n\n`Archiv-Dateiaktionen`\n`Copy`: `C` kopiert den gewählten Archiveintrag, und `C-k` kopiert die markierten Archiveinträge über denselben Prompt.\n`Move`: `M` verschiebt den gewählten Archiveintrag, und `C-n` verschiebt die markierten Archiveinträge über denselben Prompt.\n`View`: Den gewählten Archiveintrag anzeigen, und `C-v` zeigt die markierten Archiveinträge nacheinander an.\n`Hex`: Den gewählten Archiveintrag in der Hex-Ansicht öffnen.\n`Compare`: Den gewählten Archiveintrag mit einer anderen Datei vergleichen.\n`Output`: Den gewählten Archiveintrag über den Output-Ablauf exportieren.\n`Pathcopy`: Den gewählten Archiveintrag kopieren und seinen relativen Pfad erhalten.\n`Pipe`: Einen Shell-Befehl eingeben und ihm den Inhalt des gewählten Archiveintrags über Standard-Eingabe zuführen.\n`Rename`: Den gewählten Archiveintrag umbenennen.\n`Delete`: Den gewählten Archiveintrag löschen.\n`Log`: Ein weiteres Verzeichnis oder eine Archivdatei loggen.\n`Volume`: Die Volume-Auswahl öffnen.\n`Quit`: ytnova beenden.\n\n`Archiv-Datei-Funktionstasten`\n`F1`: Kontextuelle Hilfe für die aktive Archiv-Dateioberfläche öffnen.\n`F5`: Das aktive Panel aktualisieren.\n`F6`: Die Statistik- oder Detaildarstellung des aktiven Panels ändern.\n`F7`: Die Vorschau für den aktuellen Dateikontext umschalten.\n`F8`: Den Split-Screen-Modus umschalten.\n`F9`: Das Anwendungsmenü öffnen.\n`F10`: Die Konfigurationsoberfläche öffnen.\n\n\n- [Navigation](topic:ytnova-navigation)\n- [Markiert](topic:tagged)\n- [Kopier-/Verschiebeziele](topic:copy-move-targets)\n- [Filter](topic:filter)\n- [Vergleich](topic:compare)\n- [Output](topic:output)",
        6,
        generated_help_links_de_archive_file,
    },
    {
        "filter",
        "Filter Help",
        "prompt.filter,prompt.filter-tagged",
        "Gib einen oder mehrere Filterbegriffe ein und trenne sie mit Kommas.\nDer Prompt startet mit `*`; das bedeutet „alles anzeigen“.\nAlle Begriffe wirken zusammen auf den aktuellen Dateibereich.\n\n`Häufige Begriffe`\n* `*` — alle Dateien anzeigen\n* `*.c` — Glob-Muster\n* `*.c,*.h` — mehrere Glob-Muster\n* `-*.o` — Treffer ausschließen\n* `:r` — Attributtest\n* `:x` — Attributtest\n* `>2023-01-01` — Datumstest\n* `>1M` — Größentest\n\n`Kombinationsregeln`\nStapele Begriffe mit Kommas in einem Filter, zum Beispiel `*.c,-*.tmp` oder `*.log,>2024-01-01,-debug*`.\nZitiere den Filter am Shell-Prompt nur dann, wenn deine Shell ihn vorher expandieren würde.\n\n`Bereich`\nDer Filter wirkt immer auf die aktuelle Dateilisten-Familie.\nDas kann eine normale Dateiliste, eine Archiv-Dateiliste, Showall oder Global sein.\nMit `Tab` schaltest du zwischen allen Dateien und nur markierten Dateien um, wenn der Markierungsbereich verfügbar ist.\nWenn der Markierungsbereich aktiv ist, zeigt der Prompt `FILTER [tagged only]:`.\n\n\n- [Tagged](topic:tagged)\n- [Showall](topic:showall)\n- [Global](topic:global)\n- [Command-line editing](topic:command-line-editing)",
        4,
        generated_help_links_de_filter,
    },
    {
        "compare",
        "Compare Help",
        NULL,
        "Compare startet von der aktuellen Datei, dem aktuellen Verzeichnis oder dem aktuellen geloggten Baum im aktiven Panel.\nDateivergleich prüft eine Datei gegen ein Ziel.\nVerzeichnisvergleich kann das aktuelle Verzeichnis, den geloggten Baum oder ein externes Viewer-Ziel vergleichen.\nInterner Vergleich markiert Ergebnisse nur auf der Quellseite.\n\n`Vergleichsablauf`\nWähle zuerst das Ziel.\nWähle dann den Vergleichsbereich, wenn die Quelle ein Verzeichnis ist.\nWähle dann die Vergleichsbasis, wenn die Laufzeit mehr als eine Basis anbietet.\nZum Schluss wähle die Ergebnisklasse, die auf der Quellseite markiert werden soll.\n\n`Vergleichsregeln`\n* Geloggter-Baum-Vergleich benutzt nur bereits geloggte Inhalte.\nUngeöffnete `+`-Äste werden nicht automatisch geloggt.\n* `FILEDIFF` darf `%1` und `%2` verwenden.\nFehlen diese Platzhalter, hängt ytnova Quell- und Zielpfad an den Hilfsbefehl an.\n* Ein externer Verzeichnis- oder Baumvergleich startet `DIRDIFF` oder `TREEDIFF`, statt Laufzeitergebnisse zu markieren.\n* Es gibt keinen separaten Modus „markierte Dateien vergleichen“.\n\n\n- [File mode](topic:file)\n- [Directory mode](topic:directory)\n- [Navigation](topic:ytnova-navigation)",
        3,
        generated_help_links_de_compare,
    },
    {
        "compare-target",
        "Compare Target Help",
        "prompt.compare-target",
        "Die aktuelle Datei, das aktuelle Verzeichnis oder der aktuelle geloggte Baum ist die Vergleichsquelle.\nGib genau einen Zielpfad direkt ein.\nBenutze `F2` zum Browsen.\nBenutze `Up` für die Historie.\nDrücke `F3` für den Vergleichsbereich: Datei, Verzeichnis, Baum oder externer Vergleich.\nDrücke `F4` für die Vergleichsbasis: `size`, `date`, `size+date` oder `hash`.\nDrücke `F5`, um das Ergebnis zu wählen, das nach dem Vergleich markiert wird.\nIm Split-Modus liefert das inaktive Panel das Standardziel für den Vergleich.\n\n`Zielregeln`\nGib genau einen Pfad ein.\nDer Vergleichsbereich entscheidet danach, ob dieser eine Pfad als Dateiziel, Verzeichnisziel oder geloggtes Baumziel behandelt wird.\n\n\n- [Compare Help](topic:compare)\n- [Command-line editing](topic:command-line-editing)",
        2,
        generated_help_links_de_compare_target,
    },
    {
        "compare-scope",
        "Compare Scope Help",
        NULL,
        "`Directory` vergleicht nur das aktuelle Verzeichnis.\n`Logged tree` vergleicht den aktuellen geloggten Baum und loggt ungeöffnete Äste nicht automatisch nach.\n`External viewer` startet den konfigurierten externen Vergleich statt Laufzeitergebnisse zu markieren.\n\n`Bereichswahl`\nBenutze `Directory` für eine Ebene.\nBenutze `Logged tree` für den aktuell geloggten rekursiven Baum.\nBenutze `External viewer`, wenn du ein externes Diff-Werkzeug statt markierter Vergleichsergebnisse in ytnova willst.\n\n\n- [Compare Help](topic:compare)",
        1,
        generated_help_links_de_compare_scope,
    },
    {
        "change-date",
        "Date Change Help",
        "prompt.change-date",
        "Gib das neue Datum als `YYYY-MM-DD` ein oder ergänze eine Zeit als `YYYY-MM-DD HH:MM[:SS]`.\n`F3` wechselt, ob der eingegebene Wert die Änderungszeit, die Zugriffszeit oder beide aktualisiert.\nMarkierte Datumsänderungen benutzen denselben Prompt und dieselbe Bereichsumschaltung.\n\n`Bereichswahl`\n`modified` ändert nur den Zeitstempel der letzten Änderung.\n`accessed` ändert nur den Zugriffszeitstempel.\n`both` schreibt den eingegebenen Wert in beide Zeitstempel.\n\n`Formatregeln`\nWenn du den Zeitanteil weglässt, behält ytnova Stunde, Minute und Sekunde aus dem aktuellen Wert bei.\nBenutze `Up` für die Prompt-Historie und `Esc`, um ohne Änderung abzubrechen.\n\n\n- [File mode](topic:file)\n- [Directory mode](topic:directory)\n- [Command-line editing](topic:command-line-editing)",
        3,
        generated_help_links_de_change_date,
    },
    {
        "compare-basis",
        "Compare Basis Help",
        NULL,
        "`Size` vergleicht die Dateilänge.\n`Date` vergleicht die Änderungszeit.\n`siZe+date` behandelt jede Abweichung als Unterschied.\n`Hash` öffnet beide Dateien und vergleicht den Inhalt exakt, ist also langsamer.\n\n`Basiswahl`\nWähle die billigste Basis, die deine eigentliche Frage beantwortet.\nBenutze `Hash` nur dann, wenn Metadaten nicht zuverlässig genug sind.\n\n\n- [Compare Help](topic:compare)",
        1,
        generated_help_links_de_compare_basis,
    },
    {
        "compare-results",
        "Compare Result Help",
        NULL,
        "Wähle die Ergebnisklasse, die auf der Quellseite markiert werden soll.\n`diFferent` markiert Unterschiede, `Unique` markiert nur Quell-eigene Einträge.\n`Match`, `Newer`, `Older`, `Type mismatch` und `Error` markieren jeweils nur diese eine Klasse.\n\n`Ergebnismarkierung`\nDer Vergleich überschreibt keine Dateien.\nEr markiert die gewählte Ergebnisklasse auf der aktiven Quellseite, damit du diese Teilmenge danach prüfen, kopieren, verschieben oder archivieren kannst.\n\n\n- [Compare Help](topic:compare)",
        1,
        generated_help_links_de_compare_results,
    },
    {
        "execute-file",
        "Execute File Help",
        "prompt.execute-file",
        "Der Prompt beginnt mit `{}` für den Pfad der gewählten Datei. Gib den Befehl davor ein.\nLasse `{}` dort, wo der gewählte Pfad stehen soll; Ziele, Umleitungen, Pipes und andere Shell-Syntax folgen danach.\nBenutze `C-x`, um denselben Befehl einmal pro markierter Datei zu wiederholen.\n\n`Platzhalterregeln`\n`{}` steht für genau einen ausgewählten Dateipfad. Zum Beispiel: `mv {} /tmp` oder `wc {} > count`.\nWenn du den markierten Wiederholungspfad benutzt, wird derselbe Befehl einmal pro markierter Datei wiederholt.\n\n\n- [File mode](topic:file)\n- [Command-line editing](topic:command-line-editing)",
        2,
        generated_help_links_de_execute_file,
    },
    {
        "execute-dir",
        "Execute Directory Help",
        "prompt.execute-dir",
        "Der Prompt beginnt mit `{}` für den aktuellen Verzeichnispfad. Gib den Befehl davor ein.\nLasse `{}` dort, wo der Pfad stehen soll; Ziele, Umleitungen, Pipes und andere Shell-Syntax folgen danach.\nBenutze `C-x`, um denselben Befehl einmal pro markierter Datei in der aktiven Liste zu wiederholen.\n\n`Platzhalterregeln`\n`{}` steht für den aktuellen Verzeichnispfad. Zum Beispiel: `tar -cf archive.tar {}`.\nDer markierte Wiederholungspfad läuft trotzdem über markierte Dateien aus der aktiven Liste, nicht über markierte Verzeichnisse aus irgendeiner anderen Stelle.\n\n\n- [Directory mode](topic:directory)\n- [Command-line editing](topic:command-line-editing)",
        2,
        generated_help_links_de_execute_dir,
    },
    {
        "search-tagged",
        "Search Tagged Help",
        "prompt.search-tagged",
        "Gib nur den Suchtext ein.\nytnova baut `grep -i -- PATTERN {}` für dich.\nNur markierte Dateien werden durchsucht, und Nicht-Treffer werden entmarkiert.\n\n`Regeln für markierte Suche`\nBaue zuerst eine markierte Arbeitsmenge auf.\nDurchsuche dann nur diese Menge.\nDas Ergebnis ist wieder eine kleinere markierte Menge, weil Dateien ohne Treffer ihre Markierung verlieren.\n\n\n- [Tagged](topic:tagged)\n- [Command-line editing](topic:command-line-editing)",
        2,
        generated_help_links_de_search_tagged,
    },
    {
        "create-archive",
        "Create Archive Help",
        "prompt.create-archive",
        "Benutze `.tar`, `.tar.gz` oder `.tgz`, `.tar.bz2` oder `.tbz2`, `.tar.xz` oder `.txz` oder `.zip`.\nWenn Markierungen existieren, gewinnt die markierte Menge.\nWenn nichts markiert ist, archiviert ytnova die aktuelle Datei- oder Verzeichnisauswahl.\n\n`Archivierungsregeln`\nVerzeichnisauswahlen werden rekursiv archiviert.\nArchivieren nimmt zuerst die markierte Menge, weil Markieren der normale Weg ist, um einen benutzerdefinierten Archivsatz aufzubauen.\n\n\n- [Tagged](topic:tagged)\n- [Command-line editing](topic:command-line-editing)",
        2,
        generated_help_links_de_create_archive,
    },
    {
        "output",
        "Output Help",
        NULL,
        "Output exportiert Dateiinhalte in einen Pfad oder an einen Druckbefehl.\nWähle zuerst Datei oder Hardcopy und gib dann das endgültige Ziel an.\nBei Dateiausgabe schaltet `F3` zwischen `Raw`, `Framed` und `Page break` um.\n`Framed` und `Page break` fragen den Trenner ab, bevor der endgültige Dateipfad folgt.\n\n`Output-Modell`\n`Output` ist ein Exportablauf, kein Editor.\nEr kann reinen Inhalt, gerahmten Inhalt oder durch Seitentrenner getrennten Inhalt ausgeben.\nEr kann diese Ausgabe auch an einen Druckbefehl schicken statt an einen Dateipfad.\n\n`Prompt-Reihenfolge`\nWähle zuerst Datei oder Hardcopy.\nIm Datei-Ziel-Prompt schaltet `F3` zwischen `Raw`, `Framed` und `Page break` um.\nWenn `Framed` oder `Page break` aktiv ist, wähle zuerst den Trenner und gib danach den endgültigen Dateipfad ein.\nHardcopy fragt nur nach dem Druckbefehl.\n\n\n- [File mode](topic:file)\n- [Archive file](topic:archive-file)\n- [Command-line editing](topic:command-line-editing)",
        3,
        generated_help_links_de_output,
    },
    {
        "output-format",
        "Output Format Help",
        NULL,
        "`Raw` schreibt Inhalt ohne zusätzlichen Rahmen.\n`Framed` fügt Kopf- oder Fußzeilen pro Datei hinzu.\n`Page break` fügt zwischen Dateien einen Trenner ein und lässt am Ende keinen zusätzlichen Trenner stehen.\n\n`Formatwahl`\nBenutze `Raw`, wenn ein anderes Werkzeug die Ausgabe parsen soll.\nBenutze `Framed` oder `Page break`, wenn ein Mensch den exportierten Satz lesen soll.\n\n\n- [Output Help](topic:output)",
        1,
        generated_help_links_de_output_format,
    },
    {
        "output-destination",
        "Output Destination Help",
        "prompt.output-destination",
        "Wähle zuerst Datei oder Hardcopy und gib dieses Ziel dann genau so ein, wie ytnova es benutzen soll.\nDateiausgabe schreibt in einen Pfad, und einfache Dateinamen landen in `CWD`.\nHardcopy schickt rohe Ausgabe an einen Druckbefehl wie `lpr`, `lp` oder `cat > /dev/lp1`.\nDrücke `F3` im Datei-Ziel-Prompt, um zwischen `Raw`, `Framed` und `Page break` umzuschalten.\n\n`Zielarten`\n`Dateiausgabe`: Exportierten Text in einen Pfad schreiben.\n`CWD`: Das aktuelle Arbeitsverzeichnis für einfache Dateinamen benutzen.\n`Hardcopy`: Rohe Ausgabe an einen Shell-Druckbefehl wie `lpr`, `lp` oder `cat > /dev/lp1` schicken.\n\n`Formatumschaltung`\n`F3` ist nur im Datei-Ziel-Prompt verfügbar.\nWenn `Framed` oder `Page break` gewählt ist, fragt ytnova zuerst den Trenner und kehrt dann zum Datei-Prompt zurück.\n\n\n- [Output Help](topic:output)\n- [Output Format Help](topic:output-format)\n- [Command-line editing](topic:command-line-editing)",
        3,
        generated_help_links_de_output_destination,
    },
    {
        "output-separator",
        "Output Separator Help",
        "prompt.output-separator",
        "Dieser Prompt erscheint nur, wenn `F3` `Framed` oder `Page break` auswählt.\nLasse ihn leer, um den Standard mit dreifachen Backticks zu nehmen.\nRaw-Ausgabe überspringt diesen Prompt.\n\n`Trennerregeln`\nDer Trenner wird zwischen Dateien für den aktuellen Framed- oder Page-break-Export wiederverwendet.\nNach der letzten Datei wird er nicht angehängt.\n\n\n- [Output Help](topic:output)",
        1,
        generated_help_links_de_output_separator,
    },
    {
        "showall",
        "Showall Help",
        "main.showall",
        "Showall sammelt alle Dateien des aktuellen geloggten Volumes.\nBenutze diese Seite für die Regeln der Sammelansicht und ihre Footer-Befehle.\n\n`Ansicht und Bereich`\n`Scope`: Showall listet jede Datei nur innerhalb des aktuellen geloggten Volumes auf.\n`Return`: Zum zuvor gewählten Verzeichnis zurückkehren.\n`Open owner`: Zum Besitzerverzeichnis der gewählten Datei im aktuellen Volume springen.\n`1`: Nur Name.\nDas ist die schlichte Standardansicht.\n`2`: Attribute.\nIn Dateilisten zeigt das auch `Name -> Ziel` bei Symlinks.\n`3`: Eigentümer.\n`4`: Zeiten.\n`Zurücksetzen`: `1` bringt zur schlichten Namensansicht zurück.\nWenn `2`, `3` oder `4` schon aktiv ist, springt dieselbe Taste ebenfalls zurück zu Name.\n`Gemeinsam pro Panel`: Standardmäßig sind `1..4` innerhalb eines Panels gekoppelt.\nMit `SEPARATE_DIR_FILE_VIEWS=1` trennst du Showall-/Dateifenster und Baum-/Verzeichnis-Grundansichten wieder.\n`5`: Compact nur aus der aktuellen `1`-Namensansicht umschalten.\n`6`: Datei- und Verzeichniszeilen zwischen lesbaren und rohen Größeneinheiten umschalten.\nDie Statistik bleibt lesbar.\n`7`: Mini-Vorschautext auf jeder sichtbaren Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n`8`: Datei-Detailtext auf jeder sichtbaren Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n`9`: Das Git-Band einschalten, wenn das aktuelle Verzeichnis in einem Git-Worktree liegt.\n`0`: Derzeit unbenutzt; macht nichts.\n`Sort`: `S` ändert die Sortierung, ohne Showall zu verlassen.\n`Jump`: Drücke `/`, tippe Buchstaben und drücke `Enter`, um auf dem besten sichtbaren Treffer zu landen.\n`Dotfiles`: Versteckte Dateien in der aktuellen Showall-Ergebnismenge ein- oder ausblenden.\n\n`Arbeitsmenge`\n`Filter`: Die aktuelle Showall-Ergebnismenge filtern.\n`C-s` durchsucht dort nur markierte Dateien.\nIm Prompt verengt `Tab` dieselbe Ergebnismenge auf nur markierte Zeilen.\n`Tag`: Die gewählte Datei markieren, und `C-t` markiert alle sichtbaren Dateien in der aktuellen Showall-Ergebnismenge.\n`Untag`: Die Markierung der gewählten Datei entfernen, und `C-u` löscht Markierungen in der aktuellen Showall-Ergebnismenge.\n`Invert Tags`: Den Markierungszustand in der sichtbaren Showall-Ergebnismenge umkehren.\n`Archive`: Zuerst die markierte Menge archivieren oder, wenn nichts markiert ist, die aktuelle Auswahl.\n\n`Dateiaktionen`\n`Attributes`: Das Attribut-Untermenü für die gewählte Datei öffnen.\n`Copy`: `C` kopiert die gewählte Datei, und `C-k` kopiert die markierte Menge über denselben Prompt.\n`Move`: `M` verschiebt die gewählte Datei, und `C-n` verschiebt die markierte Menge über denselben Prompt.\n`View`: Die gewählte Datei anzeigen, und `C-v` zeigt die markierten Dateien nacheinander an.\n`Edit`: Die gewählte Datei im konfigurierten Editor öffnen.\n`Hex`: Die gewählte Datei in der Hex-Ansicht öffnen.\n`Compare`: Die gewählte Datei mit einer anderen Datei vergleichen.\n`Output`: Die Auswahl exportieren.\n`C-o` benutzt dieselben Prompts für die markierte Menge, und `C-w` bleibt ein Legacy-Alias.\n`Execute`: Einen Shell-Befehl eingeben.\nGib den Befehl vor dem vorausgefüllten `{}`-Platzhalter ein. `C-x` wiederholt ihn einmal pro markierter Datei.\n`Pathcopy`: Die gewählte Datei kopieren und ihren Pfad relativ zur aktuellen Volume-Wurzel erhalten.\n`Pipe`: Einen Shell-Befehl eingeben und ihm den Inhalt der gewählten Datei über Standard-Eingabe zuführen.\n`New File`: Eine neue leere Datei anlegen.\n`Rename`: Die gewählte Datei umbenennen.\n`Delete`: Die gewählte Datei löschen.\n`Log`: Ein neues Verzeichnis oder eine Archivdatei loggen, ohne Showall zu verlassen.\n`Volume`: Die Volume-Auswahl öffnen.\n`Quit`: ytnova beenden.\n\n`Showall-Funktionstasten`\n`F1`: Kontextuelle Hilfe für die aktuelle Showall-Oberfläche öffnen.\n`F5`: Das aktive Panel aktualisieren.\n`F6`: Die Statistik- oder Detaildarstellung des aktiven Panels ändern.\n`F7`: Die Vorschau für den aktuellen Dateikontext umschalten.\n`F8`: Den Split-Screen-Modus umschalten.\n`F9`: Das Anwendungsmenü öffnen.\n`F10`: Die Konfigurationsoberfläche öffnen.\n\n\n- [Navigation](topic:ytnova-navigation)\n- [Markiert](topic:tagged)\n- [Kopierziele](topic:copy-move-targets)\n- [Filter](topic:filter)\n- [Vergleich](topic:compare)\n- [Verschiebeziele](topic:copy-move-targets)\n- [Output](topic:output)",
        7,
        generated_help_links_de_showall,
    },
    {
        "global",
        "Global Help",
        "main.global",
        "Global sammelt Dateien aus allen geloggten Volumes.\nBenutze diese Seite für die volumenübergreifenden Regeln und ihre Footer-Befehle.\n\n`Ansicht und Bereich`\n`Scope`: Global listet Dateien aus allen geloggten Volumes auf.\n`Return`: Zum zuvor gewählten Verzeichnis zurückkehren.\n`Open owner`: Zum Besitzerverzeichnis der gewählten Datei springen, auch wenn es unter einer anderen Volume-Wurzel liegt.\n`1`: Nur Name.\nDas ist die schlichte Standardansicht.\n`2`: Attribute.\nIn Dateilisten zeigt das auch `Name -> Ziel` bei Symlinks.\n`3`: Eigentümer.\n`4`: Zeiten.\n`Zurücksetzen`: `1` bringt zur schlichten Namensansicht zurück.\nWenn `2`, `3` oder `4` schon aktiv ist, springt dieselbe Taste ebenfalls zurück zu Name.\n`Gemeinsam pro Panel`: Standardmäßig sind `1..4` innerhalb eines Panels gekoppelt.\nMit `SEPARATE_DIR_FILE_VIEWS=1` trennst du Global-/Dateifenster und Baum-/Verzeichnis-Grundansichten wieder.\n`5`: Compact nur aus der aktuellen `1`-Namensansicht umschalten.\n`6`: Datei- und Verzeichniszeilen zwischen lesbaren und rohen Größeneinheiten umschalten.\nDie Statistik bleibt lesbar.\n`7`: Mini-Vorschautext auf jeder sichtbaren Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n`8`: Datei-Detailtext auf jeder sichtbaren Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n`9`: Das Git-Band einschalten, wenn das aktuelle Verzeichnis in einem Git-Worktree liegt.\n`0`: Derzeit unbenutzt; macht nichts.\n`Sort`: `S` ändert die Sortierung, ohne Global zu verlassen.\n`Jump`: Drücke `/`, tippe Buchstaben und drücke `Enter`, um auf dem besten sichtbaren Treffer zu landen.\n`Dotfiles`: Versteckte Dateien in der aktuellen Global-Ergebnismenge ein- oder ausblenden.\n\n`Arbeitsmenge`\n`Filter`: Die aktuelle Global-Ergebnismenge filtern.\n`C-s` durchsucht dort nur markierte Dateien.\nIm Prompt verengt `Tab` dieselbe Ergebnismenge auf nur markierte Zeilen.\n`Tag`: Die gewählte Datei markieren, und `C-t` markiert alle sichtbaren Dateien in der aktuellen Global-Ergebnismenge.\n`Untag`: Die Markierung der gewählten Datei entfernen, und `C-u` löscht Markierungen in der aktuellen Global-Ergebnismenge.\n`Invert Tags`: Den Markierungszustand in der sichtbaren Global-Ergebnismenge umkehren.\n`Archive`: Zuerst die markierte Menge archivieren oder, wenn nichts markiert ist, die aktuelle Auswahl.\n\n`Dateiaktionen`\n`Attributes`: Das Attribut-Untermenü für die gewählte Datei öffnen.\n`Copy`: `C` kopiert die gewählte Datei, und `C-k` kopiert die markierte Menge über denselben Prompt.\n`Move`: `M` verschiebt die gewählte Datei, und `C-n` verschiebt die markierte Menge über denselben Prompt.\n`View`: Die gewählte Datei anzeigen, und `C-v` zeigt die markierten Dateien nacheinander an.\n`Edit`: Die gewählte Datei im konfigurierten Editor öffnen.\n`Hex`: Die gewählte Datei in der Hex-Ansicht öffnen.\n`Compare`: Die gewählte Datei mit einer anderen Datei vergleichen.\n`Output`: Die Auswahl exportieren.\n`C-o` benutzt dieselben Prompts für die markierte Menge, und `C-w` bleibt ein Legacy-Alias.\n`Execute`: Einen Shell-Befehl eingeben.\nGib den Befehl vor dem vorausgefüllten `{}`-Platzhalter ein. `C-x` wiederholt ihn einmal pro markierter Datei.\n`Pathcopy`: Die gewählte Datei kopieren und ihren Pfad relativ zur Volume-Wurzel behalten.\n`Pipe`: Einen Shell-Befehl eingeben und ihm den Inhalt der gewählten Datei über Standard-Eingabe zuführen.\n`New File`: Eine neue leere Datei anlegen.\n`Rename`: Die gewählte Datei umbenennen.\n`Delete`: Die gewählte Datei löschen.\n`Log`: Ein neues Verzeichnis oder eine Archivdatei loggen, ohne Global zu verlassen.\n`Volume`: Die Volume-Auswahl öffnen.\n`Quit`: ytnova beenden.\n\n`Global-Funktionstasten`\n`F1`: Kontextuelle Hilfe für die aktuelle Global-Oberfläche öffnen.\n`F5`: Das aktive Panel aktualisieren.\n`F6`: Die Statistik- oder Detaildarstellung des aktiven Panels ändern.\n`F7`: Die Vorschau für den aktuellen Dateikontext umschalten.\n`F8`: Den Split-Screen-Modus umschalten.\n`F9`: Das Anwendungsmenü öffnen.\n`F10`: Die Konfigurationsoberfläche öffnen.\n\n\n- [Navigation](topic:ytnova-navigation)\n- [Markiert](topic:tagged)\n- [Kopierziele](topic:copy-move-targets)\n- [Filter](topic:filter)\n- [Vergleich](topic:compare)\n- [Verschiebeziele](topic:copy-move-targets)\n- [Output](topic:output)",
        7,
        generated_help_links_de_global,
    },
    {
        "f7",
        "F7 Preview Help",
        "overlay.f7-dir,overlay.f7-file",
        "Die Vorschau hält die aktuelle Datei offen, während ein kleinerer Befehlssatz aktiv bleibt.\nBenutze diese Seite für die Vorschau-Regeln und die Befehle, die ohne Verlassen der Vorschau weiterlaufen.\n\n`Vorschau-Regeln`\n`F7`: Zur darunterliegenden Verzeichnis- oder Dateiansicht zurückkehren.\n`F8`: Split hat keine Wirkung, solange die Vorschau aktiv ist.\n`F9`: Das Anwendungsmenü öffnen, ohne die Vorschau zu verlassen.\n`Tab`: Panels nicht wechseln, solange die Vorschau aktiv ist.\n`Esc`: Die Vorschau sofort verlassen.\n\n`Arbeitsmenge`\n`Filter`: Die aktuelle Vorschau-Liste filtern.\n`C-s` durchsucht dort nur markierte Dateien.\n`Tag`: Die gewählte Datei markieren, und `C-t` markiert alle sichtbaren Dateien im aktuellen Bereich.\n`Untag`: Die Markierung der gewählten Datei entfernen, und `C-u` löscht Markierungen im aktuellen Bereich.\n`Invert Tags`: Den Markierungszustand im sichtbaren Bereich umkehren.\n`Archive`: Zuerst die markierte Menge archivieren oder, wenn nichts markiert ist, die aktuelle Auswahl.\n`Jump`: Drücke `/`, tippe Buchstaben und drücke `Enter`, um auf dem besten sichtbaren Treffer zu landen.\n`Dotfiles`: Versteckte Dateien im aktuellen Vorschau-Bereich ein- oder ausblenden.\n\n`Dateiaktionen`\n`Attributes`: Das Attribut-Untermenü für die gewählte Datei öffnen.\n`Copy`: `C` kopiert die gewählte Datei, und `C-k` kopiert die markierte Menge über denselben Prompt.\n`Move`: `M` verschiebt die gewählte Datei, und `C-n` verschiebt die markierte Menge über denselben Prompt.\n`View`: Die gewählte Datei anzeigen, und `C-v` zeigt die markierten Dateien nacheinander an.\n`Edit`: Die gewählte Datei im konfigurierten Editor öffnen.\n`Compare`: Die gewählte Datei mit einer anderen Datei vergleichen.\n`Output`: Die Auswahl exportieren.\n`C-o` benutzt dieselben Prompts für die markierte Menge, und `C-w` bleibt ein Legacy-Alias.\n`Execute`: Einen Shell-Befehl eingeben.\nGib den Befehl vor dem vorausgefüllten `{}`-Platzhalter ein. `C-x` wiederholt ihn einmal pro markierter Datei.\n`Pathcopy`: Die gewählte Datei kopieren und ihren Pfad relativ zur aktuellen Volume-Wurzel erhalten.\n`Pipe`: Einen Shell-Befehl eingeben und ihm den Inhalt der gewählten Datei über Standard-Eingabe zuführen.\n`New File`: Eine neue leere Datei anlegen, ohne die Vorschau zu verlassen.\n`Rename`: Die gewählte Datei umbenennen, ohne die Vorschau zu verlassen.\n`Delete`: Die gewählte Datei löschen, ohne die Vorschau zu verlassen.\n\n\n- [Navigation](topic:ytnova-navigation)\n- [Markiert](topic:tagged)\n- [Kopier-/Verschiebeziele](topic:copy-move-targets)\n- [Filter](topic:filter)\n- [Vergleich](topic:compare)\n- [Anwendungsmenue](topic:applications-menu)\n- [Output](topic:output)",
        7,
        generated_help_links_de_f7,
    },
    {
        "f8",
        "F8 Split Help",
        NULL,
        "Der Split-Modus hält beide Panels aktiv.\nLaufzeit-`F1` öffnet die Split-Seite des aktiven Panels; diese Seite erklärt die gemeinsamen Split-Regeln.\n\n`Split-Regeln`\n`F8`: Zum Ein-Panel-Modus zurückkehren.\n`Tab`: Das aktive Panel wechseln und den Zustand des passiven Panels behalten.\n`Target defaults`: Copy-, Move- und Compare-Prompts nehmen das inaktive Panel als Standardziel.\n`Panel independence`: Jedes Panel behält eigene Auswahl, Ansicht, Markierungen, Volume und Restore-Zustand.\n\n\n- [Navigation](topic:ytnova-navigation)\n- [Directory split page](topic:f8-dir)\n- [File split page](topic:f8-file)",
        3,
        generated_help_links_de_f8,
    },
    {
        "f8-dir",
        "F8 Split Directory Help",
        "overlay.f8-dir",
        "Benutze diese Seite für den Footer des aktiven Split-Verzeichnisses.\nSie verbindet die gemeinsamen Split-Regeln mit den Befehlen des aktiven Panels.\n\n`Split-Regeln`\n`F8`: Zum Ein-Panel-Modus zurückkehren.\n`Tab`: Das aktive Panel wechseln und den Zustand des passiven Panels behalten.\n`Target defaults`: Copy-, Move- und Compare-Prompts nehmen das inaktive Panel als Standardziel.\n`Panel independence`: Jedes Panel behält eigene Auswahl, Ansicht, Markierungen, Volume und Restore-Zustand.\n\n`Ansicht und Bereich`\n`1`: Nur Name.\nDas ist die schlichte Standardansicht.\n`2`: Attribute.\nIn Dateilisten zeigt das auch `Name -> Ziel` bei Symlinks.\n`3`: Eigentümer.\n`4`: Zeiten.\n`Zurücksetzen`: `1` bringt zur schlichten Namensansicht zurück.\nWenn `2`, `3` oder `4` schon aktiv ist, springt dieselbe Taste ebenfalls zurück zu Name.\n`Gemeinsam pro Panel`: Standardmäßig sind `1..4` innerhalb eines Panels gekoppelt.\nWenn du also die Split-Baumansicht änderst, ändert sich auch das Dateifenster dieses Panels.\nMit `SEPARATE_DIR_FILE_VIEWS=1` trennst du beide wieder.\n`Baum gegen Dateifenster`: Im Split-Verzeichnisfokus ändern `5`, `7`, `8` und `9` nicht die Baumzeilen.\nSie ändern das Dateifenster des aktiven Panels.\n`5`: Compact nur aus der aktuellen `1`-Namensansicht umschalten.\n`6`: Datei- und Verzeichniszeilen zwischen lesbaren und rohen Größeneinheiten umschalten.\nDie Statistik bleibt lesbar.\n`7`: Mini-Vorschautext auf jeder sichtbaren Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n`8`: Datei-Detailtext auf jeder sichtbaren Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n`9`: Das Git-Band einschalten, wenn das aktuelle Verzeichnis in einem Git-Worktree liegt.\n`0`: Derzeit unbenutzt; macht nichts.\n`Showall`: Die Sammelliste des aktuellen Volumes für das aktive Panel öffnen.\n`Global`: Die Sammelliste über mehrere Volumes für das aktive Panel öffnen.\n`Jump`: Drücke `/`, tippe Buchstaben und drücke `Enter`, um auf dem besten sichtbaren Treffer im aktiven Baum zu landen.\n`Dotfiles`: Versteckte Namen im aktiven Baum ein- oder ausblenden.\n\n`Arbeitsmenge`\n`Filter`: Den aktuellen Dateibereich filtern.\nIm Prompt schaltet `Tab` zwischen allen Dateien und nur markierten Dateien um, wenn Markierungen existieren.\n`Tag`: Dateien im gewählten Verzeichnisbereich markieren.\n`Untag`: Dateien im gewählten Verzeichnisbereich entmarkieren.\n`Invert Tags`: Den Markierungszustand im sichtbaren Bereich umkehren.\n`Archive`: Zuerst die markierte Menge archivieren oder, wenn nichts markiert ist, die aktuelle Auswahl.\n\n`Verzeichnisaktionen`\n`Attributes`: Das Attribut-Untermenü für das gewählte Verzeichnis öffnen.\n`Copy`: Den gewählten Verzeichniszweig kopieren.\nIm Split-Modus ist das inaktive Panel das Standardziel.\n`MoveDir`: Den gewählten Verzeichniszweig verschieben.\nIm Split-Modus ist das inaktive Panel das Standardziel.\n`Compare`: Das gewählte Verzeichnis, den aktuellen geloggten Baum oder ein anderes Ziel vergleichen.\n`Output`: Die aktuelle Auswahl über den Output-Ablauf exportieren.\n`Execute`: Einen Shell-Befehl eingeben.\nSetze `{}` dort ein, wo der gewählte Verzeichnispfad stehen soll, und lasse `{}` unzitiert, damit ytnova den Pfad sicher quoten kann.\n`Pipe`: Einen Shell-Befehl eingeben. ytnova führt ihn im gewählten Verzeichnis aus und schickt die sichtbaren passenden Namen zeilenweise an seine Standard-Eingabe.\n`Makedir`: Ein neues Verzeichnis anlegen.\n`New File`: Eine neue leere Datei im aktuellen Verzeichnis anlegen.\n`Rename`: Das gewählte Verzeichnis umbenennen.\n`Delete`: Das gewählte Verzeichnis löschen.\n`Log`: Ein neues Verzeichnis oder eine Archivdatei loggen oder einen bereits geloggten Pfad von oben neu laden.\n`Volume`: Die Volume-Auswahl öffnen.\n`Quit`: ytnova beenden.\n\n`Split-Verzeichnis-Funktionstasten`\n`F1`: Kontextuelle Hilfe für die aktive Split-Verzeichnisoberfläche öffnen.\n`F5`: Das aktive Panel aktualisieren.\n`F6`: Die Statistik- oder Detaildarstellung des aktiven Panels ändern.\n`F7`: Die Vorschau für den aktuellen Dateikontext umschalten.\n`F8`: Zum Ein-Panel-Modus zurückkehren.\n`F9`: Das Anwendungsmenü öffnen.\n`F10`: Die Konfigurationsoberfläche öffnen.\n\n\n- [Navigation](topic:ytnova-navigation)\n- [F8-Split](topic:f8)\n- [Kopierziele](topic:copy-move-targets)\n- [Filter](topic:filter)\n- [Vergleich](topic:compare)\n- [Verschiebeziele](topic:copy-move-targets)\n- [Output](topic:output)",
        7,
        generated_help_links_de_f8_dir,
    },
    {
        "f8-file",
        "F8 Split File Help",
        "overlay.f8-file",
        "Benutze diese Seite für den Footer der aktiven Split-Dateiliste.\nSie verbindet die gemeinsamen Split-Regeln mit den Befehlen des aktiven Panels.\n\n`Split-Regeln`\n`F8`: Zum Ein-Panel-Modus zurückkehren.\n`Tab`: Das aktive Panel wechseln und den Zustand des passiven Panels behalten.\n`Target defaults`: Copy-, Move- und Compare-Prompts nehmen das inaktive Panel als Standardziel.\n`Panel independence`: Jedes Panel behält eigene Auswahl, Ansicht, Markierungen, Volume und Restore-Zustand.\n\n`Ansicht und Bereich`\n`1`: Nur Name.\nDas ist die schlichte Standardansicht.\n`2`: Attribute.\nIn Dateilisten zeigt das auch `Name -> Ziel` bei Symlinks.\n`3`: Eigentümer.\n`4`: Zeiten.\n`Zurücksetzen`: `1` bringt immer zur schlichten Namensansicht zurück.\nWenn `2`, `3` oder `4` schon aktiv ist, springt dieselbe Taste ebenfalls zurück zu Name.\n`Gemeinsam pro Panel`: Standardmäßig sind `1..4` innerhalb eines Panels gekoppelt.\nEine Split-Baum-Änderung ändert also auch das Dateifenster dieses Panels.\nMit `SEPARATE_DIR_FILE_VIEWS=1` machst du beide wieder unabhängig.\n`5`: Compact nur aus der aktuellen `1`-Namensansicht umschalten.\n`6`: Datei- und Verzeichniszeilen zwischen lesbaren und rohen Größeneinheiten umschalten.\nDie Statistik bleibt lesbar.\n`7`: Mini-Vorschautext auf jeder sichtbaren Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n`8`: Datei-Detailtext auf jeder sichtbaren Dateizeile zeigen und Compact verlassen, damit du ihn sehen kannst.\n`9`: Das Git-Band einschalten, wenn das aktuelle Verzeichnis in einem Git-Worktree liegt.\n`Zusatzstatus`: `5`, `7`, `8` und `9` stapeln sich nicht im Statistiklabel; dort steht immer nur der eine sichtbare Zusatzstatus.\n`0`: Derzeit unbenutzt; macht nichts.\n`Sort`: Die Sortierung der aktiven Dateiliste ändern.\n`Jump`: Drücke `/`, tippe Buchstaben und drücke `Enter`, um auf dem besten sichtbaren Treffer zu landen.\n`Dotfiles`: Versteckte Dateien im aktuellen Bereich ein- oder ausblenden.\n\n`Arbeitsmenge`\n`Filter`: Die aktuelle Liste filtern.\n`C-s` durchsucht nur markierte Dateien.\nIm Prompt schaltet `Tab` zwischen allen Dateien und nur markierten Dateien um, wenn Markierungen existieren.\n`Tag`: Die gewählte Datei markieren, und `C-t` markiert alle sichtbaren Dateien im aktuellen Bereich.\n`Untag`: Die Markierung der gewählten Datei entfernen, und `C-u` löscht Markierungen im aktuellen Bereich.\n`Invert Tags`: Den Markierungszustand im sichtbaren Bereich umkehren.\n`Archive`: Zuerst die markierte Menge archivieren oder, wenn nichts markiert ist, die aktuelle Auswahl.\n\n`Dateiaktionen`\n`Attributes`: Das Attribut-Untermenü für die gewählte Datei öffnen.\n`Copy`: `C` kopiert die gewählte Datei, und `C-k` kopiert die markierte Menge über denselben Prompt.\nIm Split-Modus ist das inaktive Panel das Standardziel.\n`Move`: `M` verschiebt die gewählte Datei, und `C-n` verschiebt die markierte Menge über denselben Prompt.\nIm Split-Modus ist das inaktive Panel das Standardziel.\n`View`: Die gewählte Datei anzeigen, und `C-v` zeigt die markierten Dateien nacheinander an.\n`Edit`: Die gewählte Datei im konfigurierten Editor öffnen.\n`Hex`: Die gewählte Datei in der Hex-Ansicht öffnen.\n`Compare`: Die gewählte Datei mit einer anderen Datei vergleichen.\n`Output`: Die Auswahl exportieren.\n`C-o` benutzt dieselben Prompts für die markierte Menge, und `C-w` bleibt ein Legacy-Alias.\n`Execute`: Einen Shell-Befehl eingeben.\nGib den Befehl vor dem vorausgefüllten `{}`-Platzhalter ein. `C-x` wiederholt ihn einmal pro markierter Datei.\n`Pathcopy`: Die gewählte Datei kopieren und ihren Pfad relativ zur aktuellen Volume-Wurzel erhalten.\n`Pipe`: Einen Shell-Befehl eingeben und ihm den Inhalt der gewählten Datei über Standard-Eingabe zuführen.\n`New File`: Eine neue leere Datei anlegen.\n`Rename`: Die gewählte Datei umbenennen.\n`Delete`: Die gewählte Datei löschen.\n`Log`: Ein neues Verzeichnis oder eine Archivdatei loggen, ohne den Split-Dateimodus zu verlassen.\n`Volume`: Die Volume-Auswahl öffnen.\n`Quit`: ytnova beenden.\n\n`Split-Datei-Funktionstasten`\n`F1`: Kontextuelle Hilfe für die aktive Split-Dateioberfläche öffnen.\n`F5`: Das aktive Panel aktualisieren.\n`F6`: Die Statistik- oder Detaildarstellung des aktiven Panels ändern.\n`F7`: Die Vorschau für den aktuellen Dateikontext umschalten.\n`F8`: Zum Ein-Panel-Modus zurückkehren.\n`F9`: Das Anwendungsmenü öffnen.\n`F10`: Die Konfigurationsoberfläche öffnen.\n\n\n- [Navigation](topic:ytnova-navigation)\n- [F8-Split](topic:f8)\n- [Markiert](topic:tagged)\n- [Kopierziele](topic:copy-move-targets)\n- [Filter](topic:filter)\n- [Vergleich](topic:compare)\n- [Verschiebeziele](topic:copy-move-targets)\n- [Output](topic:output)",
        8,
        generated_help_links_de_f8_file,
    },
    {
        "history-dialog",
        "History Help",
        "dialog.history",
        "Benutze `Up` und `Down`, um einen Eintrag zu wählen.\nBenutze `Left` und `Right`, um einen langen Eintrag horizontal zu verschieben.\nBenutze `P` zum Anheften oder Lösen.\nBenutze `D` zum Löschen.\nBenutze `Enter` zum Übernehmen.\nBenutze `Esc` zum Abbrechen.\n\n`Historienaktionen`\n`Select entry`: `Up` und `Down` bewegen sich durch die aktuelle Historienliste.\n`Scroll long entry`: `Left` und `Right` verschieben eine lange Historienzeile horizontal.\n`Pin`: `P` hält einen wichtigen Eintrag oben in der aktuellen Historienliste.\n`Delete`: `D` entfernt den gewählten Eintrag aus der aktuellen Historienliste.\n`Accept`: `Enter` übernimmt den gewählten Eintrag erneut.\n`Cancel`: `Esc` schließt den Dialog ohne Übernahme.\n\n\n- [Navigation](topic:ytnova-navigation)",
        1,
        generated_help_links_de_history_dialog,
    },
    {
        "volume-menu",
        "Volume Help",
        "dialog.volume-menu",
        "Benutze `Up` und `Down`, um ein geladenes Volume zu wählen.\nBenutze `Enter`, um dorthin zu wechseln.\nBenutze `D`, um es freizugeben, außer es ist das letzte.\nBenutze `Esc`, um das Menü zu verlassen.\n\n`Volume-Aktionen`\n`Select volume`: `Up` und `Down` bewegen sich durch die Liste geladener Volumes.\n`Switch volume`: `Enter` aktiviert das gewählte Volume.\n`Keep state`: Wenn du das bereits aktive Volume auswählst, bleibt sein Zustand im Speicher erhalten.\n`Release volume`: `D` entlädt das gewählte Volume, außer es ist das letzte verbleibende.\n`Cancel`: `Esc` schließt das Menü.\n\n\n- [Navigation](topic:ytnova-navigation)",
        1,
        generated_help_links_de_volume_menu,
    },
    {
        "applications-menu",
        "Applications Help",
        "dialog.applications",
        "Benutze `Up` und `Down`, um eine Voreinstellung zu wählen.\nBenutze `Enter`, um sie zu starten.\nBenutze `Esc`, um abzubrechen.\nBenutze `E`, um den Anwendungskatalog zu bearbeiten.\n`{}` setzt die aktuelle Datei oder den aktuellen Ordner ein.\n`{input}` setzt den zusätzlichen Prompt-Text der Voreinstellung ein.\n\n`Anwendungsaktionen`\n`Select preset`: `Up` und `Down` bewegen sich durch die Voreinstellungs-Liste.\n`Launch behavior`: `F9` startet die gewählte Voreinstellung und kehrt direkt zur TUI zurück.\nBenutze es für wiederkehrende externe Abläufe, nicht für spontane Shell-Befehle.\n`Use `eXecute` for one-offs`: Der `X`-Prompt bleibt die Ad-hoc-Shell mit Historie und Terminalausgabe.\nBenutze ihn, wenn du einen einmaligen Befehl brauchst.\n`Edit presets`: `E` öffnet den Anwendungskatalog, damit Voreinstellungen ohne Verlassen der Auswähler-Familie geändert werden können.\n`Selection and working directory`: `{}` setzt die aktuelle Datei oder den aktuellen Ordner ein.\nVoreinstellungen starten außerdem in diesem Verzeichnis, sodass Skripte auch ohne `{}` vom gewählten Ort aus laufen.\n`Prompt text`: `{input}` setzt den zusätzlichen Text ein, den du im Voreinstellungs-Prompt eingegeben hast.\n`Starter presets`: Der mitgelieferte Katalog startet mit `xdg-open`-Startern und enthält auskommentierte Beispiele für Werkzeuge wie `mpv` oder lokale Hilfsskripte.\n`Cancel menu`: `Esc` schließt den Auswähler, ohne eine Voreinstellung zu starten.\n\n\n- [Navigation](topic:ytnova-navigation)",
        1,
        generated_help_links_de_applications_menu,
    },
    {
        "f2-picker",
        "F2-Auswahl",
        "dialog.f2-picker",
        "Benutze `Up` und `Down` zum Bewegen.\nBenutze `Right`, um aufzuklappen oder in das erste Kind zu gehen.\nBenutze `Left`, um einzuklappen oder zum Elternknoten zu gehen.\nBenutze `<` und `>`, um durch geladene Volumes zu wechseln.\nBenutze `L`, um einen neuen Pfad zu protokollieren.\nBenutze `` ` ``, um Dotfiles umzuschalten.\nBenutze `Enter`, um das markierte Verzeichnis zu wählen.\nBenutze `Esc` zum Abbrechen.\n\n`Auswahlaktionen`\n`Move`: `Up` und `Down` bewegen sich durch die sichtbaren Verzeichniszeilen.\n`Expand`: `Right` klappt das aktuelle Verzeichnis um eine Ebene auf und geht dann in das erste Kind, wenn diese Ebene schon offen ist.\n`Collapse`: `Left` klappt das aktuelle Verzeichnis ein oder geht zum Elternknoten, wenn die aktuelle Zeile schon geschlossen ist.\n`Select`: `Enter` benutzt das markierte Verzeichnis für den aufrufenden Prompt.\n`Cancel`: `Esc` schließt die Auswahl, ohne den Prompt zu ändern.\n\n\n- [Navigation](topic:ytnova-navigation)\n- [Command-line editing](topic:command-line-editing)",
        2,
        generated_help_links_de_f2_picker,
    },
};

static const size_t generated_help_topic_count_de = 43;

static const GeneratedHelpCatalog generated_help_catalogs[] = {
    {
        "en",
        43,
        generated_help_topics_en,
        {
            "Left back",
            "Index",
            "I",
            "Navigation",
            "N",
            "Right/Enter follow",
            "Esc/Q quit",
        },
    },
    {
        "de",
        43,
        generated_help_topics_de,
        {
            "Links zurück",
            "Inhalt",
            "H",
            "Navigation",
            "W",
            "Right/Enter folgen",
            "Esc/Q schließen",
        },
    },
};

static const size_t generated_help_catalog_count = 2;
