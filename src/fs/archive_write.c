/***************************************************************************
 *
 * src/fs/archive_write.c
 * Archive writing and modification functions
 *
 ***************************************************************************/

#include "ytnova_fs.h"
#include <dirent.h>

#ifdef HAVE_LIBARCHIVE

#define COPY_BUF_SIZE 16384

typedef struct {
  const char *format_name;
  const char *filter_name;
} ArchiveCreateSpec;

/* Struct for Rename Callback */
typedef struct {
  const char *old_path;
  const char *new_name;
  ArchiveProgressCallback progress_cb;
  void *progress_data;
} RenameContext;

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

static BOOL ends_with_ci(const char *value, const char *suffix) {
  size_t value_len;
  size_t suffix_len;
  size_t i;

  if (!value || !suffix)
    return FALSE;

  value_len = strlen(value);
  suffix_len = strlen(suffix);

  if (suffix_len > value_len)
    return FALSE;

  for (i = 0; i < suffix_len; ++i) {
    unsigned char lhs = (unsigned char)value[value_len - suffix_len + i];
    unsigned char rhs = (unsigned char)suffix[i];
    if (tolower(lhs) != tolower(rhs))
      return FALSE;
  }

  return TRUE;
}

static int infer_archive_create_spec(const char *dest_path, ArchiveCreateSpec *spec) {
  if (!dest_path || !spec)
    return -1;

  if (ends_with_ci(dest_path, ".tar.gz") || ends_with_ci(dest_path, ".tgz")) {
    spec->format_name = "paxr";
    spec->filter_name = "gzip";
    return 0;
  }
  if (ends_with_ci(dest_path, ".tar.bz2") || ends_with_ci(dest_path, ".tbz2")) {
    spec->format_name = "paxr";
    spec->filter_name = "bzip2";
    return 0;
  }
  if (ends_with_ci(dest_path, ".tar.xz") || ends_with_ci(dest_path, ".txz")) {
    spec->format_name = "paxr";
    spec->filter_name = "xz";
    return 0;
  }
  if (ends_with_ci(dest_path, ".tar")) {
    spec->format_name = "paxr";
    spec->filter_name = NULL;
    return 0;
  }
  if (ends_with_ci(dest_path, ".zip")) {
    spec->format_name = "zip";
    spec->filter_name = NULL;
    return 0;
  }

  return UNSUPPORTED_FORMAT_ERROR;
}

static const char *archive_entry_name_from_source(const char *src_path) {
  const char *cursor;

  if (!src_path)
    return NULL;

  cursor = src_path + strlen(src_path);
  while (cursor > src_path && cursor[-1] == FILE_SEPARATOR_CHAR)
    cursor--;
  while (cursor > src_path && cursor[-1] != FILE_SEPARATOR_CHAR)
    cursor--;

  return cursor;
}

static int archive_write_file_data(struct archive *writer, int fd) {
  char buf[COPY_BUF_SIZE];
  ssize_t len;

  while ((len = read(fd, buf, sizeof(buf))) > 0) {
    if (archive_write_data(writer, buf, (size_t)len) < 0)
      return -1;
  }

  return (len < 0) ? -1 : 0;
}

int Archive_CreateFromPaths(const char *dest_path,
                            const char *const *source_paths,
                            const char *const *archive_paths,
                            size_t source_count) {
  struct archive *writer = NULL;
  ArchiveCreateSpec spec;
  size_t i;
  int rc;

  if (!dest_path || (!source_paths && source_count > 0))
    return -1;

  rc = infer_archive_create_spec(dest_path, &spec);
  if (rc != 0)
    return rc;

  writer = archive_write_new();
  if (!writer)
    return -1;

  if (archive_write_set_format_by_name(writer, spec.format_name) < ARCHIVE_OK)
    goto error;
  if (spec.filter_name &&
      archive_write_add_filter_by_name(writer, spec.filter_name) < ARCHIVE_OK)
    goto error;
  if (archive_write_open_filename(writer, dest_path) != ARCHIVE_OK)
    goto error;

  for (i = 0; i < source_count; ++i) {
    const char *src_path = source_paths[i];
    const char *entry_name;
    struct stat src_st;
    struct archive_entry *entry;
    int fd = -1;

    if (!src_path)
      goto error;

    if (lstat(src_path, &src_st) != 0)
      goto error;

    if (archive_paths && archive_paths[i] && archive_paths[i][0] != '\0')
      entry_name = archive_paths[i];
    else
      entry_name = archive_entry_name_from_source(src_path);
    if (!entry_name || entry_name[0] == '\0')
      goto error;

    entry = archive_entry_new();
    if (!entry)
      goto error;

    archive_entry_copy_stat(entry, &src_st);
    archive_entry_set_pathname(entry, entry_name);

    if (S_ISLNK(src_st.st_mode)) {
      char link_target[PATH_LENGTH + 1];
      ssize_t link_len = readlink(src_path, link_target, PATH_LENGTH);
      if (link_len < 0 || link_len > PATH_LENGTH) {
        archive_entry_free(entry);
        goto error;
      }
      link_target[link_len] = '\0';
      archive_entry_set_symlink(entry, link_target);
      archive_entry_set_size(entry, 0);
    }

    if (archive_write_header(writer, entry) != ARCHIVE_OK) {
      archive_entry_free(entry);
      goto error;
    }

    if (S_ISREG(src_st.st_mode) && src_st.st_size > 0) {
      fd = open(src_path, O_RDONLY);
      if (fd < 0) {
        archive_entry_free(entry);
        goto error;
      }
      if (archive_write_file_data(writer, fd) != 0) {
        close(fd);
        archive_entry_free(entry);
        goto error;
      }
      close(fd);
    }

    archive_entry_free(entry);
  }

  rc = archive_write_close(writer);
  archive_write_free(writer);
  return (rc == ARCHIVE_OK) ? 0 : -1;

error:
  archive_write_free(writer);
  return -1;
}

/*
 * Helper to setup writer based on reader
 */
static int configure_writer(struct archive *in, struct archive *out) {
  int format = archive_format(in);
  int filter = archive_filter_code(in, 0);
  int r;

  /* Set Filter */
  switch (filter) {
  case ARCHIVE_FILTER_NONE:
    r = archive_write_add_filter_none(out);
    break;
  case ARCHIVE_FILTER_GZIP:
    r = archive_write_add_filter_gzip(out);
    break;
  case ARCHIVE_FILTER_BZIP2:
    r = archive_write_add_filter_bzip2(out);
    break;
  case ARCHIVE_FILTER_COMPRESS:
    r = archive_write_add_filter_compress(out);
    break;
  case ARCHIVE_FILTER_LZIP:
    r = archive_write_add_filter_lzip(out);
    break;
  case ARCHIVE_FILTER_LZMA:
    r = archive_write_add_filter_lzma(out);
    break;
  case ARCHIVE_FILTER_XZ:
    r = archive_write_add_filter_xz(out);
    break;
#ifdef ARCHIVE_FILTER_LZ4
  case ARCHIVE_FILTER_LZ4:
    r = archive_write_add_filter_lz4(out);
    break;
#endif
#ifdef ARCHIVE_FILTER_ZSTD
  case ARCHIVE_FILTER_ZSTD:
    r = archive_write_add_filter_zstd(out);
    break;
#endif
  case ARCHIVE_FILTER_PROGRAM:
    r = archive_write_add_filter_program(out, archive_filter_name(in, 0));
    break;
  default:
    return -1;
  }

  if (r < ARCHIVE_OK)
    return -1;

  /* Set Format */
  format &= ARCHIVE_FORMAT_BASE_MASK;
  if (format == ARCHIVE_FORMAT_TAR) {
    r = archive_write_set_format_pax_restricted(out);
  } else if (format == ARCHIVE_FORMAT_ZIP) {
    r = archive_write_set_format_zip(out);
  } else if (format == ARCHIVE_FORMAT_CPIO) {
    r = archive_write_set_format_cpio(out);
  } else if (format == ARCHIVE_FORMAT_7ZIP) {
    r = archive_write_set_format_7zip(out);
  } else if (format == ARCHIVE_FORMAT_ISO9660) {
    r = archive_write_set_format_iso9660(out);
  } else if (format == ARCHIVE_FORMAT_AR) {
    r = archive_write_set_format_ar_bsd(out);
  } else {
    return -1; /* Unknown format */
  }

  if (r < ARCHIVE_OK)
    return -1;

  return 0;
}

/*
 * Create a clean copy of an entry for writing.
 */
static struct archive_entry *
clone_entry_for_write(struct archive_entry *in_entry) {
  struct archive_entry *out_entry = archive_entry_new();
  if (!out_entry)
    return NULL;

  archive_entry_set_pathname(out_entry, archive_entry_pathname(in_entry));
  archive_entry_set_size(out_entry, archive_entry_size(in_entry));
  archive_entry_set_mtime(out_entry, archive_entry_mtime(in_entry),
                          archive_entry_mtime_nsec(in_entry));
  archive_entry_set_filetype(out_entry, archive_entry_filetype(in_entry));
  archive_entry_set_perm(out_entry, archive_entry_perm(in_entry));

  archive_entry_set_uid(out_entry, archive_entry_uid(in_entry));
  archive_entry_set_gid(out_entry, archive_entry_gid(in_entry));
  if (archive_entry_uname(in_entry))
    archive_entry_set_uname(out_entry, archive_entry_uname(in_entry));
  if (archive_entry_gname(in_entry))
    archive_entry_set_gname(out_entry, archive_entry_gname(in_entry));

  if (archive_entry_hardlink(in_entry))
    archive_entry_set_hardlink(out_entry, archive_entry_hardlink(in_entry));
  if (archive_entry_symlink(in_entry))
    archive_entry_set_symlink(out_entry, archive_entry_symlink(in_entry));

  archive_entry_acl_clear(out_entry);
  archive_entry_xattr_clear(out_entry);

  return out_entry;
}

static int copy_data_stream(struct archive *in, struct archive *out,
                            ArchiveProgressCallback cb, void *user_data) {
  char *buf;
  int ret = ARCHIVE_OK;

  buf = xmalloc(COPY_BUF_SIZE);

  while (1) {
    ssize_t len = archive_read_data(in, buf, COPY_BUF_SIZE);
    if (len < 0) {
      ret = (int)len; /* Error code */
      break;
    }
    if (len == 0)
      break; /* EOF */

    if (archive_write_data(out, buf, len) < 0) {
      ret = ARCHIVE_FATAL;
      break;
    }
    if (cb && cb(ARCHIVE_STATUS_PROGRESS, NULL, user_data) == ARCHIVE_CB_ABORT) {
      ret = ARCHIVE_FATAL;
      break;
    }
  }
  free(buf);
  return ret;
}

/*
 * Common rewrite loop logic.
 */
static int process_rewrite_loop(struct archive *a_in, struct archive *a_out,
                                RewriteCallback rw_cb, void *rw_data,
                                ArchiveProgressCallback prog_cb,
                                void *prog_data, int *fd_tmp) {
  struct archive_entry *entry;
  int r, res;
  int success = 1; /* Assume success unless error occurs */
  int writer_initialized = 0;

  while ((r = archive_read_next_header(a_in, &entry)) == ARCHIVE_OK) {

    /* Lazy Initialization of Writer */
    if (!writer_initialized) {
      if (configure_writer(a_in, a_out) != 0) {
        REPORT_ERROR(prog_cb, prog_data,
                     "Unsupported archive format for writing");
        success = 0;
        break;
      }
      if (archive_write_open_fd(a_out, *fd_tmp) != ARCHIVE_OK) {
        REPORT_ERROR(prog_cb, prog_data,
                     "Could not open output archive stream");
        success = 0;
        break;
      }
      writer_initialized = 1;
    }

    if (prog_cb && prog_cb(ARCHIVE_STATUS_PROGRESS, NULL, prog_data) ==
                       ARCHIVE_CB_ABORT) {
      res = AR_ABORT;
    } else {
      res = rw_cb(a_in, a_out, entry, rw_data);
    }

    if (res == AR_ABORT) {
      success = 0;
      break;
    }
    if (res == AR_SKIP)
      continue;

    /* AR_KEEP: Write sanitized header and data */
    struct archive_entry *cloned = clone_entry_for_write(entry);
    if (!cloned) {
      REPORT_ERROR(prog_cb, prog_data, "Memory error cloning entry");
      success = 0;
      break;
    }

    if (archive_write_header(a_out, cloned) != ARCHIVE_OK) {
      REPORT_ERROR(prog_cb, prog_data, "Error writing archive header");
      archive_entry_free(cloned);
      success = 0;
      break;
    }
    archive_entry_free(cloned);

    if (archive_entry_size(entry) > 0) {
      if (copy_data_stream(a_in, a_out, prog_cb, prog_data) != ARCHIVE_OK) {
        REPORT_ERROR(prog_cb, prog_data, "Error writing archive data");
        success = 0;
        break;
      }
    }
  }

  if (success && !writer_initialized && r == ARCHIVE_EOF) {
    if (configure_writer(a_in, a_out) != 0) {
      REPORT_ERROR(prog_cb, prog_data,
                   "Unsupported archive format for writing");
      success = 0;
    } else if (archive_write_open_fd(a_out, *fd_tmp) != ARCHIVE_OK) {
      REPORT_ERROR(prog_cb, prog_data, "Could not open output archive stream");
      success = 0;
    } else {
      writer_initialized = 1;
    }
  }

  if (r != ARCHIVE_EOF && r != ARCHIVE_OK)
    success = 0;

  return (success && writer_initialized) ? 1 : 0;
}

static int finalize_rewrite(const char *orig_path, char *tmp_path, struct stat *st,
                            int success, ArchiveProgressCallback cb,
                            void *user_data) {
  if (success) {
    if (chmod(tmp_path, st->st_mode) != 0) {
    }
    if (rename(tmp_path, orig_path) != 0) {
      REPORT_ERROR(cb, user_data, "Failed to replace original archive");
      unlink(tmp_path);
      return -1;
    }
    return 0;
  } else {
    unlink(tmp_path);
    return -1;
  }
}

static int setup_rewrite(char *archive_path, char *tmp_path, struct stat *st,
                         int *fd_tmp, struct archive **a_in,
                         struct archive **a_out) {
  if (stat(archive_path, st) != 0)
    return -1;
  snprintf(tmp_path, PATH_LENGTH, "%s.tmpXXXXXX", archive_path);
  *fd_tmp = mkstemp(tmp_path);
  if (*fd_tmp == -1)
    return -1;

  *a_in = archive_read_new();
  archive_read_support_filter_all(*a_in);
  archive_read_support_format_all(*a_in);
  if (archive_read_open_filename(*a_in, archive_path, 10240) != ARCHIVE_OK) {
    archive_read_free(*a_in);
    close(*fd_tmp);
    unlink(tmp_path);
    return -1;
  }
  *a_out = archive_write_new();
  return 0;
}

int Archive_Rewrite(char *archive_path, RewriteCallback rw_cb, void *rw_data,
                    ArchiveProgressCallback cb, void *user_data) {
  struct archive *a_in = NULL, *a_out = NULL;
  struct stat st;
  char tmp_path[PATH_LENGTH];
  int fd_tmp = -1;
  int success;

  if (setup_rewrite(archive_path, tmp_path, &st, &fd_tmp, &a_in, &a_out) != 0) {
    REPORT_ERROR(cb, user_data, "Failed to setup rewrite engine");
    return -1;
  }

  success =
      process_rewrite_loop(a_in, a_out, rw_cb, rw_data, cb, user_data, &fd_tmp);

  archive_read_free(a_in);
  archive_write_close(a_out);
  archive_write_free(a_out);
  close(fd_tmp);

  return finalize_rewrite(archive_path, tmp_path, &st, success, cb, user_data);
}

/* Callback: Delete File */
static int cb_delete_file(struct archive *r, struct archive *w,
                          struct archive_entry *entry, void *user_data) {
  const char *target_path = (const char *)user_data;
  const char *current = archive_entry_pathname(entry);
  size_t t_len, c_len;
  (void)r;
  (void)w;

  if (current[0] == '.' && current[1] == FILE_SEPARATOR_CHAR)
    current += 2;
  while (*current == FILE_SEPARATOR_CHAR)
    current++;

  if (strcmp(target_path, current) == 0)
    return AR_SKIP;

  t_len = strlen(target_path);
  c_len = strlen(current);

  if (c_len > t_len && strncmp(current, target_path, t_len) == 0) {
    if (target_path[t_len - 1] == FILE_SEPARATOR_CHAR ||
        current[t_len] == FILE_SEPARATOR_CHAR) {
      return AR_SKIP;
    }
  }
  return AR_KEEP;
}

int Archive_DeleteEntry(char *archive_path, char *file_path,
                        ArchiveProgressCallback cb, void *user_data) {
  char internal_path[PATH_LENGTH];
  char canonical_path[PATH_LENGTH];
  size_t arch_len;
  size_t file_len;

  if (!archive_path || !file_path)
    return -1;
  if (!(Archive_ProbeCapabilities(archive_path) & ARCHIVE_CAP_DELETE)) {
    REPORT_ERROR(cb, user_data, "Archive does not support deletion");
    return -1;
  }

  arch_len = strlen(archive_path);
  file_len = strlen(file_path);

  if (file_len > arch_len && strncmp(file_path, archive_path, arch_len) == 0) {
    char *ptr = file_path + arch_len;
    while (*ptr == FILE_SEPARATOR_CHAR)
      ptr++;
    strncpy(internal_path, ptr, sizeof(internal_path));
    internal_path[sizeof(internal_path) - 1] = '\0';
  } else {
    strncpy(internal_path, file_path, sizeof(internal_path));
    internal_path[sizeof(internal_path) - 1] = '\0';
  }

  if (Archive_ValidateInternalPath(internal_path, canonical_path,
                                  sizeof(canonical_path)) != 0) {
    REPORT_ERROR(cb, user_data, "Unsafe archive internal path requested");
    return -1;
  }
  return Archive_Rewrite(archive_path, cb_delete_file, canonical_path, cb,
                         user_data);
}

/* Callback: Add (Skip collision) */
static int cb_add_skip(struct archive *r, struct archive *w,
                       struct archive_entry *entry, void *user_data) {
  const char *dest_name = (const char *)user_data;
  const char *curr = archive_entry_pathname(entry);
  (void)r;
  (void)w;

  if (strcmp(curr, dest_name) == 0)
    return AR_SKIP;
  const char *norm = curr;
  if (norm[0] == '.' && norm[1] == '/')
    norm += 2;
  if (strcmp(norm, dest_name) == 0)
    return AR_SKIP;

  return AR_KEEP;
}

static int ArchiveWriteFilesystemEntry(struct archive *writer,
                                       const char *src_path,
                                       const char *dest_name, BOOL is_dir,
                                       ArchiveProgressCallback cb,
                                       void *user_data) {
  struct archive_entry *entry;
  struct stat st;
  int src_fd = -1;
  int success = 0;

  if (is_dir && !src_path) {
    memset(&st, 0, sizeof(st));
    st.st_mode = 0755;
    st.st_mtime = time(NULL);
    st.st_uid = getuid();
    st.st_gid = getgid();
  } else if (!src_path || lstat(src_path, &st) != 0 ||
             (is_dir ? !S_ISDIR(st.st_mode) : !S_ISREG(st.st_mode))) {
    return -1;
  }
  if (!is_dir) {
    src_fd = open(src_path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (src_fd < 0 || fstat(src_fd, &st) != 0 || !S_ISREG(st.st_mode)) {
      if (src_fd >= 0)
        close(src_fd);
      return -1;
    }
  }

  entry = archive_entry_new();
  if (!entry) {
    if (src_fd >= 0)
      close(src_fd);
    return -1;
  }
  archive_entry_set_pathname(entry, dest_name);
  archive_entry_set_mtime(entry, st.st_mtime, 0);
  archive_entry_set_uid(entry, st.st_uid);
  archive_entry_set_gid(entry, st.st_gid);
  archive_entry_set_perm(entry, st.st_mode & 07777);
  archive_entry_set_filetype(entry, is_dir ? AE_IFDIR : AE_IFREG);
  archive_entry_set_size(entry, is_dir ? 0 : st.st_size);

  if (archive_write_header(writer, entry) == ARCHIVE_OK) {
    success = 1;
    if (cb && cb(ARCHIVE_STATUS_PROGRESS, NULL, user_data) == ARCHIVE_CB_ABORT)
      success = 0;
  }
  archive_entry_free(entry);

  if (success && !is_dir) {
    char *buf = xmalloc(COPY_BUF_SIZE);
    ssize_t len;

    while ((len = read(src_fd, buf, COPY_BUF_SIZE)) > 0) {
      if (archive_write_data(writer, buf, (size_t)len) < 0 ||
          (cb && cb(ARCHIVE_STATUS_PROGRESS, NULL, user_data) ==
                     ARCHIVE_CB_ABORT)) {
        success = 0;
        break;
      }
    }
    if (len < 0)
      success = 0;
    free(buf);
  }
  if (src_fd >= 0)
    close(src_fd);
  return success ? 0 : -1;
}

int Archive_AddFile(char *archive_path, const char *src_path,
                    const char *dest_name, BOOL is_dir,
                    ArchiveProgressCallback cb, void *user_data) {
  struct archive *a_in = NULL, *a_out = NULL;
  struct stat st;
  char tmp_path[PATH_LENGTH];
  int fd_tmp = -1;
  int success;
  char canonical_path[PATH_LENGTH];

  if (!(Archive_ProbeCapabilities(archive_path) & ARCHIVE_CAP_ADD)) {
    REPORT_ERROR(cb, user_data, "Archive does not support adding entries");
    return -1;
  }
  if (!dest_name || Archive_ValidateInternalPath(dest_name, canonical_path,
                                                 sizeof(canonical_path)) != 0) {
    REPORT_ERROR(cb, user_data, "Unsafe archive internal path requested");
    return -1;
  }
  if (setup_rewrite(archive_path, tmp_path, &st, &fd_tmp, &a_in, &a_out) != 0) {
    return -1;
  }

  success = process_rewrite_loop(a_in, a_out, cb_add_skip, canonical_path, cb,
                                 user_data, &fd_tmp);
  if (success &&
      ArchiveWriteFilesystemEntry(a_out, src_path, canonical_path, is_dir, cb,
                                  user_data) != 0) {
    REPORT_ERROR(cb, user_data, "Error writing new archive entry");
    success = 0;
  }

  archive_read_free(a_in);
  archive_write_close(a_out);
  archive_write_free(a_out);
  close(fd_tmp);

  return finalize_rewrite(archive_path, tmp_path, &st, success, cb, user_data);
}

/* Callback for Renaming */
static int cb_rename(struct archive *r, struct archive *w,
                     struct archive_entry *entry, void *user_data) {
  const RenameContext *ctx = (const RenameContext *)user_data;
  const char *current = archive_entry_pathname(entry);
  size_t old_len, curr_len;
  (void)r;
  (void)w;

  if (current[0] == '.' && current[1] == FILE_SEPARATOR_CHAR)
    current += 2;
  while (*current == FILE_SEPARATOR_CHAR)
    current++;

  old_len = strlen(ctx->old_path);
  curr_len = strlen(current);

  if (strncmp(current, ctx->old_path, old_len) == 0) {
    if (curr_len == old_len || current[old_len] == FILE_SEPARATOR_CHAR) {

      struct archive_entry *cloned = clone_entry_for_write(entry);
      char new_path[PATH_LENGTH];
      char parent_dir[PATH_LENGTH];
      const char *last_slash = strrchr(ctx->old_path, FILE_SEPARATOR_CHAR);
      int parent_len = 0;

      if (last_slash) {
        parent_len = last_slash - ctx->old_path;
        strncpy(parent_dir, ctx->old_path, parent_len);
        parent_dir[parent_len] = '\0';
      } else {
        parent_dir[0] = '\0';
      }

      const char *suffix = current + old_len;

      if (parent_len > 0) {
        snprintf(new_path, sizeof(new_path), "%s%c%s%s", parent_dir,
                 FILE_SEPARATOR_CHAR, ctx->new_name, suffix);
      } else {
        snprintf(new_path, sizeof(new_path), "%s%s", ctx->new_name, suffix);
      }

      archive_entry_set_pathname(cloned, new_path);

      if (archive_write_header(w, cloned) != ARCHIVE_OK) {
        archive_entry_free(cloned);
        return AR_ABORT;
      }
      archive_entry_free(cloned);

      if (archive_entry_size(entry) > 0) {
        if (copy_data_stream(r, w, ctx->progress_cb, ctx->progress_data) !=
            ARCHIVE_OK)
          return AR_ABORT;
      }

      return AR_SKIP;
    }
  }
  return AR_KEEP;
}

int Archive_RenameEntry(char *archive_path, char *old_path,
                        const char *new_name, ArchiveProgressCallback cb,
                        void *user_data) {
  char internal_old_path[PATH_LENGTH];
  char canonical_old_path[PATH_LENGTH];
  char canonical_new_path[PATH_LENGTH];
  size_t arch_len;
  size_t old_len;
  RenameContext ctx;

  if (!archive_path || !old_path || !new_name)
    return -1;
  if (!(Archive_ProbeCapabilities(archive_path) & ARCHIVE_CAP_RENAME)) {
    REPORT_ERROR(cb, user_data, "Archive does not support renaming entries");
    return -1;
  }

  arch_len = strlen(archive_path);
  old_len = strlen(old_path);

  if (old_len > arch_len && strncmp(old_path, archive_path, arch_len) == 0) {
    char *ptr = old_path + arch_len;
    while (*ptr == FILE_SEPARATOR_CHAR)
      ptr++;
    strncpy(internal_old_path, ptr, sizeof(internal_old_path));
    internal_old_path[sizeof(internal_old_path) - 1] = '\0';
  } else {
    strncpy(internal_old_path, old_path, sizeof(internal_old_path));
    internal_old_path[sizeof(internal_old_path) - 1] = '\0';
  }

  if (Archive_ValidateInternalPath(internal_old_path, canonical_old_path,
                                  sizeof(canonical_old_path)) != 0 ||
      Archive_ValidateInternalPath(new_name, canonical_new_path,
                                  sizeof(canonical_new_path)) != 0) {
    REPORT_ERROR(cb, user_data, "Unsafe archive internal path requested");
    return -1;
  }
  ctx.old_path = canonical_old_path;
  ctx.new_name = canonical_new_path;
  ctx.progress_cb = cb;
  ctx.progress_data = user_data;

  return Archive_Rewrite(archive_path, cb_rename, &ctx, cb, user_data);
}

typedef struct {
  const char *tree_path;
  size_t tree_len;
} ArchiveTreeDeleteContext;

static int cb_delete_tree(struct archive *in, struct archive *out,
                          struct archive_entry *entry, void *user_data) {
  const ArchiveTreeDeleteContext *ctx =
      (const ArchiveTreeDeleteContext *)user_data;
  char canonical_path[PATH_LENGTH + 1];
  const char *path = archive_entry_pathname(entry);

  (void)in;
  (void)out;
  if (!path || Archive_ValidateInternalPath(path, canonical_path,
                                            sizeof(canonical_path)) != 0)
    return AR_SKIP;
  if (strcmp(canonical_path, ctx->tree_path) == 0 ||
      (strncmp(canonical_path, ctx->tree_path, ctx->tree_len) == 0 &&
       canonical_path[ctx->tree_len] == FILE_SEPARATOR_CHAR))
    return AR_SKIP;
  return AR_KEEP;
}

int Archive_DeleteTree(char *archive_path, const char *tree_path,
                       ArchiveProgressCallback cb, void *user_data) {
  ArchiveTreeDeleteContext ctx;
  char canonical_path[PATH_LENGTH + 1];
  const char *internal_path = tree_path;

  if (!archive_path || !tree_path ||
      !(Archive_ProbeCapabilities(archive_path) & ARCHIVE_CAP_DELETE) ||
      (strncmp(tree_path, archive_path, strlen(archive_path)) == 0 &&
       tree_path[strlen(archive_path)] != '\0' &&
       tree_path[strlen(archive_path)] != FILE_SEPARATOR_CHAR)) {
    REPORT_ERROR(cb, user_data, "Archive does not support deleting this directory");
    return -1;
  }
  if (strncmp(tree_path, archive_path, strlen(archive_path)) == 0) {
    internal_path += strlen(archive_path);
    while (*internal_path == FILE_SEPARATOR_CHAR)
      internal_path++;
  }
  if (Archive_ValidateInternalPath(internal_path, canonical_path,
                                  sizeof(canonical_path)) != 0) {
    REPORT_ERROR(cb, user_data, "Unsafe archive internal path requested");
    return -1;
  }
  ctx.tree_path = canonical_path;
  ctx.tree_len = strlen(canonical_path);
  return Archive_Rewrite(archive_path, cb_delete_tree, &ctx, cb, user_data);
}

static int Archive_AddTreeRecursive(struct archive *writer,
                                    const char *src_path,
                                    const char *dest_path,
                                    ArchiveProgressCallback cb, void *user_data) {
  DIR *dir;
  const struct dirent *item;

  if (ArchiveWriteFilesystemEntry(writer, src_path, dest_path, TRUE, cb,
                                  user_data) != 0)
    return -1;
  dir = opendir(src_path);
  if (!dir)
    return -1;
  while ((item = readdir(dir)) != NULL) {
    char child_src[PATH_LENGTH + 1];
    char child_dest[PATH_LENGTH + 1];
    struct stat st;

    if (strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0)
      continue;
    if (Path_Join(child_src, sizeof(child_src), src_path, item->d_name) != 0 ||
        Path_Join(child_dest, sizeof(child_dest), dest_path, item->d_name) != 0 ||
        lstat(child_src, &st) != 0) {
      closedir(dir);
      return -1;
    }
    if (S_ISDIR(st.st_mode)) {
      if (Archive_AddTreeRecursive(writer, child_src, child_dest, cb,
                                   user_data) != 0) {
        closedir(dir);
        return -1;
      }
    } else if (S_ISREG(st.st_mode)) {
      if (ArchiveWriteFilesystemEntry(writer, child_src, child_dest, FALSE, cb,
                                      user_data) != 0) {
        closedir(dir);
        return -1;
      }
    } else {
      REPORT_ERROR(cb, user_data, "Unsupported directory member");
      closedir(dir);
      return -1;
    }
  }
  closedir(dir);
  return 0;
}

typedef struct {
  const char *dest_path;
  size_t dest_len;
  BOOL collision;
} ArchiveTreeAddContext;

static int cb_add_tree(struct archive *reader, struct archive *writer,
                       struct archive_entry *entry, void *user_data) {
  ArchiveTreeAddContext *ctx = (ArchiveTreeAddContext *)user_data;
  char canonical_path[PATH_LENGTH + 1];
  const char *entry_path = archive_entry_pathname(entry);
  (void)reader;
  (void)writer;

  if (entry_path &&
      Archive_ValidateInternalPath(entry_path, canonical_path,
                                   sizeof(canonical_path)) == 0 &&
      (strcmp(canonical_path, ctx->dest_path) == 0 ||
       (strncmp(canonical_path, ctx->dest_path, ctx->dest_len) == 0 &&
        canonical_path[ctx->dest_len] == FILE_SEPARATOR_CHAR))) {
    ctx->collision = TRUE;
    return AR_ABORT;
  }
  return AR_KEEP;
}

int Archive_AddTree(char *archive_path, const char *src_path,
                    const char *dest_path, ArchiveProgressCallback cb,
                    void *user_data) {
  struct archive *a_in = NULL, *a_out = NULL;
  ArchiveTreeAddContext add_ctx;
  struct stat archive_st;
  struct stat source_st;
  char canonical_path[PATH_LENGTH + 1];
  char tmp_path[PATH_LENGTH];
  int fd_tmp = -1;
  int success;

  if (!archive_path || !src_path || !dest_path ||
      !(Archive_ProbeCapabilities(archive_path) & ARCHIVE_CAP_ADD) ||
      Archive_ValidateInternalPath(dest_path, canonical_path,
                                   sizeof(canonical_path)) != 0 ||
      lstat(src_path, &source_st) != 0 || !S_ISDIR(source_st.st_mode)) {
    REPORT_ERROR(cb, user_data, "Archive does not support adding this directory");
    return -1;
  }
  if (setup_rewrite(archive_path, tmp_path, &archive_st, &fd_tmp, &a_in,
                    &a_out) != 0) {
    REPORT_ERROR(cb, user_data, "Failed to setup directory archive transfer");
    return -1;
  }

  add_ctx.dest_path = canonical_path;
  add_ctx.dest_len = strlen(canonical_path);
  add_ctx.collision = FALSE;
  success = process_rewrite_loop(a_in, a_out, cb_add_tree, &add_ctx, cb,
                                 user_data, &fd_tmp);
  if (add_ctx.collision) {
    REPORT_ERROR(cb, user_data,
                 "Archive destination already contains this directory");
    success = 0;
  } else if (success &&
             Archive_AddTreeRecursive(a_out, src_path, canonical_path, cb,
                                      user_data) != 0) {
    REPORT_ERROR(cb, user_data, "Error writing directory archive entries");
    success = 0;
  }

  archive_read_free(a_in);
  archive_write_close(a_out);
  archive_write_free(a_out);
  close(fd_tmp);
  return finalize_rewrite(archive_path, tmp_path, &archive_st, success, cb,
                          user_data);
}

#else
/* Dummy implementations if libarchive is not available */
int Archive_Rewrite(char *archive_path, void *cb, void *user_data,
                    ArchiveProgressCallback pcb, void *pdata) {
  return -1;
}
int Archive_CreateFromPaths(const char *dest_path,
                            const char *const *source_paths,
                            const char *const *archive_paths,
                            size_t source_count) {
  return -1;
}
int Archive_DeleteEntry(char *archive_path, char *file_path,
                        ArchiveProgressCallback cb, void *user_data) {
  return -1;
}
int Archive_AddFile(char *archive_path, const char *src_path,
                    const char *dest_name, BOOL is_dir,
                    ArchiveProgressCallback cb, void *user_data) {
  return -1;
}
int Archive_RenameEntry(char *archive_path, char *old_path,
                        const char *new_name,
                        ArchiveProgressCallback cb, void *user_data) {
  return -1;
}
#endif /* HAVE_LIBARCHIVE */
