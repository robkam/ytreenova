/***************************************************************************
 *
 * src/fs/tree_read.c
 * Directory tree scanning and reading
 *
 ***************************************************************************/

#include "ytnova_appstate_focus.h"
#include "ytnova_appstate_panel.h"
#include "ytnova_appstate_volume.h"
#include "ytnova_appstate_visibility.h"
#include "ytnova_fs.h"
#include <stdarg.h>

static void UnReadSubTree(ViewContext *ctx, DirEntry *dir_entry, Statistic *s);
static BOOL IsTransientScanStatError(int errnum);
static void RemoveFileWithBoundary(ViewContext *ctx, FileEntry *fe_ptr,
                                   Statistic *s);
static BOOL EscapeKeyPressedWithBoundary(const ViewContext *ctx);
static int InputChoiceWithBoundary(ViewContext *ctx, const char *msg,
                                   const char *choices);
static void ClearPromptLineWithBoundary(ViewContext *ctx);
static void MessageWithBoundary(ViewContext *ctx, const char *fmt, ...);
static void DisplayDiskStatisticWithBoundary(ViewContext *ctx,
                                             const Statistic *s);
static void RecalculateSysStatsWithBoundary(ViewContext *ctx, Statistic *s);
static void RefreshUiWithBoundary(const ViewContext *ctx);

static void RemoveFileWithBoundary(ViewContext *ctx, FileEntry *fe_ptr,
                                   Statistic *s) {
  if (!ctx || !ctx->hook_remove_file)
    return;
  (void)ctx->hook_remove_file(ctx, fe_ptr, s);
}

static BOOL EscapeKeyPressedWithBoundary(const ViewContext *ctx) {
  if (ctx && ctx->hook_escape_key_pressed)
    return ctx->hook_escape_key_pressed();
  return FALSE;
}

static int InputChoiceWithBoundary(ViewContext *ctx, const char *msg,
                                   const char *choices) {
  if (ctx && ctx->hook_input_choice)
    return ctx->hook_input_choice(ctx, msg, choices);
  return ESC;
}

static void ClearPromptLineWithBoundary(ViewContext *ctx) {
  if (ctx && ctx->hook_clear_prompt_line)
    ctx->hook_clear_prompt_line(ctx);
}

static void MessageWithBoundary(ViewContext *ctx, const char *fmt, ...) {
  va_list ap;
  char msg[MESSAGE_LENGTH];

  if (!ctx || !ctx->hook_ui_message || !fmt)
    return;

  va_start(ap, fmt);
  vsnprintf(msg, sizeof(msg), fmt, ap);
  va_end(ap);
  (void)ctx->hook_ui_message(ctx, "%s", msg);
}

static void DisplayDiskStatisticWithBoundary(ViewContext *ctx,
                                             const Statistic *s) {
  if (ctx && ctx->hook_display_disk_statistic)
    ctx->hook_display_disk_statistic(ctx, s);
}

static void RecalculateSysStatsWithBoundary(ViewContext *ctx, Statistic *s) {
  if (ctx && ctx->hook_recalculate_sys_stats)
    ctx->hook_recalculate_sys_stats(ctx, s);
}

static void RefreshUiWithBoundary(const ViewContext *ctx) {
  if (ctx && ctx->hook_refresh_ui)
    (void)ctx->hook_refresh_ui();
}

/* Read file tree: path = "root" path
 * dir_entry is filled by the function
 */

int ReadTree(ViewContext *ctx, DirEntry *dir_entry, char *path, int depth,
             Statistic *s, ScanProgressCallback cb, void *cb_data) {
  DIR *dir;
  struct stat stat_struct;
  const struct dirent *dirent;
  char new_path[PATH_LENGTH + 1];
  DirEntry first_dir_entry;
  DirEntry *des_ptr;
  DirEntry *den_ptr;
  FileEntry first_file_entry;
  FileEntry *fes_ptr;
  FileEntry *fen_ptr;
  int file_count;

  /* Safety: If this node already has children/files (e.g. from ScanSubTree),
   * free them before overwriting to prevent memory leaks.
   */
  if (dir_entry->file) {
    FileEntry *f, *n;
    for (f = dir_entry->file; f; f = n) {
      n = f->next;
      RemoveFileWithBoundary(ctx, f, s); /* Updates stats and frees memory */
    }
    if (!AppStateCommitDirEntryFileList(dir_entry, NULL))
      return (-1);
  }
  if (dir_entry->sub_tree) {
    /* Use UnReadSubTree to recursively free children and decrement stats
     * correctly */
    UnReadSubTree(ctx, dir_entry->sub_tree, s);
    if (!AppStateCommitDirEntrySubTree(dir_entry, NULL))
      return (-1);
  }

  /* Initialize dir_entry */
  /*--------------------------*/

  if (!AppStateCommitDirEntrySubTree(dir_entry, NULL))
    return (-1);
  if (!AppStateResetDirEntryPayloadCache(dir_entry))
    return (-1);
  if (!AppStateCommitDirEntryFileViewport(dir_entry, 0, 0) ||
      !AppStateCommitDirEntryGlobalFilter(dir_entry, FALSE, FALSE) ||
      !AppStateCommitDirEntryTaggedFilter(dir_entry, FALSE) ||
      !AppStateCommitDirEntryFileShape(dir_entry, FALSE))
    return (-1);
  if (!AppStateCommitDirEntryLoggedState(dir_entry, FALSE, FALSE))
    return (-1);

  if (S_ISBLK(dir_entry->stat_struct.st_mode))
    return (0); /* Block-Device */

  if (depth < 0) {
    if (dir_entry->up_tree) {
      /* Keep both parent and this node expandable for progressive scans. */
      if (!AppStateCommitDirEntryLoggedState(dir_entry, TRUE,
                                             dir_entry->unlogged_flag) ||
          !AppStateCommitDirEntryLoggedState(dir_entry->up_tree, TRUE,
                                             dir_entry->up_tree->unlogged_flag))
        return (-1);
      return (1);
    }
  }

  s->disk_total_directories++;

  /* Report progress via callback */
  if (cb)
    cb(ctx, cb_data);

  /* Silently skip unreadable or missing directories */
  if ((dir = opendir(path)) == NULL) {
    if (!AppStateCommitDirEntryAccessDenied(dir_entry, TRUE))
      return (-1);
    return 0;
  }

  first_dir_entry.prev = NULL;
  first_dir_entry.next = NULL;
  /* DirEntry uses a flexible array member for name[]; stack sentinels have no
   * storage for it. Keep this node as a pure link sentinel.
   */
  first_file_entry.next = NULL;
  fes_ptr = &first_file_entry;

  file_count = 0;
  while ((dirent = readdir(dir)) != NULL) {
    size_t entry_name_len;

    if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, ".."))
      continue;

    /* Scan every entry so later views can apply their own visibility rules. */

    if (EscapeKeyPressedWithBoundary(ctx)) {
      /* Ask whether to abort scan */
      int choice = InputChoiceWithBoundary(
          ctx, "Abort scan (Y/N)?", "YyNn\033"); /* \033 is ESC */
      if (choice == 'Y' || choice == ESC) {
        closedir(dir);
        /* CRITICAL - Attach Partial Results */
        if (first_file_entry.next)
          first_file_entry.next->prev = NULL;
        if (first_dir_entry.next)
          first_dir_entry.next->prev = NULL;
        if (!AppStateCommitDirEntryFileList(dir_entry, first_file_entry.next))
          return -1;
        if (!AppStateCommitDirEntrySubTree(dir_entry, first_dir_entry.next))
          return -1;
        return -1;
      } else {
        ClearPromptLineWithBoundary(ctx);
      }
    }

    /* Update statistics / animation every 20 files to be smoother */
    if ((file_count++ % 20) == 0) {
      /* Replaced explicit UI calls with callback */
      if (cb)
        cb(ctx, cb_data);
    }

    entry_name_len = strlen(dirent->d_name);
    if (!strcmp(path, FILE_SEPARATOR_STRING)) {
      if (snprintf(new_path, sizeof(new_path), "%s%s", path, dirent->d_name) >=
          (int)sizeof(new_path)) {
        MessageWithBoundary(ctx, "Path too long*%s%s*IGNORED", path,
                            dirent->d_name);
        continue;
      }
    } else {
      if (snprintf(new_path, sizeof(new_path), "%s%s%s", path,
                   FILE_SEPARATOR_STRING, dirent->d_name) >=
          (int)sizeof(new_path)) {
        MessageWithBoundary(ctx, "Path too long*%s%s%s*IGNORED", path,
                            FILE_SEPARATOR_STRING, dirent->d_name);
        continue;
      }
    }

    if (STAT_(new_path, &stat_struct)) {
      if (IsTransientScanStatError(errno))
        continue;
      MessageWithBoundary(ctx, "Stat failed on*%s*IGNORED", new_path);
      continue;
    }

    if (S_ISDIR(stat_struct.st_mode)) {
      /* Directory Entry */
      /*-----------------*/

      /* FIX: Added +1 to allocation for null terminator */
      den_ptr = (DirEntry *)xcalloc(1, sizeof(DirEntry) + entry_name_len + 1);

      den_ptr->up_tree = dir_entry;

      (void)memcpy(den_ptr->name, dirent->d_name, entry_name_len + 1);

      (void)memcpy(&den_ptr->stat_struct, &stat_struct, sizeof(stat_struct));
      den_ptr->prev = den_ptr->next = NULL;

      /* Recursive call with abort check, passing callback data */
      if (ReadTree(ctx, den_ptr, new_path, depth - 1, s, cb, cb_data) == -1) {
        /* Fix: Free the partially built subtree before returning */
        DeleteTree(den_ptr); /* Free the allocated DirEntry and its children */
        closedir(dir);
        /* Attach Partial Results */
        if (first_file_entry.next)
          first_file_entry.next->prev = NULL;
        if (first_dir_entry.next)
          first_dir_entry.next->prev = NULL;
        if (!AppStateCommitDirEntryFileList(dir_entry, first_file_entry.next))
          return -1;
        if (!AppStateCommitDirEntrySubTree(dir_entry, first_dir_entry.next))
          return -1;
        return -1;
      }

      /* Sort by direct insertion */
      /*------------------------------------*/

      if (first_dir_entry.next == NULL ||
          strcmp(first_dir_entry.next->name, den_ptr->name) > 0) {
        den_ptr->next = first_dir_entry.next;
        den_ptr->prev = &first_dir_entry;
        if (first_dir_entry.next)
          first_dir_entry.next->prev = den_ptr;
        first_dir_entry.next = den_ptr;
      } else {
        for (des_ptr = first_dir_entry.next; des_ptr; des_ptr = des_ptr->next) {
          if (des_ptr->next == NULL ||
              strcmp(des_ptr->next->name, den_ptr->name) > 0) {
            den_ptr->next = des_ptr->next;
            den_ptr->prev = des_ptr;
            if (des_ptr->next)
              des_ptr->next->prev = den_ptr;
            des_ptr->next = den_ptr;
            break;
          }
        }
      }
    } else {
      /* File Entry */
      /*------------*/

      char link_path[PATH_LENGTH + 1];

      /* Check if entry is symbolic link */
      /*----------------------------------------*/

      *link_path = '\0';

      if (S_ISLNK(stat_struct.st_mode)) {
        size_t link_len;
        ssize_t n;
        /* Yes, append symbolic name to "real" name */
        /*---------------------------------------------------------*/

        if ((n = readlink(new_path, link_path, sizeof(link_path) - 1)) == -1) {
          static const char unknown_link[] = "unknown";
          (void)memcpy(link_path, unknown_link, sizeof(unknown_link));
          n = (ssize_t)(sizeof(unknown_link) - 1);
        }
        link_path[n] = '\0';
        link_len = (size_t)n;

        /* FIX: Added +1 to allocation for name's null terminator (n already
         * includes it for link) */
        fen_ptr = (FileEntry *)xcalloc(
            1, sizeof(FileEntry) + entry_name_len + 1 + link_len + 1);

        (void)memcpy(fen_ptr->name, dirent->d_name, entry_name_len + 1);
        (void)memcpy(&fen_ptr->name[entry_name_len + 1], link_path,
                     link_len + 1);
      } else {
        /* FIX: Added +1 to allocation for null terminator */
        fen_ptr = (FileEntry *)xcalloc(1, sizeof(FileEntry) + entry_name_len +
                                              1);

        (void)memcpy(fen_ptr->name, dirent->d_name, entry_name_len + 1);
      }

      fen_ptr->next = NULL;
      fen_ptr->prev = NULL;
      fen_ptr->tagged = FALSE;
      fen_ptr->matching = TRUE;

      (void)memcpy(&fen_ptr->stat_struct, &stat_struct, sizeof(stat_struct));

      fen_ptr->dir_entry = dir_entry;
      fes_ptr->next = fen_ptr;
      fen_ptr->prev = fes_ptr;
      fes_ptr = fen_ptr;
      if (!AppStateCommitDirEntryTotalPayload(
              dir_entry, dir_entry->total_files + 1,
              dir_entry->total_bytes + stat_struct.st_size)) {
        (void)closedir(dir);
        return -1;
      }
      s->disk_total_files++;
      s->disk_total_bytes += stat_struct.st_size;
    }
  }

  (void)closedir(dir);

  if (first_file_entry.next)
    first_file_entry.next->prev = NULL;
  if (first_dir_entry.next)
    first_dir_entry.next->prev = NULL;

  if (!AppStateCommitDirEntryFileList(dir_entry, first_file_entry.next))
    return -1;
  if (!AppStateCommitDirEntrySubTree(dir_entry, first_dir_entry.next))
    return -1;

  /* Final UI update via callback */
  if (cb)
    cb(ctx, cb_data);

  return (0);
}

static BOOL IsTransientScanStatError(int errnum) {
  if (errnum == EACCES || errnum == ENOENT || errnum == ENOTDIR)
    return TRUE;
#ifdef ESTALE
  if (errnum == ESTALE)
    return TRUE;
#endif
  return FALSE;
}

void UnReadTree(ViewContext *ctx, DirEntry *dir_entry, Statistic *s) {
  FileEntry *fe_ptr, *next_fe_ptr;

  if (dir_entry == NULL || s == NULL)
    return;

  if (dir_entry == s->tree) {
    MessageWithBoundary(ctx, "Can't delete ROOT");
  } else {
    if (dir_entry->unlogged_flag)
      return;

    for (fe_ptr = dir_entry->file; fe_ptr; fe_ptr = next_fe_ptr) {
      next_fe_ptr = fe_ptr->next;
      RemoveFileWithBoundary(ctx, fe_ptr, s);
    }
    if (dir_entry->sub_tree) {
      UnReadSubTree(ctx, dir_entry->sub_tree, s);
      if (!AppStateCommitDirEntrySubTree(dir_entry, NULL))
        return;
    }
    if (!AppStateCommitDirEntryLoggedState(dir_entry, TRUE, TRUE))
      return;
    if (s->disk_total_directories > 0)
      s->disk_total_directories--;
    (void)GetAvailBytes(&s->disk_space, s);
    DisplayDiskStatisticWithBoundary(ctx, s);
    RefreshUiWithBoundary(ctx);
  }
}

static void UnReadSubTree(ViewContext *ctx, DirEntry *dir_entry, Statistic *s) {
  DirEntry *de_ptr, *next_de_ptr;
  FileEntry *fe_ptr, *next_fe_ptr;

  for (de_ptr = dir_entry; de_ptr; de_ptr = next_de_ptr) {
    next_de_ptr = de_ptr->next;

    for (fe_ptr = de_ptr->file; fe_ptr; fe_ptr = next_fe_ptr) {
      next_fe_ptr = fe_ptr->next;
      RemoveFileWithBoundary(ctx, fe_ptr, s);
    }

    if (de_ptr->sub_tree) {
      UnReadSubTree(ctx, de_ptr->sub_tree, s);
    }

    if (!de_ptr->up_tree->not_scanned)
      if (s->disk_total_directories > 0)
        s->disk_total_directories--;

    if (de_ptr->prev)
      de_ptr->prev->next = de_ptr->next;
    else
      de_ptr->up_tree->sub_tree = de_ptr->next;
    if (de_ptr->next)
      de_ptr->next->prev = de_ptr->prev;

    free(de_ptr);
  }
}

int RescanDir(ViewContext *ctx, DirEntry *dir_entry, int depth, Statistic *s,
              ScanProgressCallback cb, void *cb_data) {
  char path[PATH_LENGTH + 1];
  FileEntry *fe_ptr, *next_fe_ptr;

  /* Renamed usage: s->mode -> s->log_mode */
  if (!dir_entry ||
      (s->log_mode != DISK_MODE && s->log_mode != USER_MODE)) {
    return -1;
  }

  GetPath(dir_entry, path);

  /* Unlink and free all child directories recursively, updating stats. */
  if (dir_entry->sub_tree) {
    UnReadSubTree(ctx, dir_entry->sub_tree, s);
    if (!AppStateCommitDirEntrySubTree(dir_entry, NULL))
      return -1;
  }

  /* Unlink and free all files, updating stats. */
  for (fe_ptr = dir_entry->file; fe_ptr != NULL; fe_ptr = next_fe_ptr) {
    next_fe_ptr = fe_ptr->next;
    /* RemoveFile updates stats and frees the entry */
    RemoveFileWithBoundary(ctx, fe_ptr, s);
  }
  if (!AppStateCommitDirEntryFileList(dir_entry, NULL))
    return -1;

  /*
   * The dir_entry itself still exists and is counted. ReadTree will re-init
   * its contents and re-add its children's stats. It also increments
   * disk_total_directories for itself, so we must pre-decrement to avoid
   * double-counting.
   */
  s->disk_total_directories--;

  /* Call ReadTree with the passed callback context */
  ReadTree(ctx, dir_entry, path, depth, s, cb, cb_data);

  /* Since we just reloaded the tree, dot files are present in memory.
     We must now recalculate stats based on the current visibility setting. */
  RecalculateSysStatsWithBoundary(ctx, s);

  /* Global matching stats are now incorrect. Reset and recalculate. */
  s->disk_matching_files = 0L;
  s->disk_matching_bytes = 0L;
  FsApplyFilter(s->tree, s);

  return 0;
}
