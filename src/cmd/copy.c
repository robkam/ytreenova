/***************************************************************************
 *
 * src/cmd/copy.c
 * Copy files and directories
 *
 ***************************************************************************/

#include "ytnova_cmd.h"
#include "ytnova_appstate_volume.h"
#include "ytnova_fs.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FILE_SEPARATOR_CHAR '/'
#define FILE_SEPARATOR_STRING "/"
#define ESCAPE goto FNC_XIT

#define DISK_MODE 0
#define ARCHIVE_MODE 2
#define USER_MODE 3

static int CopyArchiveFile(ViewContext *ctx, char *to_path,
                           const char *from_path,
                           const Statistic *s);
typedef struct CopyTargetContext {
  DirEntry *dest_dir_entry;
  struct Volume *target_vol;
  DirEntry *target_tree;
  Statistic *target_stats;
} CopyTargetContext;
typedef struct CopyOperation {
  ViewContext *ctx;
  Statistic *source_stats;
  FileEntry *source_file;
  CopyTargetContext target;
  const char *to_file;
  int *dir_create_mode;
  int *overwrite_mode;
  ConflictCallback conflict_cb;
  ChoiceCallback choice_cb;
  BOOL path_copy;
  char from_path[PATH_LENGTH + 1];
  char from_dir[PATH_LENGTH + 1];
  char to_path[PATH_LENGTH + 1];
  char abs_path[PATH_LENGTH + 1];
  char conflict_src_path[PATH_LENGTH + 1];
  BOOL conflict_src_extracted;
} CopyOperation;

static int CopyAssignPath(char *dest, size_t dest_size, const char *src) {
  int written = snprintf(dest, dest_size, "%s", src);

  if (written < 0 || (size_t)written >= dest_size) {
    return -1;
  }

  return 0;
}

static void CopyResolveTargetContext(CopyOperation *op, const char *path) {
  op->target.target_vol = Volume_GetByPath(op->ctx, path);
  if (op->target.target_vol) {
    op->target.target_tree = op->target.target_vol->vol_stats.tree;
    op->target.target_stats = &op->target.target_vol->vol_stats;
    return;
  }

  if (op->source_stats->tree &&
      strncmp(op->source_stats->tree->name, path,
              strlen(op->source_stats->tree->name)) ==
          0) {
    op->target.target_tree = op->source_stats->tree;
    op->target.target_stats = op->source_stats;
  } else {
    op->target.target_tree = NULL;
    op->target.target_stats = NULL;
  }
}

static int CopyPrepareArchiveSourceDestination(CopyOperation *op,
                                               const char *to_dir_path) {
  if (op->path_copy) {
    char root_path[PATH_LENGTH + 1];
    char full_dest_path[PATH_LENGTH + 1];
    char *rel_path;

    GetPath(op->source_stats->tree, root_path);
    if (strncmp(op->from_dir, root_path, strlen(root_path)) == 0) {
      rel_path = op->from_dir + strlen(root_path);
      if (*rel_path == FILE_SEPARATOR_CHAR)
        rel_path++;
    } else {
      rel_path = op->from_dir;
    }

    if (Path_Join(full_dest_path, sizeof(full_dest_path), to_dir_path, rel_path) !=
        0) {
      return -1;
    }

    CopyResolveTargetContext(op, full_dest_path);
    if (op->target.target_stats &&
        op->target.target_stats->log_mode == ARCHIVE_MODE) {
      if (CopyAssignPath(op->to_path, sizeof(op->to_path), full_dest_path) != 0) {
        return -1;
      }
      op->path_copy = FALSE;
      return 0;
    }

    {
      DirEntry *tmp_dest_dir_entry = op->target.dest_dir_entry;
      BOOL created = FALSE;

      if (EnsureDirectoryExists(op->ctx, full_dest_path,
                                op->target.target_tree, &created,
                                &tmp_dest_dir_entry, op->dir_create_mode,
                                op->choice_cb) == -1) {
        return -1;
      }
      op->target.dest_dir_entry = tmp_dest_dir_entry;
    }

    if (CopyAssignPath(op->to_path, sizeof(op->to_path), full_dest_path) != 0) {
      return -1;
    }
    op->path_copy = FALSE;
    return 0;
  }

  if (CopyAssignPath(op->to_path, sizeof(op->to_path), to_dir_path) != 0) {
    return -1;
  }
  op->path_copy = FALSE;
  return 0;
}

static int CopyPrepareInitialDestination(CopyOperation *op,
                                         const char *to_dir_path) {
  if (op->source_stats->log_mode != DISK_MODE &&
      op->source_stats->log_mode != USER_MODE) {
    return CopyPrepareArchiveSourceDestination(op, to_dir_path);
  }

  *op->to_path = '\0';
  if (strcmp(to_dir_path, FILE_SEPARATOR_STRING) == 0) {
    return 0;
  }

  return CopyAssignPath(op->to_path, sizeof(op->to_path), to_dir_path);
}

static int CopyApplyPathCopy(CopyOperation *op) {
  if (!op->path_copy) {
    return 0;
  }

  {
    char root_path[PATH_LENGTH + 1];
    char src_path[PATH_LENGTH + 1];
    char *rel_path;

    GetPath(op->source_stats->tree, root_path);
    GetPath(op->source_file->dir_entry, src_path);

    if (strncmp(src_path, root_path, strlen(root_path)) == 0) {
      rel_path = src_path + strlen(root_path);
      if (*rel_path == FILE_SEPARATOR_CHAR)
        rel_path++;
    } else {
      rel_path = src_path;
      if (*rel_path == FILE_SEPARATOR_CHAR)
        rel_path++;
    }

    if (Path_Join(op->abs_path, sizeof(op->abs_path), op->to_path, rel_path) !=
            0 ||
        Path_Join(op->to_path, sizeof(op->to_path), op->abs_path, "") != 0) {
      return -1;
    }
  }

  if (*op->to_path != FILE_SEPARATOR_CHAR) {
    if (Path_Join(op->abs_path, sizeof(op->abs_path), op->from_dir,
                  op->to_path) != 0 ||
        CopyAssignPath(op->to_path, sizeof(op->to_path), op->abs_path) != 0) {
      return -1;
    }
  }

  CopyResolveTargetContext(op, op->to_path);
  if (op->target.target_stats &&
      op->target.target_stats->log_mode == ARCHIVE_MODE) {
    return 0;
  }

  {
    DirEntry *tmp_dest_dir_entry = op->target.dest_dir_entry;

    if (EnsureDirectoryExists(
            op->ctx, op->to_path,
            op->target.target_tree ? op->target.target_tree
                                   : op->source_stats->tree,
            NULL, &tmp_dest_dir_entry, op->dir_create_mode,
            op->choice_cb) == -1) {
      return -1;
    }
    op->target.dest_dir_entry = tmp_dest_dir_entry;
  }

  return 0;
}

static int CopyNormalizeDestinationPath(CopyOperation *op) {
  if (Path_Join(op->abs_path, sizeof(op->abs_path), op->to_path, "") != 0 ||
      CopyAssignPath(op->to_path, sizeof(op->to_path), op->abs_path) != 0) {
    return -1;
  }

  if (*op->to_path == FILE_SEPARATOR_CHAR) {
    return 0;
  }

  if (Path_Join(op->abs_path, sizeof(op->abs_path), op->from_dir,
                op->to_path) != 0 ||
      CopyAssignPath(op->to_path, sizeof(op->to_path), op->abs_path) != 0) {
    return -1;
  }

  return 0;
}

static int CopyPrepareArchiveSourceFile(CopyOperation *op,
                                        const char **archive_src_path,
                                        char *extracted_path,
                                        size_t extracted_path_size,
                                        BOOL *extracted_from_archive) {
  *archive_src_path = op->from_path;
  *extracted_from_archive = FALSE;
  extracted_path[0] = '\0';

  if (op->source_stats->log_mode == DISK_MODE ||
      op->source_stats->log_mode == USER_MODE) {
    return 0;
  }

#ifdef HAVE_LIBARCHIVE
  {
    int fd_tmp;

    if (CopyAssignPath(extracted_path, extracted_path_size,
                       "/tmp/ytnova_copy_XXXXXX") != 0) {
      return -1;
    }
    fd_tmp = mkstemp(extracted_path);
    if (fd_tmp == -1) {
      return -1;
    }
    close(fd_tmp);
    (void)unlink(extracted_path);

    if (ExtractArchiveNode(op->source_stats->log_path, op->from_path,
                           extracted_path, UI_ArchiveCallback, op->ctx) != 0) {
      (void)unlink(extracted_path);
      return -1;
    }
  }

  *archive_src_path = extracted_path;
  *extracted_from_archive = TRUE;
  return 0;
#else
  (void)op;
  return -1;
#endif
}

static int CopyTryArchiveDestination(CopyOperation *op) {
#ifdef HAVE_LIBARCHIVE
  BOOL target_is_archive = FALSE;
  char archive_root_path[PATH_LENGTH + 1];
  char archive_log_path[PATH_LENGTH + 1];

  archive_root_path[0] = '\0';
  archive_log_path[0] = '\0';

  if (op->target.target_stats &&
      op->target.target_stats->log_mode == ARCHIVE_MODE) {
    target_is_archive = TRUE;
    (void)snprintf(archive_log_path, sizeof(archive_log_path), "%s",
                   op->target.target_stats->log_path);
    GetPath(op->target.target_stats->tree, archive_root_path);
    DEBUG_LOG("CopyFile archive destination via logged volume: %s",
              archive_log_path);
  } else {
    char archive_candidate[PATH_LENGTH + 1];
    struct stat archive_stat;
    size_t candidate_len;

    if (CopyAssignPath(archive_candidate, sizeof(archive_candidate),
                       op->to_path) != 0) {
      return -1;
    }
    candidate_len = strlen(archive_candidate);
    if (candidate_len > 1 &&
        archive_candidate[candidate_len - 1] == FILE_SEPARATOR_CHAR) {
      archive_candidate[candidate_len - 1] = '\0';
    }

    if (STAT_(archive_candidate, &archive_stat) == 0 &&
        S_ISREG(archive_stat.st_mode)) {
      struct archive *archive_probe = archive_read_new();

      if (archive_probe) {
        archive_read_support_filter_all(archive_probe);
        archive_read_support_format_all(archive_probe);
        DEBUG_LOG("CopyFile probing archive destination candidate: %s",
                  archive_candidate);
        if (archive_read_open_filename(archive_probe, archive_candidate, 10240) ==
            ARCHIVE_OK) {
          target_is_archive = TRUE;
          (void)snprintf(archive_log_path, sizeof(archive_log_path), "%s",
                         archive_candidate);
          (void)snprintf(archive_root_path, sizeof(archive_root_path), "%s",
                         archive_candidate);
          DEBUG_LOG("CopyFile archive destination probe succeeded: %s",
                    archive_log_path);
        } else {
          DEBUG_LOG("CopyFile archive destination probe failed: %s",
                    archive_candidate);
        }
        archive_read_free(archive_probe);
      }
    }
  }

  if (!target_is_archive) {
    DEBUG_LOG("CopyFile destination treated as filesystem path: %s", op->to_path);
    return 0;
  }

  {
    char relative_path[PATH_LENGTH + 1];
    char archive_entry_path[PATH_LENGTH + 1];
    struct stat archive_entry_stat;
    const char *archive_src_path;
    char extracted_path[PATH_LENGTH + 1];
    BOOL extracted_from_archive;

    if (strncmp(op->to_path, archive_root_path, strlen(archive_root_path)) == 0) {
      char *ptr = op->to_path + strlen(archive_root_path);
      if (*ptr == FILE_SEPARATOR_CHAR)
        ptr++;
      if (CopyAssignPath(relative_path, sizeof(relative_path), ptr) != 0) {
        return -1;
      }
    } else if (CopyAssignPath(relative_path, sizeof(relative_path), op->to_path) !=
               0) {
      return -1;
    }

    if (*relative_path) {
      size_t rel_len = strlen(relative_path);
      if (relative_path[rel_len - 1] == FILE_SEPARATOR_CHAR) {
        relative_path[rel_len - 1] = '\0';
      }
    }

    if (Path_Join(archive_entry_path, sizeof(archive_entry_path), relative_path,
                  op->to_file) != 0 ||
        CopyPrepareArchiveSourceFile(op, &archive_src_path, extracted_path,
                                     sizeof(extracted_path),
                                     &extracted_from_archive) != 0) {
      return -1;
    }

    if (STAT_(archive_src_path, &archive_entry_stat) != 0) {
      if (extracted_from_archive) {
        (void)unlink(extracted_path);
      }
      return -1;
    }

    if (Archive_AddFile(archive_log_path, archive_src_path,
                        archive_entry_path, FALSE, UI_ArchiveCallback,
                        op->ctx) != 0) {
      DEBUG_LOG("CopyFile Archive_AddFile failed: archive=%s entry=%s",
                archive_log_path, archive_entry_path);
      if (extracted_from_archive) {
        (void)unlink(extracted_path);
      }
      return -1;
    }

    if (op->target.target_stats && op->target.target_stats->tree &&
        InsertArchiveFileEntry(op->ctx, op->target.target_stats->tree,
                               archive_entry_path, &archive_entry_stat,
                               op->target.target_stats) != 0) {
      if (extracted_from_archive) {
        (void)unlink(extracted_path);
      }
      return -1;
    }

    if (extracted_from_archive) {
      (void)unlink(extracted_path);
    }
  }

  return 1;
#else
  (void)op;
  return 0;
#endif
}

static int CopyEnsureFilesystemDestination(CopyOperation *op) {
  BOOL created = FALSE;
  DirEntry *tmp_dest_dir_entry = op->target.dest_dir_entry;

  if (EnsureDirectoryExists(op->ctx, op->to_path, op->target.target_tree,
                            &created, &tmp_dest_dir_entry, op->dir_create_mode,
                            op->choice_cb) == -1) {
    return -1;
  }

  op->target.dest_dir_entry = tmp_dest_dir_entry;
  return 0;
}

static const char *CopyPrepareConflictSourcePath(CopyOperation *op) {
  const char *src_path = op->from_path;

  if (op->conflict_src_extracted) {
    return op->conflict_src_path;
  }

  op->conflict_src_path[0] = '\0';
  if (CopyPrepareArchiveSourceFile(op, &src_path, op->conflict_src_path,
                                   sizeof(op->conflict_src_path),
                                   &op->conflict_src_extracted) != 0) {
    op->conflict_src_extracted = FALSE;
    op->conflict_src_path[0] = '\0';
    return op->from_path;
  }

  if (op->conflict_src_extracted) {
    return op->conflict_src_path;
  }

  return src_path;
}

static void CopyCleanupConflictSourcePath(CopyOperation *op) {
  if (!op->conflict_src_extracted || !op->conflict_src_path[0]) {
    return;
  }

  (void)unlink(op->conflict_src_path);
  op->conflict_src_extracted = FALSE;
  op->conflict_src_path[0] = '\0';
}

static int CopyHandleConflict(CopyOperation *op) {
  if (!(op->overwrite_mode && *op->overwrite_mode == CONFLICT_ALL) &&
      op->conflict_cb) {
    int conflict_res = op->conflict_cb(op->ctx,
                                       CopyPrepareConflictSourcePath(op),
                                       op->to_path,
                                       op->overwrite_mode);

    CopyCleanupConflictSourcePath(op);

    if (conflict_res == CONFLICT_ABORT) {
      return -1;
    }
    if (conflict_res == CONFLICT_SKIP) {
      return 1;
    }
  }

  return 0;
}

static int CopyReplaceExistingDestination(CopyOperation *op) {
  if (op->target.dest_dir_entry) {
    FileEntry *dest_file_entry;
    int conflict_res;

    (void)GetFileEntry(op->target.dest_dir_entry, (char *)op->to_file,
                       &dest_file_entry);
    if (!dest_file_entry) {
      return 0;
    }

    conflict_res = CopyHandleConflict(op);
    if (conflict_res != 0) {
      return conflict_res;
    }

    (void)DeleteFile(op->ctx, dest_file_entry, op->overwrite_mode,
                     op->target.target_tree ? op->target.target_stats
                                            : &op->ctx->active->vol->vol_stats,
                     op->choice_cb);
    return 0;
  }

  {
    int existing_fd = open(op->to_path, O_RDONLY);
    int conflict_res;

    if (existing_fd < 0) {
      if (errno == ENOENT) {
        return 0;
      }
      return -1;
    }
    close(existing_fd);

    conflict_res = CopyHandleConflict(op);
    if (conflict_res != 0) {
      return conflict_res;
    }

    if (unlink(op->to_path)) {
      return -1;
    }
  }

  return 0;
}

static void CopyApplyDestinationStats(CopyOperation *op, long long file_size,
                                      BOOL matching) {
  if (op->target.target_stats) {
    op->target.target_stats->disk_total_bytes += file_size;
    op->target.target_stats->disk_total_files++;
  } else {
    op->source_stats->disk_total_bytes += file_size;
    op->source_stats->disk_total_files++;
  }

  if (!matching) {
    return;
  }

  if (op->target.target_stats) {
    op->target.target_stats->disk_matching_bytes += file_size;
    op->target.target_stats->disk_matching_files++;
  } else {
    op->source_stats->disk_matching_bytes += file_size;
    op->source_stats->disk_matching_files++;
  }
}

static FileEntry *CopyCreateDestinationEntry(CopyOperation *op,
                                             const struct stat *stat) {
  FileEntry *fen_ptr;

  if (!op->target.dest_dir_entry) {
    return NULL;
  }

  fen_ptr = (FileEntry *)xmalloc(sizeof(FileEntry) + strlen(op->to_file) + 1);
  {
    size_t name_capacity = strlen(op->to_file) + 1;
    int written = snprintf(fen_ptr->name, name_capacity, "%s", op->to_file);
    if (written < 0 || (size_t)written >= name_capacity) {
      free(fen_ptr);
      return NULL;
    }
  }

  (void)memcpy(&fen_ptr->stat_struct, stat, sizeof(*stat));
  fen_ptr->dir_entry = op->target.dest_dir_entry;
  fen_ptr->tagged = FALSE;
  fen_ptr->matching = Match(fen_ptr, op->source_stats);
  return fen_ptr;
}

static void CopyLinkDestinationEntry(CopyOperation *op, FileEntry *fen_ptr) {
  fen_ptr->next = op->target.dest_dir_entry->file;
  fen_ptr->prev = NULL;
  if (op->target.dest_dir_entry->file)
    op->target.dest_dir_entry->file->prev = fen_ptr;
  op->target.dest_dir_entry->file = fen_ptr;
}

int CopyFile(ViewContext *ctx, Statistic *statistic_ptr, FileEntry *fe_ptr,
             char *to_file, DirEntry *dest_dir_entry,
             const char *to_dir_path, /* absolute path */
             BOOL path_copy, int *dir_create_mode, int *overwrite_mode,
             ConflictCallback cb, ChoiceCallback choice_cb) {
  CopyOperation op;
  FileEntry *fen_ptr;
  long long file_size;
  struct stat stat_struct;
  int replace_result;
  int result = -1;

  op.ctx = ctx;
  op.source_stats = statistic_ptr;
  op.source_file = fe_ptr;
  op.target.dest_dir_entry = dest_dir_entry;
  op.target.target_vol = NULL;
  op.target.target_tree = NULL;
  op.target.target_stats = NULL;
  op.to_file = to_file;
  op.dir_create_mode = dir_create_mode;
  op.overwrite_mode = overwrite_mode;
  op.conflict_cb = cb;
  op.choice_cb = choice_cb;
  op.path_copy = path_copy;
  op.conflict_src_path[0] = '\0';
  op.conflict_src_extracted = FALSE;

  (void)GetFileNamePath(fe_ptr, op.from_path);
  (void)GetPath(fe_ptr->dir_entry, op.from_dir);

  DEBUG_LOG("CopyFile starting: from_path=%s, to_file=%s, to_dir_path=%s",
            op.from_path, to_file, to_dir_path);

  if (CopyPrepareInitialDestination(&op, to_dir_path) != 0) {
    return result;
  }
  CopyResolveTargetContext(&op, op.to_path);

  if (CopyApplyPathCopy(&op) != 0 || CopyNormalizeDestinationPath(&op) != 0) {
    return result;
  }

#ifdef HAVE_LIBARCHIVE
  int archive_result = CopyTryArchiveDestination(&op);
  if (archive_result < 0) {
    return result;
  }
  if (archive_result > 0) {
    return 0;
  }
#endif

  if (CopyEnsureFilesystemDestination(&op) != 0 ||
      Path_Join(op.abs_path, sizeof(op.abs_path), op.to_path, op.to_file) != 0 ||
      CopyAssignPath(op.to_path, sizeof(op.to_path), op.abs_path) != 0) {
    return result;
  }

  DEBUG_LOG("CopyFile final to_path=%s", op.to_path);

  if (!strcmp(op.to_path, op.from_path)) {
    DEBUG_LOG("CopyFile error: to_path == from_path");
    /* MESSAGE( "Can't copy file into itself" ); */
    return (result);
  }

  replace_result = CopyReplaceExistingDestination(&op);
  if (replace_result < 0) {
    ESCAPE;
  }
  if (replace_result > 0) {
    result = 0;
    ESCAPE;
  }

  if (!CopyFileContent(ctx, op.to_path, op.from_path, statistic_ptr)) {
    /* File copied */
    /*-------------*/

    /* Suppress chmod for symbolic links as it targets the link destination */
    if (!S_ISLNK(fe_ptr->stat_struct.st_mode)) {
      if (chmod(op.to_path, fe_ptr->stat_struct.st_mode) == -1) {
        /* WARNING( "Can't chmod file*\"%s\"*to mode %s*IGNORED", to_path,
         * GetAttributes(fe_ptr->stat_struct.st_mode, buffer) ); */
      }
    }

    if (op.target.dest_dir_entry) {
      if (STAT_(op.to_path, &stat_struct)) {
        /* ERROR_MSG( "Stat Failed*ABORT" ); */
        exit(1);
      }

      file_size = stat_struct.st_size;
      if (!AppStateCommitDirEntryTotalPayload(
              op.target.dest_dir_entry, op.target.dest_dir_entry->total_files + 1,
              op.target.dest_dir_entry->total_bytes + file_size)) {
        ESCAPE;
      }

      fen_ptr = CopyCreateDestinationEntry(&op, &stat_struct);
      if (!fen_ptr) {
        ESCAPE;
      }
      if (fen_ptr->matching &&
          !AppStateCommitDirEntryMatchingPayload(
              op.target.dest_dir_entry,
              op.target.dest_dir_entry->matching_files + 1,
              op.target.dest_dir_entry->matching_bytes + file_size)) {
        free(fen_ptr);
        ESCAPE;
      }

      CopyApplyDestinationStats(&op, file_size, fen_ptr->matching);
      CopyLinkDestinationEntry(&op, fen_ptr);
    }

    if (op.target.target_stats) {
      (void)GetAvailBytes(&op.target.target_stats->disk_space,
                          op.target.target_stats);
    } else {
      (void)GetAvailBytes(&statistic_ptr->disk_space, statistic_ptr);
    }

    result = 0;
  }

FNC_XIT:

  return (result);
}


int CopyFileContent(ViewContext *ctx, char *to_path, char *from_path,
                    const Statistic *s) {
  int i, o, n;
  char buffer[2048];
  int spin_counter = 0;

  /* Renamed usage: s->mode -> s->log_mode */
  if (s->log_mode != DISK_MODE && s->log_mode != USER_MODE) {
    return (CopyArchiveFile(ctx, to_path, from_path, s));
  }

  /* FIX: Use realpath to resolve both paths for comparison to avoid
     false positives with relative vs absolute paths */
  char res_from[PATH_LENGTH + 1];
  char res_to[PATH_LENGTH + 1];

  if (realpath(from_path, res_from) && realpath(to_path, res_to)) {
    if (!strcmp(res_from, res_to)) {
      /* MESSAGE( "Can't copy file into itself" ); */
      return (-1);
    }
  } else {
    /* If realpath fails (e.g. destination doesn't exist yet),
       fallback to simple strcmp */
    if (!strcmp(to_path, from_path)) {
      /* MESSAGE( "Can't copy file into itself" ); */
      return (-1);
    }
  }

  if ((i = open(from_path, O_RDONLY)) == -1) {
    /* MESSAGE( "Can't open file*\"%s\"*%s", from_path, strerror(errno) ); */
    return (-1);
  }

  if ((o = open(to_path, O_CREAT | O_TRUNC | O_WRONLY,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
    /* MESSAGE( "Can't create file*\"%s\"*%s", to_path, strerror(errno) ); */
    (void)close(i);
    return (-1);
  }

  while ((n = read(i, buffer, sizeof(buffer))) > 0) {
    /* Update activity spinner every 100 chunks */
    if ((++spin_counter % 100) == 0) {
      if (UI_ArchiveCallback(ARCHIVE_STATUS_PROGRESS, NULL, ctx) ==
          ARCHIVE_CB_ABORT) {
        /* MESSAGE("Operation Interrupted"); */
        close(i);
        close(o);
        unlink(to_path);
        return -1;
      }
    }

    if (write(o, buffer, n) != n) {
      /* MESSAGE( "Write-Error!*%s", strerror(errno) ); */
      (void)close(i);
      (void)close(o);
      (void)unlink(to_path);
      return (-1);
    }
  }

  (void)close(i);
  (void)close(o);

  return (0);
}

int CopyTaggedFiles(ViewContext *ctx, FileEntry *fe_ptr,
                    WalkingPackage *walking_package) {
  Statistic *s = walking_package->function_data.copy.statistic_ptr;
  BOOL path_copy = walking_package->function_data.copy.path_copy;
  const char *to_path = walking_package->function_data.copy.to_path;
  DirEntry *dest_dir_entry = walking_package->function_data.copy.dest_dir_entry;
  int *dir_create_mode = &walking_package->function_data.copy.dir_create_mode;
  int *overwrite_mode = &walking_package->function_data.copy.overwrite_mode;
  ChoiceCallback choice_cb =
      (ChoiceCallback)walking_package->function_data.copy.choice_cb;

  /* Here we reuse the UI_AskConflict function conceptually by casting or
     passing via struct. Since we decoupled, we assume the UI set the conflict
     and choice callbacks in walking_package.
  */
  ConflictCallback cb =
      (ConflictCallback)walking_package->function_data.copy.conflict_cb;

  char new_name[PATH_LENGTH + 1];
  int result = -1;

  walking_package->new_fe_ptr = fe_ptr; /* unchanged */

  if (BuildFilename(fe_ptr->name, walking_package->function_data.copy.to_file,
                    new_name) == 0) {
    if (*new_name == '\0') {
      /* MESSAGE( "Can't copy file to*empty name" ); */
    }

    result =
        CopyFile(ctx, s, fe_ptr, new_name, dest_dir_entry, to_path, path_copy,
                 dir_create_mode, overwrite_mode, cb, choice_cb);
  }

  return (result);
}

static int CopyArchiveFile(ViewContext *ctx, char *to_path,
                           const char *from_path,
                           const Statistic *s) {
#ifdef HAVE_LIBARCHIVE
  int result =
      ExtractArchiveNode(s->log_path, from_path, to_path, UI_ArchiveCallback,
                         ctx);
  if (result != 0) {
    /* WARNING("Can't copy file*%s*to file*%s", from_path, to_path); */
    unlink(to_path); /* Clean up partial file on failure */
  }
  return result;
#else
  (void)ctx;
  (void)to_path;
  (void)from_path;
  (void)s;
  return -1;
#endif
}
