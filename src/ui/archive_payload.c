/***************************************************************************
 *
 * src/ui/archive_payload.c
 * Archive payload collection and traversal helpers
 *
 ***************************************************************************/

#include "ytnova_ui.h"

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct _VisitedDirNode {
  dev_t dev;
  ino_t ino;
  struct _VisitedDirNode *next;
} VisitedDirNode;

static char *duplicate_string(const char *src) {
  size_t len;
  char *dup;

  if (!src)
    return NULL;

  len = strlen(src);
  dup = (char *)malloc(len + 1);
  if (!dup)
    return NULL;

  memcpy(dup, src, len + 1);
  return dup;
}

static void free_path_list_local(PathList *list) {
  PathList *next;

  while (list) {
    next = list->next;
    free(list->path);
    free(list);
    list = next;
  }
}

static void free_expanded_list(ArchiveExpandedEntry *list) {
  ArchiveExpandedEntry *next;

  while (list) {
    next = list->next;
    free(list->source_path);
    free(list->archive_path);
    free(list);
    list = next;
  }
}

void UI_FreeArchivePayload(ArchivePayload *payload) {
  if (!payload)
    return;

  free_path_list_local(payload->original_source_list);
  free_expanded_list(payload->expanded_file_list);
  payload->original_source_list = NULL;
  payload->expanded_file_list = NULL;
}

static int append_original_path(PathList **head, const char *path) {
  PathList *node;
  PathList *tail;

  if (!head || !path)
    return -1;

  node = (PathList *)malloc(sizeof(*node));
  if (!node)
    return -1;

  node->path = duplicate_string(path);
  if (!node->path) {
    free(node);
    return -1;
  }
  node->next = NULL;

  if (!*head) {
    *head = node;
    return 0;
  }

  tail = *head;
  while (tail->next)
    tail = tail->next;
  tail->next = node;
  return 0;
}

static int append_expanded_entry(ArchiveExpandedEntry **head,
                                 const char *source_path,
                                 const char *archive_path) {
  ArchiveExpandedEntry *node;
  ArchiveExpandedEntry *tail;

  if (!head || !source_path || !archive_path)
    return -1;

  node = (ArchiveExpandedEntry *)malloc(sizeof(*node));
  if (!node)
    return -1;

  node->source_path = duplicate_string(source_path);
  node->archive_path = duplicate_string(archive_path);
  if (!node->source_path || !node->archive_path) {
    free(node->source_path);
    free(node->archive_path);
    free(node);
    return -1;
  }
  node->next = NULL;

  if (!*head) {
    *head = node;
    return 0;
  }

  tail = *head;
  while (tail->next)
    tail = tail->next;
  tail->next = node;
  return 0;
}

static int parent_dir_of_path(const char *path, char *parent_path,
                              size_t parent_path_size) {
  char work[PATH_LENGTH + 1];
  size_t len;
  char *slash;
  int written;

  if (!path || !parent_path || parent_path_size == 0)
    return -1;

  written = snprintf(work, sizeof(work), "%s", path);
  if (written < 0 || (size_t)written >= sizeof(work))
    return -1;

  len = strlen(work);
  while (len > 1 && work[len - 1] == FILE_SEPARATOR_CHAR) {
    work[len - 1] = '\0';
    len--;
  }

  slash = strrchr(work, FILE_SEPARATOR_CHAR);
  if (!slash)
    return -1;

  if (slash == work) {
    written = snprintf(parent_path, parent_path_size, "%c", FILE_SEPARATOR_CHAR);
  } else {
    *slash = '\0';
    written = snprintf(parent_path, parent_path_size, "%s", work);
  }

  if (written < 0 || (size_t)written >= parent_path_size)
    return -1;

  return 0;
}

static BOOL path_is_prefix(const char *prefix, const char *path) {
  size_t prefix_len;

  if (!prefix || !path)
    return FALSE;

  if (strcmp(prefix, FILE_SEPARATOR_STRING) == 0)
    return path[0] == FILE_SEPARATOR_CHAR;

  prefix_len = strlen(prefix);
  if (strncmp(prefix, path, prefix_len) != 0)
    return FALSE;

  return path[prefix_len] == '\0' || path[prefix_len] == FILE_SEPARATOR_CHAR;
}

static void strip_last_path_component(char *path) {
  char *slash;

  if (!path || path[0] == '\0' || strcmp(path, FILE_SEPARATOR_STRING) == 0)
    return;

  slash = strrchr(path, FILE_SEPARATOR_CHAR);
  if (!slash)
    return;

  if (slash == path) {
    path[1] = '\0';
  } else {
    *slash = '\0';
  }
}

static int compute_common_file_base(const char *const *source_paths,
                                    const BOOL *is_directory,
                                    size_t source_count,
                                    char *common_base,
                                    size_t common_base_size,
                                    BOOL *has_files) {
  size_t i;
  BOOL seen_file = FALSE;

  if (!source_paths || !is_directory || !common_base || !has_files ||
      common_base_size == 0) {
    return -1;
  }

  common_base[0] = '\0';
  *has_files = FALSE;

  for (i = 0; i < source_count; ++i) {
    char parent_path[PATH_LENGTH + 1];
    int written;

    if (is_directory[i])
      continue;

    if (parent_dir_of_path(source_paths[i], parent_path, sizeof(parent_path)) != 0)
      return -1;

    if (!seen_file) {
      written = snprintf(common_base, common_base_size, "%s", parent_path);
      if (written < 0 || (size_t)written >= common_base_size)
        return -1;
      seen_file = TRUE;
      continue;
    }

    while (common_base[0] != '\0' && !path_is_prefix(common_base, parent_path))
      strip_last_path_component(common_base);

    if (common_base[0] == '\0') {
      written = snprintf(common_base, common_base_size, "%c", FILE_SEPARATOR_CHAR);
      if (written < 0 || (size_t)written >= common_base_size)
        return -1;
      break;
    }
  }

  *has_files = seen_file;
  return 0;
}

static int build_relative_path(const char *base_path, const char *full_path,
                               char *relative_path, size_t relative_path_size) {
  const char *start;
  int written;

  if (!base_path || !full_path || !relative_path || relative_path_size == 0)
    return -1;

  if (strcmp(base_path, FILE_SEPARATOR_STRING) == 0) {
    start = full_path;
    while (*start == FILE_SEPARATOR_CHAR)
      start++;
  } else {
    size_t base_len = strlen(base_path);
    if (strncmp(full_path, base_path, base_len) != 0)
      return -1;

    start = full_path + base_len;
    if (*start == FILE_SEPARATOR_CHAR) {
      start++;
    } else if (*start != '\0') {
      return -1;
    }
  }

  written = snprintf(relative_path, relative_path_size, "%s", start);
  if (written < 0 || (size_t)written >= relative_path_size)
    return -1;

  return 0;
}

static BOOL visited_contains(const VisitedDirNode *visited,
                             dev_t dev,
                             ino_t ino) {
  const VisitedDirNode *node = visited;

  while (node) {
    if (node->dev == dev && node->ino == ino)
      return TRUE;
    node = node->next;
  }

  return FALSE;
}

static int visited_add(VisitedDirNode **visited, dev_t dev, ino_t ino) {
  VisitedDirNode *node;

  if (!visited)
    return -1;

  node = (VisitedDirNode *)malloc(sizeof(*node));
  if (!node)
    return -1;

  node->dev = dev;
  node->ino = ino;
  node->next = *visited;
  *visited = node;
  return 0;
}

static void free_visited(VisitedDirNode *visited) {
  VisitedDirNode *next;

  while (visited) {
    next = visited->next;
    free(visited);
    visited = next;
  }
}

static int expand_directory_path(const char *root_dir,
                                 const char *current_dir,
                                 ArchiveExpandedEntry **expanded_list,
                                 VisitedDirNode **visited,
                                 ArchiveProgressCallback cb, void *user_data) {
  DIR *dir;
  const struct dirent *entry;
  struct stat st;
  int rc = 0;

  if (lstat(current_dir, &st) != 0 || !S_ISDIR(st.st_mode))
    return -1;

  if (visited_contains(*visited, st.st_dev, st.st_ino))
    return 0;

  if (visited_add(visited, st.st_dev, st.st_ino) != 0)
    return -1;

  dir = opendir(current_dir);
  if (!dir)
    return -1;

  while ((entry = readdir(dir)) != NULL) {
    char child_path[PATH_LENGTH + 1];
    struct stat child_st;
    int written;

    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    if (strcmp(current_dir, FILE_SEPARATOR_STRING) == 0) {
      written = snprintf(child_path, sizeof(child_path), "%s%s", current_dir,
                         entry->d_name);
    } else {
      written = snprintf(child_path, sizeof(child_path), "%s%c%s", current_dir,
                         FILE_SEPARATOR_CHAR, entry->d_name);
    }

    if (written < 0 || (size_t)written >= sizeof(child_path)) {
      rc = -1;
      break;
    }

    if (lstat(child_path, &child_st) != 0) {
      rc = -1;
      break;
    }

    if (cb && cb(ARCHIVE_STATUS_PROGRESS, NULL, 0, 1, user_data) ==
                  ARCHIVE_CB_ABORT) {
      rc = -1;
      break;
    }

    if (S_ISDIR(child_st.st_mode)) {
      rc = expand_directory_path(root_dir, child_path, expanded_list, visited,
                                 cb, user_data);
      if (rc != 0)
        break;
      continue;
    }

    {
      char relative_path[PATH_LENGTH + 1];

      if (build_relative_path(root_dir, child_path, relative_path,
                              sizeof(relative_path)) != 0 ||
          relative_path[0] == '\0') {
        rc = -1;
        break;
      }

      if (append_expanded_entry(expanded_list, child_path, relative_path) != 0) {
        rc = -1;
        break;
      }
    }
  }

  if (closedir(dir) != 0 && rc == 0)
    rc = -1;

  return rc;
}

static int expand_directory_path_non_recursive(const char *root_dir,
                                               ArchiveExpandedEntry **expanded_list,
                                               VisitedDirNode **visited,
                                               ArchiveProgressCallback cb,
                                               void *user_data) {
  DIR *dir;
  const struct dirent *entry;
  struct stat st;
  int rc = 0;

  if (lstat(root_dir, &st) != 0 || !S_ISDIR(st.st_mode))
    return -1;

  if (visited_contains(*visited, st.st_dev, st.st_ino))
    return 0;

  if (visited_add(visited, st.st_dev, st.st_ino) != 0)
    return -1;

  dir = opendir(root_dir);
  if (!dir)
    return -1;

  while ((entry = readdir(dir)) != NULL) {
    char child_path[PATH_LENGTH + 1];
    struct stat child_st;
    int written;

    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    if (strcmp(root_dir, FILE_SEPARATOR_STRING) == 0) {
      written = snprintf(child_path, sizeof(child_path), "%s%s", root_dir,
                         entry->d_name);
    } else {
      written = snprintf(child_path, sizeof(child_path), "%s%c%s", root_dir,
                         FILE_SEPARATOR_CHAR, entry->d_name);
    }

    if (written < 0 || (size_t)written >= sizeof(child_path)) {
      rc = -1;
      break;
    }

    if (lstat(child_path, &child_st) != 0) {
      rc = -1;
      break;
    }

    if (cb && cb(ARCHIVE_STATUS_PROGRESS, NULL, 0, 1, user_data) ==
                  ARCHIVE_CB_ABORT) {
      rc = -1;
      break;
    }

    if (S_ISDIR(child_st.st_mode))
      continue;

    {
      char relative_path[PATH_LENGTH + 1];

      if (build_relative_path(root_dir, child_path, relative_path,
                              sizeof(relative_path)) != 0 ||
          relative_path[0] == '\0') {
        rc = -1;
        break;
      }

      if (append_expanded_entry(expanded_list, child_path, relative_path) != 0) {
        rc = -1;
        break;
      }
    }
  }

  if (closedir(dir) != 0 && rc == 0)
    rc = -1;

  return rc;
}

int UI_BuildArchivePayloadFromPathsWithProgress(
    const char *const *source_paths, size_t source_count,
    BOOL recursive_directories, ArchivePayload *payload,
    ArchiveProgressCallback cb, void *user_data) {
  size_t i;
  BOOL *is_directory = NULL;
  char common_file_base[PATH_LENGTH + 1];
  BOOL has_file_base = FALSE;

  if (!payload)
    return -1;

  UI_FreeArchivePayload(payload);

  if (!source_paths || source_count == 0)
    return -1;

  is_directory = (BOOL *)calloc(source_count, sizeof(*is_directory));
  if (!is_directory)
    return -1;

  for (i = 0; i < source_count; ++i) {
    struct stat st;

    if (!source_paths[i] || source_paths[i][0] == '\0')
      goto fail;

    if (append_original_path(&payload->original_source_list, source_paths[i]) != 0)
      goto fail;

    if (lstat(source_paths[i], &st) != 0)
      goto fail;

    is_directory[i] = S_ISDIR(st.st_mode) ? TRUE : FALSE;
    if (cb && cb(ARCHIVE_STATUS_PROGRESS, NULL, 0, 1, user_data) ==
                  ARCHIVE_CB_ABORT)
      goto fail;
  }

  if (compute_common_file_base(source_paths, is_directory, source_count,
                               common_file_base, sizeof(common_file_base),
                               &has_file_base) != 0) {
    goto fail;
  }

  for (i = 0; i < source_count; ++i) {
    if (is_directory[i]) {
      VisitedDirNode *visited = NULL;
      int expand_rc;
      if (recursive_directories) {
        expand_rc =
            expand_directory_path(source_paths[i], source_paths[i],
                                  &payload->expanded_file_list, &visited, cb,
                                  user_data);
      } else {
        expand_rc = expand_directory_path_non_recursive(
            source_paths[i], &payload->expanded_file_list, &visited, cb,
            user_data);
      }
      free_visited(visited);
      if (expand_rc != 0)
        goto fail;
    } else {
      char relative_path[PATH_LENGTH + 1];
      const char *base_path = has_file_base ? common_file_base : FILE_SEPARATOR_STRING;

      if (build_relative_path(base_path, source_paths[i], relative_path,
                              sizeof(relative_path)) != 0) {
        goto fail;
      }

      if (relative_path[0] == '\0') {
        const char *leaf = strrchr(source_paths[i], FILE_SEPARATOR_CHAR);
        int written = snprintf(relative_path, sizeof(relative_path), "%s",
                               leaf ? leaf + 1 : source_paths[i]);
        if (written < 0 || (size_t)written >= sizeof(relative_path))
          goto fail;
      }

      if (append_expanded_entry(&payload->expanded_file_list, source_paths[i],
                                relative_path) != 0) {
        goto fail;
      }
    }
  }

  free(is_directory);
  return 0;

fail:
  free(is_directory);
  UI_FreeArchivePayload(payload);
  return -1;
}

int UI_BuildArchivePayloadFromPaths(const char *const *source_paths,
                                    size_t source_count,
                                    BOOL recursive_directories,
                                    ArchivePayload *payload) {
  return UI_BuildArchivePayloadFromPathsWithProgress(
      source_paths, source_count, recursive_directories, payload, NULL, NULL);
}
