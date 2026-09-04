/***************************************************************************
 *
 * src/fs/archive_read.c
 * Archive scanning, reading, and extraction functions
 *
 ***************************************************************************/

#include "ytnova_fs.h"
#include <stdarg.h>

#define ARCHIVE_READ_BLOCK_SIZE 10240
#define ARCHIVE_EXTRACT_DIR_MODE 0700

static void ArchiveMessageWithBoundary(ViewContext *ctx, const char *fmt, ...);
static BOOL ArchiveKeyPressedWithBoundary(const ViewContext *ctx);
static void ArchiveQuitWithBoundary(ViewContext *ctx);
static void ArchiveRecalculateSysStatsWithBoundary(ViewContext *ctx,
                                                   Statistic *s);
static int ArchiveCanonicalizeRequestPath(const char *archive_path,
                                          const char *entry_path,
                                          char *canonical_path,
                                          size_t canonical_size);
static BOOL ArchiveIsRootMarkerPath(const char *path);

unsigned int Archive_ProbeCapabilities(const char *archive_path) {
#ifdef HAVE_LIBARCHIVE
  struct archive *reader;
  struct archive *writer;
  struct archive_entry *entry;
  unsigned int capabilities = 0;
  int format_code;

  if (!archive_path || archive_path[0] == '\0')
    return 0;

  reader = archive_read_new();
  if (!reader)
    return 0;
  archive_read_support_filter_all(reader);
  archive_read_support_format_all(reader);
  if (archive_read_open_filename(reader, archive_path,
                                 ARCHIVE_READ_BLOCK_SIZE) != ARCHIVE_OK) {
    archive_read_free(reader);
    return 0;
  }

  capabilities = ARCHIVE_CAP_BROWSE | ARCHIVE_CAP_COPY_OUT;
  (void)archive_read_next_header(reader, &entry);
  format_code = archive_format(reader);
  writer = archive_write_new();
  if (writer && format_code != 0 &&
      archive_write_set_format(writer, format_code) == ARCHIVE_OK) {
    int filter_index;

    for (filter_index = 0;
         archive_filter_code(reader, filter_index) != ARCHIVE_FILTER_NONE;
         ++filter_index) {
      if (archive_write_add_filter(writer,
                                   archive_filter_code(reader, filter_index)) !=
          ARCHIVE_OK) {
        break;
      }
    }
    if (archive_filter_code(reader, filter_index) == ARCHIVE_FILTER_NONE) {
      capabilities |= ARCHIVE_CAP_ADD | ARCHIVE_CAP_DELETE |
                      ARCHIVE_CAP_RENAME | ARCHIVE_CAP_MOVE;
    }
  }
  if (writer)
    archive_write_free(writer);
  archive_read_free(reader);
  return capabilities;
#else
  (void)archive_path;
  return 0;
#endif
}

int Archive_ValidateInternalPath(const char *path, char *canonical_path,
                                 size_t canonical_size) {
  const char *segment_start;
  size_t out_len = 0;
  BOOL has_trailing_separator = FALSE;

  if (!path || !canonical_path || canonical_size == 0) {
    return -1;
  }

  canonical_path[0] = '\0';
  if (path[0] == '\0' || path[0] == FILE_SEPARATOR_CHAR || path[0] == '\\') {
    return -1;
  }

  segment_start = path;
  while (1) {
    const char *separator = strchr(segment_start, FILE_SEPARATOR_CHAR);
    size_t segment_len =
        separator ? (size_t)(separator - segment_start) : strlen(segment_start);

    if (segment_len == 0) {
      if (!separator && segment_start != path) {
        has_trailing_separator = TRUE;
        break;
      }
      return -1;
    }

    if ((segment_len == 1 && segment_start[0] == '.') ||
        (segment_len == 2 && segment_start[0] == '.' &&
         segment_start[1] == '.')) {
      return -1;
    }

    if (memchr(segment_start, '\\', segment_len) != NULL) {
      return -1;
    }

    if (out_len > 0) {
      if (out_len + 1 >= canonical_size) {
        return -1;
      }
      canonical_path[out_len++] = FILE_SEPARATOR_CHAR;
      canonical_path[out_len] = '\0';
    }

    if (out_len + segment_len >= canonical_size) {
      return -1;
    }
    memcpy(canonical_path + out_len, segment_start, segment_len);
    out_len += segment_len;
    canonical_path[out_len] = '\0';

    if (!separator) {
      break;
    }
    segment_start = separator + 1;
    if (*segment_start == '\0') {
      has_trailing_separator = TRUE;
      break;
    }
  }

  if (out_len == 0) {
    return -1;
  }

  if (has_trailing_separator) {
    if (out_len + 1 >= canonical_size) {
      return -1;
    }
    canonical_path[out_len++] = FILE_SEPARATOR_CHAR;
    canonical_path[out_len] = '\0';
  }

  return 0;
}

static int ArchiveCanonicalizeRequestPath(const char *archive_path,
                                          const char *entry_path,
                                          char *canonical_path,
                                          size_t canonical_size) {
  const char *candidate_path = entry_path;

  if (!entry_path || !canonical_path || canonical_size == 0) {
    return -1;
  }

  if (archive_path && archive_path[0] != '\0') {
    size_t archive_len = strlen(archive_path);
    if (strncmp(entry_path, archive_path, archive_len) == 0 &&
        (entry_path[archive_len] == '\0' ||
         entry_path[archive_len] == FILE_SEPARATOR_CHAR)) {
      candidate_path = entry_path + archive_len;
      while (*candidate_path == FILE_SEPARATOR_CHAR) {
        candidate_path++;
      }
    }
  }

  return Archive_ValidateInternalPath(candidate_path, canonical_path,
                                      canonical_size);
}

static BOOL ArchiveIsRootMarkerPath(const char *path) {
  const char *p;

  if (!path || path[0] != '.') {
    return FALSE;
  }

  for (p = path + 1; *p != '\0'; ++p) {
    if (*p != FILE_SEPARATOR_CHAR) {
      return FALSE;
    }
  }

  return TRUE;
}

static void ArchiveMessageWithBoundary(ViewContext *ctx, const char *fmt, ...) {
  va_list ap;
  char msg[MESSAGE_LENGTH];

  if (!ctx || !ctx->hook_ui_message || !fmt)
    return;

  va_start(ap, fmt);
  vsnprintf(msg, sizeof(msg), fmt, ap);
  va_end(ap);
  (void)ctx->hook_ui_message(ctx, "%s", msg);
}

static BOOL ArchiveKeyPressedWithBoundary(const ViewContext *ctx) {
  if (ctx && ctx->hook_key_pressed)
    return ctx->hook_key_pressed();
  return FALSE;
}

static void ArchiveQuitWithBoundary(ViewContext *ctx) {
  if (ctx && ctx->hook_quit)
    ctx->hook_quit(ctx);
}

static void ArchiveRecalculateSysStatsWithBoundary(ViewContext *ctx,
                                                   Statistic *s) {
  if (ctx && ctx->hook_recalculate_sys_stats)
    ctx->hook_recalculate_sys_stats(ctx, s);
}

#ifdef HAVE_LIBARCHIVE

/* Helper macros for callback reporting */
#define REPORT_PROGRESS(cb, user_data)                                         \
  do {                                                                         \
    if ((cb) &&                                                                \
        (cb)(ARCHIVE_STATUS_PROGRESS, NULL, (user_data)) == ARCHIVE_CB_ABORT)  \
      return ARCHIVE_CB_ABORT;                                                 \
  } while (0)

#define REPORT_ERROR(cb, user_data, fmt, ...)                                  \
  do {                                                                         \
    char _msg[1024];                                                           \
    snprintf(_msg, sizeof(_msg), fmt, ##__VA_ARGS__);                          \
    if (cb)                                                                    \
      (cb)(ARCHIVE_STATUS_ERROR, _msg, (user_data));                           \
  } while (0)

#define REPORT_WARNING(cb, user_data, fmt, ...)                                \
  do {                                                                         \
    char _msg[1024];                                                           \
    snprintf(_msg, sizeof(_msg), fmt, ##__VA_ARGS__);                          \
    if (cb)                                                                    \
      (cb)(ARCHIVE_STATUS_WARNING, _msg, (user_data));                         \
  } while (0)

/*
 * Uses libarchive to find a specific entry within an archive and write its
 * contents to the provided output file descriptor.
 * Returns 0 on success, -1 on failure.
 */
int ExtractArchiveEntry(const char *archive_path, const char *entry_path,
                        int out_fd, ArchiveProgressCallback cb,
                        void *user_data) {
  struct archive *a;
  struct archive_entry *entry;
  int r;
  const void *buff;
  size_t size;
  la_int64_t offset;
  int spin_counter = 0; /* Activity spinner counter */
  int found = 0;
  char canonical_entry_path[PATH_LENGTH + 1];

  if (!entry_path)
    return -1;
  if (ArchiveCanonicalizeRequestPath(archive_path, entry_path,
                                     canonical_entry_path,
                                     sizeof(canonical_entry_path)) != 0) {
    REPORT_ERROR(cb, user_data, "Unsafe archive internal path requested");
    return -1;
  }

  a = archive_read_new();
  if (a == NULL) {
    return -1;
  }
  archive_read_support_filter_all(a);
  archive_read_support_format_all(a);

  r = archive_read_open_filename(a, archive_path, 10240);
  if (r != ARCHIVE_OK) {
    archive_read_free(a);
    return -1;
  }

  while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
    const char *member_path = archive_entry_pathname(entry);
    char canonical_member_path[PATH_LENGTH + 1];

    if (member_path == NULL) {
      continue;
    }
    if (Archive_ValidateInternalPath(member_path, canonical_member_path,
                                     sizeof(canonical_member_path)) != 0) {
      if (ArchiveIsRootMarkerPath(member_path)) {
        continue;
      }
      REPORT_WARNING(cb, user_data, "Skipped unsafe archive member path: %s",
                     member_path);
      continue;
    }

    if (strcmp(canonical_entry_path, canonical_member_path) == 0) {
      found = 1;
      while ((r = archive_read_data_block(a, &buff, &size, &offset)) ==
             ARCHIVE_OK) {
        if ((++spin_counter % 100) == 0) {
          if (cb && cb(ARCHIVE_STATUS_PROGRESS, NULL, user_data) ==
                        ARCHIVE_CB_ABORT) {
            REPORT_ERROR(cb, user_data, "Operation Interrupted");
            found = 0;
            break;
          }
        }
        if (write(out_fd, buff, size) != (ssize_t)size) {
          found = 0; /* Write error */
          break;
        }
      }
      if (r != ARCHIVE_EOF && r != ARCHIVE_OK && found)
        found = 0; /* Read error or interrupt */
      break;       /* Stop searching */
    }

    /* Update spinner while searching headers too */
    if ((++spin_counter % 50) == 0) {
      if (cb &&
          cb(ARCHIVE_STATUS_PROGRESS, NULL, user_data) == ARCHIVE_CB_ABORT) {
        REPORT_ERROR(cb, user_data, "Operation Interrupted");
        found = 0;
        break;
      }
    }
  }

  archive_read_free(a);
  return (found) ? 0 : -1;
}

int ExtractArchiveNode(const char *archive_path, const char *entry_path,
                       const char *dest_path, ArchiveProgressCallback cb,
                       void *user_data) {
  struct archive *a;
  struct archive_entry *entry;
  int r;
  const void *buff;
  size_t size;
  la_int64_t offset;
  int spin_counter = 0;
  int found = 0;
  int fd;
  char canonical_entry_path[PATH_LENGTH + 1];

  if (!entry_path || !dest_path)
    return -1;
  if (ArchiveCanonicalizeRequestPath(archive_path, entry_path,
                                     canonical_entry_path,
                                     sizeof(canonical_entry_path)) != 0) {
    REPORT_ERROR(cb, user_data, "Unsafe archive internal path requested");
    return -1;
  }

  a = archive_read_new();
  if (a == NULL) {
    return -1;
  }
  archive_read_support_filter_all(a);
  archive_read_support_format_all(a);

  r = archive_read_open_filename(a, archive_path, 10240);
  if (r != ARCHIVE_OK) {
    archive_read_free(a);
    return -1;
  }

  while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
    /* Update spinner and check ESC */
    if ((++spin_counter % 50) == 0) {
      if (cb &&
          cb(ARCHIVE_STATUS_PROGRESS, NULL, user_data) == ARCHIVE_CB_ABORT) {
        REPORT_ERROR(cb, user_data, "Operation Interrupted");
        archive_read_free(a);
        return -1;
      }
    }

    const char *member_path = archive_entry_pathname(entry);
    char canonical_member_path[PATH_LENGTH + 1];

    if (member_path == NULL) {
      continue;
    }
    if (Archive_ValidateInternalPath(member_path, canonical_member_path,
                                     sizeof(canonical_member_path)) != 0) {
      if (ArchiveIsRootMarkerPath(member_path)) {
        continue;
      }
      REPORT_WARNING(cb, user_data, "Skipped unsafe archive member path: %s",
                     member_path);
      continue;
    }

    if (strcmp(canonical_entry_path, canonical_member_path) == 0) {
      /* MATCH FOUND! */
      found = 1;
      mode_t type = archive_entry_filetype(entry);

      if (type == AE_IFLNK) {
        const char *target = archive_entry_symlink(entry);
        if (target) {
          if (symlink(target, dest_path) != 0) {
            if (errno == EEXIST) {
              unlink(dest_path);
              if (symlink(target, dest_path) != 0)
                found = 0;
            } else {
              found = 0;
            }
          }
        } else {
          found = 0;
        }
      } else if (type == AE_IFREG) {
        fd = open(dest_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd == -1) {
          found = 0;
        } else {
          while ((r = archive_read_data_block(a, &buff, &size, &offset)) ==
                 ARCHIVE_OK) {
            if ((++spin_counter % 100) == 0) {
              if (cb && cb(ARCHIVE_STATUS_PROGRESS, NULL, user_data) ==
                            ARCHIVE_CB_ABORT) {
                REPORT_ERROR(cb, user_data, "Operation Interrupted");
                found = 0;
                break;
              }
            }
            if (write(fd, buff, size) != (ssize_t)size) {
              found = 0; /* Write error */
              break;
            }
          }
          close(fd);
          if (r != ARCHIVE_EOF && r != ARCHIVE_OK && found) {
            found = 0;
            /* Cleanup partial file on error/interrupt */
            unlink(dest_path);
          } else if (found == 0) {
            unlink(dest_path);
          }
        }
      } else {
        /* Unsupported file type */
        found = 0;
      }
      break; /* Stop searching */
    }
  }

  archive_read_free(a);

  if (!found) {
    /* Optionally report warning, but found=0 acts as return -1 */
  }
  return (found) ? 0 : -1;
}

static int ArchiveEnsureParentDirectories(char *path) {
  char *cursor;

  for (cursor = path + 1; *cursor != '\0'; ++cursor) {
    if (*cursor == FILE_SEPARATOR_CHAR) {
      *cursor = '\0';
      if (mkdir(path, ARCHIVE_EXTRACT_DIR_MODE) != 0 && errno != EEXIST) {
        *cursor = FILE_SEPARATOR_CHAR;
        return -1;
      }
      *cursor = FILE_SEPARATOR_CHAR;
    }
  }
  return 0;
}

int ExtractArchiveTree(const char *archive_path, const char *tree_path,
                       const char *dest_path, ArchiveProgressCallback cb,
                       void *user_data) {
  struct archive *archive;
  struct archive_entry *entry;
  char canonical_tree[PATH_LENGTH + 1];
  int found = 0;

  if (!archive_path || !tree_path || !dest_path ||
      ArchiveCanonicalizeRequestPath(archive_path, tree_path, canonical_tree,
                                     sizeof(canonical_tree)) != 0)
    return -1;

  archive = archive_read_new();
  if (!archive)
    return -1;
  archive_read_support_filter_all(archive);
  archive_read_support_format_all(archive);
  if (archive_read_open_filename(archive, archive_path,
                                 ARCHIVE_READ_BLOCK_SIZE) != ARCHIVE_OK) {
    archive_read_free(archive);
    return -1;
  }

  while (archive_read_next_header(archive, &entry) == ARCHIVE_OK) {
    const char *member = archive_entry_pathname(entry);
    char canonical_member[PATH_LENGTH + 1];
    char output_path[PATH_LENGTH + 1];
    const char *suffix;
    int fd;
    const void *buffer;
    size_t size;
    la_int64_t offset;

    if (!member || Archive_ValidateInternalPath(member, canonical_member,
                                                sizeof(canonical_member)) != 0)
      continue;
    {
      size_t member_len = strlen(canonical_member);
      while (member_len > 0 &&
             canonical_member[member_len - 1] == FILE_SEPARATOR_CHAR)
        canonical_member[--member_len] = '\0';
    }
    if (strcmp(canonical_member, canonical_tree) == 0) {
      suffix = "";
    } else if (strncmp(canonical_member, canonical_tree,
                       strlen(canonical_tree)) == 0 &&
               canonical_member[strlen(canonical_tree)] == FILE_SEPARATOR_CHAR) {
      suffix = canonical_member + strlen(canonical_tree) + 1;
    } else {
      continue;
    }
    if (Path_Join(output_path, sizeof(output_path), dest_path, suffix) != 0 ||
        ArchiveEnsureParentDirectories(output_path) != 0)
      goto failed;
    found = 1;
    if (archive_entry_filetype(entry) == AE_IFDIR) {
      if (mkdir(output_path, ARCHIVE_EXTRACT_DIR_MODE) != 0 && errno != EEXIST)
        goto failed;
      continue;
    }
    if (archive_entry_filetype(entry) != AE_IFREG)
      goto failed;
    fd = open(output_path, O_CREAT | O_WRONLY | O_TRUNC, 0600);
    if (fd < 0)
      goto failed;
    while (archive_read_data_block(archive, &buffer, &size, &offset) == ARCHIVE_OK) {
      if (write(fd, buffer, size) != (ssize_t)size) {
        close(fd);
        goto failed;
      }
    }
    close(fd);
  }
  archive_read_free(archive);
  return found ? 0 : -1;

failed:
  archive_read_free(archive);
  return -1;
}

#else
/* Dummy implementations if libarchive is not available */
int ExtractArchiveEntry(const char *archive_path, const char *entry_path,
                        int out_fd, ArchiveProgressCallback cb,
                        void *user_data) {
  return -1;
}
int ExtractArchiveNode(const char *archive_path, const char *entry_path,
                       const char *dest_path, ArchiveProgressCallback cb,
                       void *user_data) {
  return -1;
}
int ExtractArchiveTree(const char *archive_path, const char *tree_path,
                       const char *dest_path, ArchiveProgressCallback cb,
                       void *user_data) {
  return -1;
}
#endif /* HAVE_LIBARCHIVE */

static int AppendBoundedString(char *dst, size_t dst_size, const char *src) {
  size_t used;
  int written;

  if (!dst || dst_size == 0 || !src) {
    return -1;
  }

  used = strlen(dst);
  if (used >= dst_size) {
    dst[dst_size - 1] = '\0';
    return -1;
  }

  written = snprintf(dst + used, dst_size - used, "%s", src);
  if (written < 0 || (size_t)written >= (dst_size - used)) {
    dst[dst_size - 1] = '\0';
    return -1;
  }

  return 0;
}

static int GetArchiveDirEntry(DirEntry *tree, const char *path, DirEntry **dir_entry);

static int InsertArchiveDirEntry(ViewContext *ctx, DirEntry *tree, char *path,
                                 const struct stat *stat, Statistic *s) {
  DirEntry *df_ptr, *de_ptr, *ds_ptr;
  char father_path[PATH_LENGTH + 1];
  char name[PATH_LENGTH + 1];
  size_t name_len;

  if (strlen(path) >= PATH_LENGTH) {
    ArchiveMessageWithBoundary(ctx,
                               "Archive path too long*skipping directory insert");
    return -1;
  }

  Fnsplit(path, father_path, name);

  if (GetArchiveDirEntry(tree, father_path, &df_ptr)) {
    ArchiveMessageWithBoundary(ctx, "can't find subdir*%s", father_path);
    return (-1);
  }

  /* FIX: Allocate exact size for name + null terminator */
  name_len = strlen(name);
  de_ptr = (DirEntry *)xcalloc(1, sizeof(DirEntry) + name_len + 1);

  (void)memcpy(de_ptr->name, name, name_len + 1);
  (void)memcpy((char *)&de_ptr->stat_struct, (char *)stat, sizeof(struct stat));

  if (df_ptr->sub_tree == NULL) {
    de_ptr->up_tree = df_ptr;
    df_ptr->sub_tree = de_ptr;
  } else {
    de_ptr->up_tree = df_ptr;

    for (ds_ptr = df_ptr->sub_tree; ds_ptr; ds_ptr = ds_ptr->next) {
      if (strcmp(ds_ptr->name, de_ptr->name) > 0) {
        de_ptr->next = ds_ptr;
        de_ptr->prev = ds_ptr->prev;
        if (ds_ptr->prev)
          ds_ptr->prev->next = de_ptr;
        else
          de_ptr->up_tree->sub_tree = de_ptr;
        ds_ptr->prev = de_ptr;
        break;
      }

      if (ds_ptr->next == NULL) {
        de_ptr->prev = ds_ptr;
        de_ptr->next = ds_ptr->next;
        ds_ptr->next = de_ptr;
        break;
      }
    }
  }
  s->disk_total_directories++;
  return (0);
}

int InsertArchiveFileEntry(ViewContext *ctx, DirEntry *tree, char *path,
                           const struct stat *stat, Statistic *s) {
  char dir[PATH_LENGTH + 1];
  char file[PATH_LENGTH + 1];
  DirEntry *de_ptr;
  FileEntry *fs_ptr, *fe_ptr;
  struct stat stat_struct;
  size_t file_len;
  size_t path_len;
  size_t link_len = 0;
  int n;

  if (ArchiveKeyPressedWithBoundary(ctx)) {
    ArchiveQuitWithBoundary(ctx);
  }

  Fnsplit(path, dir, file);

  if (GetArchiveDirEntry(tree, dir, &de_ptr)) {
#ifdef DEBUG
    fprintf(stderr, "can't get directory for file*%s*trying recover\n", path);
#endif

    (void)memset((char *)&stat_struct, 0, sizeof(struct stat));
    stat_struct.st_mode = S_IFDIR;

    if (TryInsertArchiveDirEntry(ctx, tree, dir, &stat_struct, s)) {
      ArchiveMessageWithBoundary(ctx, "inserting directory failed");
      return (-1);
    }
    if (GetArchiveDirEntry(tree, dir, &de_ptr)) {
      ArchiveMessageWithBoundary(
          ctx, "again: can't get directory for file*%s*giving up", path);
      return (-1);
    }
  }

  file_len = strlen(file);
  path_len = strlen(path);

  if (S_ISLNK(stat->st_mode)) {
    link_len = strlen(&path[path_len + 1]) + 1;
    n = (int)link_len;
  } else {
    n = 0;
  }

  /* FIX: Allocate exact size for name + null + link data */
  fe_ptr = (FileEntry *)xcalloc(1, sizeof(FileEntry) + file_len + 1 + n + 1);

  (void)memcpy((char *)&fe_ptr->stat_struct, (char *)stat, sizeof(struct stat));
  (void)memcpy(fe_ptr->name, file, file_len + 1);

  if (S_ISLNK(stat->st_mode)) {
    (void)memcpy(&fe_ptr->name[file_len + 1], &path[path_len + 1], link_len);
  }

  fe_ptr->dir_entry = de_ptr;
  de_ptr->total_files++;
  de_ptr->total_bytes += stat->st_size;
  s->disk_total_files++;
  s->disk_total_bytes += stat->st_size;

  if (de_ptr->file == NULL) {
    de_ptr->file = fe_ptr;
  } else {
    for (fs_ptr = de_ptr->file; fs_ptr->next; fs_ptr = fs_ptr->next)
      ;

    fe_ptr->prev = fs_ptr;
    fs_ptr->next = fe_ptr;
  }
  return (0);
}

static int GetArchiveDirEntry(DirEntry *tree, const char *path,
                              DirEntry **dir_entry) {
  char *path_copy;
  const char *token;
  char *saveptr;
  DirEntry *current = tree;
  DirEntry *child;

  if (!path || *path == '\0' || strcmp(path, ".") == 0) {
    *dir_entry = tree;
    return 0;
  }

  path_copy = xstrdup(path);

  token = strtok_r(path_copy, FILE_SEPARATOR_STRING, &saveptr);
  while (token) {
    int found = 0;
    /* Search children of 'current' */
    for (child = current->sub_tree; child; child = child->next) {
      if (strcmp(child->name, token) == 0) {
        current = child;
        found = 1;
        break;
      }
    }

    if (!found) {
      free(path_copy);
      return -1;
    }

    token = strtok_r(NULL, FILE_SEPARATOR_STRING, &saveptr);
  }

  free(path_copy);
  *dir_entry = current;
  return 0;
}

int TryInsertArchiveDirEntry(ViewContext *ctx, DirEntry *tree, const char *dir,
                             const struct stat *stat, Statistic *s) {
  char *path_copy;
  const char *token;
  char *saveptr;
  char current_path[PATH_LENGTH + 1];
  DirEntry *dummy;

  if (!dir || *dir == '\0')
    return 0;

  path_copy = xstrdup(dir);

  current_path[0] = '\0';

  token = strtok_r(path_copy, FILE_SEPARATOR_STRING, &saveptr);
  while (token) {
    /* Append token to current_path */
    if (current_path[0] != '\0' &&
        AppendBoundedString(current_path, sizeof(current_path),
                            FILE_SEPARATOR_STRING) != 0) {
      ArchiveMessageWithBoundary(
          ctx, "Archive path too long*skipping directory insert");
      free(path_copy);
      return -1;
    }
    if (AppendBoundedString(current_path, sizeof(current_path), token) != 0) {
      ArchiveMessageWithBoundary(
          ctx, "Archive path too long*skipping directory insert");
      free(path_copy);
      return -1;
    }

    /* Check if this partial path exists */
    if (GetArchiveDirEntry(tree, current_path, &dummy) != 0) {
      /* Not found, insert it */
      if (InsertArchiveDirEntry(ctx, tree, current_path, stat, s) != 0) {
        free(path_copy);
        return -1;
      }
    }

    token = strtok_r(NULL, FILE_SEPARATOR_STRING, &saveptr);
  }

  free(path_copy);
  return 0;
}

/*
 * Copy stat data from libarchive's const struct to our mutable one.
 */
static void copy_stat_from_entry(struct stat *dest,
                                 struct archive_entry *entry) {
  const struct stat *st;

  memset(dest, 0, sizeof(struct stat));
  st = archive_entry_stat(entry);

  if (st) {
    dest->st_mode = st->st_mode;
    dest->st_uid = st->st_uid;
    dest->st_gid = st->st_gid;
    dest->st_size = st->st_size;
    dest->st_mtime = st->st_mtime;
    dest->st_nlink = st->st_nlink;
  } else {
    /* Fallback if stat is not available */
    dest->st_mode = archive_entry_mode(entry);
    dest->st_uid = archive_entry_uid(entry);
    dest->st_gid = archive_entry_gid(entry);
    dest->st_size = archive_entry_size(entry);
    dest->st_mtime = archive_entry_mtime(entry);
    dest->st_nlink = archive_entry_nlink(entry);
  }

  /*
   * Strictly enforce file type bits based on libarchive's explicit filetype.
   * This prevents regular files from being misidentified as directories if
   * the archive header's mode bits are ambiguous or non-standard.
   */
  switch (archive_entry_filetype(entry)) {
  case AE_IFREG:
    dest->st_mode = (dest->st_mode & ~S_IFMT) | S_IFREG;
    break;
  case AE_IFDIR:
    dest->st_mode = (dest->st_mode & ~S_IFMT) | S_IFDIR;
    break;
  case AE_IFLNK:
    dest->st_mode = (dest->st_mode & ~S_IFMT) | S_IFLNK;
    break;
  default:
    /* Keep existing mode for sockets, block devices, etc. */
    break;
  }
}

/*
 * Dispatcher function to read file tree from archive using libarchive.
 * This replaces all old ReadTreeFrom... and GetStatFrom... functions.
 *
 * Now accepts DirEntry **dir_entry_ptr to allow updating the root pointer
 * if MinimizeArchiveTree swaps it.
 */
int ReadTreeFromArchive(ViewContext *ctx, DirEntry **dir_entry_ptr,
                        const char *filename, Statistic *s,
                        ScanProgressCallback cb, void *cb_data) {
  struct archive *a;
  struct archive_entry *entry;
  int r;
  char path_buffer[PATH_LENGTH * 2]; /* Buffer for path + symlink target */
  char canonical_path[PATH_LENGTH + 1];
  struct stat stat_buf;
  int count = 0;
  const char *clean_path;
  DirEntry *dir_entry = *dir_entry_ptr;


  a = archive_read_new();
  if (a == NULL) {
    ArchiveMessageWithBoundary(ctx, "archive_read_new() failed");
    return -1;
  }

  archive_read_support_filter_all(a);
  archive_read_support_format_all(a);

  r = archive_read_open_filename(a, filename, 10240);
  if (r != ARCHIVE_OK) {
    ArchiveMessageWithBoundary(ctx, "Could not open archive*'%s'*%s", filename,
                               archive_error_string(a));
    archive_read_free(a);
    return -1;
  }

  s->archive_capabilities = Archive_ProbeCapabilities(filename);

  while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
    const char *pathname = archive_entry_pathname(entry);

    if (pathname == NULL) {
      continue;
    }

    if (Archive_ValidateInternalPath(pathname, canonical_path,
                                     sizeof(canonical_path)) != 0) {
      if (ArchiveIsRootMarkerPath(pathname)) {
        continue;
      }
      ArchiveMessageWithBoundary(ctx, "Skipped unsafe archive member path*%s",
                                 pathname);
      continue;
    }
    clean_path = canonical_path;

    copy_stat_from_entry(&stat_buf, entry);

    if (S_ISDIR(stat_buf.st_mode)) {
      /* Safer string copy */
      int copied = snprintf(path_buffer, sizeof(path_buffer), "%s", clean_path);
      if (copied < 0 || (size_t)copied >= sizeof(path_buffer)) {
        ArchiveMessageWithBoundary(ctx, "Archive entry path too long*skipping");
        continue;
      }
      /* Ensure directory paths end with a separator for
       * TryInsertArchiveDirEntry */
      size_t len = strlen(path_buffer);
      if (len > 0 && path_buffer[len - 1] != FILE_SEPARATOR_CHAR) {
        if (AppendBoundedString(path_buffer, sizeof(path_buffer),
                                FILE_SEPARATOR_STRING) != 0) {
          ArchiveMessageWithBoundary(
              ctx, "Archive directory path too long*skipping");
          continue;
        }
      }
      (void)TryInsertArchiveDirEntry(ctx, dir_entry, path_buffer, &stat_buf, s);

    } else if (S_ISREG(stat_buf.st_mode) || S_ISLNK(stat_buf.st_mode)) {
      /* Safer string copy */
      int copied = snprintf(path_buffer, sizeof(path_buffer), "%s", clean_path);
      if (copied < 0 || (size_t)copied >= sizeof(path_buffer)) {
        ArchiveMessageWithBoundary(ctx, "Archive entry path too long*skipping");
        continue;
      }

      if (S_ISLNK(stat_buf.st_mode)) {
        const char *link_target = archive_entry_symlink(entry);
        size_t len = strlen(path_buffer);

        path_buffer[len + 1] = '\0';
        if (link_target) {
          /* Append symlink target after the null terminator of the path */
          size_t target_len = strlen(link_target);
          if (len + 1 + target_len + 1 <= sizeof(path_buffer)) {
            (void)memcpy(&path_buffer[len + 1], link_target, target_len + 1);
          } else {
            ArchiveMessageWithBoundary(
                ctx, "Archive symlink target too long*skipping");
            continue;
          }
        }
      }
      (void)InsertArchiveFileEntry(ctx, dir_entry, path_buffer, &stat_buf, s);
    }

    if (ArchiveKeyPressedWithBoundary(ctx)) {
      ArchiveQuitWithBoundary(ctx);
    }

    /* Update statistics / animation every 20 files */
    if ((count++ % 20) == 0) {
      if (cb)
        cb(ctx, cb_data);
    }
  }

  archive_read_free(a);

  /* Important: Recalculate visibility based on current settings immediately
   * after load */
  ArchiveRecalculateSysStatsWithBoundary(ctx, s);

  return 0;
}
