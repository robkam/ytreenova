#define _GNU_SOURCE
#include <archive.h>
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static unsigned long archive_block_count;

static void GateArchiveRead(void) {
  const char *gate_path;
  const char *marker_path;
  const char *arm_path;
  char marker[32];
  char token;
  int gate_fd;
  int marker_fd;
  int written;

  arm_path = getenv("YTNOVA_TEST_ARCHIVE_BLOCK_ARM");
  if (!arm_path || access(arm_path, F_OK) != 0)
    return;
  archive_block_count++;
  marker_path = getenv("YTNOVA_TEST_ARCHIVE_BLOCK_MARKER");
  gate_path = getenv("YTNOVA_TEST_ARCHIVE_BLOCK_GATE");
  if (!marker_path || !gate_path)
    return;
  marker_fd = open(marker_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  written = snprintf(marker, sizeof(marker), "%lu", archive_block_count);
  if (marker_fd >= 0 && written > 0 && (size_t)written < sizeof(marker))
    (void)write(marker_fd, marker, (size_t)written);
  if (marker_fd >= 0)
    (void)close(marker_fd);

  gate_fd = open(gate_path, O_RDONLY);
  if (gate_fd >= 0) {
    (void)read(gate_fd, &token, 1);
    (void)close(gate_fd);
  }
}

static void GateScannedDirectory(DIR *dirp, const char *entry_name) {
  const char *target;
  char fd_path[64];
  char resolved[4097];
  int directory_fd;
  int written;
  ssize_t resolved_len;

  target = getenv("YTNOVA_TEST_FS_SCAN_TARGET");
  if (!entry_name || !target || !target[0] || strcmp(entry_name, ".") == 0 ||
      strcmp(entry_name, "..") == 0)
    return;

  directory_fd = dirfd(dirp);
  written = snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", directory_fd);
  if (written < 0 || (size_t)written >= sizeof(fd_path))
    return;
  resolved_len = readlink(fd_path, resolved, sizeof(resolved) - 1);
  if (resolved_len < 0)
    return;
  resolved[resolved_len] = '\0';
  if (strcmp(resolved, target) == 0)
    GateArchiveRead();
}

struct dirent *readdir(DIR *dirp) {
  static struct dirent *(*real_readdir)(DIR *);
  struct dirent *result;

  if (!real_readdir)
    real_readdir = dlsym(RTLD_NEXT, "readdir");
  if (!real_readdir)
    return NULL;
  result = real_readdir(dirp);
  if (result)
    GateScannedDirectory(dirp, result->d_name);
  return result;
}

struct dirent64 *readdir64(DIR *dirp) {
  static struct dirent64 *(*real_readdir64)(DIR *);
  struct dirent64 *result;

  if (!real_readdir64)
    real_readdir64 = dlsym(RTLD_NEXT, "readdir64");
  if (!real_readdir64)
    return NULL;
  result = real_readdir64(dirp);
  if (result)
    GateScannedDirectory(dirp, result->d_name);
  return result;
}

int archive_read_data_block(struct archive *archive, const void **buffer,
                            size_t *size, la_int64_t *offset) {
  static int (*real_read_data_block)(struct archive *, const void **, size_t *,
                                     la_int64_t *);
  int result;

  if (!real_read_data_block)
    real_read_data_block = dlsym(RTLD_NEXT, "archive_read_data_block");
  if (!real_read_data_block)
    return ARCHIVE_FATAL;
  result = real_read_data_block(archive, buffer, size, offset);
  if (result != ARCHIVE_OK) {
    return result;
  }

  GateArchiveRead();
  return result;
}

la_ssize_t archive_read_data(struct archive *archive, void *buffer,
                             size_t size) {
  static la_ssize_t (*real_read_data)(struct archive *, void *, size_t);
  la_ssize_t result;

  if (!real_read_data)
    real_read_data = dlsym(RTLD_NEXT, "archive_read_data");
  if (!real_read_data)
    return ARCHIVE_FATAL;
  result = real_read_data(archive, buffer, size);
  if (result > 0)
    GateArchiveRead();
  return result;
}

ssize_t read(int fd, void *buffer, size_t size) {
  static ssize_t (*real_read)(int, void *, size_t);
  const char *target;
  char fd_path[64];
  char resolved[4097];
  ssize_t result;
  ssize_t resolved_len;

  if (!real_read)
    real_read = dlsym(RTLD_NEXT, "read");
  if (!real_read)
    return -1;
  result = real_read(fd, buffer, size);
  target = getenv("YTNOVA_TEST_FS_BLOCK_TARGET");
  if (result <= 0 || !target || !target[0])
    return result;
  if (snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", fd) < 0)
    return result;
  resolved_len = readlink(fd_path, resolved, sizeof(resolved) - 1);
  if (resolved_len < 0)
    return result;
  resolved[resolved_len] = '\0';
  if (strcmp(resolved, target) == 0)
    GateArchiveRead();
  return result;
}

ssize_t write(int fd, const void *buffer, size_t size) {
  static ssize_t (*real_write)(int, const void *, size_t);
  const char *target;
  char fd_path[64];
  char resolved[4097];
  ssize_t result;
  ssize_t resolved_len;

  if (!real_write)
    real_write = dlsym(RTLD_NEXT, "write");
  if (!real_write)
    return -1;
  result = real_write(fd, buffer, size);
  target = getenv("YTNOVA_TEST_FS_BLOCK_TARGET");
  if (result <= 0 || !target || !target[0])
    return result;
  if (snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", fd) < 0)
    return result;
  resolved_len = readlink(fd_path, resolved, sizeof(resolved) - 1);
  if (resolved_len < 0)
    return result;
  resolved[resolved_len] = '\0';
  if (strcmp(resolved, target) == 0)
    GateArchiveRead();
  return result;
}
