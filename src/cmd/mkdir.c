/***************************************************************************
 *
 * src/cmd/mkdir.c
 * Creating directories
 *
 ***************************************************************************/

#include "ytnova_cmd.h"
#include "ytnova_appstate_focus.h"
#include "ytnova_appstate_panel.h"
#include "ytnova_appstate_volume.h"
#include "ytnova_appstate_visibility.h"
#include "ytnova_fs.h"
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FILE_SEPARATOR_CHAR '/'
#define FILE_SEPARATOR_STRING "/"
#define ESCAPE goto FNC_XIT

#define DISK_MODE 0
#define ARCHIVE_MODE 2
#define USER_MODE 3

static DirEntry *MakeDirEntry(const ViewContext *ctx, YtreeNovaPanel *panel,
                              DirEntry *father_dir_entry, const char *dir_name,
                              Statistic *s);
static DirEntry *MakeArchiveDirEntry(const ViewContext *ctx,
                                     YtreeNovaPanel *panel,
                                     DirEntry *father_dir_entry,
                                     const char *dir_name, Statistic *s,
                                     const char *parent_path);

/* Helper for Archive Callback */
static int ArchiveUICallback(int status, const char *msg, void *user_data) {
  ViewContext *ctx = (ViewContext *)user_data;

  if (status == ARCHIVE_STATUS_PROGRESS && ctx && ctx->hook_draw_spinner &&
      Progress_ShouldRender(ctx))
    ctx->hook_draw_spinner(ctx);
  (void)status;
  (void)msg;
  /* Archive callbacks stay non-interactive in this flow. */
  return ARCHIVE_CB_CONTINUE;
}

int MakeDirectory(const ViewContext *ctx, YtreeNovaPanel *panel,
                  DirEntry *father_dir_entry, const char *dir_name,
                  Statistic *s) {
  int result = -1;

  if (!dir_name || !*dir_name)
    return -1;
  if (panel && panel->vol && panel->vol->vol_stats.log_mode == ARCHIVE_MODE &&
      !(panel->vol->vol_stats.archive_capabilities & ARCHIVE_CAP_ADD))
    return -1;

  if (MakeDirEntry(ctx, panel, father_dir_entry, dir_name, s) != NULL) {
    result = 0;
  }

  return (result);
}

static DirEntry *MakeArchiveDirEntry(const ViewContext *ctx,
                                     YtreeNovaPanel *panel,
                                     DirEntry *father_dir_entry,
                                     const char *dir_name, Statistic *s,
                                     const char *parent_path) {
#ifdef HAVE_LIBARCHIVE
  char root_path[PATH_LENGTH + 1];
  char relative_path[PATH_LENGTH + 1];
  char archive_path[PATH_LENGTH + 1];
  char archive_dir_path[PATH_LENGTH + 1];
  struct stat archive_stat;

  {
    int n = snprintf(root_path, sizeof(root_path), "%s",
                     panel->vol->vol_stats.log_path);
    if (n < 0 || n >= (int)sizeof(root_path)) {
      return NULL;
    }
  }

  if (strcmp(parent_path, root_path) == 0) {
    relative_path[0] = '\0';
  } else if (strncmp(parent_path, root_path, strlen(root_path)) == 0) {
    char *ptr = (char *)parent_path + strlen(root_path);
    if (*ptr == FILE_SEPARATOR_CHAR)
      ptr++;
    if (snprintf(relative_path, sizeof(relative_path), "%s", ptr) < 0) {
      return NULL;
    }
  } else {
    if (snprintf(relative_path, sizeof(relative_path), "%s", parent_path) < 0) {
      return NULL;
    }
  }

  if (Path_Join(archive_path, sizeof(archive_path), relative_path, dir_name) !=
      0) {
    return NULL;
  }

  if (ctx && ctx->hook_draw_spinner)
    ctx->hook_draw_spinner((ViewContext *)ctx);

  if (Archive_AddFile(panel->vol->vol_stats.log_path, NULL, archive_path, TRUE,
                      ArchiveUICallback, (void *)ctx) != 0) {
    return NULL;
  }

  (void)memset(&archive_stat, 0, sizeof(archive_stat));
  archive_stat.st_mode = S_IFDIR;
  if (snprintf(archive_dir_path, sizeof(archive_dir_path), "%s/", archive_path) <
          0 ||
      TryInsertArchiveDirEntry((ViewContext *)ctx, panel->vol->vol_stats.tree,
                               archive_dir_path, &archive_stat, s) != 0) {
    return NULL;
  }

  return father_dir_entry;
#else
  (void)ctx;
  (void)panel;
  (void)father_dir_entry;
  (void)dir_name;
  (void)s;
  (void)parent_path;
  return NULL;
#endif
}

static DirEntry *MakeDirEntry(const ViewContext *ctx, YtreeNovaPanel *panel,
                              DirEntry *father_dir_entry, const char *dir_name,
                              Statistic *s) {
  DirEntry *den_ptr = NULL, *des_ptr;
  char parent_path[PATH_LENGTH + 1];
  char buffer[PATH_LENGTH + 1];
  struct stat stat_struct;

  /* Pre-check: Does a directory with this name (case-insensitive) already exist
   * in the tree? */
  for (des_ptr = father_dir_entry->sub_tree; des_ptr; des_ptr = des_ptr->next) {
    if (strcasecmp(des_ptr->name, dir_name) == 0) {
      /* Found it! Return the existing node */
      return des_ptr;
    }
  }

  (void)GetPath(father_dir_entry, parent_path);
  if (Path_Join(buffer, sizeof(buffer), parent_path, dir_name) != 0)
    return NULL;

/* ARCHIVE MODE HANDLER */
#ifdef HAVE_LIBARCHIVE
  if (panel && panel->vol && panel->vol->vol_stats.log_mode == ARCHIVE_MODE) {
    return MakeArchiveDirEntry(ctx, panel, father_dir_entry, dir_name, s,
                               parent_path);
  }
#endif

  if (mkdir(buffer, (S_IREAD | S_IWRITE | S_IEXEC | S_IRGRP | S_IWGRP |
                     S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH) &
                        ~ctx->user_umask)) {
    /* Modified Logic: Allow existing directories if they are valid. */
    if (errno == EEXIST) {
      if (STAT_(buffer, &stat_struct) == 0 && S_ISDIR(stat_struct.st_mode)) {
        /* It exists and is a directory. Fall through to creation logic. */
        goto CREATE_NODE;
      }
    }

    return NULL;
  } else {
  CREATE_NODE:
    /* Directory created
     * ==> link into tree
     */

    /* FIX: Added +1 to allocation for null terminator */
    den_ptr = (DirEntry *)xcalloc(1, sizeof(DirEntry) + strlen(dir_name) + 1);

    den_ptr->next = NULL;
    den_ptr->prev = NULL;
    den_ptr->sub_tree = NULL;
    if (!AppStateResetDirEntryPayloadCache(den_ptr)) {
      free(den_ptr);
      return NULL;
    }
    if (!AppStateCommitDirEntryFileViewport(den_ptr, 0, 0) ||
        !AppStateCommitDirEntryGlobalFilter(den_ptr, FALSE, FALSE) ||
        !AppStateCommitDirEntryTaggedFilter(den_ptr, FALSE) ||
        !AppStateCommitDirEntryFileShape(den_ptr, FALSE)) {
      free(den_ptr);
      return NULL;
    }
    den_ptr->up_tree = father_dir_entry;
    if (!AppStateCommitDirEntryLoggedState(den_ptr, FALSE, FALSE)) {
      free(den_ptr);
      return NULL;
    }

    if (s)
      s->disk_total_directories++;

    {
      size_t name_capacity = strlen(dir_name) + 1;
      int written = snprintf(den_ptr->name, name_capacity, "%s", dir_name);
      if (written < 0 || (size_t)written >= name_capacity) {
        free(den_ptr);
        return NULL;
      }
    }

    if (STAT_(buffer, &stat_struct)) {
      /* ERROR_MSG( "Stat Failed*ABORT" ); */
      exit(1);
    }

    (void)memcpy(&den_ptr->stat_struct, &stat_struct, sizeof(stat_struct));

    /* Sort by direct insertion */
    /*------------------------------------*/

    for (des_ptr = father_dir_entry->sub_tree; des_ptr;
         des_ptr = des_ptr->next) {
      if (strcmp(des_ptr->name, den_ptr->name) > 0) {
        /* des-element is larger */
        /*--------------------------*/

        den_ptr->next = des_ptr;
        den_ptr->prev = des_ptr->prev;
        if (des_ptr->prev)
          des_ptr->prev->next = den_ptr;
        else
          father_dir_entry->sub_tree = den_ptr;
        des_ptr->prev = den_ptr;
        break;
      }

      if (des_ptr->next == NULL) {
        /* End of list reached; ==> insert */
        /*----------------------------------------*/

        den_ptr->prev = des_ptr;
        den_ptr->next = des_ptr->next;
        des_ptr->next = den_ptr;
        break;
      }
    }

    if (father_dir_entry->sub_tree == NULL) {
      /* First element */
      /*----------------*/

      father_dir_entry->sub_tree = den_ptr;
      den_ptr->prev = NULL;
      den_ptr->next = NULL;
    }

    if (s)
      (void)GetAvailBytes(&s->disk_space, s);
  }

  return (den_ptr);
}

int MakePath(const ViewContext *ctx, DirEntry *tree, char *dir_path,
             DirEntry **dest_dir_entry) {
  DirEntry *de_ptr, *sde_ptr;
  char path[PATH_LENGTH + 1];
  char *token, *old;
  int n;
  int result = -1;
  char *search_start;

  /* Variables for external mkdir -p logic */
  char tmp[PATH_LENGTH + 1];
  char *p;
  size_t len;

  if (tree == NULL)
    goto CREATE_EXTERNAL;

  NormPath(dir_path, path);
  if (path[0] == '\0') {
    if (errno == 0) {
      errno = ENAMETOOLONG;
    }
    return (result);
  }
  if (dest_dir_entry)
    *dest_dir_entry = NULL;

  n = strlen(tree->name);
  /*
   * Check if path matches tree root.
   * Special handling for root "/" to ensure "path inside tree" logic works
   * correctly.
   */
  if (strcmp(tree->name, FILE_SEPARATOR_STRING) == 0) {
    /* Tree is "/". Path must start with "/". */
    if (path[0] == FILE_SEPARATOR_CHAR) {
      de_ptr = tree;
      /* Tokenize starting from char 1 to skip leading slash */
      search_start = &path[1];
      goto SEARCH_TREE;
    }
  } else if (!strncmp(tree->name, path, n) &&
             (path[n] == FILE_SEPARATOR_CHAR || path[n] == '\0')) {
    /* Normal case: Path starts with tree root prefix */
    de_ptr = tree;
    search_start = &path[n];
    goto SEARCH_TREE;
  }

  /* Fallback: Destination not in current memory tree */
  goto CREATE_EXTERNAL;

SEARCH_TREE:
  /* Path is in the (Sub)-Tree */
  /*----------------------------------*/

  token = strtok_r(search_start, FILE_SEPARATOR_STRING, &old);
  while (token) {
    for (sde_ptr = de_ptr->sub_tree; sde_ptr; sde_ptr = sde_ptr->next) {
      /* FIX: Case-insensitive search for existing directories */
      if (!strcasecmp(sde_ptr->name, token)) {
        /* Subtree found */
        /*------------------*/

        de_ptr = sde_ptr;
        break;
      }
    }
    if (sde_ptr == NULL) {
      /* Subsequent directory does not exist */
      /*----------------------------------*/
      /* MakeDirEntry returns the new node (or existing one), or NULL on error
       */
      if ((de_ptr = MakeDirEntry(ctx, NULL, de_ptr, token, NULL)) == NULL) {
        return (result);
      }
    }
    token = strtok_r(NULL, FILE_SEPARATOR_STRING, &old);
  }
  if (dest_dir_entry)
    *dest_dir_entry = de_ptr;
  result = 0;
  return result;

CREATE_EXTERNAL:
  /* Robust mkdir -p implementation */
  snprintf(tmp, sizeof(tmp), "%s", dir_path);
  len = strlen(tmp);
  if (len > 0 && tmp[len - 1] == FILE_SEPARATOR_CHAR)
    tmp[len - 1] = 0;

  /* Handle root: start after the first slash if absolute path */
  p = tmp;
  if (*p == FILE_SEPARATOR_CHAR)
    p++;

  for (; *p; p++) {
    if (*p == FILE_SEPARATOR_CHAR) {
      *p = 0;
      if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        /* Error handling: ignore intermediate failures if final works */
      }
      *p = FILE_SEPARATOR_CHAR;
    }
  }
  if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
    /* MESSAGE("Can't create directory*\"%s\"*%s", tmp, strerror(errno)); */
    return -1;
  }
  result = 0;

  return (result);
}

int EnsureDirectoryExists(ViewContext *ctx, char *dir_path, DirEntry *tree,
                          BOOL *created, DirEntry **result_ptr,
                          int *auto_create, ChoiceCallback choice_cb) {
  DIR *tmpdir;
  int create_mode = 0;
  const char *detail = NULL;

  if (created)
    *created = FALSE;
  if (result_ptr)
    *result_ptr = NULL;

  /* Check if directory exists on disk */
  if ((tmpdir = opendir(dir_path)) == NULL) {
    /* If it doesn't exist, ask the user */
    if (errno == ENOENT) {
      if (auto_create && *auto_create) {
        create_mode = 1;
      } else {
        if (choice_cb) {
          char prompt[PATH_LENGTH + 64];
          (void)snprintf(prompt, sizeof(prompt),
                         "Create missing directory? (y/N) %s", dir_path);
          if (choice_cb(ctx, prompt, "YN\033") != 'Y')
            return -1;
          create_mode = 1;
        } else {
          return -1;
        }
      }

      if (create_mode) {
        /* Proceed to create */
        if (created)
          *created = TRUE;
      }
    } else {
      /* Some other error opening directory (e.g. permission) */
      MESSAGE(ctx, "Can't access destination directory*%s*\"%s\"",
              strerror(errno), dir_path);
      return -1;
    }
  } else {
    /* Exists on disk */
    closedir(tmpdir);
  }

  if (create_mode && auto_create)
    *auto_create = 1;

  /*
   * Directory exists (or user wants to create it).
   * Call MakePath to resolve the DirEntry pointer.
   * MakePath will create the node in memory if it's missing (even if dir exists
   * on disk), thanks to the update in MakeDirEntry.
   */
  if (MakePath(ctx, tree, dir_path, result_ptr) == 0)
    return 0;

  detail = (errno != 0) ? strerror(errno) : "operation failed";
  MESSAGE(ctx, "Can't create destination directory*%s*\"%s\"", detail,
          dir_path);
  return -1;
}
