/***************************************************************************
 *
 * src/cmd/log.c
 * Read file tree (UI Controller)
 *
 ***************************************************************************/

#include "ytnova_cmd.h"
#include "ytnova_appstate_focus.h"
#include "ytnova_appstate_mode.h"
#include "ytnova_appstate_panel.h"
#include "ytnova_appstate_session.h"
#include "ytnova_fs.h"
#include "ytnova_panel_anchor.h"
#include <assert.h>

/* Runtime UI helpers used by volume-switch restore flow. */
extern void FreeFileEntryList(YtreeNovaPanel *panel);
extern void BuildFileEntryList(ViewContext *ctx, YtreeNovaPanel *panel);
extern int FileNav_GetMaxDispFiles(const ViewContext *ctx);

static void ResetPanelFileContext(YtreeNovaPanel *panel) {
  if (!panel)
    return;

  FreeFileEntryList(panel);
  if (!AppStateCommitPanelFileViewport(panel, 0, 0))
    return;
  if (!AppStateCommitPanelFileAnchor(panel, NULL))
    return;
  (void)AppStateCommitPanelFileShape(panel, FALSE);
}

static void SavePanelFileSelection(YtreeNovaPanel *panel) {
  PanelVolumeFileState *state;
  char saved_file_dir_path[PATH_LENGTH + 1];
  BOOL saved_big_file_view = FALSE;

  if (!panel || !panel->vol)
    return;
  assert(panel->saved_focus != FOCUS_FILE ||
         panel->file_selection_dir_path[0] != '\0');

  state = GetPanelVolumeFileState(panel, panel->vol->id);
  saved_file_dir_path[0] = '\0';
  if (panel->file_selection_dir_path[0] != '\0') {
    (void)snprintf(saved_file_dir_path, sizeof(saved_file_dir_path), "%s",
                   panel->file_selection_dir_path);
    saved_file_dir_path[PATH_LENGTH] = '\0';
  } else if (panel->file_dir_entry != NULL) {
    GetPath(panel->file_dir_entry, saved_file_dir_path);
    saved_file_dir_path[PATH_LENGTH] = '\0';
  }
  if (panel->file_dir_entry != NULL)
    saved_big_file_view = panel->file_dir_entry->big_window;
  if (!AppStateCommitPanelVolumeFileSnapshot(
          state, panel->start_file, panel->file_cursor_pos,
          panel->panel_generation, panel->vol->volume_generation,
          panel->saved_focus, saved_big_file_view, saved_file_dir_path,
          panel->file_selection_dir_path, panel->file_selection_name))
    return;
  if (!AppStateCommitPanelFileShape(panel, saved_big_file_view))
    return;
}

static void PositionSavedFileSelection(ViewContext *ctx, YtreeNovaPanel *panel,
                                       DirEntry *dir_entry,
                                       const char *file_name) {
  int i;
  int selected_idx = -1;
  int max_disp_files;
  int start;

  if (!ctx || !panel || !dir_entry || !file_name || file_name[0] == '\0')
    return;

  if (!AppStateCommitPanelFileAnchor(panel, dir_entry))
    return;
  BuildFileEntryList(ctx, panel);
  for (i = 0; i < (int)panel->file_count; i++) {
    const FileEntry *file = panel->file_entry_list[i].file;
    if (file && strcmp(file->name, file_name) == 0) {
      selected_idx = i;
      break;
    }
  }
  if (selected_idx < 0)
    return;

  max_disp_files = FileNav_GetMaxDispFiles(ctx);
  if (max_disp_files < 1)
    max_disp_files = 1;

  start = panel->start_file;
  if (start < 0)
    start = 0;
  if (selected_idx < start)
    start = selected_idx;
  else if (selected_idx >= start + max_disp_files)
    start = selected_idx - max_disp_files + 1;
  if (start < 0)
    start = 0;

  if (!AppStateCommitPanelFileViewport(panel, start, selected_idx - start))
    return;
  if (!AppStateCommitDirEntryFileViewport(dir_entry, start,
                                          selected_idx - start))
    return;
}

static void RestorePanelFileSelection(ViewContext *ctx, YtreeNovaPanel *panel) {
  const struct Volume *vol;
  const PanelVolumeFileState *state;
  DirEntry *resolved_file_dir = NULL;
  const char *file_dir_path = NULL;
  unsigned int restore_generation = 0;

  if (!panel || !panel->vol)
    return;

  restore_generation = panel->panel_generation;
  ResetPanelFileContext(panel);
  if (!AppStateRestorePanelGeneration(panel, restore_generation))
    return;
  vol = panel->vol;
  state = FindPanelVolumeFileState(panel, vol->id);
  if (!AppStateCommitPanelFileViewport(panel, 0, 0))
    return;
  if (!AppStateCommitPanelFileSelection(panel, NULL, NULL))
    return;
  if (!AppStateRestorePanelGeneration(panel, restore_generation))
    return;
  if (!AppStateCommitPanelFileAnchor(panel, NULL))
    return;
  if (!AppStateCommitPanelFileShape(panel, FALSE))
    return;
  if (!AppStateRestorePanelGeneration(panel, restore_generation))
    return;
  if (!state)
    return;

  {
    int start_file = state->saved_file_start;
    int file_cursor_pos = state->saved_file_cursor;
    if (start_file < 0)
      start_file = 0;
    if (file_cursor_pos < 0)
      file_cursor_pos = 0;
    if (state->saved_panel_generation != panel->panel_generation ||
        state->saved_volume_generation != vol->volume_generation) {
      start_file = 0;
      file_cursor_pos = 0;
    }
    if (!AppStateCommitPanelFileViewport(panel, start_file, file_cursor_pos))
      return;
  }
  if (!AppStateCommitPanelFileSelection(
          panel, state->saved_file_selection_dir_path,
          state->saved_file_selection_name))
    return;
  if (!AppStateRestorePanelGeneration(panel, restore_generation))
    return;

  assert(state->saved_focus != FOCUS_FILE ||
         state->saved_file_selection_dir_path[0] != '\0');
  if (state->saved_file_selection_dir_path[0] != '\0')
    file_dir_path = state->saved_file_selection_dir_path;

  if (file_dir_path) {
    resolved_file_dir = ResolvePanelAnchorTarget(panel, vol, file_dir_path);
    if (!AppStateCommitPanelFileAnchor(panel, resolved_file_dir))
      return;
    if (resolved_file_dir) {
      if (!AppStateCommitDirEntryFileShape(resolved_file_dir,
                                           state->saved_big_file_view))
        return;
      PositionSavedFileSelection(ctx, panel, resolved_file_dir,
                                 state->saved_file_selection_name);
    }
  }
  if (!AppStateRestorePanelGeneration(panel, restore_generation))
    return;
  if (!AppStateCommitPanelFocus(ctx, panel, state->saved_focus))
    return;
  (void)AppStateCommitPanelFileShape(panel, state->saved_big_file_view);
}

static void SavePanelTreeSelection(YtreeNovaPanel *panel) {
  SavePanelTreeViewportSnapshot(panel);
}

static void RestorePanelTreeSelection(ViewContext *ctx, YtreeNovaPanel *panel) {
  (void)RestorePanelTreeViewportSnapshot(ctx, panel);
}

/* Helper function to handle scan progress updates */
static void Log_Progress(ViewContext *ctx, const void *data) {
  const Statistic *s = (const Statistic *)data;

  if (ctx->hook_draw_spinner)
    ctx->hook_draw_spinner(ctx);
  if (ctx->hook_clock_handler)
    ctx->hook_clock_handler(ctx, 0);

  if (ctx->animation_method == 1) {
    /* If animating, redraw the animation step. */
    if (ctx->hook_draw_animation_step)
      ctx->hook_draw_animation_step(ctx, ctx->ctx_file_window);
    if (ctx->hook_refresh_ui)
      ctx->hook_refresh_ui();
  } else {
    /* Update statistics panel periodically */
    if (s && ctx->hook_display_disk_statistic)
      ctx->hook_display_disk_statistic(ctx, s);
    if (ctx->hook_refresh_ui)
      ctx->hook_refresh_ui();
  }
}

/*
 * LogDisk
 * UI Controller for loading or switching to a disk volume.
 *
 * Returns:
 * -1 on error
 * 0  on successfully reading a new tree or switching to an existing one
 */
int LogDisk(ViewContext *ctx, YtreeNovaPanel *panel, char *path) {
  char saved_filter[FILE_SPEC_LENGTH + 1];
  char resolved_path[PATH_LENGTH + 1];
  struct Volume *s_vol, *tmp;
  struct Volume *found_vol = NULL;
  struct Volume *loaded_vol = NULL;
  struct Volume *old_vol = NULL;
  struct Volume *reuse_vol = NULL;
  BOOL reload_requested = FALSE;
  Statistic *s;
  struct stat st_check;

  DEBUG_LOG("ENTER LogDisk: path=%s", path);

  if (ctx->hook_suspend_clock)
    ctx->hook_suspend_clock(ctx); /* Suspend clock during critical operations */

  if (panel->vol != NULL) {
    if (!AppStateCommitVolumeFocusMirror(panel->vol, panel->saved_focus))
      return -1;
  }

  /* Keep per-volume tree selection before switching away. */
  SavePanelTreeSelection(panel);
  SavePanelFileSelection(panel);

  /* 1. Resolve Path (for UI searching/display purposes) */
  if (realpath(path, resolved_path) == NULL) {
    strncpy(resolved_path, path, PATH_LENGTH);
    resolved_path[PATH_LENGTH] = '\0';
  }

  /* Save current filter to preserve it across transitions. */
  if (panel->vol != NULL && strlen(panel->vol->vol_stats.file_spec) > 0) {
    strncpy(saved_filter, panel->vol->vol_stats.file_spec, FILE_SPEC_LENGTH);
    saved_filter[FILE_SPEC_LENGTH] = '\0';
  } else {
    strncpy(saved_filter, DEFAULT_FILE_SPEC, FILE_SPEC_LENGTH);
    saved_filter[FILE_SPEC_LENGTH] = '\0';
  }

  /* 2. Search Existing Volume */
  HASH_ITER(hh, ctx->volumes_head, s_vol, tmp) {
    if (strcmp(s_vol->vol_stats.log_path, resolved_path) == 0) {
      found_vol = s_vol;
      break;
    }
  }

  if (found_vol != NULL) {
    /* Case A: Volume Found - Check Access and Switch */
    BOOL access_ok = FALSE;

    if (found_vol->vol_stats.log_mode == ARCHIVE_MODE) {
      /* For archives, check if the file still exists */
      if (stat(found_vol->vol_stats.log_path, &st_check) == 0 &&
          !S_ISDIR(st_check.st_mode)) {
        access_ok = TRUE;
        /* Optional: Attempt to chdir to the parent directory for context */
        char parent_dir[PATH_LENGTH + 1];
        strncpy(parent_dir, found_vol->vol_stats.log_path, PATH_LENGTH);
        parent_dir[PATH_LENGTH] = '\0';
        char *slash = strrchr(parent_dir, FILE_SEPARATOR_CHAR);
        if (slash) {
          *slash = '\0';
          if (chdir(parent_dir)) {
            ; /* Explicitly ignore result */
          }
        }
      }
    } else {
      /* For normal disks, try to enter the directory */
      if (chdir(found_vol->vol_stats.log_path) == 0) {
        access_ok = TRUE;
      }
    }

    if (access_ok) {
      if (found_vol == panel->vol &&
          strcmp(found_vol->vol_stats.log_path, resolved_path) == 0) {
        reload_requested = TRUE;
      } else {
        const PanelVolumeFileState *state;

        ResetPanelFileContext(panel);
        if (!AppStateCommitPanelVolume(panel, found_vol))
          return -1;
        s = &panel->vol->vol_stats;
        if (!AppStateCommitPanelFocus(ctx, panel,
                                      (ViewFocus)panel->vol->saved_focus))
          return -1;
        state = FindPanelVolumeFileState(panel, panel->vol->id);
        if (state && !AppStateRestorePanelGeneration(
                         panel, state->saved_tree_panel_generation))
          return -1;
        if (!AppStateCommitGlobalSearchTerm(ctx, NULL))
          return -1;
        if (!AppStateCommitViewMode(ctx, panel->vol->vol_stats.log_mode))
          return -1;

        /* Re-apply the volume's own filter */
        (void)SetFilter(s->file_spec, s);
        if (ctx->hook_recalculate_sys_stats)
          ctx->hook_recalculate_sys_stats(ctx, s);

        /* Refresh display */
        if (ctx->hook_display_menu)
          ctx->hook_display_menu(ctx);
        if (ctx->hook_build_dir_entry_list)
          ctx->hook_build_dir_entry_list(ctx, panel->vol, &(int){0});
        RestorePanelFileSelection(ctx, panel);
        RestorePanelTreeSelection(ctx, panel);

        if (ctx->hook_display_tree)
          ctx->hook_display_tree(
              ctx, panel->vol, ctx->ctx_dir_window, panel->disp_begin_pos,
              panel->disp_begin_pos + panel->cursor_pos, TRUE);
        if (ctx->hook_display_disk_statistic)
          ctx->hook_display_disk_statistic(ctx, s);
        if (ctx->hook_display_avail_bytes)
          ctx->hook_display_avail_bytes(ctx, s);
        if (ctx->hook_init_clock)
          ctx->hook_init_clock(ctx);
        return 0;
      }
    } else {
      /* Volume exists but is inaccessible */
      if (ctx->hook_ui_message)
        ctx->hook_ui_message(ctx, "Volume \"%s\" not accessible (Error: %s). Removed.",
                             found_vol->vol_stats.log_path, strerror(errno));

      if (found_vol == panel->vol) {
        /* If the current volume is bad, we must delete it. */
        Volume_Delete(ctx, found_vol);
        if (!AppStateCommitPanelVolume(panel, NULL))
          return -1;
      } else {
        Volume_Delete(ctx, found_vol);
      }
      /* Proceed to try loading it fresh */
      found_vol = NULL;
    }
  }

  /* Case B: Load Volume (New or Reuse) */
  old_vol = panel->vol;

  /* Determine if we can reuse the "virgin" volume */
  if (old_vol != NULL &&
      (old_vol->vol_stats.log_path[0] == '\0' || reload_requested)) {
    reuse_vol = old_vol;
  }

  /* Prepare UI for loading */
  if (ctx->hook_display_menu)
    ctx->hook_display_menu(ctx);
  if (panel->vol && ctx->hook_display_disk_statistic)
    ctx->hook_display_disk_statistic(
        ctx, &panel->vol->vol_stats); /* Maintain frame if possible */

  if (ctx->animation_method == 1) {
    if (ctx->hook_switch_to_big_file_window)
      ctx->hook_switch_to_big_file_window(ctx);
    if (ctx->hook_init_animation)
      ctx->hook_init_animation(ctx);
  } else {
    if (ctx->hook_refresh_window) {
      ctx->hook_refresh_window(stdscr);
      ctx->hook_refresh_window(ctx->ctx_dir_window);
    }
  }
  if (ctx->hook_refresh_ui)
    ctx->hook_refresh_ui();

  if (ctx->animation_method == 0 && ctx->hook_ui_notice)
    ctx->hook_ui_notice(ctx, "Scanning...");

  /* Call Logic Core */
  loaded_vol = Volume_Load(ctx, resolved_path, reuse_vol,
                           (ScanProgressCallback)Log_Progress, NULL);

  DEBUG_LOG("LogDisk: Volume_Load returned %p", (void *)loaded_vol);

  if (ctx->animation_method == 1 && ctx->hook_stop_animation)
    ctx->hook_stop_animation(ctx);
  if (ctx->hook_switch_to_small_file_window)
    ctx->hook_switch_to_small_file_window(ctx);

  /* Handle Result */
  if (loaded_vol == NULL) {
    /* Failure */
    /* Note: Volume_Load displays its own error message now. */

    /* Restore state */
    if (reuse_vol) {
      /* We reused the virgin volume and it failed. It's now empty/reset. */
      /* panel->vol is still pointing to it (old_vol == reuse_vol). */
      /* Display empty state. */
      if (ctx->hook_display_menu)
        ctx->hook_display_menu(ctx);
      if (ctx->hook_build_dir_entry_list)
        ctx->hook_build_dir_entry_list(ctx, panel->vol, &(int){0});
      if (ctx->hook_display_tree)
        ctx->hook_display_tree(ctx, panel->vol, ctx->ctx_dir_window, 0, 0, TRUE);
      if (ctx->hook_display_disk_statistic)
        ctx->hook_display_disk_statistic(ctx, &panel->vol->vol_stats);
      if (ctx->hook_display_avail_bytes)
        ctx->hook_display_avail_bytes(ctx, &panel->vol->vol_stats);
    } else if (old_vol != NULL) {
      /* We tried to create a NEW volume and failed. */
      /* panel->vol is still old_vol (valid). Restore its display. */
      if (panel->vol != old_vol && !AppStateCommitPanelVolume(panel, old_vol))
        return -1;
      if (!AppStateCommitViewMode(ctx, panel->vol->vol_stats.log_mode))
        return -1;
      s = &panel->vol->vol_stats;

      if (ctx->hook_display_menu)
        ctx->hook_display_menu(ctx);
      if (ctx->hook_build_dir_entry_list)
        ctx->hook_build_dir_entry_list(ctx, panel->vol, &(int){0});
      RestorePanelFileSelection(ctx, panel);
      RestorePanelTreeSelection(ctx, panel);

      if (ctx->hook_display_tree)
        ctx->hook_display_tree(ctx, panel->vol, ctx->ctx_dir_window,
                               panel->disp_begin_pos,
                               panel->disp_begin_pos + panel->cursor_pos, TRUE);
      if (ctx->hook_display_disk_statistic)
        ctx->hook_display_disk_statistic(ctx, s);
      if (ctx->hook_display_avail_bytes)
        ctx->hook_display_avail_bytes(ctx, s);
    } else {
      /* Critical: No old volume, failed to load new one (e.g., initial startup
       * fail) */
      /* Main will handle exit if panel->vol is invalid */
    }
    if (ctx->hook_init_clock)
      ctx->hook_init_clock(ctx);
    return -1;
  }

  /* Success */
  ResetPanelFileContext(panel);
  if (panel->vol != loaded_vol && !AppStateCommitPanelVolume(panel, loaded_vol))
    return -1;
  s = &panel->vol->vol_stats;
  if (!AppStateCommitPanelFocus(ctx, panel,
                                (ViewFocus)panel->vol->saved_focus))
    return -1;
  if (!AppStateCommitGlobalSearchTerm(ctx, NULL))
    return -1;
  if (!AppStateCommitViewMode(ctx, s->log_mode))
    return -1;

  /* If this is a new volume (not the startup one), apply saved filter from
   * previous context */
  /* If it's the very first volume (old_vol was NULL or we reused virgin),
   * default filter is already set by Volume_Load */
  if (old_vol != NULL && old_vol->vol_stats.log_path[0] != '\0') {
    strncpy(s->file_spec, saved_filter, FILE_SPEC_LENGTH);
    s->file_spec[FILE_SPEC_LENGTH] = '\0';
  }

  (void)SetFilter(s->file_spec, s);
  if (ctx->hook_recalculate_sys_stats)
    ctx->hook_recalculate_sys_stats(ctx, s);

  /* Final Refresh */
  if (ctx->hook_build_dir_entry_list)
    ctx->hook_build_dir_entry_list(ctx, panel->vol, &(int){0});
  RestorePanelFileSelection(ctx, panel);
  if (reload_requested) {
    if (!AppStateCommitPanelTreeViewport(panel, 0, 0))
      return -1;
    ResetPanelTreeViewportSnapshot(panel);
  } else {
    RestorePanelTreeSelection(ctx, panel);
  }
  if (ctx->hook_display_tree)
    ctx->hook_display_tree(ctx, panel->vol, ctx->ctx_dir_window,
                           panel->disp_begin_pos,
                           panel->disp_begin_pos + panel->cursor_pos, TRUE);
  if (ctx->hook_display_disk_statistic)
    ctx->hook_display_disk_statistic(ctx, s);
  if (ctx->hook_display_avail_bytes)
    ctx->hook_display_avail_bytes(ctx, s);

  if (ctx->hook_init_clock)
    ctx->hook_init_clock(ctx);
  return 0;
}

int GetNewLogPath(ViewContext *ctx, YtreeNovaPanel *panel, char *path) {
  int result;
  int copied_len;
  char user_input[PATH_LENGTH * 2 + 1] = "";
  char current_dir_path[PATH_LENGTH + 1];

  result = -1;

  if (ctx->hook_clear_help)
    ctx->hook_clear_help(ctx);

  if (ctx->hook_mv_add_str)
    ctx->hook_mv_add_str(ctx->layout.prompt_y, 1, "LOG:");

  /* Save the current directory context and set it as default for user input */
  copied_len = snprintf(current_dir_path, sizeof(current_dir_path), "%s", path);
  if (copied_len < 0 || (size_t)copied_len >= sizeof(current_dir_path)) {
    return result;
  }

  copied_len = snprintf(user_input, sizeof(user_input), "%s", path);
  if (copied_len < 0 || (size_t)copied_len >= sizeof(user_input)) {
    return result;
  }

  if (ctx->view_mode == LL_FILE_MODE && *path == '<') {
    char *cptr;
    for (cptr = user_input; (*cptr = *(cptr + 1)); cptr++)
      ;
    if (user_input[strlen(user_input) - 1] == '>')
      user_input[strlen(user_input) - 1] = '\0';
  }

  if (ctx->hook_read_string &&
      (ctx->hook_read_string)(ctx, panel, "Log Path:", user_input,
                              PATH_LENGTH - 1, HST_LOG) == CR) {
    char temp_path[PATH_LENGTH * 3 + 2];
    char resolved_path[PATH_LENGTH + 1];

    /* InputString expands '~', so check if the result is an absolute path. */
    if (user_input[0] != FILE_SEPARATOR_CHAR) {
      /* It's a relative path. Construct the full path to be resolved. */
      if (Path_Join(temp_path, sizeof(temp_path), current_dir_path, user_input) !=
          0) {
        return result;
      }
    } else {
      /* It's an absolute path. */
      copied_len = snprintf(temp_path, sizeof(temp_path), "%s", user_input);
      if (copied_len < 0 || (size_t)copied_len >= sizeof(temp_path)) {
        return result;
      }
    }

    if (realpath(temp_path, resolved_path) != NULL) {
      copied_len = snprintf(path, PATH_LENGTH + 1, "%s", resolved_path);
      if (copied_len < 0 || copied_len > PATH_LENGTH) {
        return result;
      }
      result = 0;
    } else {
      NormPath(temp_path, path);
      if (path[0] != '\0') {
        result = 0;
      }
    }
  }

  return (result);
}

/*
 * CycleLoadedVolume
 * Cycles through the list of currently loaded volumes.
 * direction: -1 for previous, 1 for next.
 * Returns 0 on successful switch, -1 if no switch occurred or on error.
 */
int CycleLoadedVolume(ViewContext *ctx, YtreeNovaPanel *panel, int direction) {
  struct Volume *s, *tmp;
  struct Volume **vol_array = NULL;
  int num_volumes = 0;

  int retries = 0;
  int max_retries;
  BOOL changes_made = FALSE;

  num_volumes = HASH_COUNT(ctx->volumes_head);
  max_retries = num_volumes + 1;

  while (retries++ < max_retries) {
    num_volumes = HASH_COUNT(ctx->volumes_head);

    if (num_volumes <= 1) {
      if (ctx->hook_ui_message)
        ctx->hook_ui_message(ctx, "Only one volume loaded.*No cycling possible.");
      return (changes_made ? 0 : -1);
    }

    vol_array = (struct Volume **)malloc(num_volumes * sizeof(struct Volume *));
    if (vol_array == NULL) {
      if (ctx->hook_ui_message)
        ctx->hook_ui_message(ctx,
                             "Failed to allocate memory for volume list during cycle.");
      return -1;
    }

    int current_index = -1;
    int i = 0;
    HASH_ITER(hh, ctx->volumes_head, s, tmp) {
      vol_array[i] = s;
      if (s == panel->vol) {
        current_index = i;
      }
      i++;
    }

    if (current_index == -1) {
      if (ctx->hook_ui_message)
        ctx->hook_ui_message(ctx, "Current volume not found in list during cycle.");
      free(vol_array);
      return -1;
    }

    int target_index = (current_index + direction + num_volumes) % num_volumes;

    if (target_index == current_index && retries > 1) {
      free(vol_array);
      if (ctx->hook_ui_message)
        ctx->hook_ui_message(ctx, "No other accessible volumes found.");
      return (changes_made ? 0 : -1);
    }

    const struct Volume *target = vol_array[target_index];
    char target_path[PATH_LENGTH + 1];
    strncpy(target_path, target->vol_stats.log_path, PATH_LENGTH);
    target_path[PATH_LENGTH] = '\0';

    free(vol_array);

    /* Use LogDisk to attempt the switch.
     * LogDisk handles validation, cleanup if inaccessible, and UI updates. */
    if (LogDisk(ctx, panel, target_path) == 0) {
      if (ctx->hook_recreate_windows)
        ctx->hook_recreate_windows(ctx);
      if (ctx->hook_clock_handler)
        ctx->hook_clock_handler(ctx, 0);
      return 0;
    } else {
      /* If LogDisk failed, the target volume was likely removed.
         The loop will retry. */
      changes_made = TRUE;
    }
  }

  if (ctx->hook_ui_message)
    ctx->hook_ui_message(
        ctx, "Failed to switch to an accessible volume after multiple attempts.");
  return (changes_made ? 0 : -1);
}
