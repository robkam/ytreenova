/* Auto-generated from etc/help/f1.en.md, etc/help/f1.de.md by scripts/generate_help_assets.py. */
#include <stddef.h>

typedef struct {
    const char *topic_id;
    const char *title;
    const char *contexts_csv;
    const char *contextual_f1;
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

static const GeneratedHelpTopic generated_help_topics_en[] = {
    {
        "index",
        "Help Index",
        NULL,
        "Choose a topic with `Up` and `Down`.\nPress `Enter` or `Right` to open it.\n`Left` returns to the previous help page.\n`Esc` closes help.\n\n`Navigation` in this list explains movement through YtreeNova. The help strip's `Navigation` command explains movement inside the help popup.\n\n- [Applications](topic:applications-menu)\n- [Archive Directory](topic:archive-dir)\n- [Archive File](topic:archive-file)\n- [Command-line Editing](topic:command-line-editing)\n- [Command-line Parameters](topic:command-line-parameters)\n- [Compare](topic:compare)\n- [Compare Basis](topic:compare-basis)\n- [Compare Result](topic:compare-results)\n- [Compare Scope](topic:compare-scope)\n- [Compare Target](topic:compare-target)\n- [Configuration Files](topic:configuration-files)\n- [Copy/Move Targets](topic:copy-move-targets)\n- [Create Archive](topic:create-archive)\n- [Date Change](topic:change-date)\n- [Directory](topic:directory)\n- [Execute Directory](topic:execute-dir)\n- [Execute File](topic:execute-file)\n- [F10 Config](topic:f10)\n- [F2 Picker](topic:f2-picker)\n- [F7 Preview](topic:f7)\n- [F8 Split](topic:f8)\n- [F8 Split Directory](topic:f8-dir)\n- [F8 Split File](topic:f8-file)\n- [File](topic:file)\n- [Filter](topic:filter)\n- [Global](topic:global)\n- [History](topic:history-dialog)\n- [List Jump](topic:list-jump)\n- [Navigation](topic:ytnova-navigation)\n- [Output](topic:output)\n- [Output Destination](topic:output-destination)\n- [Output Format](topic:output-format)\n- [Output Separator](topic:output-separator)\n- [Search Tagged](topic:search-tagged)\n- [Shared Commands](topic:shared-commands)\n- [Showall](topic:showall)\n- [Tagged](topic:tagged)\n- [Tagged Viewer](topic:tagged-viewer)\n- [Theming](topic:theming)\n- [Vi Keys](topic:vi-keys)\n- [Volume](topic:volume-menu)",
    },
    {
        "f1-navigation",
        "Help Navigation",
        NULL,
        "Use the `Up` and `Down` arrow keys to scroll help one line at a time.\nUse the `Up` and `Down` arrow keys to move between links.\n`Page Up` and `Page Down` move one help screen at a time.\n`Home` and `End` go to the top and bottom.\n`Enter` or the `Right` arrow key opens the selected link.\nThe `Left` arrow key returns to the previous help page.\nPress `I` to go to `Help Index`.\n`Esc` or `Q` closes help.",
    },
    {
        "ytnova-navigation",
        "YtreeNova Navigation",
        NULL,
        "YtreeNova is built for keyboard use. Mouse effects may occur, but they are incidental rather than designed controls.\nUse the `Up` and `Down` arrow keys to move one row at a time.\n`Page Up` and `Page Down` move one screen at a time.\n`Home` and `End` go to the first and last visible rows.\n`Enter` opens the selected item.\n`Right` opens or expands the selected item where that view supports it.\n`Left` goes back or collapses the selected item where that view supports it.\n[/ jump](topic:list-jump) moves to a matching name as you type in the current list. `Enter` selects it; `Esc` cancels.\n`Tab` moves to the next visible sibling in the tree and wraps at the end; `Shift-Tab` reverses it.\n[F8 split](topic:f8) mode: `Tab` switches active panels; some prompts give `Tab` a local meaning.\n[F7 preview](topic:f7) mode: ordinary navigation keys move the file selection; their shifted forms scroll the preview.\n\n`Terminal input limits`\n`Alt` is deliberately unsupported because terminals handle it inconsistently.\n`Kitty keyboard protocol`\nYtreeNova uses the kitty keyboard protocol when the active terminal path supports it. This lets it tell `C-m` from Enter, so `C-m` moves tagged files.\nIn Kitty, add `keyboard_protocol kitty` to `~/.config/kitty/kitty.conf` and restart Kitty. Other terminals that support the protocol work too.\nIf you use a multiplexer or remote session, it must pass the protocol through unchanged.\nWithout protocol support, YtreeNova silently uses `C-n` to move tagged files.\n`C-[` is Esc.",
    },
    {
        "list-jump",
        "List Jump",
        NULL,
        "Press `/`, type part of a name, and press `Enter` to keep the best visible match.\nThe selection moves as you type.\n`Esc` restores the selection you had before the jump.\n\nThe jump searches only the current visible list. In a directory tree, repeat it after entering a directory when you want to go deeper.",
    },
    {
        "shared-commands",
        "Shared Commands",
        NULL,
        "`F1` opens help for the current screen or prompt.\n`F5` refreshes the current view.\n`F6` changes the statistics or details shown for the active view.\n[F7 preview](topic:f7) opens or closes the file preview.\n[F8 split](topic:f8) opens or closes the second panel.\n`F9` opens the [Applications menu](topic:applications-menu).\n`F10` opens [configuration](topic:f10).\n`Esc` leaves the current prompt, menu, preview, or popup.\n\nThe footer shows the active bindings. `commands.conf` may change its keys and labels.\nUse `C--` and `C-+` to reduce or increase terminal text size when the footer does not fit.",
    },
    {
        "tagged",
        "Tagged",
        NULL,
        "Tags select several files, then apply one operation to the group.\nTagged files are indicated by `*` in the margin.\n\nIn a file list, press `T` to tag the selected file or `U` to untag it. The selection moves down to the next file.\nIn a directory tree, `T` and `U` affect files that the current filter allows in the selected directory, then move down to the next directory.\n\n`I` reverses tags on files that the current filter allows in the current scope.\n\nWhen statistics are shown, tagged-file totals appear there.\n\nHold the Control key with the letter after `C-` to run the matching operation on all tagged files. `^` in a footer label means the same tagged operation, as in `C/^Copy`.\n\n`C-a` opens the attributes prompt.\n`C-c` opens the [copy](topic:copy-move-targets) prompt.\n`C-d` asks before deleting tagged files.\n`C-m` opens the [move](topic:copy-move-targets) prompt when the protocol is available; without it, use `C-n`.\n`C-o` opens the [output](topic:output) prompt.\n`C-p` opens the pipe prompt.\n`C-r` opens the rename prompt.\n`C-s` [searches tagged files](topic:search-tagged) and untags files without a hit.\n`C-t` tags all files in the active file list. In a directory tree, it tags files allowed by the current filter in all logged directories.\n`C-u` untags all files in the active file list. In a directory tree, it untags files allowed by the current filter in all logged directories.\n`C-v` views tagged files one after another.\n`C-x` opens a [command prompt](topic:execute-file) and runs the operation once per tagged file.\n`C-y` [pathcopies](topic:copy-move-targets) the tagged files.\n`C-z` opens the [archive directory](topic:archive-dir) or [archive file](topic:archive-file) prompt, depending on the current selection.\n\nFor a tagged-only [filter](topic:filter) scope, press `F`, then `Tab`. This does not change tags.",
    },
    {
        "tagged-viewer",
        "Tagged Viewer",
        "viewer.tagged",
        "Use `n` and `p` to open the next or previous tagged file.\nUse `Space`, `Page Down`, and `Page Up` to move within the current file.\nAfter a tagged search, use `/` and `\?` for the next or previous hit in that file.\n`Esc` closes the viewer.\n\nWith `TAGGEDVIEWER=external`, the configured pager controls paging and search instead. See [Tagged](topic:tagged) for creating and using the tagged set.",
    },
    {
        "command-line-editing",
        "Command-line Editing",
        NULL,
        "Most prompts use the same editing keys.\n`Left` and `Right` move one character.\n`Home` and `End` go to the start or end.\n`C-a` and `C-e` do the same.\n`Backspace` or `C-h` deletes the character to the left.\n`Delete` or `C-d` deletes the character under the cursor.\n`C-w` deletes the word to the left.\n`C-u` deletes back to the start.\n`C-k` deletes to the end.\n`Up` opens or cycles saved values when the prompt keeps history.\n`F2` opens the [directory picker](topic:f2-picker) when browsing is available.\n`F1` opens help for the current prompt.\n`Enter` accepts the value.\n`Esc` cancels without accepting it.",
    },
    {
        "command-line-parameters",
        "Command-line Parameters",
        NULL,
        "Give ytnova one or more directory or archive paths to log them at startup. With no path, it logs the current directory.\n\n`-d depth` sets the startup scan depth. Use a number, `min` or `root` for zero, or `max` or `all` for 100.\n`-f filter` starts with a [file filter](topic:filter). Quote shell patterns such as `\"*.c\"` so the shell does not expand them first.\n`-h history_file` selects another command-history file.\n`-p config_file` selects another main configuration file.\n`--init` creates missing starter [configuration files](topic:configuration-files) and exits.\n`-v`, `-V`, or `--version` prints the version and exits.",
    },
    {
        "configuration-files",
        "Configuration Files",
        NULL,
        "User configuration normally lives under `~/.config/ytnova`.\n`ytnova.conf` contains profile settings such as scan depth, `VI_KEYS`, and `SEPARATE_DIR_FILE_VIEWS`.\n`commands.conf` contains command labels, bindings, and preset selection.\n`themes.conf` selects a theme and overrides theme roles.\n`applications.conf` contains the presets opened by `F9`.\nCommand history is stored separately; use `-h` to select another history file.\n\nUse `-p` to select another `ytnova.conf`. Existing legacy home files and packaged defaults remain fallbacks. Use [F10 configuration](topic:f10) to edit the active files.",
    },
    {
        "copy-move-targets",
        "Copy/Move Targets",
        NULL,
        "`Copy`, `move`, and `pathcopy` first ask for a name or rename pattern, then for the destination directory.\n\nWith one file, leave its autofilled name to keep it, or edit it to rename it. You can use wildcards in either case.\n\nFor [Tagged](topic:tagged) files, leave the autofilled `*` to keep their names, or enter a rename pattern:\n`*` keeps the remaining original name. Use `copy-*` to add a prefix or `*.bak` to change the extension.\n`\?` keeps one character from the original name. For example, `\?\?-*` keeps the first two characters, adds `-`, then keeps the rest.\nOther characters are used as written.\n\nUse `Up` to reuse a previous name or pattern.\n\nEnter the destination directory in `To Directory`. In [F8 split](topic:f8) mode, it starts as the other panel’s currently selected directory.\n\nUse `Up` to reuse a previous destination, or press [F2](topic:f2-picker) to choose one.\n\nThe same prompts apply to [archive files](topic:archive-file) and [archive directories](topic:archive-dir). Copying an archive file extracts it as needed; moving it copies it, then removes the original. You can also copy into a logged archive.\n\n`pathcopy` recreates the selected file’s existing path below the destination directory.\n\nIf the destination directory is missing, ytnova asks whether to create it. If a destination file already exists, ytnova asks before replacing it.",
    },
    {
        "vi-keys",
        "Vi Keys",
        NULL,
        "Set `VI_KEYS=1` to use lowercase vi movement keys.\n`h`, `j`, `k`, and `l` become `Left`, `Down`, `Up`, and `Right`.\n`C-u` and `C-d` become page up and page down.\n\nCommands that would collide move to another binding. The footer shows the active keys; examples include `J` for compare, `K` for the volume menu, `D` for deleting tagged files, and `U` for untagging all.",
    },
    {
        "f10",
        "F10 Config Help",
        NULL,
        "Press `F10` when you want to change persistent setup rather than perform a file operation.\nChoose the profile command to edit `ytnova.conf`.\nChoose the commands entry to edit labels and bindings in `commands.conf`.\nChoose the theme entry to edit `themes.conf`.\nChoose reload to apply saved configuration again.\n\nSee [Configuration Files](topic:configuration-files) for file locations and [Theming](topic:theming) for help roles.",
    },
    {
        "theming",
        "Theming",
        NULL,
        "Themes assign colours and attributes to named interface roles.\n`footer` styles the main program footer.\n`help` styles the F1 text body.\n`help_footer` styles the help strip.\n`help_keybind` highlights its bound letter.\n`help_heading` styles help titles and headings.\n`help_topic` styles backticked keys and terms.\n`help_link` and `help_link_selection` style links and the selected link.\n`help_box_lines` styles the popup border.\n\nUse [F10 configuration](topic:f10) to edit the active theme file. Keep selection, footer, picker, and help text readable against the chosen background.",
    },
    {
        "directory",
        "Directory Help",
        "main.dir",
        "This is the `directory window`. It shows the hierarchical directory tree that ytnova has read in the current `volume`.\nThe selected directory’s files appear in the `small window`.\n\n`+` means ytnova has not yet expanded or logged that directory’s subdirectory branch in the tree.\n\nThe `footer` shows the commands available here.\nSome commands have their own `F1` help.\n\n`Left` collapses the selected branch. If that branch is already hidden, it moves to the directory above.\n`Right` moves into the first shown directory below the selected one. If ytnova has not read the selected directory yet, it reads it first.\n`+` reads the selected directory and adds its branch to the tree without moving the selection.\n`*` reads every directory below the selected directory without moving the selection.\n`-` collapses the selected directory and unlogs its files from the logged tree. They no longer appear in Showall or the statistics.\nPress `Enter` to switch into the selected directory’s `file window`. If its tree branch is not logged yet, ytnova reads it first.\n\nThe number keys change what you see.\n`1` shows names, the default.\n`2` shows attributes.\n`3` shows the owner.\n`4` shows times.\nPress an active `2`, `3`, or `4` again to return to names.\nThese views normally change both this panel’s tree and its `file window`.\nSet `SEPARATE_DIR_FILE_VIEWS=1` in `ytnova.conf` to keep them separate.\n\n`5` turns compact names on or off in the `file window`.\n`6` switches size units in both tree and file rows.\n`7` shows a small text preview for each displayed file.\n`8` shows file details for each displayed file.\n`9` shows Git information for files in a Git project.\n`5`, `7`, `8`, and `9` change only the `file window`.\n\n`Attributes` opens a submenu to alter the selected directory’s attributes.\n[Copy](topic:copy-move-targets) copies the selected directory and its contents.\n`Delete` deletes the selected directory.\n[Filter](topic:filter) changes the files shown in the current view.\n`filter-matching` means files allowed by the current Filter.\n`Global` shows files from every logged volume in one list.\n`I` (`Invert`) reverses tags only on filter-matching visible files in the selected directory.\n`J` [compare](topic:compare) compares this directory with another.\n`K volume` opens the volume menu.\n`Log` reads the selected directory or archive, or reads a logged directory again.\n`Makedir` creates a directory below the selected directory.\n`Newfile` creates an empty file inside the selected directory.\n[Output](topic:output) exports the current selection.\n`Pipe` runs a command in the selected directory and sends it the displayed file names.\n`Quit` exits ytnova.\n`Rename` renames the selected directory.\n`Showall` shows all files in the current volume in one list.\n`T` and `U` tag and untag all matching files in the selected directory, then move to the next directory.\n`C-t` and `C-u` [tag and untag](topic:tagged) all matching files in every logged directory in the current volume.\n[moVedir](topic:copy-move-targets) moves the selected directory and its contents.\n`eXecute` runs a command for the selected directory.\n`Z archive` creates an archive from the current selection.\n`/ jump` moves to a displayed name as you type.\n`\\` shows or hides hidden names.\n\n`F5` refreshes the active view.\n`F6` shows or hides statistics.\n`F7` turns autoview on or off.\n`F8` turns split-screen on or off.\n`F9` opens the Applications menu.\n`F10` opens configuration.\n`Esc` cancels the current operation.",
    },
    {
        "file",
        "File Help",
        "main.file",
        "This is the `file window`. The selected row is a `file`.\nCommands in this `footer` act on it unless the line says it uses tagged files.\n\nThe `footer` shows the commands available here.\nSome commands have their own `F1` help.\n\nThe number keys change what you see.\n`1`: Show names.\n`2`: Show attributes, including `name -> target` for symlinks.\n`3`: Show the owner.\n`4`: Show times.\nPress the active `2`, `3`, or `4` again to return to names.\n`5`: Turn `Compact` on or off from the names view.\n`6`: Switch visible file sizes between readable and raw units.\n`7`: Show a small text preview on each visible file row.\n`8`: Show file details on each visible file row.\n`9`: Show the `Git band` when this directory is in a Git worktree.\n\n`A`: Open attributes for the selected file.\n`C-a`: Open attributes for matching [tagged files](topic:tagged).\n`C`: Open [Copy](topic:copy-move-targets) for the selected file.\n`C-c`: Copy matching tagged files.\n`D`: Delete the selected file.\n`C-d`: Delete matching tagged files.\n\n`E`: Edit the selected file in the configured editor.\n`F`: Open [Filter](topic:filter) for this list.\n`H`: Open the selected file in hex view.\n`I`: Reverse tags in the visible list.\n\n`J`: [Compare](topic:compare) the selected file with another file.\n`K`: Open the volume menu.\n`L`: Open `Log Path:` without leaving this list. It starts with the selected path; `C-u` clears it for another path. Log accepts directories and recognized archive files only.\n`M`: Open [Move](topic:copy-move-targets) for the selected file.\n`C-m`: Move matching tagged files when the protocol is available.\n`C-n`: Move matching tagged files without protocol support.\n`N`: Create a new empty file.\n\n`O`: Open [Output](topic:output) for the selected file.\n`C-o`: Output matching tagged files.\n`P`: Run a command with the selected file’s contents as input.\n`C-p`: Run that command for matching tagged files.\n`Q`: Quit ytnova.\n`R`: Rename the selected file.\n`C-r`: Rename matching tagged files.\n\n`S`: Choose the file-list sort order.\n`C-s`: Search tagged files.\n`T`: [Tag](topic:tagged) the selected file, then move to the next file.\n`C-t`: Tag every visible file.\n`U`: Remove the selected file’s tag, then move to the next file.\n`C-u`: Remove tags from every visible file.\n\n`V`: View the selected file.\n`C-v`: View matching tagged files one after another.\n`X`: Type a shell command. `{}` is the selected file’s path.\n`C-x`: Run that command once for each matching tagged file.\n`Y`: [Pathcopy](topic:copy-move-targets) the selected file and keep its path relative to the current volume root.\n`C-y`: Pathcopy matching tagged files.\n`Z`: Archive tagged files, or the selected file when none are tagged.\n`C-z`: Archive matching tagged files.\n\n`/ jump` moves to a displayed name as you type.\n`\\` shows or hides hidden names.\n\n`F5`: Refresh the active view.\n`F6`: Show or hide the statistics strip.\n`F7`: Turn autoview on or off.\n`F8`: Turn split-screen on or off.\n`F9`: Open the Applications menu.\n`F10`: Open configuration.\n`Esc`: Cancel the current operation.",
    },
    {
        "archive-dir",
        "Archive Directory Help",
        "main.archive-dir",
        "This is a directory tree inside an archive. Its rows describe archive entries, not ordinary filesystem directories.\n`Enter`, `Left`, and `Right` move through the archive tree.\n`\\` goes to the archive root; at the root it leaves the archive.\n\n`1` to `8` change the archive view.\n`9` has no effect because archive entries do not have Git status.\n`0` shows or hides Size, Packed, and Ratio on archive file rows.\n\n[Copy](topic:copy-move-targets) and `Pathcopy` extract entries to a destination.\n`Move`, `Delete`, `Rename`, and `Makedir` are offered only when the archive and installed libarchive support writing the result.\n[Filter](topic:filter), `Showall`, `Global`, `Tag`, `Untag`, and `I` work on the archive-backed entries in their current scope.\n[Compare](topic:compare), `Output`, and `Pipe` read from archive-backed paths.\n`Log` opens a supported archive entry as another archive.\n`K` opens the volume menu.\n`F5` refreshes the view.\n`F6` changes the statistics shown.\n`F7` opens preview.\n`F8` opens split mode.\n`F9` opens Applications.\n`F10` opens configuration.\n`Q` exits ytnova.",
    },
    {
        "archive-file",
        "Archive File Help",
        "main.archive-file",
        "This is a file list inside an archive. The selected row is an archive entry, not an ordinary writable file.\n`Enter` returns to the archive directory tree.\n`Left` and `Right` move across file columns.\n\n`1` to `8` change the archive view.\n`9` has no effect because archive entries do not have Git status.\n`0` shows or hides Size, Packed, and Ratio.\n\n`V` views the selected entry and `H` opens it in hex view.\n[Copy](topic:copy-move-targets) and `Y` pathcopy extract the selected entry; their `C-` variants use tagged entries.\n`Move`, `Delete`, and `Rename` are available only when archive write-back is supported.\n`eXecute` is not available for archive entries.\n[Filter](topic:filter), `Sort`, `Tag`, `Untag`, `I`, and tagged search stay within the archive-backed list.\n[Compare](topic:compare), `Output`, and `Pipe` read the selected entry or tagged set.\n`Log` opens the selected entry as another archive when its format is supported.\n`K` opens the volume menu.\n`F5` refreshes the view.\n`F6` changes the statistics shown.\n`F7` opens preview.\n`F8` opens split mode.\n`F9` opens Applications.\n`F10` opens configuration.\n`Q` exits ytnova.",
    },
    {
        "filter",
        "Filter Help",
        "prompt.filter,prompt.filter-tagged",
        "Type a pattern and press `Enter` to filter the current file list. The prompt starts with `*`, which shows all files.\nUse `*.c` for one glob pattern or `*.c,*.h` for either pattern.\nPrefix a term with `-` to exclude it, as in `*.c,-test*`.\nUse attributes such as `:r` or `:x`, dates such as `>2024-01-01`, and sizes such as `>1M` with the same comma-separated syntax.\n\nWhen the current scope contains [tagged files](topic:tagged), `Tab` switches the prompt between all files and tagged files without changing any tags.\nThe filter affects only the current normal, archive, Showall, or Global file list. It is different from [/ jump](topic:list-jump), which only moves the selection.",
    },
    {
        "compare",
        "Compare Help",
        NULL,
        "Press `J` to compare the selected file or directory with another target.\nFirst choose the [target](topic:compare-target).\nFor a directory, choose the [scope](topic:compare-scope).\nChoose the [comparison basis](topic:compare-basis) when more than one is available.\nThen choose which [result](topic:compare-results) to tag on the source side.\n\nLogged-tree comparison uses only directories already logged in the tree; it does not open unread `+` branches.\nExternal directory or tree comparison launches `DIRDIFF` or `TREEDIFF` instead of tagging results.\nFile comparison uses `FILEDIFF`; `%1` and `%2` stand for the two paths, and missing placeholders are appended automatically.\nCompare never changes file contents and has no separate tagged-files mode.",
    },
    {
        "compare-target",
        "Compare Target Help",
        "prompt.compare-target",
        "Enter the file or directory to compare with the current selection, then press `Enter`.\n`Up` reuses an earlier target.\n`F2` opens the [directory picker](topic:f2-picker) where that target can be browsed.\nIn [F8 split](topic:f8) mode, the other panel supplies the initial target.\nThe later [compare scope](topic:compare-scope) decides whether a directory target means one directory or its logged tree.\n`Esc` cancels the comparison.",
    },
    {
        "change-date",
        "Date Change Help",
        "prompt.change-date",
        "Enter a date as `YYYY-MM-DD`, optionally followed by `HH:MM` or `HH:MM:SS`.\nIf you omit the time, ytnova keeps the existing hour, minute, and second.\n`F3` cycles between changing the modified time, accessed time, or both.\n`Enter` applies the selected choice.\n`Esc` cancels.\nThe tagged-file action uses the same value and choice for each tagged file.",
    },
    {
        "compare-scope",
        "Compare Scope Help",
        NULL,
        "Choose how much of the selected directory to compare.\n`Directory` compares one directory level.\n`Logged tree` includes the recursive tree that ytnova has already logged; unopened `+` branches are not read automatically.\n`External viewer` runs the configured directory or tree diff tool instead of tagging results inside ytnova.\nPress `Enter` to continue or `Esc` to cancel. See [Compare](topic:compare) for the complete flow.",
    },
    {
        "compare-basis",
        "Compare Basis Help",
        NULL,
        "Choose the facts used to decide whether entries match.\nName, size, and time comparisons use file metadata.\nUse `Hash` when you need a content-based check and metadata is not sufficient.\nPress `Enter` to compare or `Esc` to cancel. See [Compare](topic:compare) for the complete flow.",
    },
    {
        "compare-results",
        "Compare Result Help",
        NULL,
        "Choose the comparison result class you want to keep.\nYtreeNova tags matching entries from that class on the active source side.\nYou can then view, copy, move, output, or archive that tagged set.\nThe comparison does not rewrite either side. See [Tagged](topic:tagged) for the resulting working set.",
    },
    {
        "execute-file",
        "Execute File Help",
        "prompt.execute-file",
        "The prompt starts with `{}`, which stands for the selected file path.\nType the command before `{}` and any redirection, pipe, or other shell syntax after it.\nFor example, enter `wc {} > count`.\n`Enter` runs the command once for the selected file.\nThe tagged `C-x` action repeats the same command once for each tagged file.\n`Esc` cancels. See [Command-line Editing](topic:command-line-editing) for editing keys.",
    },
    {
        "execute-dir",
        "Execute Directory Help",
        "prompt.execute-dir",
        "The prompt starts with `{}`, which stands for the selected directory path.\nType the command before `{}` and any following shell syntax after it.\nFor example, enter `tar -cf archive.tar {}`.\n`Enter` runs the command for that directory.\nThe tagged `C-x` action still repeats the command for tagged files in the active list; it does not process tagged directories.\n`Esc` cancels. See [Command-line Editing](topic:command-line-editing) for editing keys.",
    },
    {
        "search-tagged",
        "Search Tagged Help",
        "prompt.search-tagged",
        "Type search text and press `Enter` to search only the current [tagged](topic:tagged) files.\nFiles without a match lose their tags, leaving a smaller tagged set.\n`Up` reuses an earlier search.\n`Esc` cancels without changing the set.",
    },
    {
        "create-archive",
        "Create Archive Help",
        "prompt.create-archive",
        "Enter the new archive path and press `Enter`.\nYtreeNova archives tagged files first. If no files are tagged, it archives the current selection.\nA selected directory is included recursively.\nThe filename suffix chooses the archive format, subject to installed libarchive support.\n`Up` reuses an earlier path.\n`F2` opens the [directory picker](topic:f2-picker).\n`Esc` cancels.",
    },
    {
        "output",
        "Output Help",
        NULL,
        "`Output` exports the selected file, or the tagged set when you use its `C-` form.\nChoose a [destination](topic:output-destination): a file path or Hardcopy.\nFor file output, `F3` cycles [Raw, Framed, and Page break](topic:output-format).\nFramed and Page break output then ask for a [separator](topic:output-separator) before returning to the destination prompt.\nHardcopy asks for a printer command and always sends raw output.\n`Enter` accepts each choice; `Esc` cancels the current prompt.",
    },
    {
        "output-format",
        "Output Format Help",
        NULL,
        "Use `F3` on the file-destination prompt to choose the output format.\n`Raw` joins the exported file contents without presentation framing.\n`Framed` separates files with the text entered at the separator prompt.\n`Page break` separates files for page-oriented output.\nPress `Enter` to continue. Hardcopy always uses Raw and does not offer this choice. See [Output](topic:output) for the full flow.",
    },
    {
        "output-destination",
        "Output Destination Help",
        "prompt.output-destination",
        "Choose where the exported text goes.\nSelect file output to enter a path. A bare filename is relative to `CWD`, the current working directory shown by the prompt.\nPress `F3` here to change the [output format](topic:output-format).\nSelect Hardcopy to enter a printer command such as `lpr`, `lp`, or `cat > /dev/lp1`; Hardcopy sends raw output.\n`Up` reuses an earlier destination.\n`F2` opens the [directory picker](topic:f2-picker) for file output.\n`Enter` accepts the destination and `Esc` cancels.",
    },
    {
        "output-separator",
        "Output Separator Help",
        "prompt.output-separator",
        "Enter the text placed between exported files when the format is Framed or Page break.\nThe separator is inserted between files, not after the last one.\n`Up` reuses an earlier separator.\n`Enter` returns to the destination prompt.\n`Esc` cancels. Raw and Hardcopy output skip this prompt. See [Output](topic:output) for the full flow.",
    },
    {
        "showall",
        "Showall Help",
        "main.showall",
        "`Showall` displays files from every logged directory in the current volume as one list. It does not include other volumes.\n`Esc` returns to the directory you came from.\n`\\` opens the owning directory of the selected file in the current volume.\n\n[File commands](topic:file) act on the selected row or the tagged set, but filtering, sorting, tagging, and `/` use this aggregated result list as their scope.\n[Filter](topic:filter) changes only this Showall list.\n`G` opens [Global](topic:global) when you need every logged volume.\n`F7` opens preview and `F8` opens split mode.",
    },
    {
        "global",
        "Global Help",
        "main.global",
        "`Global` displays files from every logged directory in every logged volume as one list.\n`Esc` returns to the directory view you came from.\n`\\` opens the owning volume and directory of the selected file.\n\n[File commands](topic:file) act on the selected row or the tagged set, but filtering, sorting, tagging, and `/` use the complete Global result list as their scope.\n[Filter](topic:filter) changes only this Global list.\nPressing `G` again has no effect because the Global view is already open.\n`F7` opens preview and `F8` opens split mode.",
    },
    {
        "f7",
        "F7 Preview Help",
        "overlay.f7-dir,overlay.f7-file",
        "`F7` opens a preview of the selected file without leaving its file-selection context.\n`Up`, `Down`, `Page Up`, `Page Down`, `Home`, and `End` continue to move the file selection.\n`Shift-Up`, `Shift-Down`, `C-p`, and `C-n` scroll the preview by lines.\n`Shift-Page Up` and `Shift-Page Down` scroll it by pages.\n`Shift-Home` and `Shift-End` go to the start or end of the preview.\n`F7` or `Esc` returns to the underlying view.\n\nThe usual [file actions](topic:file) remain available for the selected file. Tagged variants keep their current scope.\n`F8` and `Tab` do not open or switch split panels while preview is active.\n`F9` opens Applications without first closing preview.",
    },
    {
        "f8",
        "F8 Split Help",
        NULL,
        "`F8` opens a second panel. The highlighted panel is active and receives the next command.\n`Tab` switches the active panel.\nEach panel keeps its own selection, logged volume, tags, view settings, and return state.\n\nCopy, move, and compare prompts use the other panel as their initial destination or target where applicable. You can edit that value before continuing.\nPress `F8` again to return to one panel. See [split directory](topic:f8-dir) or [split file](topic:f8-file) help for local commands.",
    },
    {
        "f8-dir",
        "F8 Split Directory Help",
        "overlay.f8-dir",
        "This is the active directory tree in [F8 split](topic:f8) mode.\n`Tab` activates the other panel.\n`F8` returns to one panel.\n\nThe tree and directory commands are the same as in [Directory Help](topic:directory).\nCopy, move, and compare start with the other panel's selected directory as the target where applicable.\nA prompt may use `Tab` for its own local choice instead of switching panels.\nCommands change only the active panel unless their prompt explicitly names the other panel.",
    },
    {
        "f8-file",
        "F8 Split File Help",
        "overlay.f8-file",
        "This is the active file list in [F8 split](topic:f8) mode.\n`Tab` activates the other panel.\n`F8` returns to one panel.\n\nThe file commands are the same as in [File Help](topic:file).\nCopy, move, and compare start with the other panel's selected directory or file as the target where applicable.\nTagged commands use the active panel's tagged set.\nA prompt may use `Tab` for its own local choice instead of switching panels.",
    },
    {
        "history-dialog",
        "History Help",
        "dialog.history",
        "Use `Up` and `Down` to select an earlier prompt value.\nUse `Left` and `Right` to scroll a long value horizontally.\n`P` pins or unpins the selected value.\n`D` deletes it.\n`Enter` puts it back into the prompt.\n`Esc` closes history without choosing a value.",
    },
    {
        "volume-menu",
        "Volume Help",
        "dialog.volume-menu",
        "Use `Up` and `Down` to select a logged volume.\n`Enter` switches to it and restores its in-memory state.\nChoosing the active volume keeps its current state.\n`D` releases the selected volume; the last remaining volume cannot be released.\n`Esc` closes the menu without switching.",
    },
    {
        "applications-menu",
        "Applications Help",
        "dialog.applications",
        "Use `Up` and `Down` to select a configured application preset.\n`Enter` starts it and returns immediately to ytnova.\n`E` edits the applications catalogue.\n`Esc` closes the menu.\n\n`{}` inserts the selected file or directory. The preset starts in that selection's directory even when `{}` is absent.\n`{input}` inserts text collected by the preset's input prompt.\nUse `F9` for repeatable presets; use `eXecute` for a one-off shell command.",
    },
    {
        "f2-picker",
        "F2 Picker Help",
        "dialog.f2-picker",
        "Use `F2` from a prompt that accepts a browsed directory.\n`Up` and `Down` move through the tree.\n`Left` collapses a branch or moves to its parent.\n`Right` expands a branch or moves into it.\n`<` and `>` switch between logged volumes.\n`L` logs another directory or archive.\n`Backtick` shows or hides dotfiles.\n`Enter` chooses the highlighted directory and returns it to the prompt.\n`Esc` returns without changing the prompt.",
    },
};

static const size_t generated_help_topic_count_en = 43;

static const GeneratedHelpTopic generated_help_topics_de[] = {
    {
        "index",
        "Hilfeindex",
        NULL,
        "Wähle mit `Up` und `Down` ein Thema.\nDrücke `Enter` oder `Right`, um es zu öffnen.\n`Left` kehrt zur vorherigen Hilfeseite zurück.\n`Esc` schließt die Hilfe.\n\n`Navigation` in dieser Liste erklärt die Bewegung durch YtreeNova. Der Befehl `Navigation` in der Hilfeleiste erklärt die Bewegung innerhalb des Hilfe-Popups.\n\n- [Anwendungen](topic:applications-menu)\n- [Archiv erstellen](topic:create-archive)\n- [Archivdatei](topic:archive-file)\n- [Archivverzeichnis](topic:archive-dir)\n- [Ausgabe](topic:output)\n- [Ausgabeformat](topic:output-format)\n- [Ausgabetrenner](topic:output-separator)\n- [Ausgabeziel](topic:output-destination)\n- [Datei](topic:file)\n- [Datei ausführen](topic:execute-file)\n- [Datum ändern](topic:change-date)\n- [F10-Konfiguration](topic:f10)\n- [F2-Verzeichnisauswahl](topic:f2-picker)\n- [F7-Vorschau](topic:f7)\n- [F8-Split](topic:f8)\n- [F8-Split-Datei](topic:f8-file)\n- [F8-Split-Verzeichnis](topic:f8-dir)\n- [Filter](topic:filter)\n- [Gemeinsame Befehle](topic:shared-commands)\n- [Global](topic:global)\n- [Kommandozeilenbearbeitung](topic:command-line-editing)\n- [Kommandozeilenparameter](topic:command-line-parameters)\n- [Konfigurationsdateien](topic:configuration-files)\n- [Kopier- und Verschiebeziele](topic:copy-move-targets)\n- [Listensprung](topic:list-jump)\n- [Markierte Dateien](topic:tagged)\n- [Markierte durchsuchen](topic:search-tagged)\n- [Markierungsanzeige](topic:tagged-viewer)\n- [Navigation](topic:ytnova-navigation)\n- [Showall](topic:showall)\n- [Themes](topic:theming)\n- [Vergleich](topic:compare)\n- [Vergleichsbasis](topic:compare-basis)\n- [Vergleichsbereich](topic:compare-scope)\n- [Vergleichsergebnis](topic:compare-results)\n- [Vergleichsziel](topic:compare-target)\n- [Verlauf](topic:history-dialog)\n- [Verzeichnis](topic:directory)\n- [Verzeichnis ausführen](topic:execute-dir)\n- [vi-Tasten](topic:vi-keys)\n- [Volumen](topic:volume-menu)",
    },
    {
        "f1-navigation",
        "Hilfe-Navigation",
        NULL,
        "Mit den Pfeiltasten `Up` und `Down` scrollst du die Hilfe zeilenweise.\nMit den Pfeiltasten `Up` und `Down` wechselst du zwischen Links.\n`Page Up` und `Page Down` bewegen eine Hilfeseite.\n`Home` und `End` gehen zum Anfang und Ende.\n`Enter` oder die Pfeiltaste `Right` öffnet den gewählten Link.\nDie Pfeiltaste `Left` kehrt zur vorherigen Hilfeseite zurück.\nDrücke `H`, um zum `Hilfeindex` zu gehen.\n`Esc` oder `Q` schließt die Hilfe.",
    },
    {
        "ytnova-navigation",
        "YtreeNova-Navigation",
        NULL,
        "YtreeNova ist für die Tastatur gemacht.\nMauseffekte können vorkommen, sind aber keine vorgesehenen Steuerelemente.\nMit den Pfeiltasten `Up` und `Down` bewegst du dich jeweils eine Zeile.\n`Page Up` und `Page Down` bewegen jeweils eine Bildschirmseite.\n`Home` und `End` gehen zur ersten oder letzten sichtbaren Zeile.\n`Enter` öffnet das gewählte Element.\n`Right` öffnet oder erweitert das gewählte Element, wenn die Ansicht das unterstützt.\n`Left` geht zurück oder klappt das gewählte Element ein, wenn die Ansicht das unterstützt.\n[/ Sprung](topic:list-jump) geht beim Tippen zu einem passenden Namen in der aktuellen Liste. `Enter` wählt ihn aus; `Esc` bricht ab.\n`Tab` geht zum nächsten sichtbaren Geschwisterelement im Baum und springt am Ende zum Anfang; `Shift-Tab` kehrt dies um.\n[F8 Split](topic:f8)-Modus: `Tab` wechselt das aktive Panel; manche Prompts geben `Tab` eine eigene lokale Bedeutung.\n[F7 Vorschau](topic:f7): Gewöhnliche Navigationstasten bewegen die Dateiauswahl; ihre Umschaltvarianten scrollen die Vorschau.\n\n`Terminal-Eingabegrenzen`\n`Alt` wird absichtlich nicht unterstützt, weil Terminals es uneinheitlich behandeln.\n`Kitty-Tastaturprotokoll`\nYtreeNova verwendet das Kitty-Tastaturprotokoll, wenn der aktive Terminalpfad es unterstützt. Dadurch kann es `C-m` von Enter unterscheiden, sodass `C-m` markierte Dateien verschiebt.\nFüge in Kitty `keyboard_protocol kitty` zu `~/.config/kitty/kitty.conf` hinzu und starte Kitty neu. Auch andere Terminals mit Protokollunterstützung funktionieren.\nBei einem Multiplexer oder einer Remote-Sitzung muss das Protokoll unverändert durchgereicht werden.\nOhne Protokollunterstützung verwendet YtreeNova stillschweigend `C-n`, um markierte Dateien zu verschieben.\n`C-[` ist Esc.",
    },
    {
        "list-jump",
        "Listensprung",
        NULL,
        "Drücke `/`, tippe einen Teil eines Namens und drücke `Enter`, um den besten sichtbaren Treffer zu behalten.\nDie Auswahl bewegt sich bereits während der Eingabe.\n`Esc` stellt die Auswahl vor dem Sprung wieder her.\n\nDer Sprung durchsucht nur die aktuell sichtbare Liste. In einem Verzeichnisbaum wiederholst du ihn nach dem Öffnen eines Verzeichnisses, wenn du tiefer springen willst.",
    },
    {
        "shared-commands",
        "Gemeinsame Befehle",
        NULL,
        "`F1` öffnet die Hilfe für den aktuellen Bildschirm oder Prompt.\n`F5` aktualisiert die aktuelle Ansicht.\n`F6` ändert die Statistik- oder Detailanzeige der aktiven Ansicht.\n[F7 Vorschau](topic:f7) öffnet oder schließt die Dateivorschau.\n[F8 Split](topic:f8) öffnet oder schließt das zweite Panel.\n`F9` öffnet das [Anwendungsmenü](topic:applications-menu).\n`F10` öffnet die [Konfiguration](topic:f10).\n`Esc` verlässt den aktuellen Prompt, das Menü, die Vorschau oder das Popup.\n\nDer Footer zeigt die aktiven Belegungen. `commands.conf` kann Tasten und Beschriftungen ändern.\nMit `C--` und `C-+` verkleinerst oder vergrößerst du die Terminalschrift, wenn der Footer nicht passt.",
    },
    {
        "tagged",
        "Markierungen",
        NULL,
        "Markierungen wählen mehrere Dateien aus, auf die dann ein Vorgang gemeinsam angewendet wird.\nMarkierte Dateien sind am `*` im Rand zu erkennen.\n\nDrücke in einer Dateiliste `T`, um die gewählte Datei zu markieren, oder `U`, um die Markierung zu entfernen. Die Auswahl geht zur nächsten Datei.\nIn einem Verzeichnisbaum wirken `T` und `U` auf die Dateien, die der aktuelle Filter im gewählten Verzeichnis zulässt, und gehen dann zum nächsten Verzeichnis nach unten.\n\n`I` kehrt die Markierungen der Dateien um, die der aktuelle Filter im aktuellen Bereich zulässt.\n\nWenn Statistiken angezeigt werden, erscheinen dort auch die Gesamtwerte der markierten Dateien.\n\nHalte die Steuerungstaste mit dem Buchstaben nach `C-` gedrückt, um den passenden Vorgang für alle markierten Dateien auszuführen. `^` in einem Footer-Label bedeutet denselben Markierungsvorgang, zum Beispiel `C/^Copy`.\n\n`C-a` öffnet den Attribute-Prompt für markierte Dateien.\n`C-c` öffnet den [Kopier-](topic:copy-move-targets)Prompt für markierte Dateien.\n`C-d` fragt vor dem Löschen markierter Dateien nach.\n`C-m` öffnet bei verfügbarer Protokollunterstützung den [Verschiebe-](topic:copy-move-targets)Prompt; ohne diese Unterstützung verwendest du `C-n`.\n`C-o` öffnet den [Ausgabe-](topic:output)Prompt für markierte Dateien.\n`C-p` öffnet den Pipe-Prompt für markierte Dateien.\n`C-r` öffnet den Umbenennen-Prompt für markierte Dateien.\n`C-s` [durchsucht markierte Dateien](topic:search-tagged) und entfernt die Markierung von Dateien ohne Treffer.\n`C-t` markiert alle Dateien in der aktiven Dateiliste. In einem Verzeichnisbaum markiert es die Dateien, die der aktuelle Filter in allen geloggten Verzeichnissen zulässt.\n`C-u` entfernt die Markierungen aller Dateien in der aktiven Dateiliste. In einem Verzeichnisbaum entfernt es die Markierungen der Dateien, die der aktuelle Filter in allen geloggten Verzeichnissen zulässt.\n`C-v` zeigt markierte Dateien nacheinander an.\n`C-x` öffnet einen [Befehlsprompt](topic:execute-file) und führt den Vorgang einmal für jede markierte Datei aus.\n`C-y` [Kopiert Pfade](topic:copy-move-targets) der markierten Dateien.\n`C-z` öffnet je nach Auswahl den Prompt für ein [Archivverzeichnis](topic:archive-dir) oder eine [Archivdatei](topic:archive-file).\n\nDrücke für einen [Filter](topic:filter)-Bereich nur mit markierten Dateien `F` und dann `Tab`. Das ändert die Markierungen nicht.",
    },
    {
        "tagged-viewer",
        "Markierungsanzeige",
        "viewer.tagged",
        "Mit `n` und `p` öffnest du die nächste oder vorherige markierte Datei.\nMit `Space`, `Page Down` und `Page Up` bewegst du dich innerhalb der aktuellen Datei.\nNach einer Suche in markierten Dateien führen `/` und `\?` zum nächsten oder vorherigen Treffer in dieser Datei.\n`Esc` schließt die Anzeige.\n\nBei `TAGGEDVIEWER=external` übernimmt der konfigurierte Pager das Blättern und Suchen. Unter [Markierte Dateien](topic:tagged) steht, wie du die Arbeitsmenge erstellst und verwendest.",
    },
    {
        "command-line-editing",
        "Bearbeitung in Eingabezeilen",
        NULL,
        "Die meisten Prompts verwenden dieselben Bearbeitungstasten.\n`Left` und `Right` bewegen den Cursor um ein Zeichen.\n`Home` und `End` gehen zum Anfang oder Ende.\n`C-a` und `C-e` tun dasselbe.\n`Backspace` oder `C-h` löscht das Zeichen links vom Cursor.\n`Delete` oder `C-d` löscht das Zeichen unter dem Cursor.\n`C-w` löscht das Wort links vom Cursor.\n`C-u` löscht bis zum Anfang.\n`C-k` löscht bis zum Ende.\n`Up` öffnet oder durchläuft gespeicherte Werte, wenn der Prompt einen Verlauf führt.\n`F2` öffnet die [Verzeichnisauswahl](topic:f2-picker), wenn Browsen möglich ist.\n`F1` öffnet die Hilfe für den aktuellen Prompt.\n`Enter` übernimmt den Wert.\n`Esc` bricht ohne Übernahme ab.",
    },
    {
        "command-line-parameters",
        "Kommandozeilenparameter",
        NULL,
        "Gib ytnova beim Start ein oder mehrere Verzeichnis- oder Archivpfade, um sie einzulesen. Ohne Pfad liest es das aktuelle Verzeichnis ein.\n\n`-d Tiefe` setzt die Einlesetiefe beim Start. Verwende eine Zahl, `min` oder `root` für null oder `max` oder `all` für 100.\n`-f Filter` startet mit einem [Dateifilter](topic:filter). Setze Shell-Muster wie `\"*.c\"` in Anführungszeichen, damit die Shell sie nicht vorher erweitert.\n`-h Verlaufsdatei` wählt eine andere Datei für den Befehlsverlauf.\n`-p Konfigurationsdatei` wählt eine andere Hauptkonfiguration.\n`--init` erstellt fehlende [Konfigurationsdateien](topic:configuration-files) und beendet das Programm.\n`-v`, `-V` oder `--version` gibt die Version aus und beendet das Programm.",
    },
    {
        "configuration-files",
        "Konfigurationsdateien",
        NULL,
        "Die Benutzerkonfiguration liegt normalerweise unter `~/.config/ytnova`.\n`ytnova.conf` enthält Profileinstellungen wie Einlesetiefe, `VI_KEYS` und `SEPARATE_DIR_FILE_VIEWS`.\n`commands.conf` enthält Befehlsbeschriftungen, Tastenbelegungen und die Preset-Auswahl.\n`themes.conf` wählt ein Theme und überschreibt Theme-Rollen.\n`applications.conf` enthält die mit `F9` geöffneten Presets.\nDer Befehlsverlauf wird getrennt gespeichert; mit `-h` wählst du eine andere Verlaufsdatei.\n\nMit `-p` wählst du eine andere `ytnova.conf`. Vorhandene ältere Dateien im Home-Verzeichnis und paketierte Vorgaben bleiben Rückfalloptionen. Über die [F10-Konfiguration](topic:f10) bearbeitest du die aktiven Dateien.",
    },
    {
        "copy-move-targets",
        "Kopier- und Verschiebeziele",
        NULL,
        "`Copy`, `move` und `pathcopy` fragen zuerst nach einem Namen oder Umbenennungsmuster, dann nach dem Zielverzeichnis.\n\nBei einer Datei lässt du ihren vorbelegten Namen stehen, um ihn beizubehalten, oder änderst ihn zum Umbenennen. Du kannst in beiden Fällen Wildcards verwenden.\n\nBei [markierten Dateien](topic:tagged) lässt du das vorbelegte `*` stehen, um ihre Namen beizubehalten, oder gibst ein Umbenennungsmuster ein:\n`*` übernimmt den restlichen ursprünglichen Namen. Nutze `copy-*`, um ein Präfix hinzuzufügen, oder `*.bak`, um die Erweiterung zu ändern.\n`\?` übernimmt ein Zeichen des ursprünglichen Namens. Zum Beispiel übernimmt `\?\?-*` die ersten beiden Zeichen, fügt `-` hinzu und übernimmt dann den Rest.\nAndere Zeichen werden wie geschrieben verwendet.\n\nNutze `Up`, um einen früheren Namen oder ein früheres Muster wiederzuverwenden.\n\nGib das Zielverzeichnis in `To Directory` ein. Im [F8-Split](topic:f8)-Modus beginnt es mit dem derzeit ausgewählten Verzeichnis des anderen Panels.\n\nNutze `Up`, um ein früheres Ziel wiederzuverwenden, oder drücke [F2](topic:f2-picker), um eines zu wählen.\n\nDieselben Prompts gelten für [Archivdateien](topic:archive-file) und [Archivverzeichnisse](topic:archive-dir). Beim Kopieren einer Archivdatei extrahiert ytnova sie bei Bedarf; beim Verschieben kopiert ytnova sie und entfernt dann das Original. Du kannst auch in ein geloggtes Archiv kopieren.\n\n`pathcopy` stellt den bestehenden Pfad der ausgewählten Datei unterhalb des Zielverzeichnisses wieder her.\n\nWenn das Zielverzeichnis fehlt, fragt ytnova, ob es erstellt werden soll. Existiert bereits eine Zieldatei, fragt ytnova vor dem Ersetzen.",
    },
    {
        "vi-keys",
        "VI-Tasten",
        NULL,
        "Setze `VI_KEYS=1`, um die kleinen vi-Bewegungstasten zu verwenden.\n`h`, `j`, `k` und `l` werden zu `Left`, `Down`, `Up` und `Right`.\n`C-u` und `C-d` werden zu Seite hoch und Seite runter.\n\nBefehle mit einer Kollision erhalten eine andere Belegung. Der Footer zeigt die aktiven Tasten; Beispiele sind `J` für Vergleichen, `K` für das Volumenmenü, `D` zum Löschen markierter Dateien und `U` zum Entfernen aller Markierungen.",
    },
    {
        "f10",
        "F10-Konfiguration",
        NULL,
        "Drücke `F10`, wenn du die dauerhafte Einrichtung ändern willst statt eine Dateiaktion auszuführen.\nWähle den Profilbefehl, um `ytnova.conf` zu bearbeiten.\nWähle den Befehlseintrag für Beschriftungen und Belegungen in `commands.conf`.\nWähle den Theme-Eintrag, um `themes.conf` zu bearbeiten.\nWähle Neuladen, um die gespeicherte Konfiguration erneut anzuwenden.\n\nUnter [Konfigurationsdateien](topic:configuration-files) stehen die Speicherorte und unter [Themes](topic:theming) die Hilfe-Rollen.",
    },
    {
        "theming",
        "Themes",
        NULL,
        "Themes weisen benannten Oberflächenrollen Farben und Attribute zu.\n`footer` gestaltet den Footer des Hauptprogramms.\n`help` gestaltet den Textkörper der F1-Hilfe.\n`help_footer` gestaltet die Hilfeleiste.\n`help_keybind` hebt den gebundenen Buchstaben hervor.\n`help_heading` gestaltet Hilfetitel und Überschriften.\n`help_topic` gestaltet Tasten und Begriffe in Backticks.\n`help_link` und `help_link_selection` gestalten Links und den ausgewählten Link.\n`help_box_lines` gestaltet den Rahmen des Popups.\n\nÜber die [F10-Konfiguration](topic:f10) bearbeitest du die aktive Theme-Datei. Auswahl, Footer, Picker und Hilfetext müssen vor dem gewählten Hintergrund lesbar bleiben.",
    },
    {
        "directory",
        "Verzeichnishilfe",
        "main.dir",
        "Dies ist das `Verzeichnisfenster`. Es zeigt den hierarchischen Verzeichnisbaum, den ytnova im aktuellen `Volume` gelesen hat.\nDie Dateien des gewählten Verzeichnisses erscheinen im `kleinen Fenster`.\n\n`+` bedeutet, dass ytnova den Unterverzeichniszweig dieses Verzeichnisses im Baum noch nicht erweitert oder geloggt hat.\n\n`Left` klappt den ausgewählten Zweig ein. Wenn dieser Zweig bereits verborgen ist, geht die Auswahl zum übergeordneten Verzeichnis.\n`Right` geht zum ersten angezeigten Verzeichnis unter dem ausgewählten Verzeichnis. Wenn ytnova es noch nicht gelesen hat, liest es dieses zuerst ein.\n`+` liest das ausgewählte Verzeichnis ein und fügt seinen Zweig zum Baum hinzu, ohne die Auswahl zu bewegen.\n`*` liest jedes Verzeichnis unter dem ausgewählten Verzeichnis ein, ohne die Auswahl zu bewegen.\n`-` klappt das ausgewählte Verzeichnis ein und entfernt seine Dateien aus dem eingelesenen Baum. Sie erscheinen nicht mehr in Showall oder den Statistiken.\nDrücke `Enter`, um in das `Dateifenster` des ausgewählten Verzeichnisses zu wechseln. Wenn dessen Baumzweig noch nicht geloggt ist, liest ytnova ihn zuerst ein.\n\nDer `Footer` zeigt die hier verfügbaren Befehle.\nEinige Befehle haben ihre eigene `F1`-Hilfe.\n\nDie Zahlentasten ändern, was du siehst.\n`1` zeigt Namen, die Standardansicht.\n`2` zeigt Attribute.\n`3` zeigt den Eigentümer.\n`4` zeigt Zeiten.\nDrücke eine aktive `2`, `3` oder `4` erneut, um zu den Namen zurückzukehren.\nDiese Ansichten ändern normalerweise sowohl den Baum als auch das `Dateifenster` dieses Panels.\nSetze `SEPARATE_DIR_FILE_VIEWS=1` in `ytnova.conf`, um sie getrennt zu halten.\n\n`5` schaltet kompakte Namen im `Dateifenster` ein oder aus.\n`6` schaltet die Größeneinheiten in Baum- und Dateizeilen um.\n`7` zeigt eine kleine Textvorschau für jede angezeigte Datei.\n`8` zeigt Dateidetails für jede angezeigte Datei.\n`9` zeigt Git-Informationen für Dateien in einem Git-Projekt.\n`5`, `7`, `8` und `9` ändern nur das `Dateifenster`.\n\n`Attributes` öffnet ein Untermenü, um die Attribute des gewählten Verzeichnisses zu ändern.\n[Copy](topic:copy-move-targets) kopiert das gewählte Verzeichnis mit seinem Inhalt.\n`Delete` löscht das gewählte Verzeichnis.\n[Filter](topic:filter) ändert die Dateien in der aktuellen Ansicht.\n`filter-passend` bedeutet: Dateien, die der aktuelle Filter zulässt.\n`Global` zeigt Dateien aus allen geloggten Volumes in einer Liste.\n`I` (`Invert`) kehrt nur die Markierungen filterpassender sichtbarer Dateien im gewählten Verzeichnis um.\n`J` [compare](topic:compare) vergleicht dieses Verzeichnis mit einem anderen.\n`K volume` öffnet das Volume-Menü.\n`Log` liest das gewählte Verzeichnis oder Archiv oder liest ein geloggtes Verzeichnis erneut.\n`Makedir` erstellt ein Verzeichnis unter dem gewählten Verzeichnis.\n`Newfile` erstellt eine leere Datei im gewählten Verzeichnis.\n[Output](topic:output) exportiert die aktuelle Auswahl.\n`Pipe` führt einen Befehl im gewählten Verzeichnis aus und gibt ihm die angezeigten Dateinamen.\n`Quit` beendet ytnova.\n`Rename` benennt das gewählte Verzeichnis um.\n`Showall` zeigt alle Dateien im aktuellen Volume in einer Liste.\n`T` und `U` markieren oder entmarkieren alle passenden Dateien im gewählten Verzeichnis und gehen dann zum nächsten Verzeichnis.\n`C-t` und `C-u` [markieren oder entmarkieren](topic:tagged) alle passenden Dateien in jedem geloggten Verzeichnis des aktuellen Volumes.\n[moVedir](topic:copy-move-targets) verschiebt das gewählte Verzeichnis mit seinem Inhalt.\n`eXecute` führt einen Befehl für das gewählte Verzeichnis aus.\n`Z archive` erstellt ein Archiv aus der aktuellen Auswahl.\n`/ jump` springt beim Tippen zu einem angezeigten Namen.\n`\\` zeigt oder versteckt verborgene Namen.\n\n`F5` aktualisiert die aktive Ansicht.\n`F6` zeigt oder versteckt Statistiken.\n`F7` schaltet Autoview ein oder aus.\n`F8` schaltet den Split-Screen ein oder aus.\n`F9` öffnet das Anwendungsmenü.\n`F10` öffnet die Konfiguration.\n`Esc` bricht den aktuellen Vorgang ab.",
    },
    {
        "file",
        "Dateihilfe",
        "main.file",
        "Dies ist das `Dateifenster`. Die ausgewählte Zeile ist eine `Datei`.\nDie Befehle in diesem `Footer` wirken auf sie, sofern die Zeile nicht ausdrücklich markierte Dateien nennt.\n\nDer `Footer` zeigt die hier verfügbaren Befehle.\nEinige Befehle haben eine eigene `F1`-Hilfe.\n\nDie Zifferntasten ändern die Anzeige.\n`1`: Namen anzeigen.\n`2`: Attribute anzeigen, bei symbolischen Links einschließlich `Name -> Ziel`.\n`3`: Eigentümer anzeigen.\n`4`: Zeiten anzeigen.\nDrücke die aktive Taste `2`, `3` oder `4` erneut, um zu den Namen zurückzukehren.\n`5`: In der Namensansicht `Kompakt` ein- oder ausschalten.\n`6`: Sichtbare Dateigrößen zwischen lesbaren und rohen Einheiten umschalten.\n`7`: Für jede sichtbare Datei eine kleine Textvorschau anzeigen.\n`8`: Für jede sichtbare Datei Details anzeigen.\n`9`: Das `Git-Band` anzeigen, wenn dieses Verzeichnis zu einem Git-Worktree gehört.\n\n`A`: Attribute der ausgewählten Datei öffnen.\n`C-a`: Attribute passender [markierter Dateien](topic:tagged) öffnen.\n`C`: [Kopieren](topic:copy-move-targets) für die ausgewählte Datei öffnen.\n`C-c`: Passende markierte Dateien kopieren.\n`D`: Die ausgewählte Datei löschen.\n`C-d`: Passende markierte Dateien löschen.\n\n`E`: Die ausgewählte Datei im konfigurierten Editor bearbeiten.\n`F`: Den [Filter](topic:filter) für diese Liste öffnen.\n`H`: Die ausgewählte Datei in der Hex-Ansicht öffnen.\n`I`: Markierungen in der sichtbaren Liste umkehren.\n\n`J`: Die ausgewählte Datei mit einer anderen Datei [vergleichen](topic:compare).\n`K`: Das Volumenmenü öffnen.\n`L`: `Pfad einlesen:` öffnen, ohne diese Liste zu verlassen. Der ausgewählte Pfad ist vorbelegt; `C-u` leert das Feld für einen anderen Pfad. Eingelesen werden nur Verzeichnisse und erkannte Archivdateien.\n`M`: [Verschieben](topic:copy-move-targets) für die ausgewählte Datei öffnen.\n`C-m`: Passende markierte Dateien verschieben, wenn das Tastaturprotokoll verfügbar ist.\n`C-n`: Passende markierte Dateien ohne Protokollunterstützung verschieben.\n`N`: Eine neue leere Datei anlegen.\n\n`O`: [Ausgabe](topic:output) für die ausgewählte Datei öffnen.\n`C-o`: Passende markierte Dateien ausgeben.\n`P`: Einen Befehl mit dem Inhalt der ausgewählten Datei als Eingabe ausführen.\n`C-p`: Diesen Befehl für passende markierte Dateien ausführen.\n`Q`: ytnova beenden.\n`R`: Die ausgewählte Datei umbenennen.\n`C-r`: Passende markierte Dateien umbenennen.\n\n`S`: Die Sortierreihenfolge der Dateiliste wählen.\n`C-s`: Markierte Dateien durchsuchen.\n`T`: Die ausgewählte Datei [markieren](topic:tagged) und zur nächsten Datei gehen.\n`C-t`: Alle sichtbaren Dateien markieren.\n`U`: Die Markierung der ausgewählten Datei entfernen und zur nächsten Datei gehen.\n`C-u`: Markierungen von allen sichtbaren Dateien entfernen.\n\n`V`: Die ausgewählte Datei ansehen.\n`C-v`: Passende markierte Dateien nacheinander ansehen.\n`X`: Einen Shell-Befehl eingeben. `{}` steht für den Pfad der ausgewählten Datei.\n`C-x`: Diesen Befehl einmal für jede passende markierte Datei ausführen.\n`Y`: Die ausgewählte Datei per [Pfadkopie](topic:copy-move-targets) kopieren und ihren Pfad relativ zum aktuellen Volumenstamm beibehalten.\n`C-y`: Passende markierte Dateien per Pfadkopie kopieren.\n`Z`: Markierte Dateien archivieren, oder die ausgewählte Datei, wenn keine markiert ist.\n`C-z`: Passende markierte Dateien archivieren.\n\n`/ jump` springt beim Tippen zu einem angezeigten Namen.\n`\\` zeigt oder versteckt verborgene Namen.\n\n`F5`: Die aktive Ansicht aktualisieren.\n`F6`: Die Statistikzeile zeigen oder verbergen.\n`F7`: Autoview ein- oder ausschalten.\n`F8`: Den Split-Screen ein- oder ausschalten.\n`F9`: Das Anwendungsmenü öffnen.\n`F10`: Die Konfiguration öffnen.\n`Esc`: Den aktuellen Vorgang abbrechen.",
    },
    {
        "archive-dir",
        "Archivverzeichnishilfe",
        "main.archive-dir",
        "Dies ist ein Verzeichnisbaum innerhalb eines Archivs. Seine Zeilen beschreiben Archiveinträge und keine normalen Dateisystemverzeichnisse.\nMit `Enter`, `Left` und `Right` bewegst du dich durch den Archivbaum.\n`\\` geht zur Archivwurzel; an der Wurzel verlässt es das Archiv.\n\n`1` bis `8` ändern die Archivansicht.\n`9` hat keine Wirkung, weil Archiveinträge keinen Git-Status haben.\n`0` zeigt oder verbirgt Größe, gepackte Größe und Verhältnis der Archivdateien.\n\n[Kopieren](topic:copy-move-targets) und `Pathcopy` extrahieren Einträge an ein Ziel.\n`Move`, `Delete`, `Rename` und `Makedir` werden nur angeboten, wenn das Archiv und die installierte libarchive das Schreiben des Ergebnisses unterstützen.\n[Filter](topic:filter), `Showall`, `Global`, `Tag`, `Untag` und `I` arbeiten im jeweiligen Bereich mit den Archiveinträgen.\n[Vergleichen](topic:compare), `Output` und `Pipe` lesen archivgestützte Pfade.\n`Log` öffnet einen unterstützten Archiveintrag als weiteres Archiv.\n`K` öffnet das Volumenmenü.\n`F5` aktualisiert die Ansicht.\n`F6` ändert die angezeigte Statistik.\n`F7` öffnet die Vorschau.\n`F8` öffnet den Split-Modus.\n`F9` öffnet Anwendungen.\n`F10` öffnet die Konfiguration.\n`Q` beendet ytnova.",
    },
    {
        "archive-file",
        "Archivdateihilfe",
        "main.archive-file",
        "Dies ist eine Dateiliste innerhalb eines Archivs. Die ausgewählte Zeile ist ein Archiveintrag und keine normale beschreibbare Datei.\n`Enter` kehrt zum Archivverzeichnisbaum zurück.\n`Left` und `Right` bewegen sich zwischen Dateispalten.\n\n`1` bis `8` ändern die Archivansicht.\n`9` hat keine Wirkung, weil Archiveinträge keinen Git-Status haben.\n`0` zeigt oder verbirgt Größe, gepackte Größe und Verhältnis.\n\n`V` zeigt den ausgewählten Eintrag an und `H` öffnet ihn hexadezimal.\n[Kopieren](topic:copy-move-targets) und `Y` Pathcopy extrahieren den ausgewählten Eintrag; ihre `C-`-Varianten verwenden markierte Einträge.\n`Move`, `Delete` und `Rename` sind nur verfügbar, wenn das Zurückschreiben ins Archiv unterstützt wird.\n`eXecute` ist für Archiveinträge nicht verfügbar.\n[Filter](topic:filter), `Sort`, `Tag`, `Untag`, `I` und die Suche in markierten Dateien bleiben in der archivgestützten Liste.\n[Vergleichen](topic:compare), `Output` und `Pipe` lesen den ausgewählten Eintrag oder die markierte Menge.\n`Log` öffnet den ausgewählten Eintrag als weiteres Archiv, wenn sein Format unterstützt wird.\n`K` öffnet das Volumenmenü.\n`F5` aktualisiert die Ansicht.\n`F6` ändert die angezeigte Statistik.\n`F7` öffnet die Vorschau.\n`F8` öffnet den Split-Modus.\n`F9` öffnet Anwendungen.\n`F10` öffnet die Konfiguration.\n`Q` beendet ytnova.",
    },
    {
        "filter",
        "Filterhilfe",
        "prompt.filter,prompt.filter-tagged",
        "Tippe ein Muster und drücke `Enter`, um die aktuelle Dateiliste zu filtern. Der Prompt beginnt mit `*`; das zeigt alle Dateien.\nVerwende `*.c` für ein Glob-Muster oder `*.c,*.h` für eines von beiden.\nEin vorangestelltes `-` schließt Treffer aus, zum Beispiel `*.c,-test*`.\nAttribute wie `:r` oder `:x`, Daten wie `>2024-01-01` und Größen wie `>1M` verwenden dieselbe kommagetrennte Syntax.\n\nWenn der aktuelle Bereich [markierte Dateien](topic:tagged) enthält, wechselt `Tab` zwischen allen und nur markierten Dateien, ohne Markierungen zu ändern.\nDer Filter betrifft nur die aktuelle normale, Archiv-, Showall- oder Global-Dateiliste. Er unterscheidet sich vom [/ Sprung](topic:list-jump), der nur die Auswahl bewegt.",
    },
    {
        "compare",
        "Vergleichshilfe",
        NULL,
        "Drücke `J`, um die ausgewählte Datei oder das Verzeichnis mit einem anderen Ziel zu vergleichen.\nWähle zuerst das [Ziel](topic:compare-target).\nWähle bei einem Verzeichnis den [Bereich](topic:compare-scope).\nWähle die [Vergleichsbasis](topic:compare-basis), wenn mehrere Möglichkeiten verfügbar sind.\nWähle danach, welches [Ergebnis](topic:compare-results) auf der Quellseite markiert wird.\n\nEin Vergleich des eingelesenen Baums verwendet nur bereits eingelesene Verzeichnisse; ungeöffnete `+`-Zweige werden nicht geöffnet.\nEin externer Verzeichnis- oder Baumvergleich startet `DIRDIFF` oder `TREEDIFF`, statt Ergebnisse zu markieren.\nDer Dateivergleich verwendet `FILEDIFF`; `%1` und `%2` stehen für die beiden Pfade, fehlende Platzhalter werden automatisch angehängt.\nDer Vergleich verändert keine Dateiinhalte und hat keinen eigenen Modus für markierte Dateien.",
    },
    {
        "compare-target",
        "Vergleichsziel",
        "prompt.compare-target",
        "Gib die Datei oder das Verzeichnis für den Vergleich mit der aktuellen Auswahl ein und drücke `Enter`.\n`Up` verwendet ein früheres Ziel.\n`F2` öffnet die [Verzeichnisauswahl](topic:f2-picker), wenn das Ziel gebrowst werden kann.\nIm [F8-Split](topic:f8) liefert das andere Panel das anfängliche Ziel.\nDer spätere [Vergleichsbereich](topic:compare-scope) entscheidet, ob ein Verzeichnisziel nur dieses Verzeichnis oder den eingelesenen Baum meint.\n`Esc` bricht den Vergleich ab.",
    },
    {
        "compare-scope",
        "Vergleichsbereich",
        NULL,
        "Wähle, wie viel vom ausgewählten Verzeichnis verglichen wird.\n`Directory` vergleicht eine Verzeichnisebene.\n`Logged tree` schließt den rekursiven Baum ein, den ytnova bereits eingelesen hat; ungeöffnete `+`-Zweige werden nicht automatisch gelesen.\n`External viewer` startet das konfigurierte Verzeichnis- oder Baumvergleichsprogramm, statt Ergebnisse in ytnova zu markieren.\nDrücke `Enter`, um fortzufahren, oder `Esc`, um abzubrechen. Unter [Vergleichen](topic:compare) steht der gesamte Ablauf.",
    },
    {
        "change-date",
        "Datum ändern",
        "prompt.change-date",
        "Gib ein Datum als `YYYY-MM-DD` ein, optional gefolgt von `HH:MM` oder `HH:MM:SS`.\nOhne Uhrzeit behält ytnova die vorhandene Stunde, Minute und Sekunde.\n`F3` wechselt zwischen Änderungszeit, Zugriffszeit und beiden Zeiten.\n`Enter` wendet die gewählte Einstellung an.\n`Esc` bricht ab.\nDie Aktion für markierte Dateien verwendet denselben Wert und dieselbe Auswahl für jede Datei.",
    },
    {
        "compare-basis",
        "Vergleichsbasis",
        NULL,
        "Wähle die Merkmale, anhand derer Einträge verglichen werden.\nNamens-, Größen- und Zeitvergleiche verwenden Dateimetadaten.\nVerwende `Hash`, wenn ein Inhaltsvergleich nötig ist und Metadaten nicht genügen.\nDrücke `Enter` zum Vergleichen oder `Esc` zum Abbrechen. Unter [Vergleichen](topic:compare) steht der gesamte Ablauf.",
    },
    {
        "compare-results",
        "Vergleichsergebnis",
        NULL,
        "Wähle die Ergebnisgruppe, die du behalten willst.\nYtreeNova markiert passende Einträge dieser Gruppe auf der aktiven Quellseite.\nDanach kannst du diese markierte Menge anzeigen, kopieren, verschieben, ausgeben oder archivieren.\nDer Vergleich überschreibt keine Seite. Unter [Markierte Dateien](topic:tagged) steht mehr zur entstehenden Arbeitsmenge.",
    },
    {
        "execute-file",
        "Datei ausführen",
        "prompt.execute-file",
        "Der Prompt beginnt mit `{}`; das steht für den Pfad der ausgewählten Datei.\nTippe den Befehl vor `{}` und Umleitung, Pipe oder andere Shell-Syntax dahinter.\nEin Beispiel ist `wc {} > count`.\n`Enter` führt den Befehl einmal für die ausgewählte Datei aus.\nDie markierte Aktion `C-x` wiederholt denselben Befehl für jede markierte Datei.\n`Esc` bricht ab. Unter [Kommandozeilenbearbeitung](topic:command-line-editing) stehen die Bearbeitungstasten.",
    },
    {
        "execute-dir",
        "Verzeichnis ausführen",
        "prompt.execute-dir",
        "Der Prompt beginnt mit `{}`; das steht für den Pfad des ausgewählten Verzeichnisses.\nTippe den Befehl vor `{}` und weitere Shell-Syntax dahinter.\nEin Beispiel ist `tar -cf archive.tar {}`.\n`Enter` führt den Befehl für dieses Verzeichnis aus.\nDie markierte Aktion `C-x` wiederholt den Befehl weiterhin für markierte Dateien der aktiven Liste und nicht für markierte Verzeichnisse.\n`Esc` bricht ab. Unter [Kommandozeilenbearbeitung](topic:command-line-editing) stehen die Bearbeitungstasten.",
    },
    {
        "search-tagged",
        "Markierte durchsuchen",
        "prompt.search-tagged",
        "Tippe einen Suchtext und drücke `Enter`, um nur die aktuellen [markierten](topic:tagged) Dateien zu durchsuchen.\nDateien ohne Treffer verlieren ihre Markierung; übrig bleibt eine kleinere markierte Menge.\n`Up` verwendet eine frühere Suche.\n`Esc` bricht ab, ohne die Menge zu ändern.",
    },
    {
        "create-archive",
        "Archiv erstellen",
        "prompt.create-archive",
        "Gib den Pfad des neuen Archivs ein und drücke `Enter`.\nYtreeNova archiviert zuerst markierte Dateien. Sind keine Dateien markiert, archiviert es die aktuelle Auswahl.\nEin ausgewähltes Verzeichnis wird rekursiv aufgenommen.\nDie Dateiendung wählt das Archivformat, soweit die installierte libarchive es unterstützt.\n`Up` verwendet einen früheren Pfad.\n`F2` öffnet die [Verzeichnisauswahl](topic:f2-picker).\n`Esc` bricht ab.",
    },
    {
        "output",
        "Ausgabehilfe",
        NULL,
        "`Output` exportiert die ausgewählte Datei oder bei der `C-`-Variante die markierte Menge.\nWähle ein [Ziel](topic:output-destination): einen Dateipfad oder Hardcopy.\nBei Dateiausgabe wechselt `F3` zwischen [Raw, Framed und Page break](topic:output-format).\nFramed und Page break fragen danach nach einem [Trenner](topic:output-separator) und kehren zum Zielprompt zurück.\nHardcopy fragt nach einem Druckbefehl und sendet immer Rohdaten.\n`Enter` übernimmt jede Auswahl; `Esc` bricht den aktuellen Prompt ab.",
    },
    {
        "output-format",
        "Ausgabeformat",
        NULL,
        "Mit `F3` wählst du im Dateiziel-Prompt das Ausgabeformat.\n`Raw` verbindet die exportierten Dateiinhalte ohne Darstellungsrahmen.\n`Framed` trennt Dateien mit dem Text aus dem Trenner-Prompt.\n`Page break` trennt Dateien für eine seitenorientierte Ausgabe.\nDrücke `Enter`, um fortzufahren. Hardcopy verwendet immer Raw und bietet diese Wahl nicht an. Unter [Ausgabe](topic:output) steht der gesamte Ablauf.",
    },
    {
        "output-destination",
        "Ausgabeziel",
        "prompt.output-destination",
        "Wähle, wohin der exportierte Text geschrieben wird.\nWähle Dateiausgabe, um einen Pfad einzugeben. Ein einfacher Dateiname ist relativ zu `CWD`, dem im Prompt gezeigten Arbeitsverzeichnis.\nMit `F3` änderst du hier das [Ausgabeformat](topic:output-format).\nWähle Hardcopy für einen Druckbefehl wie `lpr`, `lp` oder `cat > /dev/lp1`; Hardcopy sendet Rohdaten.\n`Up` verwendet ein früheres Ziel.\n`F2` öffnet bei Dateiausgabe die [Verzeichnisauswahl](topic:f2-picker).\n`Enter` übernimmt das Ziel und `Esc` bricht ab.",
    },
    {
        "output-separator",
        "Ausgabetrenner",
        "prompt.output-separator",
        "Gib den Text ein, der bei Framed oder Page break zwischen exportierten Dateien steht.\nDer Trenner wird zwischen Dateien und nicht hinter der letzten Datei eingefügt.\n`Up` verwendet einen früheren Trenner.\n`Enter` kehrt zum Zielprompt zurück.\n`Esc` bricht ab. Raw und Hardcopy überspringen diesen Prompt. Unter [Ausgabe](topic:output) steht der gesamte Ablauf.",
    },
    {
        "showall",
        "Showall-Hilfe",
        "main.showall",
        "`Showall` zeigt Dateien aus allen eingelesenen Verzeichnissen des aktuellen Volumens in einer Liste. Andere Volumen sind nicht enthalten.\n`Esc` kehrt zum vorherigen Verzeichnis zurück.\n`\\` öffnet das besitzende Verzeichnis der ausgewählten Datei im aktuellen Volumen.\n\n[Dateibefehle](topic:file) wirken auf die ausgewählte Zeile oder die markierte Menge; Filter, Sortierung, Markierungen und `/` verwenden jedoch diese zusammengefasste Ergebnisliste als Bereich.\nDer [Filter](topic:filter) ändert nur diese Showall-Liste.\n`G` öffnet [Global](topic:global), wenn du alle eingelesenen Volumen brauchst.\n`F7` öffnet die Vorschau und `F8` den Split-Modus.",
    },
    {
        "global",
        "Global-Hilfe",
        "main.global",
        "`Global` zeigt Dateien aus allen eingelesenen Verzeichnissen aller eingelesenen Volumen in einer Liste.\n`Esc` kehrt zur vorherigen Verzeichnisansicht zurück.\n`\\` öffnet das besitzende Volumen und Verzeichnis der ausgewählten Datei.\n\n[Dateibefehle](topic:file) wirken auf die ausgewählte Zeile oder die markierte Menge; Filter, Sortierung, Markierungen und `/` verwenden jedoch die vollständige Global-Ergebnisliste als Bereich.\nDer [Filter](topic:filter) ändert nur diese Global-Liste.\nErneutes `G` hat keine Wirkung, weil Global bereits geöffnet ist.\n`F7` öffnet die Vorschau und `F8` den Split-Modus.",
    },
    {
        "f7",
        "F7-Vorschau",
        "overlay.f7-dir,overlay.f7-file",
        "`F7` öffnet eine Vorschau der ausgewählten Datei, ohne ihren Auswahlkontext zu verlassen.\n`Up`, `Down`, `Page Up`, `Page Down`, `Home` und `End` bewegen weiterhin die Dateiauswahl.\n`Shift-Up`, `Shift-Down`, `C-p` und `C-n` scrollen die Vorschau zeilenweise.\n`Shift-Page Up` und `Shift-Page Down` scrollen seitenweise.\n`Shift-Home` und `Shift-End` gehen zum Anfang oder Ende der Vorschau.\n`F7` oder `Esc` kehrt zur darunterliegenden Ansicht zurück.\n\nDie üblichen [Dateiaktionen](topic:file) bleiben für die ausgewählte Datei verfügbar. Markierte Varianten behalten ihren aktuellen Bereich.\n`F8` und `Tab` öffnen oder wechseln keine Split-Panels, solange die Vorschau aktiv ist.\n`F9` öffnet Anwendungen, ohne zuerst die Vorschau zu schließen.",
    },
    {
        "f8",
        "F8-Split",
        NULL,
        "`F8` öffnet ein zweites Panel. Das hervorgehobene Panel ist aktiv und erhält den nächsten Befehl.\n`Tab` wechselt das aktive Panel.\nJedes Panel behält seine Auswahl, sein eingelesenes Volumen, seine Markierungen, Ansichtseinstellungen und seinen Rückkehrzustand.\n\nKopier-, Verschiebe- und Vergleichsprompts verwenden gegebenenfalls das andere Panel als anfängliches Ziel. Du kannst den Wert vor dem Fortfahren ändern.\nDrücke erneut `F8`, um zu einem Panel zurückzukehren. Unter [Split-Verzeichnis](topic:f8-dir) oder [Split-Datei](topic:f8-file) stehen die lokalen Befehle.",
    },
    {
        "f8-dir",
        "F8-Split-Verzeichnis",
        "overlay.f8-dir",
        "Dies ist der aktive Verzeichnisbaum im [F8-Split](topic:f8).\n`Tab` aktiviert das andere Panel.\n`F8` kehrt zu einem Panel zurück.\n\nBaum- und Verzeichnisbefehle entsprechen der [Verzeichnishilfe](topic:directory).\nKopieren, Verschieben und Vergleichen beginnen gegebenenfalls mit dem ausgewählten Verzeichnis des anderen Panels als Ziel.\nEin Prompt kann `Tab` für seine eigene lokale Wahl verwenden, statt das Panel zu wechseln.\nBefehle ändern nur das aktive Panel, sofern ihr Prompt das andere Panel nicht ausdrücklich nennt.",
    },
    {
        "f8-file",
        "F8-Split-Datei",
        "overlay.f8-file",
        "Dies ist die aktive Dateiliste im [F8-Split](topic:f8).\n`Tab` aktiviert das andere Panel.\n`F8` kehrt zu einem Panel zurück.\n\nDateibefehle entsprechen der [Dateihilfe](topic:file).\nKopieren, Verschieben und Vergleichen beginnen gegebenenfalls mit dem ausgewählten Verzeichnis oder der Datei des anderen Panels als Ziel.\nMarkierte Befehle verwenden die markierte Menge des aktiven Panels.\nEin Prompt kann `Tab` für seine eigene lokale Wahl verwenden, statt das Panel zu wechseln.",
    },
    {
        "history-dialog",
        "Verlauf",
        "dialog.history",
        "Mit `Up` und `Down` wählst du einen früheren Prompt-Wert.\nMit `Left` und `Right` scrollst du einen langen Wert waagerecht.\n`P` heftet den gewählten Wert an oder löst ihn.\n`D` löscht ihn.\n`Enter` setzt ihn wieder in den Prompt ein.\n`Esc` schließt den Verlauf ohne Auswahl.",
    },
    {
        "volume-menu",
        "Volumen",
        "dialog.volume-menu",
        "Mit `Up` und `Down` wählst du ein eingelesenes Volumen.\n`Enter` wechselt dorthin und stellt seinen Zustand im Speicher wieder her.\nDie Wahl des aktiven Volumens behält dessen aktuellen Zustand.\n`D` gibt das gewählte Volumen frei; das letzte verbleibende Volumen kann nicht freigegeben werden.\n`Esc` schließt das Menü ohne Wechsel.",
    },
    {
        "applications-menu",
        "Anwendungen",
        "dialog.applications",
        "Mit `Up` und `Down` wählst du ein konfiguriertes Anwendungs-Preset.\n`Enter` startet es und kehrt sofort zu ytnova zurück.\n`E` bearbeitet den Anwendungskatalog.\n`Esc` schließt das Menü.\n\n`{}` setzt die ausgewählte Datei oder das Verzeichnis ein. Das Preset startet auch ohne `{}` im Verzeichnis dieser Auswahl.\n`{input}` setzt den Text ein, den der Eingabeprompt des Presets erfragt.\nVerwende `F9` für wiederholbare Presets und `eXecute` für einen einmaligen Shell-Befehl.",
    },
    {
        "f2-picker",
        "F2-Auswahl",
        "dialog.f2-picker",
        "Verwende `F2` in einem Prompt, der ein gebrowstes Verzeichnis akzeptiert.\n`Up` und `Down` bewegen sich durch den Baum.\n`Left` klappt einen Zweig zu oder geht zum übergeordneten Verzeichnis.\n`Right` klappt einen Zweig auf oder geht hinein.\n`<` und `>` wechseln zwischen eingelesenen Volumen.\n`L` liest ein weiteres Verzeichnis oder Archiv ein.\n`Backtick` zeigt oder verbirgt versteckte Einträge.\n`Enter` übernimmt das hervorgehobene Verzeichnis in den Prompt.\n`Esc` kehrt zurück, ohne den Prompt zu ändern.",
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
            "Enter/Right follow",
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
            "N",
            "Enter/Right folgen",
            "Esc/Q schließen",
        },
    },
};

static const size_t generated_help_catalog_count = 2;
