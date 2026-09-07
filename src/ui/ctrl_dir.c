/***************************************************************************
 *
 * src/ui/ctrl_dir.c
 * Directory Window Controller (Input & Event Handling)
 *
 ***************************************************************************/

#define NO_YTNOVA_MACROS
#include "watcher.h"
#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_render.h"
#include "ytnova_appstate_focus.h"
#include "ytnova_appstate_layout.h"
#include "ytnova_appstate_modal.h"
#include "ytnova_appstate_mode.h"
#include "ytnova_appstate_panel.h"
#include "ytnova_appstate_session.h"
#include "ytnova_cmd.h"
#include "ytnova_fs.h"
#include "ytnova_panel_anchor.h"
#include "ytnova_split_transition.h"
#include "ytnova_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* TREEDEPTH uses GetProfileValue which is 2-arg in NO_YTNOVA_MACROS context */
#undef TREEDEPTH
#define TREEDEPTH (GetProfileValue)(ctx, "TREEDEPTH")

/* dir_list.c: BuildDirEntryList, FreeDirEntryList, FreeVolumeCache,
 *             GetPanelDirEntry, GetSelectedDirEntry */

/* dir_ops.c */
void HandlePlus(ViewContext *ctx, DirEntry *dir_entry, DirEntry *de_ptr,
                char *new_log_path, BOOL *need_dsp_help, YtreeNovaPanel *p);
void HandleReadSubTree(ViewContext *ctx, DirEntry *dir_entry,
                       BOOL *need_dsp_help, YtreeNovaPanel *p);
void HandleCollapseSubTree(ViewContext *ctx, DirEntry *dir_entry,
                           BOOL *need_dsp_help, YtreeNovaPanel *p);
void HandleUnreadSubTree(ViewContext *ctx, DirEntry *dir_entry,
                         DirEntry *de_ptr, BOOL *need_dsp_help, YtreeNovaPanel *p);

void HandleSwitchWindow(ViewContext *ctx, DirEntry *dir_entry,
                        BOOL *need_dsp_help, int *ch, YtreeNovaPanel *p);
static void DirListJump(ViewContext *ctx, DirEntry **dir_entry_ptr,
                        const Statistic *s);
static void DrawDirListJumpPrompt(ViewContext *ctx, WINDOW *win,
                                  const char *search_buf);
static void HandleDirectoryCompare(ViewContext *ctx, DirEntry *source_dir);

static BOOL IsPathInside(const char *outer, const char *candidate) {
  size_t outer_len;

  if (!outer || !candidate)
    return FALSE;
  outer_len = strlen(outer);
  if (outer_len == 0)
    return FALSE;
  if (strncmp(outer, candidate, outer_len) != 0)
    return FALSE;
  return (candidate[outer_len] == FILE_SEPARATOR_CHAR ||
          candidate[outer_len] == '\0');
}

static BOOL BuildDirOpCommand(BOOL move_dir, const char *quoted_src,
                              const char *quoted_dst, char *command_line,
                              size_t command_size) {
  const char *prefix = move_dir ? "mv -- " : "cp -a -- ";
  size_t prefix_len;
  size_t src_len;
  size_t dst_len;
  size_t total_len;
  size_t pos = 0;

  if (!quoted_src || !quoted_dst || !command_line || command_size == 0)
    return FALSE;

  prefix_len = strlen(prefix);
  src_len = strlen(quoted_src);
  dst_len = strlen(quoted_dst);
  total_len = prefix_len + src_len + 1 + dst_len + 1;
  if (total_len > command_size)
    return FALSE;

  memcpy(command_line + pos, prefix, prefix_len);
  pos += prefix_len;
  memcpy(command_line + pos, quoted_src, src_len);
  pos += src_len;
  command_line[pos++] = ' ';
  memcpy(command_line + pos, quoted_dst, dst_len);
  pos += dst_len;
  command_line[pos] = '\0';
  return TRUE;
}

typedef struct {
  char src_path[PATH_LENGTH + 1];
  char dest_dir_path[PATH_LENGTH + 1];
  char dest_path[PATH_LENGTH + 1];
} DirTransferPaths;

static int ResolveDirTargetPath(const ViewContext *ctx, DirEntry *dir_entry,
                                const char *to_dir_in, const char *to_file,
                                BOOL path_copy, DirTransferPaths *paths) {
  char resolved_dir[PATH_LENGTH + 1];
  const char *leaf;

  if (!dir_entry || !to_dir_in || !to_file || !*to_file || !paths)
    return -1;

  GetPath(dir_entry, paths->src_path);
  paths->src_path[PATH_LENGTH] = '\0';

  if (to_dir_in[0] == FILE_SEPARATOR_CHAR) {
    (void)snprintf(resolved_dir, sizeof(resolved_dir), "%s", to_dir_in);
  } else {
    char parent_path[PATH_LENGTH + 1];
    const char *sep;
    sep = strrchr(paths->src_path, FILE_SEPARATOR_CHAR);
    if (!sep) {
      return -1;
    } else if (sep == paths->src_path) {
      parent_path[0] = FILE_SEPARATOR_CHAR;
      parent_path[1] = '\0';
    } else {
      size_t parent_len = (size_t)(sep - paths->src_path);
      if (parent_len >= sizeof(parent_path))
        return -1;
      memcpy(parent_path, paths->src_path, parent_len);
      parent_path[parent_len] = '\0';
    }

    if (Path_Join(resolved_dir, sizeof(resolved_dir), parent_path, to_dir_in) !=
        0)
      return -1;
  }

  leaf = Path_LeafName(to_file);
  if (!leaf || !*leaf)
    return -1;

  if (path_copy && ctx && ctx->active && ctx->active->vol &&
      ctx->active->vol->vol_stats.tree) {
    char source_root[PATH_LENGTH + 1];
    const char *relative;
    const char *last_separator;

    GetPath(ctx->active->vol->vol_stats.tree, source_root);
    relative = paths->src_path;
    if (strncmp(relative, source_root, strlen(source_root)) == 0) {
      relative += strlen(source_root);
      while (*relative == FILE_SEPARATOR_CHAR)
        relative++;
    }
    last_separator = strrchr(relative, FILE_SEPARATOR_CHAR);
    if (last_separator) {
      char relative_parent[PATH_LENGTH + 1];
      size_t parent_len = (size_t)(last_separator - relative);

      if (parent_len >= sizeof(relative_parent))
        return -1;
      memcpy(relative_parent, relative, parent_len);
      relative_parent[parent_len] = '\0';
      if (relative_parent[0] != '\0' &&
          Path_Join(resolved_dir, sizeof(resolved_dir), resolved_dir,
                    relative_parent) != 0)
        return -1;
    }
  }

  (void)snprintf(paths->dest_dir_path, sizeof(paths->dest_dir_path), "%s",
                 resolved_dir);
  if (Path_Join(paths->dest_path, sizeof(paths->dest_path), resolved_dir, leaf) != 0)
    return -1;
  return 0;
}

static DirEntry *HandleDirCopyMove(ViewContext *ctx, DirEntry *dir_entry,
                                   BOOL move_dir, BOOL path_copy,
                                   BOOL *need_dsp_help) {
  char to_file[PATH_LENGTH + 1];
  char to_dir[PATH_LENGTH + 1];
  DirTransferPaths paths;
  char quoted_src[PATH_LENGTH * 4 + 8];
  char quoted_dst[PATH_LENGTH * 4 + 8];
  char command_line[COMMAND_LINE_LENGTH + 1];
  DirEntry *dest_dir_entry = NULL;
  struct stat st;
  int dir_create_mode = 0;
  int cmd_res;
  DirEntry *anchor;
  DirEntry *source_entry = NULL;
  int active_log_mode;
#ifdef HAVE_LIBARCHIVE
  struct Volume *destination_vol = NULL;
#endif

  if (!ctx || !ctx->active || !ctx->active->vol || !dir_entry)
    return dir_entry;
  active_log_mode = ctx->active->vol->vol_stats.log_mode;

  if (active_log_mode != DISK_MODE && active_log_mode != USER_MODE &&
      active_log_mode != ARCHIVE_MODE) {
    UI_ShowStatusLineError(
        ctx, "Directory copy/move is only supported in filesystem mode");
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return dir_entry;
  }

#ifdef HAVE_LIBARCHIVE
  if (active_log_mode == ARCHIVE_MODE) {
    const Statistic *source_stats = &ctx->active->vol->vol_stats;
    unsigned int required = move_dir ? ARCHIVE_CAP_MOVE : ARCHIVE_CAP_COPY_OUT;

    if (!(source_stats->archive_capabilities & required)) {
      UI_ShowStatusLineError(ctx, "This archive does not support directory transfer");
      if (need_dsp_help)
        *need_dsp_help = TRUE;
      return dir_entry;
    }
  }
#endif

  if (move_dir && dir_entry == ctx->active->vol->vol_stats.tree) {
    UI_ShowStatusLineError(ctx, "Cannot move the logged root directory");
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return dir_entry;
  }

  to_file[0] = '\0';
  to_dir[0] = '\0';
  if (move_dir) {
    if (GetMoveParameter(ctx, dir_entry->name, to_file, to_dir) != 0) {
      if (need_dsp_help)
        *need_dsp_help = TRUE;
      return dir_entry;
    }
  } else {
    if (GetCopyParameter(ctx, dir_entry->name, path_copy, to_file, to_dir) != 0) {
      if (need_dsp_help)
        *need_dsp_help = TRUE;
      return dir_entry;
    }
  }

  if (ResolveDirTargetPath(ctx, dir_entry, to_dir, to_file, path_copy,
                           &paths) != 0) {
    UI_ShowStatusLineError(ctx, "Invalid destination path");
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return dir_entry;
  }

#ifdef HAVE_LIBARCHIVE
  destination_vol = Volume_GetByPath(ctx, paths.dest_dir_path);
  if (active_log_mode != ARCHIVE_MODE && destination_vol != NULL &&
      destination_vol->vol_stats.log_mode == ARCHIVE_MODE) {
    (void)FilesystemDirectoryTransferToArchive(
        ctx, move_dir ? ARCHIVE_DIRECTORY_MOVE : ARCHIVE_DIRECTORY_COPY,
        paths.src_path, paths.dest_dir_path, paths.dest_path);
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return RefreshTreeSafe(ctx, ctx->active, dir_entry);
  }
#endif

  if (strcmp(paths.src_path, paths.dest_path) == 0) {
    UI_ShowStatusLineError(ctx, "Source and destination are the same");
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return dir_entry;
  }
  if (IsPathInside(paths.src_path, paths.dest_path)) {
    UI_ShowStatusLineError(ctx,
                           "Destination cannot be inside the source directory");
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return dir_entry;
  }

#ifdef HAVE_LIBARCHIVE
  if (active_log_mode == ARCHIVE_MODE) {
    ArchiveDirectoryTransfer(ctx, &dir_entry,
                             move_dir ? ARCHIVE_DIRECTORY_MOVE
                                      : ARCHIVE_DIRECTORY_COPY,
                             paths.src_path, paths.dest_dir_path,
                             paths.dest_path);
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return dir_entry;
  }
#endif
  while (1) {
    int ensure_result = UI_EnsureCopyMoveDestinationDirectory(
        ctx, paths.dest_dir_path, ctx->active->vol->vol_stats.tree, &dest_dir_entry,
        &dir_create_mode);
    if (ensure_result == 0)
      break;
    if (ensure_result < 0) {
      ReCreateWindows(ctx);
      RefreshView(ctx, dir_entry);
      if (need_dsp_help)
        *need_dsp_help = TRUE;
      return dir_entry;
    }
    RefreshView(ctx, dir_entry);
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return dir_entry;
  }
  if (lstat(paths.dest_path, &st) == 0) {
    UI_ShowStatusLineError(ctx, "Destination already exists");
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return dir_entry;
  } else if (errno != ENOENT) {
    UI_ShowStatusLineError(ctx, "Cannot access destination path");
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return dir_entry;
  }

  if (!Path_ShellQuote(paths.src_path, quoted_src, sizeof(quoted_src)) ||
      !Path_ShellQuote(paths.dest_path, quoted_dst, sizeof(quoted_dst))) {
    UI_ShowStatusLineError(ctx, "Path too long");
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return dir_entry;
  }

  if (!BuildDirOpCommand(move_dir, quoted_src, quoted_dst, command_line,
                         sizeof(command_line))) {
    UI_ShowStatusLineError(ctx, "Command too long");
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return dir_entry;
  }

  cmd_res = SilentSystemCall(ctx, command_line, &ctx->active->vol->vol_stats);
  touchwin(stdscr);
  if (cmd_res != 0) {
    UI_ShowStatusLineError(ctx, move_dir ? "Directory move failed"
                                         : "Directory copy failed");
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return dir_entry;
  }

  if (!move_dir && dest_dir_entry != NULL) {
    anchor = dest_dir_entry;
  } else {
    anchor = DirOps_ResolveCopyMoveRefreshAnchor(
        ctx, paths.src_path, paths.dest_dir_path,
        dir_entry->up_tree ? dir_entry->up_tree : ctx->active->vol->vol_stats.tree);
  }
  dir_entry = RefreshTreeSafe(ctx, ctx->active, anchor);
  if (!move_dir) {
    source_entry = DirOps_FindDirEntryByPath(ctx, paths.src_path);
    if (source_entry != NULL) {
      (void)DirOps_SelectVisibleDirAndRefresh(ctx, ctx->active, source_entry,
                                              &dir_entry);
    }
  }
  RefreshView(ctx, dir_entry);
  if (need_dsp_help)
    *need_dsp_help = TRUE;
  return dir_entry;
}

static BOOL ExitArchiveRootToParent(ViewContext *ctx, DirEntry **dir_entry_ptr,
                                    Statistic **s_ptr,
                                    const struct Volume **start_vol_ptr,
                                    BOOL retain_archive_volume,
                                    BOOL focus_file) {
  struct Volume *old_vol;
  char archive_path[PATH_LENGTH + 1];
  char parent_dir[PATH_LENGTH + 1];
  char archive_name[PATH_LENGTH + 1];
  int target_file_idx = -1;
  int visible_rows = 1;
  int file_idx;
  int file_start = 0;
  int file_cursor = focus_file ? 0 : -1;

  if (!ctx || !ctx->active || !ctx->active->vol || !dir_entry_ptr ||
      !*dir_entry_ptr || !s_ptr || !*s_ptr || !start_vol_ptr) {
    return FALSE;
  }

  old_vol = ctx->active->vol;

  (void)snprintf(archive_path, sizeof(archive_path), "%s", (*s_ptr)->log_path);
  Fnsplit(archive_path, parent_dir, archive_name);

  if (LogDisk(ctx, ctx->active, parent_dir) != 0) {
    return FALSE;
  }

  if (!retain_archive_volume) {
    BOOL vol_in_use = FALSE;
    if (ctx->is_split_screen) {
      const YtreeNovaPanel *other =
          (ctx->active == ctx->left) ? ctx->right : ctx->left;
      if (other && other->vol == old_vol)
        vol_in_use = TRUE;
    }
    if (!vol_in_use)
      Volume_Delete(ctx, old_vol);
  }

  *s_ptr = &ctx->active->vol->vol_stats;
  *start_vol_ptr = ctx->active->vol;
  if (!AppStateCommitGlobalSearchTerm(ctx, NULL))
    return FALSE;

  if (ctx->active->vol->total_dirs > 0) {
    *dir_entry_ptr = ctx->active->vol
                         ->dir_entry_list[ctx->active->disp_begin_pos +
                                          ctx->active->cursor_pos]
                         .dir_entry;
  } else {
    *dir_entry_ptr = (*s_ptr)->tree;
  }

  if (!AppStateCommitDirEntryFileViewport(*dir_entry_ptr, file_start,
                                          file_cursor))
    return FALSE;

  BuildFileEntryList(ctx, ctx->active);
  if (ctx->ctx_small_file_window) {
    visible_rows = getmaxy(ctx->ctx_small_file_window);
  } else if (ctx->ctx_file_window) {
    visible_rows = getmaxy(ctx->ctx_file_window);
  }
  if (visible_rows < 1)
    visible_rows = 1;

  for (file_idx = 0; file_idx < (int)ctx->active->file_count; file_idx++) {
    const FileEntry *fe = ctx->active->file_entry_list[file_idx].file;
    if (fe && strcmp(fe->name, archive_name) == 0) {
      target_file_idx = file_idx;
      break;
    }
  }

  if (target_file_idx >= visible_rows) {
    file_start = target_file_idx - (visible_rows - 1);
  }
  if (file_start >= (int)ctx->active->file_count) {
    file_start = MAXIMUM(0, (int)ctx->active->file_count - 1);
  }
  if (file_start < 0) {
    file_start = 0;
  }

  if (focus_file && target_file_idx >= 0 && ctx->active->file_count > 0) {
    file_cursor = target_file_idx - file_start;
    if (file_cursor < 0)
      file_cursor = 0;
    if (file_cursor >= visible_rows)
      file_cursor = visible_rows - 1;
    if (!AppStateCommitDirEntryFileViewport(*dir_entry_ptr, file_start,
                                            file_cursor))
      return FALSE;
    if (!AppStateCommitPanelFocus(ctx, ctx->active, FOCUS_FILE))
      return FALSE;
    if (!AppStateCommitPanelFileShape(ctx->active, FALSE))
      return FALSE;
    CapturePanelSelectionAnchor(ctx, ctx->active, *dir_entry_ptr);
  } else {
    file_cursor = -1;
    if (!AppStateCommitDirEntryFileViewport(*dir_entry_ptr, file_start,
                                            file_cursor))
      return FALSE;
    if (!AppStateCommitPanelFocus(ctx, ctx->active, FOCUS_TREE))
      return FALSE;
    if (!AppStateCommitPanelFileShape(ctx->active, FALSE))
      return FALSE;
  }

  if (!AppStateCommitPanelFileAnchor(ctx->active, *dir_entry_ptr))
    return FALSE;
  if (!AppStateCommitPanelFileViewport(ctx->active, file_start, file_cursor))
    return FALSE;

  if (!AppStateCommitViewMode(ctx, ctx->active->vol->vol_stats.log_mode))
    return FALSE;
  RefreshView(ctx, *dir_entry_ptr);
  return TRUE;
}

static void HandleDirectoryCompare(ViewContext *ctx, DirEntry *source_dir) {
  CompareRequest request;
  BOOL external_launch = FALSE;

  if (!ctx || !source_dir)
    return;

  if (UI_BuildDirectoryCompareRequest(ctx, source_dir, &request,
                                      &external_launch) !=
      0) {
    return;
  }

  if (external_launch) {
    DirCompare_LaunchExternal(ctx, &request);
    RefreshView(ctx, source_dir);
    return;
  }

  if (request.flow_type == COMPARE_FLOW_DIRECTORY) {
    DirCompare_RunInternalDirectory(ctx, source_dir, &request);
    return;
  }

  DirCompare_RunInternalLoggedTree(ctx, &request);
}

extern int HandleDirWindow(ViewContext *ctx, const DirEntry *start_dir_entry) {
  if (!AppStateValidatedDispatchSurface("surface.directory-window-action-dispatch"))
    return ESC;

  DirEntry *dir_entry, *de_ptr;
  int ch, unput_char;
  BOOL need_dsp_help;
  char new_log_path[PATH_LENGTH + 1];
  YtreeNovaAction action = ACTION_NONE;
  const struct Volume *start_vol = NULL;
  Statistic *s = NULL;
  int height;
  char watcher_path[PATH_LENGTH + 1];
  char left_anchor_path[PATH_LENGTH + 1];
  char right_anchor_path[PATH_LENGTH + 1];
  BOOL has_left_anchor = FALSE;
  BOOL has_right_anchor = FALSE;
  const struct Volume *active_vol = NULL;
  BOOL left_panel_matches_active_vol = FALSE;
  BOOL right_panel_matches_active_vol = FALSE;

  DEBUG_LOG("HandleDirWindow: Recalculating layout");
  Layout_Recalculate(ctx);
  DEBUG_LOG("HandleDirWindow: Calling DisplayMenu");
  DisplayMenu(ctx);

  unput_char = 0;
  de_ptr = NULL;

  /* Safety fallback if Init() has not set up panels */
  if (ctx->active == NULL &&
      !AppStateCommitActivePanel(ctx, ctx->left))
    return ESC;
  if (ctx->active == NULL || ctx->active->vol == NULL)
    return ESC;

  start_vol = ctx->active->vol;
  s = &ctx->active->vol->vol_stats;
  active_vol = ctx->active->vol;
  left_panel_matches_active_vol = (!ctx->left || !ctx->left->vol ||
                                   ctx->left->vol == active_vol);
  right_panel_matches_active_vol = (!ctx->right || !ctx->right->vol ||
                                    ctx->right->vol == active_vol);

  if (ctx->active) {
    DEBUG_LOG("HandleDirWindow: Syncing panel state");
    if (!AppStateMirrorActivePanelFocus(ctx))
      return ESC;

    /* Ensure global context windows follow the active panel. */
    SyncActivePanelWindows(ctx);
  }

  /* Safety Reset for Preview Mode */
  if (ctx->preview_mode) {
    DEBUG_LOG("HandleDirWindow: Resetting preview mode");
    if (!AppStateCommitPreviewMode(ctx, FALSE))
      return ESC;
    ReCreateWindows(ctx);
    DisplayMenu(ctx);
    /* Update context again after ReCreateWindows */
    SyncActivePanelWindows(ctx);
  }

  height = getmaxy(ctx->ctx_dir_window);
  DEBUG_LOG("HandleDirWindow: Window height=%d", height);

  /* Clear flags */
  /*-----------------*/

  SetDirMode(ctx, MODE_3);

  need_dsp_help = TRUE;

  DEBUG_LOG("HandleDirWindow: Building DirEntryList for vol=%p",
            (void *)active_vol);
  if (left_panel_matches_active_vol) {
    has_left_anchor = CapturePanelAnchorPath(ctx->left, active_vol,
                                             left_anchor_path,
                                             sizeof(left_anchor_path));
  }
  if (right_panel_matches_active_vol) {
    has_right_anchor = CapturePanelAnchorPath(ctx->right, active_vol,
                                              right_anchor_path,
                                              sizeof(right_anchor_path));
  }
  if (has_left_anchor || has_right_anchor) {
    DEBUG_LOG("HandleDirWindow:anchors before rebuild left='%s' right='%s'",
              has_left_anchor ? left_anchor_path : "<none>",
              has_right_anchor ? right_anchor_path : "<none>");
  }
  BuildDirEntryList(ctx, active_vol, &ctx->active->current_dir_entry);
  if (has_left_anchor && left_panel_matches_active_vol)
    RestorePanelAnchorPath(active_vol, ctx->left, left_anchor_path);
  if (has_right_anchor && right_panel_matches_active_vol)
    RestorePanelAnchorPath(active_vol, ctx->right, right_anchor_path);
  if (left_panel_matches_active_vol)
    EnsurePanelAnchorVisible(ctx, active_vol, ctx->left, "LEFT");
  if (right_panel_matches_active_vol)
    EnsurePanelAnchorVisible(ctx, active_vol, ctx->right, "RIGHT");
  if (ctx->initial_directory != NULL) {
    if (!strcmp(ctx->initial_directory, ".")) /* Entry just a single "." */
    {
      if (!AppStateCommitPanelTreeViewport(ctx->active, 0, 0))
        return ESC;
      unput_char = CR;
    } else {
      int log_path_len = -1;
      if (*ctx->initial_directory == '.') { /* Entry of form "./alpha/beta" */
        log_path_len =
            snprintf(new_log_path, sizeof(new_log_path), "%s%s",
                     start_dir_entry->name, ctx->initial_directory + 1);
      } else if (*ctx->initial_directory == '~') {
        const char *home = getenv("HOME");
        if (home) {
          /* Entry of form "~/alpha/beta" */
          log_path_len = snprintf(new_log_path, sizeof(new_log_path), "%s%s",
                                  home, ctx->initial_directory + 1);
        } else {
          /* Entry of form "beta" or "/full/path/alpha/beta" */
          log_path_len = snprintf(new_log_path, sizeof(new_log_path), "%s",
                                  ctx->initial_directory);
        }
      } else { /* Entry of form "beta" or "/full/path/alpha/beta" */
        log_path_len = snprintf(new_log_path, sizeof(new_log_path), "%s",
                                ctx->initial_directory);
      }
      if (log_path_len >= 0 && (size_t)log_path_len < sizeof(new_log_path)) {
        int i;
        for (i = 0; i < ctx->active->vol->total_dirs; i++) {
          char new_name[PATH_LENGTH + 1];

          if (*new_log_path == FILE_SEPARATOR_CHAR) {
            GetPath(ctx->active->vol->dir_entry_list[i].dir_entry, new_name);
          } else {
            (void)snprintf(new_name, sizeof(new_name), "%s",
                           ctx->active->vol->dir_entry_list[i].dir_entry->name);
          }
          if (!strcmp(new_log_path, new_name)) {
            if (!AppStateCommitPanelTreeViewport(ctx->active, i, 0))
              return ESC;
            unput_char = CR;
            break;
          }
        }
      }
    }
    ctx->initial_directory = NULL;
  }

  {
    int safe_idx = ctx->active->disp_begin_pos + ctx->active->cursor_pos;
    if (ctx->active->vol->total_dirs <= 0) {
      if (!AppStateCommitPanelTreeViewport(ctx->active, 0, 0))
        return ESC;
      safe_idx = 0;
    } else if (safe_idx < 0 || safe_idx >= ctx->active->vol->total_dirs) {
      if (!AppStateCommitPanelTreeViewport(ctx->active, 0, 0))
        return ESC;
      safe_idx = 0;
    }

    if (ctx->active->vol->dir_entry_list) {
      dir_entry = ctx->active->vol->dir_entry_list[safe_idx].dir_entry;
    } else {
      dir_entry = ctx->active->vol->vol_stats.tree;
    }
  }

  {
    BOOL active_focus_is_file =
        (AppStateResolveActivePanelFocus(ctx) == FOCUS_FILE);

    if (!dir_entry->log_flag) {
      if (ctx->active && active_focus_is_file) {
        int file_start = dir_entry->start_file;
        int file_cursor = dir_entry->cursor_pos;

        if (ctx->active->file_dir_entry == dir_entry) {
          file_start = ctx->active->start_file;
          file_cursor = ctx->active->file_cursor_pos;
        }
        if (file_start < 0)
          file_start = 0;
        if (file_cursor < 0 && dir_entry->total_files > 0)
          file_cursor = 0;

        if (!AppStateCommitDirEntryFileViewport(dir_entry, file_start,
                                                file_cursor))
          return ESC;
        if (!AppStateCommitPanelFileAnchor(ctx->active, dir_entry))
          return ESC;
        (void)AppStateCommitPanelFileViewport(ctx->active,
                                              dir_entry->start_file,
                                              dir_entry->cursor_pos);
        BuildFileEntryList(ctx, ctx->active);
      } else {
        if (!AppStateCommitDirEntryFileViewport(dir_entry, 0, -1))
          return ESC;
      }
    }
    RefreshView(ctx, dir_entry);

    if (dir_entry->log_flag) {
      if ((dir_entry->global_flag) || (dir_entry->tagged_flag)) {
        unput_char = 'S';
      } else {
        unput_char = CR;
      }
    } else if (ctx->active && active_focus_is_file &&
               ctx->active->file_count > 0 && dir_entry->total_files > 0) {
      unput_char = CR;
    }
  }
  do {
    /* Detect Global Volume Change (Split Brain Fix) */
    if (ctx->active == NULL || ctx->active->vol == NULL ||
        ctx->active->vol != start_vol)
      return ESC;

    if (need_dsp_help || ctx->view_mode == ARCHIVE_MODE) {
      need_dsp_help = FALSE;
      DisplayDirHelp(ctx, dir_entry);
    }
    DisplayDirParameter(ctx, dir_entry);
    RefreshWindow(ctx->ctx_dir_window);

    if (ctx->is_split_screen) {
      YtreeNovaPanel *inactive =
          (ctx->active == ctx->left) ? ctx->right : ctx->left;
      EnsurePanelAnchorVisible(ctx, inactive->vol, inactive, "INACTIVE");
      RenderInactivePanel(ctx, inactive);
    }

    if (s->log_mode == DISK_MODE || s->log_mode == USER_MODE) {
      if (ctx->refresh_mode & REFRESH_WATCHER) {
        GetPath(dir_entry, watcher_path);
        Watcher_SetDir(ctx, watcher_path);
      }
    }

    if (unput_char) {
      ch = unput_char;
      DEBUG_LOG("DirLoop:consuming_unput ch=%d", ch);
      unput_char = '\0';
    } else {
      doupdate();
      ch = (ctx->resize_request) ? -1 : GetEventOrKey(ctx);
      /* LF to CR normalization is now handled by GetKeyAction */
    }

    if (IsUserActionDefined(ctx)) { /* User commands take precedence */
      ch = DirUserMode(ctx, dir_entry, ch, &ctx->active->vol->vol_stats);
    }

    /* ViKey processing is now handled inside GetKeyAction */

    if (ctx->resize_request) {
      /* SIMPLIFIED RESIZE: Just call Global Refresh */
      RefreshView(ctx, dir_entry);
      need_dsp_help = TRUE;
      (void)AppStateClearResizeRequest(ctx);
    }

#ifdef KEY_F
    if (ch == KEY_F(9)) {
      (void)UI_OpenApplicationsMenu(ctx);
      need_dsp_help = TRUE;
      continue;
    }
#endif

    action = GetKeyAction(ctx, ch); /* Translate raw input to YtreeNovaAction */
    action = AppStateValidatedKeyAction(action);
    DebugLogDirLoopState("before_dispatch", ctx, dir_entry, ch, action,
                         unput_char);

    switch (action) {
    case ACTION_RESIZE:
      (void)AppStateMarkResizeRequest(ctx);
      break;

    case ACTION_EDIT_CONFIG:
      UI_OpenConfigProfile(ctx, dir_entry);
      need_dsp_help = TRUE;
      break;

    case ACTION_HELP:
      (void)UI_ShowIntegratedHelp(ctx, dir_entry);
      need_dsp_help = TRUE;
      break;

    case ACTION_TOGGLE_STATS: if (ctx->is_split_screen && ctx->active &&
                                  !AppStateCommitPanelStatsVisibility(ctx->active, !ctx->active->show_stats))
        break;
      if (!ctx->is_split_screen || !ctx->active) ctx->show_stats = !ctx->show_stats;
      (void)AppStateMarkResizeRequest(ctx);
      break;
    case ACTION_VIEW_PREVIEW: {
      DirWindowDispatchResult panel_result =
          HandleDirWindowPanelAction(ctx, action, &dir_entry, &s, &start_vol,
                                     &need_dsp_help, &ch, &unput_char);
      if (panel_result == DIR_WINDOW_DISPATCH_RETURN_ESC)
        return ESC;
      if (panel_result == DIR_WINDOW_DISPATCH_CONTINUE)
        continue;
      break;
    }

    case ACTION_SPLIT_SCREEN:
    case ACTION_SWITCH_PANEL:
      if (SplitTransition_HandleDirWindowAction(
              ctx, action, &dir_entry, &s, &start_vol, &need_dsp_help, &ch,
              &unput_char)) {
        break;
      }
      break;

    case ACTION_NONE: /* -1 or unhandled keys */
      if (ch == -1)
        break; /* Ignore -1 (ctx->resize_request handled above) */
      /* Fall through for other unhandled keys to beep */
      UI_Beep(ctx, FALSE);
      break;

    case ACTION_MOVE_DOWN:
      DirNav_Movedown(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_MOVE_UP:
      DirNav_Moveup(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_MOVE_SIBLING_NEXT: {
      const DirEntry *target =
          DirNav_FindVisibleSibling(ctx->active, dir_entry, 1);

      if (target != NULL && target != dir_entry) {
        (void)DirOps_SelectVisibleDirAndRefresh(ctx, ctx->active, target,
                                                &dir_entry);
        need_dsp_help = TRUE;
      }
    }
      break;
    case ACTION_MOVE_SIBLING_PREV: {
      const DirEntry *target =
          DirNav_FindVisibleSibling(ctx->active, dir_entry, -1);

      if (target != NULL && target != dir_entry) {
        (void)DirOps_SelectVisibleDirAndRefresh(ctx, ctx->active, target,
                                                &dir_entry);
        need_dsp_help = TRUE;
      }
    }
      break;
    case ACTION_PAGE_DOWN:
      DirNav_Movenpage(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_PAGE_UP:
      DirNav_Moveppage(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_HOME:
      DirNav_MoveHome(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_END:
      DirNav_MoveEnd(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_MOVE_RIGHT:
      if (!dir_entry->not_scanned && dir_entry->sub_tree != NULL) {
        if (DirOps_SelectVisibleDirAndRefresh(ctx, ctx->active,
                                              dir_entry->sub_tree,
                                              &dir_entry)) {
          need_dsp_help = TRUE;
        }
        break;
      }

      HandlePlus(ctx, dir_entry, de_ptr, new_log_path, &need_dsp_help,
                 ctx->active);
      break;
    case ACTION_TREE_EXPAND_ALL:
      HandlePlus(ctx, dir_entry, de_ptr, new_log_path, &need_dsp_help,
                 ctx->active);
      break;
    case ACTION_ASTERISK:
      HandleReadSubTree(ctx, dir_entry, &need_dsp_help, ctx->active);
      break;
    case ACTION_TREE_EXPAND:
      HandleReadSubTree(ctx, dir_entry, &need_dsp_help, ctx->active);
      break;
    case ACTION_MOVE_LEFT:
      if (dir_entry->up_tree == NULL) {
        if (!dir_entry->not_scanned && dir_entry->sub_tree != NULL) {
          HandleCollapseSubTree(ctx, dir_entry, &need_dsp_help, ctx->active);
        } else if (!dir_entry->unlogged_flag) {
          HandleUnreadSubTree(ctx, dir_entry, de_ptr, &need_dsp_help,
                              ctx->active);
        }
        /* At root, LEFT collapse is a state reset; once reset, LEFT is no-op. */
        break;
      }

      if (!dir_entry->not_scanned && dir_entry->sub_tree != NULL) {
        HandleCollapseSubTree(ctx, dir_entry, &need_dsp_help, ctx->active);
        break;
      }

      (void)DirOps_SelectVisibleDirAndRefresh(ctx, ctx->active,
                                              dir_entry->up_tree, &dir_entry);
      break;
    case ACTION_TO_DIR:
      if (ctx->view_mode != ARCHIVE_MODE) {
        break;
      }
      if (dir_entry->up_tree != NULL) {
        DirEntry *archive_root = dir_entry;
        int k;

        while (archive_root->up_tree != NULL) {
          int parent_idx = -1;
          for (k = 0; k < ctx->active->vol->total_dirs; k++) {
            if (ctx->active->vol->dir_entry_list[k].dir_entry ==
                archive_root->up_tree) {
              parent_idx = k;
              break;
            }
          }
          if (parent_idx < 0) {
            break;
          }
          archive_root = archive_root->up_tree;
        }
        if (DirOps_SelectVisibleDirAndRefresh(ctx, ctx->active, archive_root,
                                              &dir_entry)) {
          DisplayDirHelp(ctx, dir_entry);
        }
        break;
      }
      if (ExitArchiveRootToParent(ctx, &dir_entry, &s, &start_vol, TRUE,
                                  TRUE)) {
        need_dsp_help = TRUE;
        unput_char = CR;
      } else {
        MESSAGE(ctx, "Can't exit archive root.");
        need_dsp_help = TRUE;
      }
      break;
    case ACTION_TREE_COLLAPSE:
      if (!dir_entry->not_scanned && dir_entry->sub_tree != NULL) {
        HandleCollapseSubTree(ctx, dir_entry, &need_dsp_help, ctx->active);
      } else {
        HandleUnreadSubTree(ctx, dir_entry, de_ptr, &need_dsp_help,
                            ctx->active);
      }
      break;
    case ACTION_TOGGLE_HIDDEN: {
      ToggleDotFiles(ctx, ctx->active);

      dir_entry = ResolveActiveDirEntry(ctx, s);
      if (dir_entry == NULL)
        return ESC;

      need_dsp_help = TRUE;
    } break;
    case ACTION_FILTER:
      if (UI_ReadFilter(ctx) == 0) {
        RecalculateSysStats(ctx, s);
        if (!AppStateCommitDirEntryFileViewport(dir_entry, 0, -1))
          return ESC;
        RefreshView(ctx, dir_entry);
      }
      need_dsp_help = TRUE;
      break;
    case ACTION_LIST_JUMP:
      DirListJump(ctx, &dir_entry, s);
      need_dsp_help = TRUE;
      break;
    case ACTION_TAG:
    case ACTION_UNTAG:
    case ACTION_TAG_ALL:
    case ACTION_UNTAG_ALL:
    case ACTION_INVERT:
    case ACTION_TOGGLE_TAGGED_MODE:
    case ACTION_CMD_TAGGED_S:
      if (HandleDirTagActions(ctx, action, &dir_entry, &need_dsp_help, &ch)) {
        break;
      }
      break;

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
      if (!FileInfoHandleDirAction(ctx, action, dir_entry, s))
        return ESC;
      need_dsp_help = TRUE;
      break;

    case ACTION_TOGGLE_MODE:
      RotateDirMode(ctx);
      /*DisplayFileWindow(ctx,  dir_entry, 0, -1 );*/
      DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                  ctx->active->disp_begin_pos,
                  ctx->active->disp_begin_pos + ctx->active->cursor_pos, TRUE);
      /*RefreshWindow( ctx->ctx_file_window );*/
      DisplayDiskStatistic(ctx, s);
      UpdateStatsPanel(ctx, dir_entry, s);
      need_dsp_help = TRUE;
      break;

    case ACTION_CMD_S:
      HandleShowAll(ctx, FALSE, FALSE, dir_entry, &need_dsp_help, &ch,
                    ctx->active);
      break;
    case ACTION_COMPARE_DIR:
      HandleDirectoryCompare(ctx, dir_entry);
      need_dsp_help = TRUE;
      break;
    case ACTION_COMPARE_TREE:
      HandleDirectoryCompare(ctx, dir_entry);
      need_dsp_help = TRUE;
      break;
    case ACTION_ENTER: {
      if (!AppStateCommitSmallWindowBypass(
              ctx,
              ParseSmallWindowSkipValue(GetProfileValue(ctx, "SMALLWINDOWSKIP"))))
        return ESC;
      DirWindowDispatchResult enter_result =
          HandleDirWindowEnterAction(ctx, &dir_entry, &s, &start_vol,
                                     &need_dsp_help, &ch, &unput_char, &action);
      if (enter_result == DIR_WINDOW_DISPATCH_RETURN_ESC)
        return ESC;
      if (enter_result == DIR_WINDOW_DISPATCH_CONTINUE)
        continue;
    } break;
    case ACTION_CMD_X:
      if (!AppStateValidatedDispatchSurface("surface.command-completion-dispatch"))
        return ESC;
      if (!AppStateValidatedEvent("event.command-completion"))
        return ESC;
      if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
      } else {
        char command_template[COMMAND_LINE_LENGTH + 1];
        command_template[0] = '\0';
        if (GetCommandLine(ctx, command_template) == 0) {
          (void)Execute(ctx, dir_entry, NULL, command_template,
                        &ctx->active->vol->vol_stats, UI_ArchiveCallback);
          dir_entry = RefreshTreeSafe(
              ctx, ctx->active, dir_entry); /* Auto-Refresh after command */
          RefreshView(ctx, dir_entry);
        }
      }
      need_dsp_help = TRUE;
      DisplayAvailBytes(ctx, s);
      DisplayDiskStatistic(ctx, s);
      UpdateStatsPanel(ctx, dir_entry, s);
      break;
    case ACTION_CMD_MKFILE:
      if (HandleDirMakeFile(ctx, dir_entry))
        need_dsp_help = TRUE;
      break;

    case ACTION_CMD_M:
      HandleDirMakeDirectory(ctx, dir_entry, s);
      need_dsp_help = TRUE;
      break;
    case ACTION_CMD_D:
      dir_entry = HandleDirDeleteDirectory(ctx, dir_entry);
      need_dsp_help = TRUE;
      break;

    case ACTION_CMD_R:
      HandleDirRenameDirectory(ctx, dir_entry);
      need_dsp_help = TRUE;
      break;
    case ACTION_REFRESH: /* Rescan */
      dir_entry = RefreshTreeSafe(ctx, ctx->active, dir_entry);
      need_dsp_help = TRUE;
      break;
    case ACTION_CMD_G:
      HandleShowAll(ctx, FALSE, TRUE, dir_entry, &need_dsp_help, &ch,
                    ctx->active);
      break;
    case ACTION_CMD_C:
      dir_entry = HandleDirCopyMove(ctx, dir_entry, FALSE, FALSE, &need_dsp_help);
      break;
    case ACTION_CMD_Y:
      dir_entry = HandleDirCopyMove(ctx, dir_entry, FALSE, TRUE, &need_dsp_help);
      break;
    case ACTION_CMD_V:
      dir_entry = HandleDirCopyMove(ctx, dir_entry, TRUE, FALSE, &need_dsp_help);
      break;
    case ACTION_CMD_O:
      UI_Beep(ctx, FALSE);
      break;
    case ACTION_CMD_A:
      if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
        UI_Beep(ctx, FALSE);
        break;
      }
      need_dsp_help = TRUE;
      switch (UI_PromptAttributeAction(ctx, FALSE, TRUE)) {
      case 'M':
        if (ChangeDirModus(ctx, dir_entry))
          break;
        DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                    ctx->active->disp_begin_pos,
                    ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                    TRUE);
        DisplayDiskStatistic(ctx, s);
        UpdateStatsPanel(ctx, dir_entry, s);
        break;
      case 'O':
        if (HandleDirOwnership(ctx, dir_entry, TRUE, FALSE))
          break;
        DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                    ctx->active->disp_begin_pos,
                    ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                    TRUE);
        DisplayDiskStatistic(ctx, s);
        UpdateStatsPanel(ctx, dir_entry, s);
        break;
      case 'G':
        if (HandleDirOwnership(ctx, dir_entry, FALSE, TRUE))
          break;
        DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                    ctx->active->disp_begin_pos,
                    ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                    TRUE);
        DisplayDiskStatistic(ctx, s);
        UpdateStatsPanel(ctx, dir_entry, s);
        break;
      case 'D':
        if (ChangeDirDate(ctx, dir_entry))
          break;
        DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                    ctx->active->disp_begin_pos,
                    ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                    TRUE);
        DisplayDiskStatistic(ctx, s);
        UpdateStatsPanel(ctx, dir_entry, s);
        break;
      default:
        break;
      }
      break;

    case ACTION_CMD_I: {
      ArchivePayload payload;
      int gather_result;
      payload.original_source_list = NULL;
      payload.expanded_file_list = NULL;

      gather_result = UI_GatherArchivePayload(ctx, dir_entry, NULL, &payload);
      if (gather_result != 0) {
        if (gather_result < 0)
          UI_ShowStatusLineError(ctx, "Nothing to archive");
        need_dsp_help = FALSE;
      } else {
        int create_result;
        create_result = UI_CreateArchiveFromPayload(ctx, &payload);
        if (create_result == 0) {
          dir_entry = RefreshTreeSafe(ctx, ctx->active, dir_entry);
          need_dsp_help = TRUE;
        } else if (create_result < 0) {
          need_dsp_help = FALSE;
        } else {
          need_dsp_help = TRUE;
        }
      }
      UI_FreeArchivePayload(&payload);
    } break;

    case ACTION_TOGGLE_COMPACT:
      if (!AppStateCommitFixedColumnWidth(
              ctx, (ctx->fixed_col_width == 0)
                       ? ResolveCompactFileWidth(ctx, ctx->active)
                       : 0))
        return ESC;
      (void)AppStateMarkResizeRequest(ctx);
      break;

    case ACTION_CMD_P: /* Pipe Directory */
    {
      char pipe_cmd[PATH_LENGTH + 1];
      pipe_cmd[0] = '\0';
      if (GetPipeCommand(ctx, pipe_cmd) == 0) {
        PipeDirectory(ctx, dir_entry, pipe_cmd);
      }
    }
      need_dsp_help = TRUE;
      break;

    case ACTION_CMD_PRINT: /* Print Directory */
      UI_HandlePrintController(ctx, dir_entry, FALSE);
      need_dsp_help = TRUE;
      break;

    /* Volume Cycling and Selection */
    case ACTION_VOL_MENU: /* Shift-K: Select Loaded Volume */
    case ACTION_VOL_PREV: /* Previous Volume */
    case ACTION_VOL_NEXT: /* Next Volume */
    {
      DirWindowDispatchResult volume_result = HandleDirWindowVolumeAction(
          ctx, action, &dir_entry, &s, start_vol, &need_dsp_help);
      if (volume_result == DIR_WINDOW_DISPATCH_RETURN_ESC)
        return ESC;
    } break;

    case ACTION_QUIT_DIR:
      need_dsp_help = TRUE;
      QuitTo(ctx, dir_entry);
      break;

    case ACTION_QUIT:
      need_dsp_help = TRUE;
      Quit(ctx);
      action = ACTION_NONE;
      break;

    case ACTION_LOG: {
      DirWindowDispatchResult log_result = HandleDirWindowLogAction(
          ctx, &dir_entry, &s, start_vol, &need_dsp_help, new_log_path,
          sizeof(new_log_path));
      if (log_result == DIR_WINDOW_DISPATCH_RETURN_ESC)
        return ESC;
    } break;
    /* Ctrl-L is now ACTION_REFRESH, handled above */
    default: /* Unhandled action, beep */
      UI_Beep(ctx, FALSE);
      break;
    } /* switch */

    dir_entry = ResolveActiveDirEntry(ctx, s);
    if (dir_entry == NULL)
      return ESC;
    DebugLogDirLoopState("after_dispatch", ctx, dir_entry, ch, action,
                         unput_char);

  } while (action != ACTION_QUIT && action != ACTION_ENTER &&
           action != ACTION_ESCAPE &&
           action != ACTION_LOG); /* Loop until explicit quit, escape or log */


  return (ch); /* Return the last raw character that caused exit */
}

static void DirListJump(ViewContext *ctx, DirEntry **dir_entry_ptr,
                        const Statistic *s) {
  char search_buf[256];
  int buf_len = 0;
  int i;
  int found_idx;
  int height;
  int original_disp_begin_pos;
  int original_cursor_pos;
  WINDOW *jump_win =
      (ctx && ctx->ctx_menu_window) ? ctx->ctx_menu_window : stdscr;

  if (!ctx || !ctx->active || !ctx->active->vol ||
      !ctx->active->vol->dir_entry_list || ctx->active->vol->total_dirs <= 0 ||
      !dir_entry_ptr || !*dir_entry_ptr) {
    return;
  }

  height = getmaxy(ctx->ctx_dir_window);
  if (height <= 0)
    return;

  original_disp_begin_pos = ctx->active->disp_begin_pos;
  original_cursor_pos = ctx->active->cursor_pos;

  memset(search_buf, 0, sizeof(search_buf));

  ClearHelp(ctx);
  DrawDirListJumpPrompt(ctx, jump_win, search_buf);

  while (1) {
    int ch;

    DrawDirListJumpPrompt(ctx, jump_win, search_buf);

    ch = Getch(ctx);
    if (ch == -1)
      break;

    if (ch == ESC) {
      (void)AppStateCommitPanelTreeViewport(
          ctx->active, original_disp_begin_pos, original_cursor_pos);
    } else if (ch == CR || ch == LF) {
      break;
    } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b' || ch == KEY_DC) {
      if (buf_len > 0) {
        buf_len--;
        search_buf[buf_len] = '\0';

        if (buf_len == 0) {
          (void)AppStateCommitPanelTreeViewport(
              ctx->active, original_disp_begin_pos, original_cursor_pos);
        } else {
          found_idx = -1;
          for (i = 0; i < ctx->active->vol->total_dirs; i++) {
            DirEntry *candidate = ctx->active->vol->dir_entry_list[i].dir_entry;
            if (candidate && PanelDirIsVisible(ctx->active, candidate) &&
                strncasecmp(candidate->name, search_buf, buf_len) == 0) {
              found_idx = i;
              break;
            }
          }

          if (found_idx != -1) {
            PositionPanelAtIndex(ctx->active, found_idx);
          }
        }
      }
    } else if (isprint(ch)) {
      if (buf_len < (int)sizeof(search_buf) - 1) {
        search_buf[buf_len] = ch;
        search_buf[buf_len + 1] = '\0';

        found_idx = -1;
        for (i = 0; i < ctx->active->vol->total_dirs; i++) {
          DirEntry *candidate = ctx->active->vol->dir_entry_list[i].dir_entry;
          if (candidate && PanelDirIsVisible(ctx->active, candidate) &&
              strncasecmp(candidate->name, search_buf, (size_t)buf_len + 1) ==
                  0) {
            found_idx = i;
            break;
          }
        }

        if (found_idx != -1) {
          buf_len++;
          PositionPanelAtIndex(ctx->active, found_idx);
        } else {
          /* Sticky cursor: keep current selection when input has no match. */
          buf_len++;
        }
      }
    }

    if (ctx->active->cursor_pos < 0)
      (void)AppStateCommitPanelTreeViewport(ctx->active,
                                            ctx->active->disp_begin_pos, 0);

    if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
        ctx->active->vol->total_dirs) {
      int last_idx = ctx->active->vol->total_dirs - 1;
      int next_begin;
      int next_cursor;
      if (last_idx < 0)
        last_idx = 0;
      if (last_idx >= height) {
        next_begin = last_idx - (height - 1);
        next_cursor = height - 1;
      } else {
        next_begin = 0;
        next_cursor = last_idx;
      }
      (void)AppStateCommitPanelTreeViewport(ctx->active, next_begin,
                                            next_cursor);
    }

    *dir_entry_ptr = ctx->active->vol
                         ->dir_entry_list[ctx->active->disp_begin_pos +
                                          ctx->active->cursor_pos]
                         .dir_entry;

    DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                ctx->active->disp_begin_pos,
                ctx->active->disp_begin_pos + ctx->active->cursor_pos, TRUE);
    DisplayFileWindow(ctx, ctx->active, *dir_entry_ptr);
    DisplayDiskStatistic(ctx, s);
    UpdateStatsPanel(ctx, *dir_entry_ptr, s);
    DisplayAvailBytes(ctx, s);
    {
      char path[PATH_LENGTH];
      GetPath(*dir_entry_ptr, path);
      DisplayHeaderPath(ctx, path);
    }
    RefreshWindow(ctx->ctx_dir_window);
    RefreshWindow(ctx->ctx_file_window);
    doupdate();

    if (ch == ESC)
      break;
  }
}

static void DrawDirListJumpPrompt(ViewContext *ctx, WINDOW *win,
                                  const char *search_buf) {
  int y = 0;

  if (!ctx || !win || !search_buf)
    return;

  if (win == stdscr) {
    if (ctx->layout.prompt_y > 0) {
      wmove(win, ctx->layout.prompt_y - 1, 0);
      wclrtoeol(win);
    }
    wmove(win, ctx->layout.prompt_y, 0);
    wclrtoeol(win);
    wmove(win, ctx->layout.status_y, 0);
    wclrtoeol(win);
    y = ctx->layout.prompt_y;
  } else {
    werase(win);
    y = 1; /* Center line in 3-line footer window */
  }

  mvwaddstr(win, y, 1, "Jump to: ");
  mvwaddstr(win, y, 10, search_buf);
  wclrtoeol(win);
  wnoutrefresh(win);
  doupdate();
}
