/***************************************************************************
 *
 * src/ui/interactions.c
 * UI Prompts and Interaction Functions
 *
 ***************************************************************************/

#include "sort.h"
#include "watcher.h"
#include "ytnova_appstate_focus.h"
#include "ytnova_appstate_panel.h"
#include "ytnova_appstate_visibility.h"
#include "ytnova_cmd.h"
#include "ytnova_dialog.h"
#include "ytnova_fs.h"
#include "ytnova_ui.h"
#include "interactions_panel_paths.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <libgen.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

typedef enum {
  PROMPT_HELP_EXECUTE_DIRECTORY = 0,
  PROMPT_HELP_EXECUTE_FILE,
  PROMPT_HELP_SEARCH_TAGGED,
  PROMPT_HELP_CREATE_ARCHIVE
} PromptHelpTopic;

static const UICommandStripCommand sort_by_commands[] = {
    {UI_COMMAND_LAYOUT_MNEMONIC, NP_("sort.commands", "AccTime"), "A", NULL,
     "sort.commands"},
    {UI_COMMAND_LAYOUT_MNEMONIC, NP_("sort.commands", "ChgTime"), "C", NULL,
     "sort.commands"},
    {UI_COMMAND_LAYOUT_MNEMONIC, NP_("sort.commands", "Extension"), "E", NULL,
     "sort.commands"},
    {UI_COMMAND_LAYOUT_MNEMONIC, NP_("sort.commands", "Group"), "G", NULL,
     "sort.commands"},
    {UI_COMMAND_LAYOUT_MNEMONIC, NP_("sort.commands", "ModTime"), "M", NULL,
     "sort.commands"},
    {UI_COMMAND_LAYOUT_MNEMONIC, NP_("sort.commands", "Name"), "N", NULL,
     "sort.commands"},
    {UI_COMMAND_LAYOUT_MNEMONIC, NP_("sort.commands", "Size"), "S", NULL,
     "sort.commands"}};
static const UICommandStripCommand sort_commands_ascending[] = {
    {UI_COMMAND_LAYOUT_MNEMONIC, NP_("sort.commands", "Owner"), "W", NULL,
     "sort.commands"},
    {UI_COMMAND_LAYOUT_MNEMONIC, NP_("sort.commands", "Order [ascending]"),
     "O", NULL, "sort.commands"}};
static const UICommandStripCommand sort_commands_descending[] = {
    {UI_COMMAND_LAYOUT_MNEMONIC, NP_("sort.commands", "Owner"), "W", NULL,
     "sort.commands"},
    {UI_COMMAND_LAYOUT_MNEMONIC, NP_("sort.commands", "Order [descending]"),
     "O", NULL, "sort.commands"}};

static void CopyBoundedString(char *dst, size_t dst_size, const char *src) {
  int written;

  if (!dst || dst_size == 0)
    return;

  if (!src)
    src = "";

  written = snprintf(dst, dst_size, "%s", src);
  if (written < 0) {
    dst[0] = '\0';
  } else if ((size_t)written >= dst_size) {
    dst[dst_size - 1] = '\0';
  }
}

static void SeedDestinationDirectoryFromInactivePanel(ViewContext *ctx,
                                                      char *to_dir) {
  if (!ctx || !to_dir || to_dir[0] != '\0' || !ctx->is_split_screen ||
      !ctx->active)
    return;

  {
    YtreeNovaPanel *target = (ctx->active == ctx->left) ? ctx->right : ctx->left;
    if (target && target->vol && target->vol->total_dirs > 0) {
      int idx = target->disp_begin_pos + target->cursor_pos;
      if (idx < 0)
        idx = 0;
      if (idx >= target->vol->total_dirs)
        idx = target->vol->total_dirs - 1;

      GetPath(target->vol->dir_entry_list[idx].dir_entry, to_dir);
    }
  }
}

int GetDestinationDirectoryParameter(ViewContext *ctx, char *to_dir) {
  if (!ctx || !to_dir)
    return -1;

  ClearHelp(ctx);

  if (UI_ReadString(ctx, ctx->active, "To Directory:", to_dir, PATH_LENGTH,
                    HST_PATH) == CR) {
    if (to_dir[0] == '\0') {
      CopyBoundedString(to_dir, PATH_LENGTH + 1, ".");
    }
    return 0;
  }

  ClearHelp(ctx);
  return -1;
}

int ResolveDestinationDirectoryPath(DirEntry *current_dir_entry,
                                    const char *dir_path,
                                    char *resolved_path) {
  char path[PATH_LENGTH + 1];

  if (!current_dir_entry || !dir_path || !resolved_path)
    return -1;

  if (*dir_path == FILE_SEPARATOR_CHAR) {
    CopyBoundedString(path, sizeof(path), dir_path);
  } else {
    char current_path[PATH_LENGTH + 1];

    GetPath(current_dir_entry, current_path);
    if (Path_Join(path, sizeof(path), current_path, dir_path) != 0)
      return -1;
  }

  NormPath(path, resolved_path);
  return (resolved_path[0] != '\0') ? 0 : -1;
}

int UI_EnsureCopyMoveDestinationDirectory(ViewContext *ctx, char *dir_path,
                                          DirEntry *tree,
                                          DirEntry **result_ptr,
                                          int *auto_create) {
  DIR *tmpdir;
  int create_mode = 0;
  char prompt[PATH_LENGTH + 64];
  char normalized_path[PATH_LENGTH + 1];

  if (!ctx || !dir_path)
    return -1;

  NormPath(dir_path, normalized_path);
  if (normalized_path[0] == '\0') {
    MESSAGE(ctx, "Invalid destination path*\"%s\"", dir_path);
    return -1;
  }
  CopyBoundedString(dir_path, PATH_LENGTH + 1, normalized_path);

  tmpdir = opendir(dir_path);
  if (tmpdir != NULL) {
    closedir(tmpdir);
  } else if (errno == ENOENT) {
    (void)snprintf(prompt, sizeof(prompt), "Create missing directory? (y/N) %s",
                   dir_path);
    if (InputChoiceLiteral(ctx, prompt, "YN\033") != 'Y')
      return 1;
    create_mode = 1;
  } else {
    MESSAGE(ctx, "Can't access destination directory*\"%s\"*%s", dir_path,
            strerror(errno));
    return -1;
  }

  if (auto_create) {
    if (create_mode)
      *auto_create = 1;
    create_mode = *auto_create;
  }

  if (EnsureDirectoryExists(ctx, dir_path, tree, NULL, result_ptr,
                            &create_mode, NULL) != 0) {
    if (auto_create && create_mode)
      *auto_create = create_mode;
    return -1;
  }

  if (auto_create && create_mode)
    *auto_create = create_mode;
  return 0;
}

static void GetPromptHelpContext(PromptHelpTopic topic, const char **context_id) {
  if (!context_id)
    return;

  switch (topic) {
  case PROMPT_HELP_EXECUTE_DIRECTORY:
    *context_id = "prompt.execute-dir";
    break;
  case PROMPT_HELP_EXECUTE_FILE:
    *context_id = "prompt.execute-file";
    break;
  case PROMPT_HELP_SEARCH_TAGGED:
    *context_id = "prompt.search-tagged";
    break;
  case PROMPT_HELP_CREATE_ARCHIVE:
  default:
    *context_id = "prompt.create-archive";
    break;
  }
}

static void ShowPromptHelpPopup(ViewContext *ctx, PromptHelpTopic topic) {
  const char *context_id = NULL;

  if (!ctx)
    return;

  GetPromptHelpContext(topic, &context_id);
  if (context_id != NULL)
    (void)UI_ShowGeneratedContextHelp(ctx, context_id, NULL, 0);
}

static int ShowPromptHelpCallback(ViewContext *ctx, void *help_data) {
  PromptHelpTopic topic = PROMPT_HELP_EXECUTE_FILE;

  if (help_data != NULL)
    topic = *(PromptHelpTopic *)help_data;

  ShowPromptHelpPopup(ctx, topic);
  return 0;
}

static void DrawSortPrompt(ViewContext *ctx, WINDOW *win, BOOL ascending) {
  int y0;

  if (!ctx || !win)
    return;

  if (win == stdscr) {
    y0 = Y_PROMPT(ctx);
    wmove(win, y0, 0);
    wclrtoeol(win);
    wmove(win, y0 + 1, 0);
    wclrtoeol(win);
    if (y0 > 0) {
      wmove(win, y0 - 1, 0);
      wclrtoeol(win);
    }
  } else {
    y0 = 0;
    werase(win);
  }

  Print(win, y0, 0, "SORT by", UI_ROLE_STATIC_TEXT);
  UI_RenderAdaptiveCommandStrip(
      win, y0, StrVisualLength("SORT by") + 2, sort_by_commands,
      sizeof(sort_by_commands) / sizeof(sort_by_commands[0]),
      UI_ROLE_STATIC_TEXT, UI_ROLE_KEYBIND);
  Print(win, y0 + 1, 0, "COMMANDS", UI_ROLE_STATIC_TEXT);
  UI_RenderAdaptiveCommandStrip(
      win, y0 + 1, StrVisualLength("COMMANDS") + 2,
      ascending ? sort_commands_ascending : sort_commands_descending, 2,
      UI_ROLE_STATIC_TEXT, UI_ROLE_KEYBIND);
  wnoutrefresh(win);
  doupdate();
}

int UI_ConflictResolverWrapper(ViewContext *ctx, const char *src_path,
                               const char *dst_path, int *mode_flags) {
  return UI_AskConflict(ctx, src_path, dst_path, mode_flags);
}

int UI_ChoiceResolver(ViewContext *ctx, const char *prompt,
                      const char *choices) {
  return InputChoice(ctx, prompt, choices);
}

int UI_CoreQuitConfirm(ViewContext *ctx, const char *msg, const char *choices) {
  return InputChoice(ctx, msg, choices);
}

int UI_CoreQuitSaveHistory(ViewContext *ctx, const char *path_for_history) {
  return SaveHistory(ctx, path_for_history);
}

void UI_CoreQuitCloseWatcher(ViewContext *ctx) { Watcher_Close(ctx); }

void UI_CoreQuitCleanupVolumeTree(ViewContext *ctx) {
  Volume_FreeAll(ctx);
  FreeDirEntryList(ctx);
}

void UI_CoreQuitSuspendClock(ViewContext *ctx) { SuspendClock(ctx); }

void UI_CoreQuitShutdownTerminal(ViewContext *ctx) {
  attrset(0);  /* Reset attributes */
  clear();     /* Clear internal buffer */
  refresh();   /* Push clear to screen */
  curs_set(1); /* Restore visible cursor */
  ShutdownCurses(ctx);
#ifdef XCURSES
  XCursesExit();
#endif
}

int GetMoveParameter(ViewContext *ctx, const char *from_file, char *to_file,
                     char *to_dir) {
  char prompt_header[PATH_LENGTH + 50];
  UIPromptOptions options;

  if (from_file == NULL) {
    from_file = "TAGGED FILES";
    CopyBoundedString(to_file, PATH_LENGTH + 1, "*");
  } else {
    CopyBoundedString(to_file, PATH_LENGTH + 1, from_file);
  }

  (void)snprintf(prompt_header, sizeof(prompt_header), "MOVE: %s AS:",
                 from_file);

  ClearHelp(ctx);

  memset(&options, 0, sizeof(options));
  options.suppress_final_refresh = TRUE;
  if (UI_ReadStringWithPromptOptions(ctx, ctx->active, prompt_header, to_file,
                                     PATH_LENGTH, HST_FILE, &options) == CR) {
    SeedDestinationDirectoryFromInactivePanel(ctx, to_dir);
    if (GetDestinationDirectoryParameter(ctx, to_dir) == 0)
      return 0;
  }
  ClearHelp(ctx);
  return (-1);
}

int GetCopyParameter(ViewContext *ctx, const char *from_file, BOOL path_copy,
                     char *to_file, char *to_dir) {
  char prompt_header[PATH_LENGTH + 50];
  UIPromptOptions options;

  if (from_file == NULL) {
    from_file = "TAGGED FILES";
    CopyBoundedString(to_file, PATH_LENGTH + 1, "*");
  } else {
    CopyBoundedString(to_file, PATH_LENGTH + 1, from_file);
  }

  if (path_copy) {
    (void)snprintf(prompt_header, sizeof(prompt_header), "PATHCOPY: %s AS:",
                   from_file);
  } else {
    (void)snprintf(prompt_header, sizeof(prompt_header), "COPY: %s AS:",
                   from_file);
  }

  ClearHelp(ctx);

  memset(&options, 0, sizeof(options));
  options.suppress_final_refresh = TRUE;
  if (UI_ReadStringWithPromptOptions(ctx, ctx->active, prompt_header, to_file,
                                     PATH_LENGTH, HST_FILE, &options) == CR) {
    SeedDestinationDirectoryFromInactivePanel(ctx, to_dir);
    if (GetDestinationDirectoryParameter(ctx, to_dir) == 0)
      return 0;
  }
  ClearHelp(ctx);
  return (-1);
}

int UI_GatherArchivePayload(ViewContext *ctx, DirEntry *selected_dir,
                            FileEntry *selected_file,
                            ArchivePayload *payload) {
  char **selected_paths = NULL;
  size_t selected_count = 0;
  BOOL recursive_directories = TRUE;
  int rc = -1;

  if (!ctx || !ctx->active || !payload)
    return -1;

  UI_FreeArchivePayload(payload);

  BuildFileEntryList(ctx, ctx->active);
  if (ctx->active->file_entry_list && ctx->active->file_count > 0) {
    size_t i;
    size_t tagged_count = 0;

    for (i = 0; i < (size_t)ctx->active->file_count; ++i) {
      if (ctx->active->file_entry_list[i].file &&
          ctx->active->file_entry_list[i].file->tagged)
        tagged_count++;
    }

    if (tagged_count > 0) {
      selected_paths = (char **)xcalloc(tagged_count, sizeof(*selected_paths));
      if (!selected_paths)
        return -1;

      for (i = 0; i < (size_t)ctx->active->file_count; ++i) {
        char file_path[PATH_LENGTH + 1];
        FileEntry *fe = ctx->active->file_entry_list[i].file;

        if (!fe || !fe->tagged)
          continue;

        GetFileNamePath(fe, file_path);
        file_path[PATH_LENGTH] = '\0';
        selected_paths[selected_count] = xstrdup(file_path);
        if (!selected_paths[selected_count])
          goto cleanup;

        selected_count++;
      }
    }
  }

  if (selected_count == 0) {
    char source_path[PATH_LENGTH + 1];
    struct stat source_st;
    int recursive_choice;

    selected_paths = (char **)xcalloc(1, sizeof(*selected_paths));
    if (!selected_paths)
      return -1;

    if (selected_file) {
      GetFileNamePath(selected_file, source_path);
      source_path[PATH_LENGTH] = '\0';
    } else if (selected_dir) {
      GetPath(selected_dir, source_path);
      source_path[PATH_LENGTH] = '\0';
    } else if (UI_GetPanelSelectedFilePath(ctx, ctx->active, source_path) == 0) {
      source_path[PATH_LENGTH] = '\0';
    } else if (UI_GetPanelSelectedDirPath(ctx, ctx->active, source_path) == 0) {
      source_path[PATH_LENGTH] = '\0';
    } else {
      goto cleanup;
    }

    selected_paths[0] = xstrdup(source_path);
    if (!selected_paths[0])
      goto cleanup;
    selected_count = 1;

    if (lstat(source_path, &source_st) != 0) {
      UI_ShowStatusLineError(ctx, "Cannot access selected source path");
      goto cleanup;
    }
    if (S_ISDIR(source_st.st_mode)) {
      recursive_choice = InputChoiceLiteral(ctx, "Recursive? (Y/n)", "YN\033");
      if (recursive_choice == ESC) {
        rc = 1;
        goto cleanup;
      }
      if (recursive_choice == 'N')
        recursive_directories = FALSE;
    }
  }

  rc = UI_BuildArchivePayloadFromPaths((const char *const *)selected_paths,
                                       selected_count, recursive_directories,
                                       payload);

cleanup:
  if (selected_paths) {
    size_t i;
    for (i = 0; i < selected_count; ++i)
      free(selected_paths[i]);
    free(selected_paths);
  }
  if (rc < 0)
    UI_FreeArchivePayload(payload);
  return rc;
}

static int ResolveArchiveDestinationPath(ViewContext *ctx, const char *input_path,
                                         char *resolved_path,
                                         size_t resolved_size) {
  char absolute_input[PATH_LENGTH + 1];
  char normalized_input[PATH_LENGTH + 1];
  char parent_path[PATH_LENGTH + 1];
  char resolved_parent[PATH_LENGTH + 1];
  const char *slash;
  const char *file_name;
  int written;
  int parent_written;

  if (!input_path || !resolved_path || resolved_size == 0)
    return -1;
  if (input_path[0] == '\0')
    return -1;

  if (input_path[0] == FILE_SEPARATOR_CHAR) {
    written = snprintf(absolute_input, sizeof(absolute_input), "%s", input_path);
  } else {
    char cwd[PATH_LENGTH + 1];
    if (ctx && ctx->active &&
        UI_GetPanelSelectedDirPath(ctx, ctx->active, cwd) == 0) {
      cwd[PATH_LENGTH] = '\0';
    } else if (!getcwd(cwd, sizeof(cwd))) {
      return -1;
    }
    written = snprintf(absolute_input, sizeof(absolute_input), "%s%c%s", cwd,
                       FILE_SEPARATOR_CHAR, input_path);
  }
  if (written < 0 || (size_t)written >= sizeof(absolute_input))
    return -1;

  NormPath(absolute_input, normalized_input);
  if (realpath(normalized_input, resolved_path) != NULL)
    return 0;

  if (errno != ENOENT)
    return -1;

  slash = strrchr(normalized_input, FILE_SEPARATOR_CHAR);
  if (!slash)
    return -1;

  file_name = slash + 1;
  if (file_name[0] == '\0')
    return -1;

  if (slash == normalized_input) {
    parent_written = snprintf(parent_path, sizeof(parent_path), "%s",
                              FILE_SEPARATOR_STRING);
  } else {
    size_t parent_len = (size_t)(slash - normalized_input);
    parent_written = snprintf(parent_path, sizeof(parent_path), "%.*s",
                              (int)parent_len, normalized_input);
  }
  if (parent_written < 0 || (size_t)parent_written >= sizeof(parent_path))
    return -1;

  if (realpath(parent_path, resolved_parent) == NULL)
    return -1;

  if (strcmp(resolved_parent, FILE_SEPARATOR_STRING) == 0) {
    written = snprintf(resolved_path, resolved_size, "%s%s", resolved_parent,
                       file_name);
  } else {
    written = snprintf(resolved_path, resolved_size, "%s%c%s", resolved_parent,
                       FILE_SEPARATOR_CHAR, file_name);
  }
  if (written < 0 || (size_t)written >= resolved_size)
    return -1;

  return 0;
}

static BOOL SourcePathMatchesArchiveDestination(const char *source_path,
                                                const char *dest_path) {
  char resolved_source[PATH_LENGTH + 1];

  if (!source_path || !dest_path)
    return FALSE;

  if (realpath(source_path, resolved_source) == NULL)
    return FALSE;

  return strcmp(resolved_source, dest_path) == 0;
}

static const char *UnsupportedArchiveLabel(const char *dest_path) {
  const char *dot;

  if (!dest_path)
    return "(none)";

  dot = strrchr(dest_path, '.');
  if (!dot || dot[1] == '\0')
    return "(none)";
  return dot;
}

int UI_CreateArchiveFromPayload(ViewContext *ctx, const ArchivePayload *payload) {
  char destination_input[PATH_LENGTH + 1];
  char destination_path[PATH_LENGTH + 1];
  const char *filename = NULL;
  int input_result;
  PromptHelpTopic help_topic = PROMPT_HELP_CREATE_ARCHIVE;
  struct stat dest_stat;
  int prompt_written;
  char overwrite_prompt[PATH_LENGTH + 64];

  if (!ctx || !ctx->active || !payload)
    return -1;
  if (!payload->original_source_list)
    return -1;

  destination_input[0] = '\0';
  input_result = UI_ReadStringWithHelp(
      ctx, ctx->active,
      "Create archive: (suffix .tar .tar.gz/.tgz .tar.bz2/.tbz2 .tar.xz/.txz .zip) ",
      destination_input, PATH_LENGTH, HST_FILE, NULL, 0,
      ShowPromptHelpCallback, &help_topic);
  if (input_result != CR || destination_input[0] == '\0')
    return 1;

  if (ResolveArchiveDestinationPath(ctx, destination_input, destination_path,
                                    sizeof(destination_path)) != 0) {
    UI_ShowStatusLineError(ctx, "Invalid archive destination path");
    return -1;
  }

  filename = strrchr(destination_path, FILE_SEPARATOR_CHAR);
  if (filename && filename[1] != '\0')
    filename++;
  else
    filename = destination_path;
  if (filename[0] == '\0' ||
      !strcasecmp(filename, ".tar") || !strcasecmp(filename, ".tar.gz") ||
      !strcasecmp(filename, ".tgz") || !strcasecmp(filename, ".tar.bz2") ||
      !strcasecmp(filename, ".tbz2") || !strcasecmp(filename, ".tar.xz") ||
      !strcasecmp(filename, ".txz") || !strcasecmp(filename, ".zip")) {
    UI_ShowStatusLineError(ctx, "Archive name required before suffix");
    return -1;
  }

  if (lstat(destination_path, &dest_stat) == 0) {
    prompt_written = snprintf(overwrite_prompt, sizeof(overwrite_prompt),
                              "Overwrite %s? (y/n)", filename);
    if (prompt_written < 0 || (size_t)prompt_written >= sizeof(overwrite_prompt))
      return -1;
    if (InputChoiceLiteral(ctx, overwrite_prompt, "YN\033") != 'Y')
      return 1;
  } else if (errno != ENOENT) {
    UI_ShowStatusLineError(ctx, "Cannot access destination path");
    return -1;
  }

#ifdef HAVE_LIBARCHIVE
  ArchiveExpandedEntry *entry;
  const char **source_paths = NULL;
  const char **archive_paths = NULL;
  size_t source_count = 0;
  size_t idx = 0;
  int rc;

  for (entry = payload->expanded_file_list; entry; entry = entry->next) {
    if (SourcePathMatchesArchiveDestination(entry->source_path, destination_path))
      continue;
    source_count++;
  }

  if (source_count == 0) {
    UI_ShowStatusLineError(ctx, "Nothing to archive");
    return -1;
  }

  source_paths = (const char **)xcalloc(source_count, sizeof(*source_paths));
  archive_paths = (const char **)xcalloc(source_count, sizeof(*archive_paths));
  if (!source_paths || !archive_paths) {
    free((void *)source_paths);
    free((void *)archive_paths);
    UI_ShowStatusLineError(ctx, "Out of memory while creating archive");
    return -1;
  }
  for (entry = payload->expanded_file_list; entry; entry = entry->next) {
    if (SourcePathMatchesArchiveDestination(entry->source_path, destination_path))
      continue;
    source_paths[idx++] = entry->source_path;
    archive_paths[idx - 1] = entry->archive_path;
  }

  rc = Archive_CreateFromPaths(destination_path, source_paths, archive_paths,
                               source_count);
  free((void *)source_paths);
  free((void *)archive_paths);
  if (rc == 0)
    return 0;
  if (rc == UNSUPPORTED_FORMAT_ERROR) {
    UI_ShowStatusLineError(ctx, "Unsupported archive format: %s",
                           UnsupportedArchiveLabel(destination_path));
  } else {
    UI_ShowStatusLineError(ctx, "Failed to create archive");
  }
  return -1;
#else
  UI_ShowStatusLineError(ctx, "Archive creation requires libarchive support");
  return -1;
#endif
}

int GetRenameParameter(ViewContext *ctx, const char *old_name, char *new_name) {
  const char *prompt;

  if (old_name == NULL) {
    prompt = "RENAME TAGGED FILES TO:";
  } else {
    prompt = "RENAME TO:";
  }

  CopyBoundedString(new_name, PATH_LENGTH + 1, (old_name) ? old_name : "*");

  if (UI_ReadString(ctx, ctx->active, prompt, new_name, PATH_LENGTH,
                    HST_FILE) != CR)
    return (-1);

  if (!strlen(new_name))
    return (-1);

  if (old_name && !strcmp(old_name, new_name)) {
    UI_Message(ctx, "Can't rename: New name same as old name.");
    return (-1);
  }

  if (strrchr(new_name, FILE_SEPARATOR_CHAR) != NULL) {
    UI_Message(ctx, "Invalid new name:*No slashes when renaming!");
    return (-1);
  }

  return (0);
}

int UI_ArchiveCallback(int status, const char *msg, void *user_data) {
  ViewContext *ctx = (ViewContext *)user_data;
  if (status == ARCHIVE_STATUS_PROGRESS) {
    if (ctx && Progress_ShouldRender(ctx))
      DrawSpinner(ctx);
    if (EscapeKeyPressed()) {
      return ARCHIVE_CB_ABORT;
    }
  } else if (status == ARCHIVE_STATUS_ERROR) {
    if (msg && ctx)
      UI_Message(ctx, "%s", msg);
  } else if (status == ARCHIVE_STATUS_WARNING) {
    if (msg && ctx)
      UI_Warning(ctx, "%s", msg);
  }
  return ARCHIVE_CB_CONTINUE;
}

int GetCommandLine(ViewContext *ctx, char *command_line) {
  int result = -1;
  PromptHelpTopic help_topic = PROMPT_HELP_EXECUTE_FILE;
  UIPromptOptions options = {0};
  const char *prompt = "COMMAND ({} inserts selected path):";

  if (!ctx || !ctx->active || !command_line)
    return -1;

  ClearHelp(ctx);
  (void)snprintf(command_line, COMMAND_LINE_LENGTH + 1, "%s", " {} ");
  options.help_callback = ShowPromptHelpCallback;
  options.help_data = &help_topic;
  options.cursor_at_start = TRUE;

  if (AppStateResolveActivePanelFocus(ctx) == FOCUS_TREE) {
    help_topic = PROMPT_HELP_EXECUTE_DIRECTORY;
  }

  if (UI_ReadStringWithPromptOptions(ctx, ctx->active, prompt, command_line,
                                     COMMAND_LINE_LENGTH, HST_EXEC,
                                     &options) == CR) {
    result = 0;
  }

  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);

  return (result);
}

int GetTaggedCommandLine(ViewContext *ctx, char *command_line) {
  int result = -1;
  PromptHelpTopic help_topic = PROMPT_HELP_EXECUTE_FILE;
  UIPromptOptions options = {0};
  const char *prompt = "COMMAND ({} inserts selected path):";

  if (!ctx || !ctx->active || !command_line)
    return -1;

  ClearHelp(ctx);
  (void)snprintf(command_line, COMMAND_LINE_LENGTH + 1, "%s", " {} ");
  options.help_callback = ShowPromptHelpCallback;
  options.help_data = &help_topic;
  options.cursor_at_start = TRUE;

  if (AppStateResolveActivePanelFocus(ctx) == FOCUS_TREE) {
    help_topic = PROMPT_HELP_EXECUTE_DIRECTORY;
  }

  if (UI_ReadStringWithPromptOptions(ctx, ctx->active, prompt, command_line,
                                     COMMAND_LINE_LENGTH, HST_EXEC,
                                     &options) == CR) {
    result = 0;
  }

  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);

  return (result);
}

int GetSearchCommandLine(ViewContext *ctx, char *command_line,
                         char *search_pattern) {
  int result = -1;
  char input_buf[256];
  PromptHelpTopic help_topic = PROMPT_HELP_SEARCH_TAGGED;

  ClearHelp(ctx);

  input_buf[0] = '\0';

  if (UI_ReadStringWithHelp(ctx, ctx->active, "SEARCH TAGGED:",
                            input_buf, 256, HST_SEARCH, NULL, 0,
                            ShowPromptHelpCallback, &help_topic) == CR) {
    size_t command_len;

    if (search_pattern) {
      (void)snprintf(search_pattern, 256, "%s", input_buf);
      search_pattern[255] = '\0';
    }

    if (!Path_CommandInit(command_line, COMMAND_LINE_LENGTH + 1, &command_len,
                          "grep -i --") ||
        !Path_CommandAppendLiteral(command_line, COMMAND_LINE_LENGTH + 1,
                                   &command_len, " ") ||
        !Path_CommandAppendQuotedArg(command_line, COMMAND_LINE_LENGTH + 1,
                                     &command_len, input_buf) ||
        !Path_CommandAppendLiteral(command_line, COMMAND_LINE_LENGTH + 1,
                                   &command_len, " {}")) {
      UI_Warning(ctx, "Search command too long.");
    } else {
      result = 0;
    }
  }

  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);

  return (result);
}

int GetPipeCommand(ViewContext *ctx, char *pipe_command) {
  int result = -1;

  ClearHelp(ctx);

  if (UI_ReadString(ctx, ctx->active, "Pipe-Command:", pipe_command,
                    PATH_LENGTH, HST_PIPE) == CR) {
    result = 0;
  }

  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);

  return (result);
}

int SystemCall(ViewContext *ctx, const char *command_line, Statistic *s) {
  int result;

  endwin(); /* Ensure terminal state is reset before external command */
  result = SilentSystemCall(ctx, command_line, s);

  (void)GetAvailBytes(&s->disk_space, s);
  /* Full screen redraw to fully restore the curses UI */
  touchwin(stdscr);
  wnoutrefresh(stdscr);
  doupdate();
  return (result);
}

int QuerySystemCall(ViewContext *ctx, const char *command_line, Statistic *s) {
  int result;

  endwin(); /* 1. Save state / Exit curses mode */

  /* 2. Execute command (runs outside curses) */
  result = SilentSystemCallEx(ctx, command_line, FALSE, s);

  /* The external command has finished. We are still in the raw terminal. */

  HitReturnToContinue(); /* 3. Print message and wait for key in raw terminal */

  if (ctx->hook_init_clock)
    ctx->hook_init_clock(ctx);
  (void)GetAvailBytes(&s->disk_space, s);
  UI_Dialog_RefreshAll(ctx);
  ClockHandler(ctx, 0);
  doupdate();

  return (result);
}

int UI_ReadFilter(ViewContext *ctx) {
  int result = -1;
  char buffer[FILE_SPEC_LENGTH * 2 + 1];
  char prompt[64];
  BOOL tagged_only = FALSE;
  DirEntry *dir_entry;

  if (!ctx || !ctx->active || !ctx->active->vol)
    return -1;

  ClearHelp(ctx);
  dir_entry = GetSelectedDirEntry(ctx, ctx->active->vol);
  CopyBoundedString(buffer, sizeof(buffer),
                    ctx->active->vol->vol_stats.file_spec);
  if (dir_entry != NULL && dir_entry->tagged_files > 0)
    tagged_only = dir_entry->tagged_flag ? TRUE : FALSE;

  while (1) {
    const char *help_context = tagged_only ? "prompt.filter-tagged"
                                           : "prompt.filter";
    const UICommandStripCommand hints[] = {
        {UI_COMMAND_LAYOUT_KEY_PREFIX,
         tagged_only ? NP_("filter.hints", "all files")
                     : NP_("filter.hints", "tagged"),
         "Tab", NULL, "filter.hints"},
        {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("filter.hints", "history"), "Up",
         NULL, "filter.hints"},
        {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("filter.hints", "OK"), "Enter",
         NULL, "filter.hints"},
        {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("filter.hints", "cancel"), "Esc",
         NULL, "filter.hints"}};

    (void)snprintf(prompt, sizeof(prompt),
                   tagged_only ? "FILTER [tagged only]:" : "FILTER:");
    {
      int term = UI_ReadStringWithHelp(
          ctx, ctx->active, prompt, buffer, FILE_SPEC_LENGTH, HST_FILTER,
          hints, sizeof(hints) / sizeof(hints[0]),
          UI_ShowGeneratedContextHelpCallback, (void *)help_context);

      if (term == '\t') {
        if (dir_entry != NULL && dir_entry->tagged_files > 0) {
          tagged_only = tagged_only ? FALSE : TRUE;
        } else {
          tagged_only = FALSE;
          UI_Beep(ctx, FALSE);
        }
        continue;
      }

      if (term != CR)
        break;
    }

    if (SetFilter(buffer, &ctx->active->vol->vol_stats)) {
      UI_Message(ctx, "Invalid Filter Spec");
      break;
    }
    CopyBoundedString(ctx->active->vol->vol_stats.file_spec,
                      sizeof(ctx->active->vol->vol_stats.file_spec), buffer);
    if (dir_entry != NULL &&
        !AppStateCommitDirEntryTaggedFilter(dir_entry, tagged_only))
      break;
    result = 0;
    break;
  }
  wmove(ctx->ctx_border_window, ctx->layout.message_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);
  return (result);
}

void UI_HandleSort(ViewContext *ctx, DirEntry *dir_entry, Statistic *s,
                   int start_x) {
  int c;
  int sort_kind = 0;
  int order = SORT_ASC;
  WINDOW *sort_win =
      (ctx && ctx->ctx_menu_window) ? ctx->ctx_menu_window : stdscr;

  if (!ctx || !ctx->active || !s)
    return;

  DrawSortPrompt(ctx, sort_win, TRUE);

  do {
    c = Getch(ctx);
    if (c == -1 || c == ESC)
      return;
    c = toupper(c);
    if (c == 'Q')
      return;

    switch (c) {
    case 'N':
      sort_kind = SORT_BY_NAME;
      break;
    case 'E':
      sort_kind = SORT_BY_EXTENSION;
      break;
    case 'M':
      sort_kind = SORT_BY_MOD_TIME;
      break;
    case 'A':
      sort_kind = SORT_BY_ACC_TIME;
      break;
    case 'C':
      sort_kind = SORT_BY_CHG_TIME;
      break;
    case 'G':
      sort_kind = SORT_BY_GROUP;
      break;
    case 'W':
      sort_kind = SORT_BY_OWNER;
      break;
    case 'S':
      sort_kind = SORT_BY_SIZE;
      break;
    case 'O':
      if (order == SORT_ASC) {
        order = SORT_DSC;
      } else {
        order = SORT_ASC;
      }
      DrawSortPrompt(ctx, sort_win, (order == SORT_ASC));
      break;
    }
  } while (!strchr("ACEGMNWS", c));

  SetKindOfSort(sort_kind + order, s);

  (void)AppStateCommitDirEntryFileViewport(dir_entry, 0, 0);

  if (ctx->active) {
    Panel_Sort(ctx->active, s->kind_of_sort);
    if (dir_entry) {
      DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                   dir_entry->start_file + dir_entry->cursor_pos, start_x,
                   ctx->ctx_file_window);
    }
  }
}
