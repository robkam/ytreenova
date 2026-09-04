/***************************************************************************
 *
 * src/ui/archive_transfer.c
 * Archive directory transfer support
 *
 ***************************************************************************/

#include "ytnova_ui.h"
#include "ytnova_cmd.h"
#include "ytnova_fs.h"
#include "ytnova_appstate_panel.h"
#include "ytnova_appstate_volume.h"
#include "ytnova_panel_anchor.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int ArchiveDirectoryTransferProgress(int status, const char *message,
                                     void *user_data) {
  ViewContext *ctx = (ViewContext *)user_data;

  if (status == ARCHIVE_STATUS_PROGRESS && ctx) {
    if (ctx->progress.active) {
      ctx->progress.items_done++;
      if (!Progress_ShouldRender(ctx))
        return ARCHIVE_CB_CONTINUE;
      Progress_Render(ctx);
    }
    if (ctx->hook_draw_spinner)
      ctx->hook_draw_spinner(ctx);
  }
  if (status == ARCHIVE_STATUS_ERROR && ctx && message)
    UI_ShowStatusLineError(ctx, "%s", message);
  return ARCHIVE_CB_CONTINUE;
}

static void ArchiveDirectoryTransferStart(ViewContext *ctx,
                                          ArchiveDirectoryTransferMode mode,
                                          const char *src_path,
                                          const char *dest_path) {
  if (!ctx)
    return;

  Progress_Start(ctx,
                 mode == ARCHIVE_DIRECTORY_MOVE ? "ARCHIVE MOVE"
                                                : "ARCHIVE COPY",
                 src_path, dest_path, 0, 0);
  Progress_Render(ctx);
  if (ctx->hook_draw_spinner)
    ctx->hook_draw_spinner(ctx);
}

static void ArchiveDirectoryTransferFinish(ViewContext *ctx) {
  if (!ctx)
    return;
  Progress_Finish(ctx);
}

static void ArchiveDirectoryReloadProgress(ViewContext *ctx, void *user_data) {
  (void)user_data;
  (void)ArchiveDirectoryTransferProgress(ARCHIVE_STATUS_PROGRESS, NULL, ctx);
}

static void ArchiveDirectoryRebindPanel(
    ViewContext *ctx, YtreeNovaPanel *panel, struct Volume *vol,
    const PanelViewportSnapshot *snapshot, BOOL was_bound) {
  DirEntry *anchor;

  if (!was_bound || !panel || panel->vol != vol)
    return;
  if (!RestorePanelViewportSnapshot(vol, panel, snapshot,
                                    snapshot->top_dir_path))
    PositionPanelAtIndex(panel, 0);
  anchor = GetPanelDirEntry(panel);
  if (!anchor)
    anchor = vol->vol_stats.tree;
  if (!AppStateCommitPanelFileViewport(panel, 0, 0))
    return;
  if (!AppStateCommitPanelFileAnchor(panel, anchor))
    return;
  BuildFileEntryList(ctx, panel);
}

static int ArchiveDirectoryReloadVolume(ViewContext *ctx, struct Volume *vol) {
  PanelViewportSnapshot left_snapshot;
  PanelViewportSnapshot right_snapshot;
  BOOL left_bound;
  BOOL right_bound;
  char archive_path[PATH_LENGTH + 1];
  char file_spec[FILE_SPEC_LENGTH + 1];
  int kind_of_sort;
  int index = 0;

  if (!ctx || !vol || vol->vol_stats.log_mode != ARCHIVE_MODE)
    return -1;

  left_bound = ctx->left && ctx->left->vol == vol;
  right_bound = ctx->right && ctx->right->vol == vol;
  CapturePanelViewportSnapshot(ctx->left, vol, &left_snapshot);
  CapturePanelViewportSnapshot(ctx->right, vol, &right_snapshot);
  (void)snprintf(archive_path, sizeof(archive_path), "%s",
                 vol->vol_stats.log_path);
  (void)snprintf(file_spec, sizeof(file_spec), "%s",
                 vol->vol_stats.file_spec);
  kind_of_sort = vol->vol_stats.kind_of_sort;

  if (!Volume_Load(ctx, archive_path, vol, ArchiveDirectoryReloadProgress, ctx))
    return -1;
  (void)snprintf(vol->vol_stats.file_spec, sizeof(vol->vol_stats.file_spec),
                 "%s", file_spec);
  SetKindOfSort(kind_of_sort, &vol->vol_stats);
  (void)SetFilter(vol->vol_stats.file_spec, &vol->vol_stats);
  if (!AppStateCommitVolumeGeneration(vol))
    return -1;

  BuildDirEntryList(ctx, vol, &index);
  ArchiveDirectoryRebindPanel(ctx, ctx->left, vol, &left_snapshot, left_bound);
  ArchiveDirectoryRebindPanel(ctx, ctx->right, vol, &right_snapshot,
                              right_bound);
  return 0;
}

static void RemoveTemporaryContents(int dir_fd) {
  DIR *dir;
  struct dirent *item;

  if (dir_fd < 0 || !(dir = fdopendir(dup(dir_fd))))
    return;
  while ((item = readdir(dir)) != NULL) {
    struct stat st;

    if (strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0 ||
        fstatat(dir_fd, item->d_name, &st, AT_SYMLINK_NOFOLLOW) != 0)
      continue;
    if (S_ISDIR(st.st_mode)) {
      int child_fd = openat(dir_fd, item->d_name,
                            O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
      if (child_fd >= 0) {
        RemoveTemporaryContents(child_fd);
        close(child_fd);
      }
      (void)unlinkat(dir_fd, item->d_name, AT_REMOVEDIR);
    } else {
      (void)unlinkat(dir_fd, item->d_name, 0);
    }
  }
  closedir(dir);
}

void ArchiveDirectoryTransferRemoveTemporary(const char *path) {
  int dir_fd;

  if (!path)
    return;
  dir_fd = open(path, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
  if (dir_fd < 0)
    return;
  RemoveTemporaryContents(dir_fd);
  close(dir_fd);
  (void)rmdir(path);
}

static int RemoveFilesystemDirectoryTreeAt(int parent_fd, const char *name) {
  DIR *dir;
  struct dirent *item;
  int dir_fd;

  dir_fd = openat(parent_fd, name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
  if (dir_fd < 0 || !(dir = fdopendir(dir_fd))) {
    if (dir_fd >= 0)
      close(dir_fd);
    return -1;
  }
  while ((item = readdir(dir)) != NULL) {
    if (strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0)
      continue;
    if (RemoveFilesystemDirectoryTreeAt(dir_fd, item->d_name) != 0 &&
        unlinkat(dir_fd, item->d_name, 0) != 0) {
      closedir(dir);
      return -1;
    }
  }
  closedir(dir);
  return unlinkat(parent_fd, name, AT_REMOVEDIR);
}

static int RemoveFilesystemDirectoryTree(const char *path) {
  char parent_path[PATH_LENGTH + 1];
  const char *name;
  const char *separator;
  int parent_fd;
  int result;

  if (!path || !(separator = strrchr(path, FILE_SEPARATOR_CHAR)) ||
      separator[1] == '\0')
    return -1;
  name = separator + 1;
  if (separator == path) {
    parent_path[0] = FILE_SEPARATOR_CHAR;
    parent_path[1] = '\0';
  } else {
    size_t parent_length = (size_t)(separator - path);

    if (parent_length >= sizeof(parent_path))
      return -1;
    memcpy(parent_path, path, parent_length);
    parent_path[parent_length] = '\0';
  }
  parent_fd = open(parent_path, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
  if (parent_fd < 0)
    return -1;
  result = RemoveFilesystemDirectoryTreeAt(parent_fd, name);
  close(parent_fd);
  return result;
}

int FilesystemDirectoryTransferToArchive(
    ViewContext *ctx, ArchiveDirectoryTransferMode mode, const char *src_path,
    const char *dest_dir_path, const char *dest_path) {
#ifdef HAVE_LIBARCHIVE
  struct Volume *target_vol;
  char internal_dest[PATH_LENGTH + 1];
  const char *root;
  const char *relative;
  const char *error_message = NULL;

  if (!ctx || !src_path || !dest_dir_path || !dest_path)
    return -1;
  target_vol = Volume_GetByPath(ctx, dest_dir_path);
  if (!target_vol || target_vol->vol_stats.log_mode != ARCHIVE_MODE)
    return -1;
  if (!(target_vol->vol_stats.archive_capabilities & ARCHIVE_CAP_ADD)) {
    UI_ShowStatusLineError(ctx,
                           "Archive destination does not support directory transfer");
    return -1;
  }
  root = target_vol->vol_stats.log_path;
  if (strncmp(dest_path, root, strlen(root)) != 0 ||
      (dest_path[strlen(root)] != '\0' &&
       dest_path[strlen(root)] != FILE_SEPARATOR_CHAR)) {
    UI_ShowStatusLineError(ctx, "Invalid archive destination path");
    return -1;
  }
  relative = dest_path + strlen(root);
  while (*relative == FILE_SEPARATOR_CHAR)
    relative++;
  if (Archive_ValidateInternalPath(relative, internal_dest,
                                   sizeof(internal_dest)) != 0) {
    UI_ShowStatusLineError(ctx, "Directory archive transfer failed");
    return -1;
  }

  ArchiveDirectoryTransferStart(ctx, mode, src_path, dest_path);
  if (Archive_AddTree(target_vol->vol_stats.log_path, src_path, internal_dest,
                      ArchiveDirectoryTransferProgress, ctx) != 0) {
    error_message = "Directory archive transfer failed";
  } else if (ArchiveDirectoryReloadVolume(ctx, target_vol) != 0) {
    error_message = "Directory archive refresh failed";
  } else if (mode == ARCHIVE_DIRECTORY_MOVE &&
             RemoveFilesystemDirectoryTree(src_path) != 0) {
    error_message = "Directory move source removal failed";
  }
  ArchiveDirectoryTransferFinish(ctx);

  if (error_message) {
    UI_ShowStatusLineError(ctx, "%s", error_message);
    return -1;
  }
  return 0;
#else
  (void)ctx;
  (void)mode;
  (void)src_path;
  (void)dest_dir_path;
  (void)dest_path;
  return -1;
#endif
}

void ArchiveDirectoryTransfer(ViewContext *ctx, DirEntry **dir_entry_ptr,
                              ArchiveDirectoryTransferMode mode,
                              const char *src_path,
                              const char *dest_dir_path,
                              const char *dest_path) {
#ifdef HAVE_LIBARCHIVE
  struct Volume *source_vol;
  Statistic *source_stats;
  struct Volume *target_vol;
  DirEntry *dir_entry;
  struct stat st;
  BOOL source_mutated = FALSE;
  BOOL source_reloaded = FALSE;
  BOOL transfer_failed = FALSE;

  if (!ctx || !dir_entry_ptr || !*dir_entry_ptr || !src_path ||
      !dest_dir_path || !dest_path)
    return;
  dir_entry = *dir_entry_ptr;
  source_vol = ctx->active->vol;
  source_stats = &source_vol->vol_stats;
  target_vol = Volume_GetByPath(ctx, dest_dir_path);

  if (!(source_stats->archive_capabilities & ARCHIVE_CAP_COPY_OUT) ||
      (mode == ARCHIVE_DIRECTORY_MOVE &&
       !(source_stats->archive_capabilities & ARCHIVE_CAP_MOVE))) {
    UI_ShowStatusLineError(ctx, "This archive does not support directory transfer");
    return;
  }

  if (target_vol && target_vol->vol_stats.log_mode == ARCHIVE_MODE) {
    char internal_dest[PATH_LENGTH + 1];
    const char *root = target_vol->vol_stats.log_path;
    const char *relative = dest_path;
    char temporary[PATH_LENGTH + 1] = "/tmp/ytnova_dir_XXXXXX";
    BOOL destination_mutated = FALSE;

    if (!(target_vol->vol_stats.archive_capabilities & ARCHIVE_CAP_ADD) ||
        (mode == ARCHIVE_DIRECTORY_MOVE &&
         !(source_stats->archive_capabilities & ARCHIVE_CAP_DELETE)) ||
        strncmp(dest_path, root, strlen(root)) != 0 ||
        (dest_path[strlen(root)] != '\0' &&
         dest_path[strlen(root)] != FILE_SEPARATOR_CHAR)) {
      UI_ShowStatusLineError(ctx,
                             "Archive destination does not support directory transfer");
      return;
    }
    relative += strlen(root);
    while (*relative == FILE_SEPARATOR_CHAR)
      relative++;
    if (Archive_ValidateInternalPath(relative, internal_dest,
                                     sizeof(internal_dest)) != 0) {
      UI_ShowStatusLineError(ctx, "Directory archive transfer failed");
      return;
    }

    ArchiveDirectoryTransferStart(ctx, mode, src_path, dest_path);
    if (!mkdtemp(temporary) ||
        ExtractArchiveTree(source_stats->log_path, src_path, temporary,
                           ArchiveDirectoryTransferProgress, ctx) != 0 ||
        Archive_AddTree(target_vol->vol_stats.log_path, temporary, internal_dest,
                        ArchiveDirectoryTransferProgress, ctx) != 0) {
      transfer_failed = TRUE;
    } else {
      destination_mutated = TRUE;
      if (mode == ARCHIVE_DIRECTORY_MOVE) {
        if (Archive_DeleteTree(source_stats->log_path, src_path,
                               ArchiveDirectoryTransferProgress, ctx) != 0)
          transfer_failed = TRUE;
        else
          source_mutated = TRUE;
      }
    }
    ArchiveDirectoryTransferRemoveTemporary(temporary);

    if (destination_mutated) {
      if (ArchiveDirectoryReloadVolume(ctx, target_vol) != 0)
        transfer_failed = TRUE;
      else if (target_vol == source_vol)
        source_reloaded = TRUE;
    }
    if (source_mutated && target_vol != source_vol) {
      if (ArchiveDirectoryReloadVolume(ctx, source_vol) != 0)
        transfer_failed = TRUE;
      else
        source_reloaded = TRUE;
    }
  } else {
    errno = 0;
    if (lstat(dest_path, &st) == 0 || errno != ENOENT) {
      UI_ShowStatusLineError(ctx, "Directory archive transfer failed");
      return;
    }

    ArchiveDirectoryTransferStart(ctx, mode, src_path, dest_path);
    if (ExtractArchiveTree(source_stats->log_path, src_path, dest_path,
                           ArchiveDirectoryTransferProgress, ctx) != 0) {
      transfer_failed = TRUE;
    } else if (mode == ARCHIVE_DIRECTORY_MOVE) {
      if (Archive_DeleteTree(source_stats->log_path, src_path,
                             ArchiveDirectoryTransferProgress, ctx) != 0)
        transfer_failed = TRUE;
      else
        source_mutated = TRUE;
    }
    if (source_mutated) {
      if (ArchiveDirectoryReloadVolume(ctx, source_vol) != 0)
        transfer_failed = TRUE;
      else
        source_reloaded = TRUE;
    }
  }

  ArchiveDirectoryTransferFinish(ctx);
  if (transfer_failed)
    UI_ShowStatusLineError(ctx, "Directory archive transfer failed");

  if (source_reloaded) {
    dir_entry = GetPanelDirEntry(ctx->active);
    if (!dir_entry)
      dir_entry = source_vol->vol_stats.tree;
  }
  *dir_entry_ptr = dir_entry;
  RefreshView(ctx, dir_entry);
#else
  (void)ctx;
  (void)dir_entry_ptr;
  (void)mode;
  (void)src_path;
  (void)dest_dir_path;
  (void)dest_path;
#endif
}
