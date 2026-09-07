/***************************************************************************
 *
 * src/ui/ctrl_file_ops.c
 * File Action Handlers (Tagging, Move, Copy, Delete dispatch)
 *
 ***************************************************************************/

#define NO_YTNOVA_MACROS
#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_render.h"
#include "ytnova_appstate_focus.h"
#include "ytnova_appstate_layout.h"
#include "ytnova_appstate_modal.h"
#include "ytnova_appstate_panel.h"
#include "ytnova_appstate_session.h"
#include "ytnova_appstate_visibility.h"
#include "ytnova_appstate_volume.h"
#include "ytnova_appstate_window.h"
#include "ytnova_cmd.h"
#include "ytnova_fs.h"
#include "ytnova_runtime_launch.h"
#include "ytnova_panel_anchor.h"
#include "ytnova_ui.h"
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <utime.h>

static void ResetPreviewAfterNavigation(
    ViewContext *ctx, DirEntry *dir_entry, long *preview_line_offset_ptr,
    void (*update_preview)(ViewContext *, const DirEntry *)) {
  if (!ctx->preview_mode)
    return;
  *preview_line_offset_ptr = 0;
  if (update_preview)
    update_preview(ctx, dir_entry);
}

static void DebugLogFilePanelState(const char *label, const YtreeNovaPanel *panel);

/* =========================================================================
 * Shared UI Helpers for Post-Action Refresh, Sync, and Render
 * ========================================================================= */

void UI_RefreshSyncPanels(ViewContext *ctx, DirEntry *dir_entry) {
  RefreshDirWindow(ctx, ctx->active);
  if (ctx->is_split_screen) {
    RefreshDirWindow(ctx, (ctx->active == ctx->left) ? ctx->right : ctx->left);
  }
  RefreshView(ctx, dir_entry);
}

void UI_RenderFilePanel(ViewContext *ctx, const DirEntry *dir_entry,
                        int start_x) {
  DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
               dir_entry->start_file + dir_entry->cursor_pos, start_x,
               ctx->ctx_file_window);
}

void CapturePanelSelectionAnchor(ViewContext *ctx, YtreeNovaPanel *panel,
                                 const DirEntry *dir_entry) {
  int idx;
  const FileEntry *selected_file;
  char file_selection_dir_path[PATH_LENGTH + 1];
  char file_selection_name[PATH_LENGTH + 1];

  if (!panel)
    return;

  if (!ctx || !dir_entry)
    return;

  if (!panel->file_entry_list || panel->file_count == 0)
    BuildFileEntryList(ctx, panel);

  file_selection_dir_path[0] = '\0';
  GetPath((DirEntry *)dir_entry, file_selection_dir_path);
  file_selection_dir_path[PATH_LENGTH] = '\0';
  file_selection_name[0] = '\0';

  if (!panel->file_entry_list || panel->file_count == 0) {
    if (!AppStateCommitPanelFileSelection(panel, file_selection_dir_path, NULL))
      return;
    return;
  }

  idx = panel->start_file + panel->file_cursor_pos;
  if (idx < 0)
    idx = 0;
  if ((unsigned int)idx >= panel->file_count)
    idx = (int)panel->file_count - 1;
  if (idx < 0)
    idx = 0;

  selected_file = panel->file_entry_list[idx].file;
  if (selected_file) {
    (void)snprintf(file_selection_name, sizeof(file_selection_name), "%s",
                   selected_file->name);
    file_selection_name[PATH_LENGTH] = '\0';
  }
  if (!AppStateCommitPanelFileSelection(panel, file_selection_dir_path,
                                        file_selection_name))
    return;
}

static void DebugLogFilePanelState(const char *label, const YtreeNovaPanel *panel);

BOOL RebindActiveFilePanelSelection(YtreeNovaPanel *panel, DirEntry **dir_entry_io) {
  DirEntry *panel_dir;

  if (!panel || !dir_entry_io)
    return FALSE;

  assert(panel->saved_focus != FOCUS_FILE ||
         panel->file_selection_dir_path[0] != '\0');
  if (!panel->vol || panel->file_selection_dir_path[0] == '\0')
    return FALSE;

  panel_dir = ResolvePanelAnchorTarget(panel, panel->vol,
                                       panel->file_selection_dir_path);
  if (!panel_dir)
    return FALSE;

  /*
   * File-panel ownership is panel-local. When the active file pane changes,
   * re-seed the shared DirEntry from the newly active panel's saved cursor and
   * window shape so the next keypress continues that panel's own selection
   * instead of inheriting the last pane's row or zoom state.
   */
  if (!AppStateCommitDirEntryFileViewport(panel_dir, panel->start_file,
                                          panel->file_cursor_pos))
    return FALSE;
  if (!panel_dir->global_flag && !panel_dir->tagged_flag) {
    if (!AppStateCommitDirEntryFileShape(panel_dir, panel->saved_big_file_view))
      return FALSE;
  }
  if (!AppStateCommitPanelFileAnchor(panel, panel_dir))
    return FALSE;
  *dir_entry_io = panel_dir;
  DebugLogFilePanelState("RebindActiveFilePanelSelection", panel);
  return TRUE;
}

static void DebugLogFilePanelState(const char *label, const YtreeNovaPanel *panel) {
  (void)label;
  (void)panel;
}

static FileEntry *GetActivePanelSelectedFile(ViewContext *ctx,
                                             const DirEntry *dir_entry) {
  int idx;

  if (!ctx || !ctx->active || !dir_entry || !ctx->active->file_entry_list ||
      ctx->active->file_count == 0) {
    return NULL;
  }

  if (ctx->active->file_selection_name[0] != '\0' &&
      ctx->active->file_selection_dir_path[0] != '\0') {
    char current_dir_path[PATH_LENGTH + 1];

    GetPath((DirEntry *)dir_entry, current_dir_path);
    current_dir_path[PATH_LENGTH] = '\0';
    if (strcmp(current_dir_path, ctx->active->file_selection_dir_path) == 0) {
      for (unsigned int i = 0; i < ctx->active->file_count; i++) {
        FileEntry *candidate = ctx->active->file_entry_list[i].file;
        if (candidate &&
            strcmp(candidate->name, ctx->active->file_selection_name) == 0) {
          return candidate;
        }
      }
    }
  }

  idx = ctx->active->start_file + ctx->active->file_cursor_pos;
  if (idx < 0)
    idx = 0;
  if ((unsigned int)idx >= ctx->active->file_count)
    idx = (int)ctx->active->file_count - 1;
  if (idx < 0)
    return NULL;

  return ctx->active->file_entry_list[idx].file;
}

static void RebuildActiveFileListAfterMutation(ViewContext *ctx,
                                               DirEntry *dir_entry) {
  int cursor_pos;
  int file_count;
  int start_file;

  if (!ctx || !ctx->active || !dir_entry)
    return;

  BuildFileEntryList(ctx, ctx->active);

  cursor_pos = dir_entry->cursor_pos;
  start_file = dir_entry->start_file;
  file_count = (int)ctx->active->file_count;
  if (file_count <= 0) {
    start_file = 0;
    cursor_pos = 0;
  } else {
    int max_disp_files;

    max_disp_files = FileNav_GetMaxDispFiles(ctx);
    if (max_disp_files < 1)
      max_disp_files = 1;

    if (start_file < 0)
      start_file = 0;
    if (start_file >= file_count)
      start_file = file_count - 1;
    if (cursor_pos < 0)
      cursor_pos = 0;
    if (cursor_pos >= max_disp_files)
      cursor_pos = max_disp_files - 1;

    if (start_file + cursor_pos >= file_count) {
      start_file = MAXIMUM(0, file_count - max_disp_files);
      cursor_pos = file_count - 1 - start_file;
    }
  }

  if (!AppStateCommitDirEntryFileViewport(dir_entry, start_file, cursor_pos))
    return;
  (void)AppStateCommitPanelFileViewport(ctx->active, dir_entry->start_file,
                                        dir_entry->cursor_pos);
}

/* Placeholder values are shell-quoted by Path_BuildCommandLine().
 * If users also wrap {} / %s in quotes, the resulting command line can
 * create broken quoting (e.g. ''value'') and re-expose shell metacharacters.
 * Normalize quoted placeholders back to bare tokens before expansion. */
static void NormalizeQuotedExecPlaceholders(char *command_template,
                                            size_t command_template_size) {
  size_t read_idx = 0;
  size_t write_idx = 0;

  if (!command_template || command_template_size == 0) {
    return;
  }

  while (command_template[read_idx] != '\0' &&
         write_idx + 1 < command_template_size) {
    char quote = command_template[read_idx];

    if ((quote == '\'' || quote == '"') && read_idx + 3 < command_template_size &&
        command_template[read_idx + 3] == quote) {
      if (command_template[read_idx + 1] == '{' &&
          command_template[read_idx + 2] == '}') {
        command_template[write_idx++] = '{';
        command_template[write_idx++] = '}';
        read_idx += 4;
        continue;
      }

      if (command_template[read_idx + 1] == '%' &&
          command_template[read_idx + 2] == 's') {
        command_template[write_idx++] = '%';
        command_template[write_idx++] = 's';
        read_idx += 4;
        continue;
      }
    }

    command_template[write_idx++] = command_template[read_idx++];
  }

  command_template[write_idx] = '\0';
}

BOOL handle_file_window_preview_action(
    ViewContext *ctx, YtreeNovaAction action, DirEntry **dir_entry_ptr,
    YtreeNovaAction *loop_action_ptr, Statistic **stats_ptr,
    struct Volume **start_vol_ptr, BOOL *need_dsp_help_ptr,
    long *preview_line_offset_ptr, int *saved_fixed_width_ptr,
    void (*update_preview)(ViewContext *, const DirEntry *)) {
  DirEntry *dir_entry;

  if (!ctx || !dir_entry_ptr)
    return FALSE;

  dir_entry = *dir_entry_ptr;
  switch (action) {
  case ACTION_VIEW_PREVIEW:
    if (!ctx->preview_mode) {
      if (!AppStateCommitPreviewReturn(ctx, ctx->active,
                                       AppStateResolveActivePanelFocus(ctx)))
        return FALSE;
    }

    if (!AppStateCommitPreviewMode(ctx, !ctx->preview_mode))
      return FALSE;
    if (ctx->preview_mode) {
      int fixed_col_width;

      *saved_fixed_width_ptr = ctx->fixed_col_width;
      ReCreateWindows(ctx);
      fixed_col_width = ctx->layout.big_file_win_width - 2;
      if (fixed_col_width < 1)
        fixed_col_width = 1;
      if (!AppStateCommitFixedColumnWidth(ctx, fixed_col_width))
        return FALSE;
      FileNav_RereadWindowSize(ctx, dir_entry);
    } else {
      Statistic *stats_local;

      if (!AppStateCommitActivePanel(ctx, ctx->preview_return_panel))
        return FALSE;
      if (!AppStateCommitPanelFocus(ctx, ctx->active, ctx->preview_return_focus))
        return FALSE;
      stats_local = &ctx->active->vol->vol_stats;
      if (stats_ptr)
        *stats_ptr = stats_local;
      if (start_vol_ptr)
        *start_vol_ptr = ctx->active->vol;

      dir_entry = ResolveActiveDirEntry(ctx, stats_local);

      if (!AppStateSyncActiveWindowHandles(ctx))
        return FALSE;

      if (!AppStateCommitFixedColumnWidth(ctx, *saved_fixed_width_ptr))
        return FALSE;
      ReCreateWindows(ctx);
      FileNav_RereadWindowSize(ctx, dir_entry);
      RefreshView(ctx, dir_entry);
    }

    RefreshView(ctx, dir_entry);
    if (ctx->preview_mode && update_preview)
      update_preview(ctx, dir_entry);

    if (!ctx->preview_mode && loop_action_ptr) {
      if (ctx->preview_return_focus == FOCUS_TREE)
        *loop_action_ptr = ACTION_ESCAPE;
      else
        *loop_action_ptr = ACTION_NONE;
    }

    *need_dsp_help_ptr = TRUE;
    *dir_entry_ptr = dir_entry;
    return TRUE;

  case ACTION_PREVIEW_SCROLL_DOWN:
    if (ctx->preview_mode) {
      (*preview_line_offset_ptr)++;
      if (update_preview)
        update_preview(ctx, dir_entry);
    }
    return TRUE;

  case ACTION_PREVIEW_SCROLL_UP:
    if (ctx->preview_mode) {
      if (*preview_line_offset_ptr > 0)
        (*preview_line_offset_ptr)--;
      if (update_preview)
        update_preview(ctx, dir_entry);
    }
    return TRUE;

  case ACTION_PREVIEW_HOME:
    if (ctx->preview_mode) {
      *preview_line_offset_ptr = 0;
      if (update_preview)
        update_preview(ctx, dir_entry);
    }
    return TRUE;

  case ACTION_PREVIEW_END:
    if (ctx->preview_mode) {
      *preview_line_offset_ptr = 2000000000L;
      if (update_preview)
        update_preview(ctx, dir_entry);
    }
    return TRUE;

  case ACTION_PREVIEW_PAGE_UP:
    if (ctx->preview_mode) {
      *preview_line_offset_ptr -= (getmaxy(ctx->ctx_preview_window) - 1);
      if (*preview_line_offset_ptr < 0)
        *preview_line_offset_ptr = 0;
      if (update_preview)
        update_preview(ctx, dir_entry);
    }
    return TRUE;

  case ACTION_PREVIEW_PAGE_DOWN:
    if (ctx->preview_mode) {
      *preview_line_offset_ptr += (getmaxy(ctx->ctx_preview_window) - 1);
      if (update_preview)
        update_preview(ctx, dir_entry);
    }
    return TRUE;

  default:
    return FALSE;
  }
}

BOOL handle_file_window_navigation_action(
    ViewContext *ctx, YtreeNovaAction action, DirEntry *dir_entry, int *start_x_ptr,
    BOOL *need_dsp_help_ptr, long *preview_line_offset_ptr,
    void (*update_preview)(ViewContext *, const DirEntry *),
    void (*list_jump)(ViewContext *, DirEntry *, char *)) {
  int cursor_pos;
  int start_file;

  if (!ctx || !dir_entry || !start_x_ptr)
    return FALSE;

  switch (action) {
  case ACTION_MOVE_DOWN:
    FileNav_MoveDown(ctx, dir_entry, *start_x_ptr);
    ResetPreviewAfterNavigation(ctx, dir_entry, preview_line_offset_ptr,
                                update_preview);
    return TRUE;

  case ACTION_MOVE_UP:
    FileNav_MoveUp(ctx, dir_entry, *start_x_ptr);
    ResetPreviewAfterNavigation(ctx, dir_entry, preview_line_offset_ptr,
                                update_preview);
    return TRUE;

  case ACTION_MOVE_RIGHT:
    if (FileNav_GetXStep(ctx) == 1) {
      *start_x_ptr = 0;
      FileNav_PageDown(ctx, dir_entry, *start_x_ptr);
    } else {
      FileNav_MoveRight(ctx, dir_entry, start_x_ptr);
    }
    ResetPreviewAfterNavigation(ctx, dir_entry, preview_line_offset_ptr,
                                update_preview);
    return TRUE;

  case ACTION_MOVE_LEFT:
    if (FileNav_GetXStep(ctx) == 1) {
      *start_x_ptr = 0;
      FileNav_PageUp(ctx, dir_entry, *start_x_ptr);
    } else {
      FileNav_MoveLeft(ctx, dir_entry, start_x_ptr);
    }
    ResetPreviewAfterNavigation(ctx, dir_entry, preview_line_offset_ptr,
                                update_preview);
    return TRUE;

  case ACTION_PAGE_DOWN:
    FileNav_PageDown(ctx, dir_entry, *start_x_ptr);
    ResetPreviewAfterNavigation(ctx, dir_entry, preview_line_offset_ptr,
                                update_preview);
    return TRUE;

  case ACTION_PAGE_UP:
    FileNav_PageUp(ctx, dir_entry, *start_x_ptr);
    ResetPreviewAfterNavigation(ctx, dir_entry, preview_line_offset_ptr,
                                update_preview);
    return TRUE;

  case ACTION_END:
    cursor_pos = dir_entry->cursor_pos;
    start_file = dir_entry->start_file;
    Nav_End(&cursor_pos, &start_file,
            (int)ctx->active->file_count, FileNav_GetMaxDispFiles(ctx));
    if (!AppStateCommitDirEntryFileViewport(dir_entry, start_file, cursor_pos))
      return FALSE;
    UI_RenderFilePanel(ctx, dir_entry, *start_x_ptr);
    FileNav_UpdateHeaderPath(ctx, dir_entry);
    ResetPreviewAfterNavigation(ctx, dir_entry, preview_line_offset_ptr,
                                update_preview);
    (void)AppStateCommitPanelFileViewport(ctx->active, dir_entry->start_file,
                                          dir_entry->cursor_pos);
    return TRUE;

  case ACTION_HOME:
    cursor_pos = dir_entry->cursor_pos;
    start_file = dir_entry->start_file;
    Nav_Home(&cursor_pos, &start_file);
    if (!AppStateCommitDirEntryFileViewport(dir_entry, start_file, cursor_pos))
      return FALSE;
    UI_RenderFilePanel(ctx, dir_entry, *start_x_ptr);
    FileNav_UpdateHeaderPath(ctx, dir_entry);
    ResetPreviewAfterNavigation(ctx, dir_entry, preview_line_offset_ptr,
                                update_preview);
    (void)AppStateCommitPanelFileViewport(ctx->active, dir_entry->start_file,
                                          dir_entry->cursor_pos);
    return TRUE;

  case ACTION_LIST_JUMP:
    if (list_jump)
      list_jump(ctx, dir_entry, "");
    *need_dsp_help_ptr = TRUE;
    return TRUE;

  default:
    return FALSE;
  }
}

BOOL handle_file_window_volume_action(ViewContext *ctx, YtreeNovaAction action,
                                      const struct Volume *start_vol,
                                      int *unput_char_ptr,
                                      BOOL *return_esc_ptr) {
  if (!ctx || !unput_char_ptr || !return_esc_ptr)
    return FALSE;

  *return_esc_ptr = FALSE;
  switch (action) {
  case ACTION_VOL_MENU:
    if (SelectLoadedVolume(ctx, NULL) == 0) {
      *unput_char_ptr = '\0';
      *return_esc_ptr = TRUE;
      return TRUE;
    }
    if (ctx->active->vol != start_vol) {
      *return_esc_ptr = TRUE;
    }
    return TRUE;

  case ACTION_VOL_PREV:
    if (CycleLoadedVolume(ctx, ctx->active, -1) == 0) {
      *unput_char_ptr = '\0';
      *return_esc_ptr = TRUE;
      return TRUE;
    }
    if (ctx->active->vol != start_vol) {
      *return_esc_ptr = TRUE;
    }
    return TRUE;

  case ACTION_VOL_NEXT:
    if (CycleLoadedVolume(ctx, ctx->active, 1) == 0) {
      *unput_char_ptr = '\0';
      *return_esc_ptr = TRUE;
      return TRUE;
    }
    if (ctx->active->vol != start_vol) {
      *return_esc_ptr = TRUE;
    }
    return TRUE;

  default:
    return FALSE;
  }
}

static int PromptCopyMoveDestination(ViewContext *ctx,
                                     DirEntry *current_dir_entry, char *to_dir,
                                     BOOL use_realpath, DirEntry *tree,
                                     char *to_path,
                                     DirEntry **dest_dir_entry) {
  while (1) {
    char candidate_path[PATH_LENGTH + 1];
    BOOL target_is_regular_file = FALSE;
    const struct Volume *target_vol = NULL;
    struct stat target_stat;

    candidate_path[0] = '\0';
    if (current_dir_entry != NULL &&
        ResolveDestinationDirectoryPath(current_dir_entry, to_dir,
                                        candidate_path) == 0) {
      target_vol = Volume_GetByPath(ctx, candidate_path);
    } else if (to_dir[0] == FILE_SEPARATOR_CHAR) {
      int copied_len =
          snprintf(candidate_path, sizeof(candidate_path), "%s", to_dir);
      if (copied_len < 0 || (size_t)copied_len >= sizeof(candidate_path)) {
        MESSAGE(ctx, "Invalid destination path*\"%s\"*path too long", to_dir);
        return -1;
      }
      target_vol = Volume_GetByPath(ctx, candidate_path);
    }
    if (target_vol != NULL && target_vol->vol_stats.log_mode == ARCHIVE_MODE) {
      int copied_len = snprintf(to_path, PATH_LENGTH + 1, "%s", candidate_path);
      if (copied_len < 0 || copied_len >= PATH_LENGTH + 1) {
        MESSAGE(ctx, "Invalid destination path*\"%s\"*path too long", to_dir);
        return -1;
      }
      *dest_dir_entry = NULL;
      return 0;
    }

    if (use_realpath) {
      if (realpath(to_dir, to_path) == NULL) {
        if (errno == ENOENT) {
          int copied_len = snprintf(to_path, PATH_LENGTH + 1, "%s", to_dir);
          if (copied_len < 0 || copied_len >= PATH_LENGTH + 1) {
            MESSAGE(ctx, "Invalid destination path*\"%s\"*path too long",
                    to_dir);
            return -1;
          }
        } else {
          MESSAGE(ctx, "Invalid destination path*\"%s\"*%s", to_dir,
                  strerror(errno));
          return -1;
        }
      } else if (STAT_(to_path, &target_stat) == 0 &&
                 S_ISREG(target_stat.st_mode)) {
        target_is_regular_file = TRUE;
      }
      *dest_dir_entry = NULL;
    } else {
      if (ResolveDestinationDirectoryPath(current_dir_entry, to_dir, to_path) !=
          0) {
        MESSAGE(ctx, "Invalid destination path*\"%s\"", to_dir);
        return -1;
      }
      *dest_dir_entry = NULL;
      if (STAT_(to_path, &target_stat) == 0 &&
          S_ISREG(target_stat.st_mode)) {
        target_is_regular_file = TRUE;
      }
    }

    if (!target_is_regular_file) {
      int dir_state = 0;
      int ensure_result = UI_EnsureCopyMoveDestinationDirectory(
          ctx, to_path, tree, dest_dir_entry, &dir_state);
      if (ensure_result > 0) {
        if (GetDestinationDirectoryParameter(ctx, to_dir) != 0)
          return -1;
        continue;
      }
      if (ensure_result < 0)
        return -1;
    }

    return 0;
  }
}

static void HandleSingleFileCopyAction(ViewContext *ctx, YtreeNovaAction action,
                                       DirEntry *dir_entry, Statistic *s) {
  FileEntry *fe_ptr = GetActivePanelSelectedFile(ctx, dir_entry);
  DirEntry *de_ptr = NULL;
  DirEntry *dest_dir_entry = NULL;
  BOOL path_copy = (action == ACTION_CMD_Y) ? TRUE : FALSE;
  BOOL use_realpath =
      (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE);
  DirEntry *tree = use_realpath ? NULL : ((s) ? s->tree : NULL);
  char to_dir[PATH_LENGTH + 1] = {0};
  char to_path[PATH_LENGTH + 1] = {0};
  char to_file[PATH_LENGTH + 1] = {0};
  char expanded_to_file[PATH_LENGTH + 1];

  if (!fe_ptr || !s)
    return;

  de_ptr = fe_ptr->dir_entry;
  if (GetCopyParameter(ctx, fe_ptr->name, path_copy, to_file, to_dir) != 0)
    return;
  if (PromptCopyMoveDestination(ctx, de_ptr, to_dir, use_realpath, tree,
                                to_path, &dest_dir_entry) != 0) {
    return;
  }

  BuildFilename(fe_ptr->name, to_file, expanded_to_file);

  {
    int dir_create_mode = 0;
    int overwrite_mode = 0;
    CopyFile(ctx, s, fe_ptr, expanded_to_file, dest_dir_entry, to_path,
             path_copy, &dir_create_mode, &overwrite_mode,
             (ConflictCallback)UI_ConflictResolverWrapper,
             (ChoiceCallback)UI_ChoiceResolver);
  }

  UI_RefreshSyncPanels(ctx, dir_entry);
}

static void HandleSingleFileMoveAction(ViewContext *ctx, DirEntry *dir_entry,
                                       Statistic *s,
                                       BOOL *maybe_change_x_step_ptr) {
  FileEntry *fe_ptr = GetActivePanelSelectedFile(ctx, dir_entry);
  FileEntry *new_fe_ptr = NULL;
  DirEntry *de_ptr = NULL;
  DirEntry *dest_dir_entry = NULL;
  BOOL use_realpath = (ctx->view_mode == ARCHIVE_MODE);
  char to_dir[PATH_LENGTH + 1] = {0};
  char to_path[PATH_LENGTH + 1] = {0};
  char to_file[PATH_LENGTH + 1] = {0};
  char expanded_to_file[PATH_LENGTH + 1];

  if (!fe_ptr || !s || !maybe_change_x_step_ptr)
    return;

  de_ptr = fe_ptr->dir_entry;
  if (GetMoveParameter(ctx, fe_ptr->name, to_file, to_dir) != 0)
    return;
  if (PromptCopyMoveDestination(ctx, de_ptr, to_dir, use_realpath, s->tree,
                                to_path, &dest_dir_entry) != 0) {
    return;
  }

  BuildFilename(fe_ptr->name, to_file, expanded_to_file);

  {
    int dir_create_mode = 0;
    int overwrite_mode = 0;
    if (!MoveFile(ctx, fe_ptr, expanded_to_file, dest_dir_entry, to_path,
                  &new_fe_ptr, &dir_create_mode, &overwrite_mode,
                  (ConflictCallback)UI_ConflictResolverWrapper,
                  (ChoiceCallback)UI_ChoiceResolver)) {
      RebuildActiveFileListAfterMutation(ctx, dir_entry);
      UI_RefreshSyncPanels(ctx, dir_entry);
      *maybe_change_x_step_ptr = TRUE;
    }
  }
}

static BOOL HandleFileMutationDispatchAction(
    ViewContext *ctx, YtreeNovaAction action, DirEntry **dir_entry_ptr,
    BOOL *need_dsp_help_ptr, BOOL *maybe_change_x_step_ptr, Statistic *s) {
  DirEntry *dir_entry = NULL;
  FileEntry *fe_ptr = NULL;

  if (!dir_entry_ptr || !need_dsp_help_ptr || !maybe_change_x_step_ptr)
    return FALSE;

  dir_entry = *dir_entry_ptr;
  if (ctx->view_mode == ARCHIVE_MODE) {
    unsigned int required = 0;
    const char *operation = NULL;

    switch (action) {
    case ACTION_CMD_M:
      required = ARCHIVE_CAP_MOVE;
      operation = "moving entries";
      break;
    case ACTION_CMD_D:
      required = ARCHIVE_CAP_DELETE;
      operation = "deleting entries";
      break;
    case ACTION_CMD_R:
      required = ARCHIVE_CAP_RENAME;
      operation = "renaming entries";
      break;
    default:
      break;
    }
    if (required != 0 && !(s->archive_capabilities & required)) {
      UI_ShowStatusLineError(ctx, "This archive does not support %s", operation);
      *need_dsp_help_ptr = TRUE;
      return TRUE;
    }
  }
  switch (action) {
  case ACTION_CMD_Y:
  case ACTION_CMD_C:
    *need_dsp_help_ptr = TRUE;
    HandleSingleFileCopyAction(ctx, action, dir_entry, s);
    return TRUE;

  case ACTION_CMD_M:
    if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE &&
        ctx->view_mode != ARCHIVE_MODE) {
      return TRUE;
    }
    *need_dsp_help_ptr = TRUE;
    HandleSingleFileMoveAction(ctx, dir_entry, s, maybe_change_x_step_ptr);
    return TRUE;

  case ACTION_CMD_D:
    if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE &&
        ctx->view_mode != ARCHIVE_MODE) {
      return TRUE;
    }

    if (InputChoice(ctx, "Delete this file (Y/N) ? ", "YN\033") != 'Y') {
      *need_dsp_help_ptr = TRUE;
      return TRUE;
    }

    *need_dsp_help_ptr = TRUE;
    fe_ptr = ctx->active
                 ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
                 .file;
    {
      int override_mode = 0;
      if (!DeleteFile(ctx, fe_ptr, &override_mode, s,
                      (ChoiceCallback)UI_ChoiceResolver)) {
        RebuildActiveFileListAfterMutation(ctx, dir_entry);
        UI_RefreshSyncPanels(ctx, dir_entry);
        *maybe_change_x_step_ptr = TRUE;
      }
    }
    return TRUE;

  case ACTION_CMD_R:
    if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE &&
        ctx->view_mode != ARCHIVE_MODE) {
      return TRUE;
    }

    fe_ptr = ctx->active
                 ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
                 .file;
    {
      FileEntry *new_fe_ptr = NULL;
      char new_name[PATH_LENGTH + 1];

      if (!GetRenameParameter(ctx, fe_ptr->name, new_name)) {
        char expanded_new_name[PATH_LENGTH + 1];

        BuildFilename(fe_ptr->name, new_name, expanded_new_name);
        if (!RenameFile(ctx, fe_ptr, expanded_new_name, &new_fe_ptr))
          *maybe_change_x_step_ptr = TRUE;
        RefreshView(ctx, dir_entry);
      }
    }
    *need_dsp_help_ptr = TRUE;
    return TRUE;

  default:
    return FALSE;
  }
}

static BOOL HandleFileCommandDispatchAction(ViewContext *ctx,
                                            YtreeNovaAction action,
                                            DirEntry **dir_entry_ptr,
                                            BOOL *need_dsp_help_ptr) {
  DirEntry *dir_entry = NULL;
  DirEntry *de_ptr = NULL;
  FileEntry *fe_ptr = NULL;

  if (!dir_entry_ptr || !need_dsp_help_ptr)
    return FALSE;

  dir_entry = *dir_entry_ptr;
  switch (action) {
  case ACTION_CMD_P:
    fe_ptr = ctx->active
                 ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
                 .file;
    de_ptr = fe_ptr->dir_entry;
    {
      char pipe_cmd[PATH_LENGTH + 1];
      pipe_cmd[0] = '\0';
      if (GetPipeCommand(ctx, pipe_cmd) == 0)
        (void)Pipe(ctx, de_ptr, fe_ptr, pipe_cmd);
    }
    RefreshView(ctx, dir_entry);
    *need_dsp_help_ptr = TRUE;
    return TRUE;

  case ACTION_CMD_PRINT:
    UI_HandlePrintController(ctx, dir_entry, FALSE);
    dir_entry = RefreshFileView(ctx, dir_entry);
    RefreshView(ctx, dir_entry);
    *dir_entry_ptr = dir_entry;
    *need_dsp_help_ptr = TRUE;
    return TRUE;

  case ACTION_CMD_X:
    fe_ptr = ctx->active
                 ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
                 .file;
    if (!fe_ptr)
      return TRUE;
    de_ptr = fe_ptr->dir_entry;
    {
      char command_template[COMMAND_LINE_LENGTH + 1];
      command_template[0] = '\0';
      if (GetCommandLine(ctx, command_template) == 0) {
        NormalizeQuotedExecPlaceholders(command_template,
                                        sizeof(command_template));
        (void)Execute(ctx, de_ptr, fe_ptr, command_template,
                      &ctx->active->vol->vol_stats, UI_ArchiveCallback);
        if (ctx->view_mode == ARCHIVE_MODE)
          HitReturnToContinue();
      }
    }
    dir_entry = RefreshFileView(ctx, dir_entry);
    RefreshView(ctx, dir_entry);
    *dir_entry_ptr = dir_entry;
    *need_dsp_help_ptr = TRUE;
    return TRUE;

  default:
    return FALSE;
  }
}

BOOL handle_file_window_command_action(ViewContext *ctx, YtreeNovaAction action,
                                       DirEntry **dir_entry_ptr,
                                       BOOL *need_dsp_help_ptr,
                                       BOOL *maybe_change_x_step_ptr,
                                       Statistic *s) {
  if (!AppStateValidatedDispatchSurface("surface.command-completion-dispatch"))
    return FALSE;
  if (!AppStateValidatedEvent("event.command-completion"))
    return FALSE;

  if (HandleFileMutationDispatchAction(ctx, action, dir_entry_ptr,
                                       need_dsp_help_ptr,
                                       maybe_change_x_step_ptr, s)) {
    return TRUE;
  }

  if (HandleFileCommandDispatchAction(ctx, action, dir_entry_ptr,
                                      need_dsp_help_ptr)) {
    return TRUE;
  }

  return FALSE;
}

BOOL handle_file_window_misc_dispatch_action(
    ViewContext *ctx, YtreeNovaAction action, DirEntry **dir_entry_ptr,
    YtreeNovaAction *loop_action_ptr, int *unput_char_ptr,
    const int *start_x_ptr,
    BOOL *need_dsp_help_ptr, BOOL *maybe_change_x_step_ptr, Statistic *s,
    long *preview_line_offset_ptr,
    void (*update_preview)(ViewContext *, const DirEntry *)) {
  DirEntry *dir_entry = NULL;
  FileEntry *fe_ptr = NULL;
  DirEntry *de_ptr = NULL;
  BOOL handled = TRUE;
  char filepath[PATH_LENGTH + 1];
  char new_log_path[PATH_LENGTH + 1];

#define loop_action (*loop_action_ptr)
#define unput_char (*unput_char_ptr)
#define start_x (*start_x_ptr)
#define need_dsp_help (*need_dsp_help_ptr)
#define maybe_change_x_step (*maybe_change_x_step_ptr)

  if (!ctx || !dir_entry_ptr || !*dir_entry_ptr || !loop_action_ptr ||
      !unput_char_ptr || !start_x_ptr || !need_dsp_help_ptr ||
      !maybe_change_x_step_ptr || !s || !preview_line_offset_ptr) {
    handled = FALSE;
    goto misc_dispatch_done;
  }

  dir_entry = *dir_entry_ptr;

  switch (action) {
  case ACTION_FILEINFO_1:
  case ACTION_FILEINFO_2:
  case ACTION_FILEINFO_3:
  case ACTION_FILEINFO_4:
  case ACTION_FILEINFO_5:
  case ACTION_FILEINFO_6:
  case ACTION_FILEINFO_7:
  case ACTION_FILEINFO_8:
  case ACTION_FILEINFO_9:
  case ACTION_FILEINFO_0:
    if (!FileInfoHandleFileAction(ctx, action, dir_entry, s, start_x,
                                  preview_line_offset_ptr, update_preview))
      return FALSE;
    need_dsp_help = TRUE;
    break;

  case ACTION_TOGGLE_HIDDEN:
    ToggleDotFiles(ctx, ctx->active);
    dir_entry = GetPanelDirEntry(ctx->active);
    DisplayFileWindow(ctx, ctx->active, dir_entry);
    RefreshWindow(ctx->ctx_file_window);
    if (ctx->preview_mode) {
      *preview_line_offset_ptr = 0;
      if (update_preview)
        update_preview(ctx, dir_entry);
    }
    need_dsp_help = TRUE;
    break;

  case ACTION_CMD_I: {
    ArchivePayload payload;
    int gather_result;
    payload.original_source_list = NULL;
    payload.expanded_file_list = NULL;
    fe_ptr =
        ctx->active
            ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
            .file;

    gather_result = UI_GatherArchivePayload(ctx, dir_entry, fe_ptr, &payload);
    if (gather_result != 0) {
      if (gather_result < 0)
        UI_ShowStatusLineError(ctx, "Nothing to archive");
      need_dsp_help = FALSE;
    } else {
      int create_result;
      create_result = UI_CreateArchiveFromPayload(ctx, &payload);
      if (create_result == 0) {
        dir_entry = RefreshFileView(ctx, dir_entry);
        maybe_change_x_step = TRUE;
        need_dsp_help = TRUE;
      } else if (create_result < 0) {
        need_dsp_help = FALSE;
      } else {
        need_dsp_help = TRUE;
      }
    }
    UI_FreeArchivePayload(&payload);
  } break;

  case ACTION_CMD_O:
  case ACTION_CMD_G:
    UI_Beep(ctx, FALSE);
    break;

  case ACTION_TOGGLE_MODE: {
    int cursor_pos;
    int list_pos;
    int start_file;
    if (ctx->preview_mode) {
      UI_Beep(ctx, FALSE);
      break;
    }
    list_pos = dir_entry->start_file + dir_entry->cursor_pos;
    cursor_pos = dir_entry->cursor_pos;
    RotatePanelFileMode(ctx, ctx->active);
    FileNav_SyncGridMetrics(ctx);
    if (cursor_pos >= FileNav_GetMaxDispFiles(ctx))
      cursor_pos = FileNav_GetMaxDispFiles(ctx) - 1;
    start_file = list_pos - cursor_pos;
    if (!AppStateCommitDirEntryFileViewport(dir_entry, start_file, cursor_pos))
      return FALSE;
    DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                 dir_entry->start_file + dir_entry->cursor_pos, start_x,
                 ctx->ctx_file_window);
  } break;

  case ACTION_CMD_V:
    fe_ptr =
        ctx->active
            ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
            .file;
    if (ctx->view_mode == ARCHIVE_MODE) {
      (void)View(ctx, dir_entry, fe_ptr->name);
    } else {
      char full_path[PATH_LENGTH + 1];
      GetFileNamePath(fe_ptr, full_path);
      (void)View(ctx, dir_entry, full_path);
    }
    break;

  case ACTION_CMD_H:
    fe_ptr =
        ctx->active
            ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
            .file;
    (void)GetRealFileNamePath(fe_ptr, filepath, ctx->view_mode);
    (void)ViewHex(ctx, filepath);
    need_dsp_help = TRUE;
    break;

  case ACTION_CMD_E:
    fe_ptr =
        ctx->active
            ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
            .file;
    de_ptr = fe_ptr->dir_entry;
    (void)GetFileNamePath(fe_ptr, filepath);
    (void)Edit(ctx, de_ptr, filepath);
    break;

  case ACTION_COMPARE_FILE:
    if (ctx->active->file_count > 0 && ctx->active->file_entry_list) {
      int compare_idx = dir_entry->start_file + dir_entry->cursor_pos;
      if (compare_idx < 0)
        compare_idx = 0;
      if ((unsigned int)compare_idx >= ctx->active->file_count)
        compare_idx = (int)ctx->active->file_count - 1;
      if (compare_idx >= 0) {
        fe_ptr = ctx->active->file_entry_list[compare_idx].file;
        FileCompare_LaunchExternal(ctx, fe_ptr);
      }
    }
    need_dsp_help = TRUE;
    break;

  case ACTION_CMD_MKFILE:
    if (ctx->view_mode == DISK_MODE || ctx->view_mode == USER_MODE) {
      char file_name[PATH_LENGTH * 2 + 1];
      ClearHelp(ctx);
      *file_name = '\0';
      if (UI_ReadString(ctx, ctx->active, "MAKE FILE:", file_name, PATH_LENGTH,
                        HST_FILE) == CR) {
        int mk_result = MakeFile(ctx, dir_entry, file_name, s, NULL,
                                 (ChoiceCallback)UI_ChoiceResolver);
        if (mk_result == 0) {
          BuildFileEntryList(ctx, ctx->active);
          RefreshView(ctx, dir_entry);
        } else if (mk_result == 1) {
          MESSAGE(ctx, "File already exists!");
        } else {
          MESSAGE(ctx, "Can't create File*\"%s\"", file_name);
        }
      }
      need_dsp_help = TRUE;
    }
    break;

  case ACTION_CMD_S:
    UI_HandleSort(ctx, dir_entry, s, start_x);
    need_dsp_help = TRUE;
    break;

  case ACTION_FILTER:
    if (UI_ReadFilter(ctx) == 0) {
      if (!AppStateCommitDirEntryFileViewport(dir_entry, 0, 0))
        return FALSE;
      BuildFileEntryList(ctx, ctx->active);
      DisplayFilter(ctx, s);
      DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                   dir_entry->start_file + dir_entry->cursor_pos, start_x,
                   ctx->ctx_file_window);

      if (dir_entry->global_flag)
        DisplayDiskStatistic(ctx, s);
      else
        DisplayDirStatistic(ctx, dir_entry, NULL, s);

      if (ctx->active->file_count == 0)
        unput_char = ESC;
      maybe_change_x_step = TRUE;
    }
    need_dsp_help = TRUE;
    break;

  case ACTION_LOG:
    fe_ptr =
        ctx->active
            ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
            .file;
    if (ctx->view_mode == DISK_MODE || ctx->view_mode == USER_MODE) {
      (void)GetFileNamePath(fe_ptr, new_log_path);
      if (!GetNewLogPath(ctx, ctx->active, new_log_path)) {
        if (!AppStateCommitDirEntryLogFlag(dir_entry, TRUE))
          return FALSE;
        if (LogDisk(ctx, ctx->active, new_log_path) == 0) {
          dir_entry = GetPanelDirEntry(ctx->active);
          if (!dir_entry) {
            loop_action = ACTION_ESCAPE;
            break;
          }
        }
        unput_char = ESC;
      }
      need_dsp_help = TRUE;
    }
    break;

  case ACTION_ENTER:
    if (ctx->preview_mode) {
      loop_action = ACTION_NONE;
      break;
    }
    if (dir_entry->big_window)
      break;
    if (!AppStateCommitPanelFileShape(ctx->active, TRUE))
      return FALSE;
    if (!AppStateCommitDirEntryFileShape(dir_entry, TRUE))
      return FALSE;
    RefreshView(ctx, dir_entry);
    FileNav_RereadWindowSize(ctx, dir_entry);
    loop_action = ACTION_NONE;
    break;

  case ACTION_QUIT_DIR:
    need_dsp_help = TRUE;
    fe_ptr =
        ctx->active
            ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
            .file;
    de_ptr = fe_ptr->dir_entry;
    QuitTo(ctx, de_ptr);
    break;

  case ACTION_QUIT:
    need_dsp_help = TRUE;
    Quit(ctx);
    loop_action = ACTION_NONE;
    break;

  case ACTION_HELP:
    (void)UI_ShowIntegratedHelp(ctx, dir_entry);
    need_dsp_help = TRUE;
    break;

  case ACTION_REFRESH:
    dir_entry = RefreshFileView(ctx, dir_entry);
    need_dsp_help = TRUE;
    break;

  case ACTION_EDIT_CONFIG:
    UI_OpenConfigProfile(ctx, dir_entry);
    need_dsp_help = TRUE;
    break;

  case ACTION_RESIZE:
    (void)AppStateMarkResizeRequest(ctx);
    break;

  case ACTION_TOGGLE_STATS: if (ctx->is_split_screen && ctx->active &&
                                !AppStateCommitPanelStatsVisibility(ctx->active, !ctx->active->show_stats))
      break;
    if (!ctx->is_split_screen || !ctx->active) ctx->show_stats = !ctx->show_stats;
    (void)AppStateMarkResizeRequest(ctx);
    break;
  case ACTION_TOGGLE_COMPACT:
    if (!AppStateCommitFixedColumnWidth(ctx,
                                        (ctx->fixed_col_width == 0)
                                            ? ResolveCompactFileWidth(
                                                  ctx, ctx->active)
                                            : 0))
      return FALSE;
    (void)AppStateMarkResizeRequest(ctx);
    break;

  case ACTION_ESCAPE:
    break;

  default:
    handled = FALSE;
    break;
  }

misc_dispatch_done:
  if (handled && dir_entry_ptr && dir_entry)
    *dir_entry_ptr = dir_entry;

#undef loop_action
#undef unput_char
#undef start_x
#undef need_dsp_help
#undef maybe_change_x_step
  return handled;
}

static BOOL HandleTaggedCopyAction(ViewContext *ctx, int action,
                                   DirEntry *dir_entry, Statistic *s) {
  WalkingPackage walking_package = {0};
  DirEntry *dest_dir_entry = NULL;
  char to_dir[PATH_LENGTH * 2 + 1] = {0};
  char to_file[PATH_LENGTH + 1] = {0};
  char to_path[PATH_LENGTH + 1] = {0};
  BOOL path_copy = (action == ACTION_CMD_TAGGED_Y) ? TRUE : FALSE;
  BOOL use_realpath =
      (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE);
  DirEntry *tree = use_realpath ? NULL : ((s) ? s->tree : NULL);

  if (GetCopyParameter(ctx, NULL, path_copy, to_file, to_dir) != 0)
    return FALSE;
  if (PromptCopyMoveDestination(ctx, dir_entry, to_dir, use_realpath, tree,
                                to_path, &dest_dir_entry) != 0) {
    return FALSE;
  }

  walking_package.function_data.copy.statistic_ptr = s;
  walking_package.function_data.copy.dest_dir_entry = dest_dir_entry;
  walking_package.function_data.copy.to_file = to_file;
  walking_package.function_data.copy.to_path = to_path;
  walking_package.function_data.copy.path_copy = path_copy;
  walking_package.function_data.copy.conflict_cb =
      (void *)(ConflictCallback)UI_ConflictResolverWrapper;
  walking_package.function_data.copy.dir_create_mode = 0;
  walking_package.function_data.copy.overwrite_mode = 0;
  walking_package.function_data.copy.choice_cb =
      (void *)(ChoiceCallback)UI_ChoiceResolver;

  FileTags_WalkTaggedFiles(ctx, dir_entry->start_file, dir_entry->cursor_pos,
                           CopyTaggedFiles, &walking_package);
  UI_RefreshSyncPanels(ctx, dir_entry);
  return TRUE;
}

static BOOL HandleTaggedMoveAction(ViewContext *ctx, DirEntry *dir_entry,
                                   Statistic *s,
                                   BOOL *maybe_change_x_step_ptr) {
  WalkingPackage walking_package = {0};
  DirEntry *dest_dir_entry = NULL;
  char to_dir[PATH_LENGTH * 2 + 1] = {0};
  char to_file[PATH_LENGTH + 1] = {0};
  char to_path[PATH_LENGTH + 1] = {0};

  if (!s || !maybe_change_x_step_ptr)
    return FALSE;

  if (GetMoveParameter(ctx, NULL, to_file, to_dir) != 0)
    return FALSE;
  if (PromptCopyMoveDestination(ctx, dir_entry, to_dir, FALSE, s->tree,
                                to_path, &dest_dir_entry) != 0) {
    return FALSE;
  }

  walking_package.function_data.mv.dest_dir_entry = dest_dir_entry;
  walking_package.function_data.mv.to_file = to_file;
  walking_package.function_data.mv.to_path = to_path;
  walking_package.function_data.mv.conflict_cb =
      (void *)(ConflictCallback)UI_ConflictResolverWrapper;
  walking_package.function_data.mv.dir_create_mode = 0;
  walking_package.function_data.mv.overwrite_mode = 0;
  walking_package.function_data.mv.choice_cb =
      (void *)(ChoiceCallback)UI_ChoiceResolver;

  FileTags_WalkTaggedFiles(ctx, dir_entry->start_file, dir_entry->cursor_pos,
                           MoveTaggedFiles, &walking_package);
  UI_RefreshSyncPanels(ctx, dir_entry);
  BuildFileEntryList(ctx, ctx->active);
  if (!AppStateCommitDirEntryFileViewport(dir_entry, 0, 0))
    return FALSE;

  RefreshView(ctx, dir_entry);
  *maybe_change_x_step_ptr = TRUE;
  return TRUE;
}

static BOOL HandleTaggedMutationDispatchAction(
    ViewContext *ctx, int action, DirEntry *dir_entry, Statistic *s,
    BOOL *need_dsp_help_ptr, BOOL *maybe_change_x_step_ptr, BOOL *handled_ptr) {
  WalkingPackage walking_package = {0};
  int max_disp_files =
      getmaxy(ctx->ctx_file_window) * GetPanelMaxColumn(ctx->active);
  char new_name[PATH_LENGTH + 1] = {0};

  if (!handled_ptr || !need_dsp_help_ptr || !maybe_change_x_step_ptr)
    return FALSE;

  *handled_ptr = TRUE;
  switch (action) {
  case ACTION_CMD_TAGGED_V:
    if (FileTags_IsMatchingTaggedFiles(ctx)) {
      UI_ViewTaggedFiles(ctx, dir_entry);
      *need_dsp_help_ptr = TRUE;
    }
    return TRUE;

  case ACTION_CMD_TAGGED_Y:
  case ACTION_CMD_TAGGED_C:
    if (FileTags_IsMatchingTaggedFiles(ctx)) {
      *need_dsp_help_ptr = TRUE;
      return HandleTaggedCopyAction(ctx, action, dir_entry, s);
    }
    return TRUE;

  case ACTION_CMD_TAGGED_M:
    if ((ctx->view_mode == DISK_MODE || ctx->view_mode == USER_MODE) &&
        FileTags_IsMatchingTaggedFiles(ctx)) {
      *need_dsp_help_ptr = TRUE;
      return HandleTaggedMoveAction(ctx, dir_entry, s,
                                    maybe_change_x_step_ptr);
    }
    return TRUE;

  case ACTION_CMD_TAGGED_D:
    if ((ctx->view_mode == DISK_MODE || ctx->view_mode == USER_MODE ||
         ctx->view_mode == ARCHIVE_MODE) &&
        FileTags_IsMatchingTaggedFiles(ctx)) {
      *need_dsp_help_ptr = TRUE;
      (void)FileTags_UI_DeleteTaggedFiles(ctx, max_disp_files, s);
      RefreshView(ctx, dir_entry);
      *maybe_change_x_step_ptr = TRUE;
    }
    return TRUE;

  case ACTION_CMD_TAGGED_R:
    if ((ctx->view_mode == DISK_MODE || ctx->view_mode == USER_MODE ||
         ctx->view_mode == ARCHIVE_MODE) &&
        FileTags_IsMatchingTaggedFiles(ctx)) {
      *need_dsp_help_ptr = TRUE;

      if (GetRenameParameter(ctx, NULL, new_name))
        return FALSE;

      walking_package.function_data.rename.new_name = new_name;
      walking_package.function_data.rename.confirm = FALSE;

      FileTags_WalkTaggedFiles(ctx, dir_entry->start_file, dir_entry->cursor_pos,
                               RenameTaggedFiles, &walking_package);
      BuildFileEntryList(ctx, ctx->active);
      RefreshView(ctx, dir_entry);
      *maybe_change_x_step_ptr = TRUE;
    }
    return TRUE;

  default:
    *handled_ptr = FALSE;
    return FALSE;
  }
}

static BOOL HandleTaggedCommandDispatchAction(
    ViewContext *ctx, int action, DirEntry *dir_entry, int start_x,
    const Statistic *s, BOOL *need_dsp_help_ptr, BOOL *handled_ptr) {
  WalkingPackage walking_package = {0};

  if (!handled_ptr || !need_dsp_help_ptr)
    return FALSE;

  *handled_ptr = TRUE;
  switch (action) {
  case ACTION_CMD_TAGGED_P:
    if (!FileTags_IsMatchingTaggedFiles(ctx)) {
    } else if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
      UI_Message(ctx, "^P is not available in archive mode");
    } else {
      char filepath[PATH_LENGTH + 1] = {0};
      *need_dsp_help_ptr = TRUE;

      filepath[0] = '\0'; /* Initialize buffer to prevent garbage prompt */
      if (GetPipeCommand(ctx, filepath) == 0) {
        int pipe_status = 0;
        pid_t pipe_child_pid = -1;

        /* Exit ncurses mode */
        endwin();
        SuspendClock(ctx);

        if (RuntimeLaunchStartShellWriter(
                filepath, NULL, &walking_package.function_data.pipe_cmd.pipe_file,
                &pipe_child_pid) != 0) {
          /* Restore ncurses mode if popen fails */
          InitClock(ctx);
          touchwin(stdscr);
          wnoutrefresh(stdscr);
          doupdate();
          UI_Message(ctx, "execution of command*%s*failed", filepath);
        } else {
          FileTags_SilentWalkTaggedFiles(ctx, PipeTaggedFiles, &walking_package);

          /* Close pipe and capture return value */
          const int close_ret = RuntimeLaunchCloseWriter(
              walking_package.function_data.pipe_cmd.pipe_file, pipe_child_pid,
              &pipe_status);

          /* Wait for user input */
          HitReturnToContinue();

          /* Restore ncurses mode */
          InitClock(ctx);
          touchwin(stdscr);
          wnoutrefresh(stdscr);
          doupdate(); /* Restore screen */

          if (close_ret != 0 || pipe_status != 0) {
            UI_Warning(ctx, "pipe command failed");
          }
        }
      }
      RefreshView(ctx, dir_entry);
    }
    return TRUE;

  case ACTION_CMD_TAGGED_PRINT:
    if (!FileTags_IsMatchingTaggedFiles(ctx)) {
    } else if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
      UI_Message(ctx, "^P is not available in archive mode");
    } else {
      *need_dsp_help_ptr = TRUE;
      UI_HandlePrint(ctx, dir_entry, TRUE);
      RefreshView(ctx, dir_entry);
    }
    return TRUE;

  case ACTION_CMD_TAGGED_S:
    /* STRICT FILTER MODE: Only allow if tags exist */
    if (!FileTags_IsMatchingTaggedFiles(ctx)) {
      /* If no tags, this command does nothing (or shows error) */
      MESSAGE(ctx, "No tagged files");
    } else if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE &&
               ctx->view_mode != ARCHIVE_MODE) {
      MESSAGE(ctx, "^S is not available in archive mode");
    } else {
      char *command_line;
      char search_pattern[256];

      if ((command_line = (char *)malloc(COMMAND_LINE_LENGTH + 1)) == NULL) {
        UI_Error(ctx, "", 0, "Malloc failed");
        return TRUE;
      }

      /* Allocate new buffer for silent command */
      size_t silent_cmd_size =
          (size_t)COMMAND_LINE_LENGTH + sizeof(" > /dev/null 2>&1");
      char *silent_cmd = (char *)malloc(silent_cmd_size);
      if (!silent_cmd) {
        UI_Error(ctx, "", 0, "Malloc failed");
        free(command_line);
        return TRUE;
      }

      *need_dsp_help_ptr = TRUE;
      *command_line = '\0';
      search_pattern[0] = '\0';

      /* Filter Mode */
      if (!GetSearchCommandLine(ctx, command_line, search_pattern)) {
        if (!AppStateCommitGlobalSearchTerm(ctx, search_pattern)) {
          free(command_line);
          free(silent_cmd);
          return FALSE;
        }
        NormalizeQuotedExecPlaceholders(
            command_line, (size_t)COMMAND_LINE_LENGTH + 1U);
        /* Construct Silent Command */
        {
          int n = snprintf(silent_cmd, silent_cmd_size, "%s > /dev/null 2>&1",
                           command_line);
          if (n < 0 || (size_t)n >= silent_cmd_size) {
            UI_Message(ctx, "Command too long");
          } else {
            walking_package.function_data.execute.command = silent_cmd;
            /* Use modified SilentTagWalk (Untag on Fail) */
            FileTags_SilentTagWalkTaggedFiles(ctx, ExecuteCommand,
                                              &walking_package);
            /* No HitReturnToContinue - purely silent */
          }
        }
      }

      /* Refresh Display */
      UI_RenderFilePanel(ctx, dir_entry, start_x);
      DisplayDiskStatistic(ctx, s);
      if (dir_entry->global_flag)
        DisplayDiskStatistic(ctx, s);
      else
        DisplayDirStatistic(ctx, dir_entry, NULL, s);

      free(command_line);
      free(silent_cmd);
    }
    return TRUE;

  case ACTION_CMD_TAGGED_X:
    if (!FileTags_IsMatchingTaggedFiles(ctx)) {
    } else if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE &&
               ctx->view_mode != ARCHIVE_MODE) {
      UI_Message(ctx, "^X is not available in archive mode");
    } else {
      char *command_line;

      if ((command_line = (char *)malloc(COMMAND_LINE_LENGTH + 1)) == NULL) {
        UI_Error(ctx, __FILE__, __LINE__, "Malloc failed");
        return TRUE;
      }

      *need_dsp_help_ptr = TRUE;
      *command_line = '\0';
      if (GetTaggedCommandLine(ctx, command_line) == 0) {
        NormalizeQuotedExecPlaceholders(
            command_line, (size_t)COMMAND_LINE_LENGTH + 1U);
        endwin();
        SuspendClock(ctx);
        walking_package.function_data.execute.command = command_line;
        FileTags_SilentWalkTaggedFiles(ctx, ExecuteCommand, &walking_package);
        HitReturnToContinue();

        InitClock(ctx);
        touchwin(stdscr);
        wnoutrefresh(stdscr);
        doupdate();

        dir_entry = RefreshFileView(ctx, dir_entry);

        /* Insert: Explicit Global Refresh */
        RefreshView(ctx, dir_entry);
      }
      free(command_line);
    }
    return TRUE;

  default:
    *handled_ptr = FALSE;
    return FALSE;
  }
}

static BOOL HandleTaggedFileOpDispatchAction(
    ViewContext *ctx, int action, DirEntry *dir_entry, int start_x,
    Statistic *s, BOOL *need_dsp_help_ptr, BOOL *maybe_change_x_step_ptr,
    BOOL *handled_ptr) {
  if (HandleTaggedMutationDispatchAction(ctx, action, dir_entry, s,
                                         need_dsp_help_ptr,
                                         maybe_change_x_step_ptr,
                                         handled_ptr)) {
    if (!AppStateCommitDirEntryFileViewport(
            dir_entry, dir_entry->start_file, dir_entry->cursor_pos))
      return FALSE;
    return TRUE;
  }

  if (!HandleTaggedCommandDispatchAction(ctx, action, dir_entry, start_x, s,
                                         need_dsp_help_ptr, handled_ptr)) {
    return FALSE;
  }

  if (!AppStateCommitDirEntryFileViewport(
          dir_entry, dir_entry->start_file, dir_entry->cursor_pos))
    return FALSE;
  return TRUE;
}

static BOOL HandleTaggedAttributeDispatchAction(
    ViewContext *ctx, int action, const DirEntry *dir_entry, int start_x,
    BOOL *need_dsp_help_ptr, BOOL *handled_ptr) {
  WalkingPackage walking_package = {0};
  FileEntry *fe_ptr = NULL;
  int owner_id = 0, group_id = 0;

  if (!handled_ptr)
    return FALSE;

  *handled_ptr = TRUE;
  if (action != ACTION_CMD_TAGGED_A) {
    *handled_ptr = FALSE;
    return FALSE;
  }

#define need_dsp_help (*need_dsp_help_ptr)

  if ((ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) ||
      !FileTags_IsMatchingTaggedFiles(ctx)) {
    return TRUE;
  }

  need_dsp_help = TRUE;
  {
    int attr_action = UI_PromptAttributeAction(ctx, TRUE, FALSE);

    if (attr_action == 'M') {
      mode_t mask = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
      char mode[16] = {0};

      (void)GetAttributes(mask, mode);
      ClearHelp(ctx);
      if (UI_ReadString(ctx, ctx->active, "MODE (octal/rwx):", mode,
                        (int)sizeof(mode), HST_CHANGE_MODUS) == CR) {
        char parsed_mode[12] = {0};
        char preview_mode[10] = {0};

        if (UI_ParseModeInput(mode, parsed_mode, preview_mode) != 0) {
          UI_Message(ctx, "Invalid mode. Use 3/4-digit octal or -rwxrwxrwx");
          wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
          wclrtoeol(ctx->ctx_border_window);
          wnoutrefresh(ctx->ctx_border_window);
          return TRUE;
        }

        if (snprintf(walking_package.function_data.change_mode.new_mode,
                     sizeof(walking_package.function_data.change_mode.new_mode),
                     "%s", parsed_mode) >=
            (int)sizeof(walking_package.function_data.change_mode.new_mode)) {
          UI_Message(ctx, "Mode value too long.");
          wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
          wclrtoeol(ctx->ctx_border_window);
          wnoutrefresh(ctx->ctx_border_window);
          return TRUE;
        }

        FileTags_WalkTaggedFiles(ctx, dir_entry->start_file, dir_entry->cursor_pos,
                                 SetFileModus, &walking_package);
        UI_RenderFilePanel(ctx, dir_entry, start_x);
      }

      wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
      wclrtoeol(ctx->ctx_border_window);
      wnoutrefresh(ctx->ctx_border_window);
    } else if (attr_action == 'O') {
      if ((owner_id = GetNewOwner(ctx, -1)) >= 0) {
        walking_package.function_data.change_owner.new_owner_id = owner_id;
        FileTags_WalkTaggedFiles(ctx, dir_entry->start_file, dir_entry->cursor_pos,
                                 SetFileOwner, &walking_package);
        UI_RenderFilePanel(ctx, dir_entry, start_x);
      }
    } else if (attr_action == 'G') {
      if ((group_id = GetNewGroup(ctx, -1)) >= 0) {
        walking_package.function_data.change_group.new_group_id = group_id;
        FileTags_WalkTaggedFiles(ctx, dir_entry->start_file, dir_entry->cursor_pos,
                                 SetFileGroup, &walking_package);
        UI_RenderFilePanel(ctx, dir_entry, start_x);
      }
    } else if (attr_action == 'D' || attr_action == 0x04) {
      time_t new_time = time(NULL);
      int scope_mask = 0;
      int idx = 0;

      if (UI_GetDateChangeSpec(ctx, &new_time, &scope_mask) != 0) {
        return TRUE;
      }

      for (idx = 0; idx < (int)ctx->active->file_count; idx++) {
        struct utimbuf times;
        struct stat updated_stat;
        char path[PATH_LENGTH + 1];

        fe_ptr = ctx->active->file_entry_list[idx].file;
        if (!fe_ptr || !fe_ptr->tagged)
          continue;

        GetFileNamePath(fe_ptr, path);
        times.actime = (scope_mask & DATE_SCOPE_ACCESS)
                           ? new_time
                           : fe_ptr->stat_struct.st_atime;
        times.modtime = (scope_mask & DATE_SCOPE_MODIFY)
                            ? new_time
                            : fe_ptr->stat_struct.st_mtime;

        if (utime(path, &times) != 0)
          continue;
        if (stat(path, &updated_stat) == 0)
          fe_ptr->stat_struct = updated_stat;
      }

      UI_RenderFilePanel(ctx, dir_entry, start_x);
    }
  }

  return TRUE;

#undef need_dsp_help
}

static void UpdateTaggedActionStatistics(ViewContext *ctx,
                                         const DirEntry *dir_entry, int start_x,
                                         const Statistic *s) {
  UI_RenderFilePanel(ctx, dir_entry, start_x);
  DisplayDiskStatistic(ctx, s);
  DisplayDirStatistic(ctx, dir_entry, NULL, s);
}

static void SetFileTaggedState(YtreeNovaPanel *panel, FileEntry *fe_ptr,
                               Statistic *s, BOOL tagged) {
  DirEntry *de_ptr = NULL;
  off_t file_size = 0;

  if (!fe_ptr)
    return;

  de_ptr = fe_ptr->dir_entry;
  file_size = fe_ptr->stat_struct.st_size;
  if (tagged) {
    if (fe_ptr->tagged)
      return;

    fe_ptr->tagged = TRUE;
    (void)AppStateCommitDirEntryTaggedPayload(
        de_ptr, de_ptr->tagged_files + 1, de_ptr->tagged_bytes + file_size);
    s->disk_tagged_files++;
    s->disk_tagged_bytes += file_size;
    PanelTags_RecordFileState(panel, fe_ptr, TRUE);
    return;
  }

  if (!fe_ptr->tagged)
    return;

  fe_ptr->tagged = FALSE;
  (void)AppStateCommitDirEntryTaggedPayload(
      de_ptr, de_ptr->tagged_files - 1, de_ptr->tagged_bytes - file_size);
  s->disk_tagged_files--;
  s->disk_tagged_bytes -= file_size;
  PanelTags_RecordFileState(panel, fe_ptr, FALSE);
}

static void SetFileTaggedStateRange(ViewContext *ctx, int start_idx, BOOL tagged,
                                    Statistic *s) {
  int i = 0;

  for (i = start_idx; i < (int)ctx->active->file_count; i++) {
    FileEntry *fe_ptr = ctx->active->file_entry_list[i].file;
    SetFileTaggedState(ctx->active, fe_ptr, s, tagged);
  }
}

static BOOL HandleTaggedSelectionDispatchAction(
    ViewContext *ctx, int action, DirEntry *dir_entry, int *unput_char_ptr,
    int start_x, Statistic *s, BOOL *need_dsp_help_ptr,
    BOOL *maybe_change_x_step_ptr, BOOL *handled_ptr) {
  FileEntry *fe_ptr = NULL;

  if (!handled_ptr)
    return FALSE;

  *handled_ptr = TRUE;
  switch (action) {
  case ACTION_TAG:
    fe_ptr =
        ctx->active
            ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
            .file;
    SetFileTaggedState(ctx->active, fe_ptr, s, TRUE);
    UpdateTaggedActionStatistics(ctx, dir_entry, start_x, s);
    if (unput_char_ptr)
      *unput_char_ptr = KEY_DOWN;
    return TRUE;

  case ACTION_UNTAG:
    fe_ptr =
        ctx->active
            ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
            .file;
    SetFileTaggedState(ctx->active, fe_ptr, s, FALSE);
    UpdateTaggedActionStatistics(ctx, dir_entry, start_x, s);
    if (unput_char_ptr)
      *unput_char_ptr = KEY_DOWN;
    return TRUE;

  case ACTION_TAG_ALL:
    SetFileTaggedStateRange(ctx, 0, TRUE, s);
    UpdateTaggedActionStatistics(ctx, dir_entry, start_x, s);
    return TRUE;

  case ACTION_UNTAG_ALL:
    SetFileTaggedStateRange(ctx, 0, FALSE, s);
    UpdateTaggedActionStatistics(ctx, dir_entry, start_x, s);
    return TRUE;

  case ACTION_TAG_REST:
    SetFileTaggedStateRange(ctx, dir_entry->start_file + dir_entry->cursor_pos,
                            TRUE, s);
    UpdateTaggedActionStatistics(ctx, dir_entry, start_x, s);
    return TRUE;

  case ACTION_UNTAG_REST:
    SetFileTaggedStateRange(ctx, dir_entry->start_file + dir_entry->cursor_pos,
                            FALSE, s);
    UpdateTaggedActionStatistics(ctx, dir_entry, start_x, s);
    return TRUE;

  case ACTION_TOGGLE_TAGGED_MODE:
    if (!AppStateCommitDirEntryTaggedFilter(
            dir_entry,
            dir_entry->tagged_files ? !dir_entry->tagged_flag : FALSE))
      return FALSE;

    BuildFileEntryList(ctx, ctx->active);
    if (!AppStateCommitDirEntryFileViewport(dir_entry, 0, 0))
      return FALSE;
    UI_RenderFilePanel(ctx, dir_entry, start_x);

    if (maybe_change_x_step_ptr)
      *maybe_change_x_step_ptr = TRUE;
    return TRUE;

  case ACTION_INVERT:
    FileTags_HandleInvertTags(ctx, dir_entry, s);
    if (need_dsp_help_ptr)
      *need_dsp_help_ptr = TRUE;
    return TRUE;

  default:
    *handled_ptr = FALSE;
    return FALSE;
  }
}

BOOL handle_tag_file_action(ViewContext *ctx, int action, DirEntry *dir_entry,
                            int *unput_char_ptr, BOOL *need_dsp_help_ptr,
                            int start_x, Statistic *s,
                            BOOL *maybe_change_x_step_ptr) {
  BOOL handled = FALSE;

  if (HandleTaggedFileOpDispatchAction(ctx, action, dir_entry, start_x, s,
                                       need_dsp_help_ptr,
                                       maybe_change_x_step_ptr, &handled)) {
    return TRUE;
  }
  if (handled)
    return FALSE;

  if (HandleTaggedAttributeDispatchAction(ctx, action, dir_entry, start_x,
                                          need_dsp_help_ptr, &handled)) {
    return TRUE;
  }
  if (handled)
    return FALSE;

  if (HandleTaggedSelectionDispatchAction(ctx, action, dir_entry, unput_char_ptr,
                                          start_x, s, need_dsp_help_ptr,
                                          maybe_change_x_step_ptr, &handled)) {
    return TRUE;
  }
  if (handled)
    return FALSE;

  switch (action) {
  case ACTION_CMD_TAGGED_O:
  case ACTION_CMD_TAGGED_G:
    UI_Beep(ctx, FALSE);
    return TRUE;
  }

  return FALSE;
}
