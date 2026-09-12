/***************************************************************************
 *
 * src/core/volume.c
 * Volume Management Functions
 *
 ***************************************************************************/

#include "ytnova_appstate_focus.h"
#include "ytnova_appstate_panel.h"
#include "ytnova_appstate_volume.h"
#include "ytnova_appstate_volume_registry.h"
#include "ytnova_defs.h"

extern void FreePathList(PathList *list);

static void Volume_ClearPanelFileEntries(YtreeNovaPanel *panel) {
  if (panel && panel->file_entry_list) {
    free(panel->file_entry_list);
    panel->file_entry_list = NULL;
    panel->file_entry_list_capacity = 0;
    panel->file_count = 0;
  }
}

static void Volume_ClearPanelFileAnchor(YtreeNovaPanel *panel) {
  if (!panel)
    return;
  if (!AppStateCommitPanelFileViewport(panel, 0, 0))
    return;
  if (!AppStateCommitPanelFileAnchor(panel, NULL))
    return;
}

static void Volume_ClearPanelTags(YtreeNovaPanel *panel) {
  if (!panel)
    return;
  FreePathList(panel->tagged_paths);
  panel->tagged_paths = NULL;
}

static void Volume_FreeCache(struct Volume *vol) {
  if (!vol)
    return;
  (void)AppStateReleaseVolumeDirEntryList(vol);
}

static BOOL Volume_HasUserActions(const ViewContext *ctx) {
  if (!ctx || !ctx->hook_has_user_action)
    return FALSE;
  return ctx->hook_has_user_action(ctx);
}

static int Volume_ReadTreeDepth(const ViewContext *ctx) {
  const char *depth_str = "0";
  if (ctx && ctx->hook_get_profile_value)
    depth_str = ctx->hook_get_profile_value(ctx, "TREEDEPTH");
  return (int)strtol(depth_str, NULL, 0);
}

static void Volume_DeleteTree(ViewContext *ctx, DirEntry *tree) {
  if (!ctx || !tree)
    return;
  if (ctx->core_storage_ops.delete_tree != NULL)
    ctx->core_storage_ops.delete_tree(tree);
}

static int Volume_GetDiskParameter(ViewContext *ctx, char *path,
                                   char *volume_name, long long *avail_bytes,
                                   long long *capacity, Statistic *s) {
  if (!ctx || ctx->core_storage_ops.get_disk_parameter == NULL)
    return -1;
  return ctx->core_storage_ops.get_disk_parameter(path, volume_name, avail_bytes,
                                                  capacity, s);
}

static int Volume_ReadTreePort(ViewContext *ctx, DirEntry *dir_entry, char *path,
                               int depth, Statistic *s,
                               ScanProgressCallback cb, void *cb_data) {
  if (!ctx || ctx->core_storage_ops.read_tree == NULL)
    return -1;
  return ctx->core_storage_ops.read_tree(ctx, dir_entry, path, depth, s, cb,
                                         cb_data);
}

static int Volume_ReadTreeFromArchivePort(ViewContext *ctx,
                                          DirEntry **dir_entry_ptr,
                                          const char *filename, Statistic *s,
                                          ScanProgressCallback cb,
                                          void *cb_data) {
  if (!ctx || ctx->core_storage_ops.read_tree_from_archive == NULL)
    return -1;
  return ctx->core_storage_ops.read_tree_from_archive(
      ctx, dir_entry_ptr, filename, s, cb, cb_data);
}

/*
 * Volume_Create
 *
 * Allocates and initializes a new Volume structure, assigns a unique ID,
 * and adds it to the global GlobalView->volumes_head hash table.
 *
 * Returns:
 *   A pointer to the newly created Volume on success.
 *   Exits with an error message on memory allocation failure.
 */
struct Volume *Volume_Create(ViewContext *ctx) {
  struct Volume *new_vol = NULL;

  /* Allocate memory for a new Volume and initialize it to zero */
  new_vol = (struct Volume *)xcalloc(1, sizeof(struct Volume));

  /* Assign a unique ID and increment the serial counter */
  new_vol->id = ctx->volume_serial++;
  new_vol->volume_generation = 0;
  if (!AppStateCommitVolumeFocusMirror(new_vol, FOCUS_TREE)) {
    free(new_vol);
    return NULL;
  }

  if (!AppStateRegisterVolume(ctx, new_vol)) {
    free(new_vol);
    return NULL;
  }

  return new_vol;
}

/*
 * Volume_Delete
 *
 * Safely destroys a Volume: removes it from the global hash table,
 * frees its associated directory tree, and then frees the Volume structure
 * itself.
 *
 * Parameters:
 *   vol - A pointer to the Volume structure to be deleted.
 */
void Volume_Delete(ViewContext *ctx, struct Volume *vol) {
  if (vol == NULL) {
    return;
  }
  if (!AppStateUnregisterVolume(ctx, vol))
    return;

  if (vol->dir_entry_list && !AppStateReleaseVolumeDirEntryList(vol))
    return;
  /* Invalidate any panels using this volume to prevent stale references */
  if (ctx->left && ctx->left->vol == vol) {
    Volume_ClearPanelFileEntries(ctx->left);
    Volume_ClearPanelFileAnchor(ctx->left);
    Volume_ClearPanelTags(ctx->left);
    if (!AppStateCommitPanelVolume(ctx->left, NULL))
      return;
  }
  if (ctx->right && ctx->right->vol == vol) {
    Volume_ClearPanelFileEntries(ctx->right);
    Volume_ClearPanelFileAnchor(ctx->right);
    Volume_ClearPanelTags(ctx->right);
    if (!AppStateCommitPanelVolume(ctx->right, NULL))
      return;
  }

  /* Free the associated directory tree if it exists */
  if (vol->vol_stats.tree != NULL) {
    Volume_DeleteTree(ctx, vol->vol_stats.tree);
    vol->vol_stats.tree = NULL; /* Good practice to nullify after freeing */
  }

  /* Free the Volume structure itself */
  free(vol);
}

/*
 * Volume_FreeAll
 *
 * Iterates through all loaded volumes in GlobalView->volumes_head, deletes each
 * one, and then clears the global GlobalView->volumes_head and CurrentVolume
 * pointers. This is intended for a safe shutdown to prevent memory leaks.
 */
void Volume_FreeAll(ViewContext *ctx) {
  struct Volume *s, *tmp;

  /* Safely iterate and delete everything */
  HASH_ITER(hh, ctx->volumes_head, s, tmp) { Volume_Delete(ctx, s); }
  if (ctx->active && ctx->active->vol &&
      !AppStateCommitPanelVolume(ctx->active, NULL))
    return;
  if (ctx->left && ctx->left->vol &&
      !AppStateCommitPanelVolume(ctx->left, NULL))
    return;
  if (ctx->right && ctx->right->vol &&
      !AppStateCommitPanelVolume(ctx->right, NULL))
    return;
  (void)AppStateClearVolumeRegistry(ctx);
}

/*
 * Volume_GetByPath
 *
 * Finds the volume that contains the given path.
 * Returns the volume with the longest matching log_path prefix.
 * This handles cases where volumes are nested or distinct.
 */
struct Volume *Volume_GetByPath(ViewContext *ctx, const char *path) {
  (void)ctx;
  struct Volume *s, *tmp;
  struct Volume *best_match = NULL;
  size_t best_len = 0;
  size_t len;

  if (!path)
    return NULL;

  HASH_ITER(hh, ctx->volumes_head, s, tmp) {
    /* Check if this volume is valid (has a path) */
    if (s->vol_stats.log_path[0] == '\0')
      continue;

    /* Check if 'path' starts with this volume's login path */
    if (strncmp(path, s->vol_stats.log_path,
                strlen(s->vol_stats.log_path)) == 0) {
      len = strlen(s->vol_stats.log_path);

      /* Ensure it's a true path prefix match (e.g. "/usr" matches "/usr/bin"
       * but not "/usrlocal") */
      /* Logic: prefix must be full path (equal) OR followed by separator in
       * 'path' */
      /* Exception: Root "/" matches everything */

      if (path[len] == '\0' || path[len] == FILE_SEPARATOR_CHAR ||
          len == 1) { /* len=1 handles root "/" case */
        if (len > best_len) {
          best_len = len;
          best_match = s;
        }
      }
    }
  }
  return best_match;
}

/*
 * Volume_Load
 *
 * Core logic to load a volume from a path.
 * Separates I/O and validation from UI interaction.
 *
 * Parameters:
 *   path      - The file system path to load.
 *   reuse_vol - Pointer to an existing volume to reuse (e.g., the empty virgin
 * volume). If NULL, a new volume is created. cb        - Callback function for
 * scanning progress.
 *
 * Returns:
 *   Pointer to the successfully loaded Volume, or NULL on failure.
 *   In case of failure, an error message is written to the global 'message'
 * buffer.
 */
struct Volume *Volume_Load(ViewContext *ctx, const char *path,
                           struct Volume *reuse_vol, ScanProgressCallback cb,
                           void *cb_user_data) {
  char resolved_path[PATH_LENGTH + 1];
  struct stat stat_struct;
  struct Volume *vol = NULL;
  Statistic *s;
  int depth;
  int res;

  /* 1. Path Resolution */
  if (realpath(path, resolved_path) == NULL) {
    strncpy(resolved_path, path, PATH_LENGTH);
    resolved_path[PATH_LENGTH] = '\0';
  }

  /* 2. Validation */
  if (STAT_(resolved_path, &stat_struct)) {
    MESSAGE(ctx, "Can't access*\"%s\"*%s", resolved_path, strerror(errno));
    return NULL;
  }

  /* 3. Archive Check */
  if (!S_ISDIR(stat_struct.st_mode)) {
#ifdef HAVE_LIBARCHIVE
    struct archive *a_test;
    int r_test;

    a_test = archive_read_new();
    if (a_test == NULL) {
      MESSAGE(ctx, "archive_read_new() failed");
      return NULL;
    }
    archive_read_support_filter_all(a_test);
    archive_read_support_format_all(a_test);
    r_test = archive_read_open_filename(a_test, resolved_path, 10240);
    archive_read_free(a_test);

    if (r_test != ARCHIVE_OK) {
      MESSAGE(ctx,
              "Not a recognized archive file*or format not supported*\"%s\"",
              resolved_path);
      return NULL;
    }
#else
    MESSAGE(ctx,
            "Cannot open file as archive*ytnova not compiled with*libarchive "
            "support");
    return NULL;
#endif
  }

  /* 4. Allocation */
  if (reuse_vol) {
    vol = reuse_vol;
    if (vol->vol_stats.tree) {
      if (ctx->left && ctx->left->vol == vol) {
        Volume_ClearPanelFileEntries(ctx->left);
        Volume_ClearPanelFileAnchor(ctx->left);
      }
      if (ctx->right && ctx->right->vol == vol) {
        Volume_ClearPanelFileEntries(ctx->right);
        Volume_ClearPanelFileAnchor(ctx->right);
      }
      Volume_DeleteTree(ctx, vol->vol_stats.tree);
      vol->vol_stats.tree = NULL;
    }
    memset(&vol->vol_stats, 0, sizeof(Statistic));
    Volume_FreeCache(vol);
    if (ctx->left && ctx->left->vol == vol)
      Volume_ClearPanelTags(ctx->left);
    if (ctx->right && ctx->right->vol == vol)
      Volume_ClearPanelTags(ctx->right);
  } else {
    vol = Volume_Create(ctx);
    if (!vol)
      return NULL; /* Volume_Create exits on failure, but safety check */
  }

  s = &vol->vol_stats;

  /* 5. Setup */
  s->kind_of_sort = SORT_BY_NAME + SORT_ASC;
  strncpy(s->file_spec, DEFAULT_FILE_SPEC, FILE_SPEC_LENGTH);
  s->file_spec[FILE_SPEC_LENGTH] = '\0';

  if (!s->tree) {
    s->tree = (DirEntry *)xcalloc(1, sizeof(DirEntry) + PATH_LENGTH);
  }

  strncpy(s->log_path, resolved_path, PATH_LENGTH);
  s->log_path[PATH_LENGTH] = '\0';
  strncpy(s->path, resolved_path, PATH_LENGTH);
  s->path[PATH_LENGTH] = '\0';

  memcpy(&s->tree->stat_struct, &stat_struct, sizeof(stat_struct));

  /* 6. Mode Detection */
  if (!S_ISDIR(stat_struct.st_mode)) {
    /* "root" node is always a directory structure for YtreeNova, even for archives
     */
    memset(&s->tree->stat_struct, 0, sizeof(struct stat));
    s->tree->stat_struct.st_mode = S_IFDIR;
    s->disk_total_directories = 1;
    s->log_mode = ARCHIVE_MODE;
  } else if (Volume_HasUserActions(ctx)) {
    s->log_mode = USER_MODE;
  } else {
    s->log_mode = DISK_MODE;
  }

  if (Volume_GetDiskParameter(ctx, resolved_path, s->disk_name, &s->disk_space,
                              &s->disk_capacity, s) != 0) {
    s->disk_name[0] = '\0';
    s->disk_space = 0;
    s->disk_capacity = 0;
  }
  strncpy(s->tree->name, resolved_path, PATH_LENGTH - 1);
  s->tree->name[PATH_LENGTH - 1] = '\0';

  /* 7. Scanning */
  if (s->log_mode == ARCHIVE_MODE) {
#ifdef HAVE_LIBARCHIVE
    if (Volume_ReadTreeFromArchivePort(ctx, &s->tree, s->log_path, s, cb,
                                       cb_user_data)) {
      /* Error message is set by ReadTreeFromArchive or we can set a generic one
       */
      /* Note: ReadTreeFromArchive typically prints to 'message' on error */
      if (!reuse_vol)
        Volume_Delete(ctx, vol);
      else {
        Volume_DeleteTree(ctx, s->tree);
        s->tree = NULL;
        memset(s, 0, sizeof(Statistic));
        s->kind_of_sort = SORT_BY_NAME + SORT_ASC;
        strncpy(s->file_spec, DEFAULT_FILE_SPEC, FILE_SPEC_LENGTH);
        s->file_spec[FILE_SPEC_LENGTH] = '\0';
      }
      return NULL;
    }
#endif
  } else {
    s->tree->next = s->tree->prev = NULL;
    depth = Volume_ReadTreeDepth(ctx);
    res = Volume_ReadTreePort(ctx, s->tree, resolved_path, depth, s, cb,
                              cb_user_data);
    if (res != 0) {
      if (res != -1)
        MESSAGE(ctx, "ReadTree Failed");
      if (!reuse_vol)
        Volume_Delete(ctx, vol);
      else {
        Volume_DeleteTree(ctx, s->tree);
        s->tree = NULL;
        memset(s, 0, sizeof(Statistic));
        s->kind_of_sort = SORT_BY_NAME + SORT_ASC;
        strncpy(s->file_spec, DEFAULT_FILE_SPEC, FILE_SPEC_LENGTH);
        s->file_spec[FILE_SPEC_LENGTH] = '\0';
      }
      return NULL;
    }
    /* Copy stats to disk_stats for later reference */
    memcpy(&vol->vol_disk_stats, s, sizeof(Statistic));
  }

  return vol;
}
void SetKindOfSort(int kind_of_sort, Statistic *s) {
  if (s) {
    s->kind_of_sort = kind_of_sort;
  }
}
