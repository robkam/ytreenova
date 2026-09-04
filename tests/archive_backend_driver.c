#include "ytnova.h"
#include "ytnova_fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *xmalloc(size_t size) {
  void *ptr = malloc(size);
  if (!ptr && size > 0) {
    fprintf(stderr, "xmalloc failed\n");
    exit(1);
  }
  return ptr;
}

/* The creation driver links only archive_write.c; these mutation-only seams
 * are not exercised by its Archive_CreateFromPaths contract. */
int Archive_ValidateInternalPath(const char *path, char *canonical_path,
                                 size_t canonical_size) {
  if (!path || !path[0] || path[0] == '/' || strstr(path, "..") ||
      strlen(path) >= canonical_size)
    return -1;
  (void)snprintf(canonical_path, canonical_size, "%s", path);
  return 0;
}

unsigned int Archive_ProbeCapabilities(const char *archive_path) {
  (void)archive_path;
  return ARCHIVE_CAP_ADD;
}

int Path_Join(char *dest, size_t size, const char *dir, const char *leaf) {
  int written = snprintf(dest, size, "%s/%s", dir, leaf);
  return (written < 0 || (size_t)written >= size) ? -1 : 0;
}

static int progress_count;

static int CountProgress(int status, const char *message, void *user_data) {
  (void)message;
  (void)user_data;
  if (status == ARCHIVE_STATUS_PROGRESS)
    progress_count++;
  return ARCHIVE_CB_CONTINUE;
}

int main(int argc, char **argv) {
  const char *dest_path;
  const char *const *source_paths;
  size_t source_count;
  int rc;

  if (argc == 5 && strcmp(argv[1], "--add-tree") == 0) {
    rc = Archive_AddTree(argv[2], argv[3], argv[4], CountProgress, NULL);
    printf("%d %d\n", rc, progress_count);
    return 0;
  }
  if (argc == 4 && strcmp(argv[1], "--add-dir") == 0) {
    rc = Archive_AddFile(argv[2], NULL, argv[3], TRUE, CountProgress, NULL);
    printf("%d %d\n", rc, progress_count);
    return 0;
  }

  if (argc < 2) {
    fprintf(stderr, "usage: %s <dest_path> [source_path ...]\n", argv[0]);
    return 2;
  }

  dest_path = argv[1];
  source_paths = (const char *const *)(argv + 2);
  source_count = (size_t)((argc > 2) ? (argc - 2) : 0);

  rc = Archive_CreateFromPaths(dest_path, source_paths, NULL, source_count);
  printf("%d\n", rc);
  return 0;
}
