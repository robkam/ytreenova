/***************************************************************************
 *
 * src/cmd/rename.c
 * Renaming files/directories
 *
 ***************************************************************************/

#include "ytnova_cmd.h"
#include "ytnova_fs.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define FILE_SEPARATOR_CHAR '/'

#if defined(S_IFLNK)
#define STAT_(a, b) lstat(a, b)
#else
#define STAT_(a, b) stat(a, b)
#endif

static int ArchiveUICallback(int status, const char *msg, void *user_data) {
  ViewContext *ctx = (ViewContext *)user_data;

  if (status == ARCHIVE_STATUS_PROGRESS && ctx && ctx->hook_draw_spinner &&
      Progress_ShouldRender(ctx))
    ctx->hook_draw_spinner(ctx);
  (void)status;
  (void)msg;
  return ARCHIVE_CB_CONTINUE;
}

static int RenameDirEntry(const char *to_path, const char *from_path);
static int RenameFileEntry(const char *to_path, const char *from_path);

int RenameDirectory(ViewContext *ctx, DirEntry *de_ptr, const char *new_name) {
  DirEntry *den_ptr;
  DirEntry *sde_ptr;
  DirEntry *ude_ptr;
  FileEntry *fe_ptr;
  char from_path[PATH_LENGTH + 1];
  char to_path[PATH_LENGTH + 1];
  char parent_path[PATH_LENGTH + 1];
  struct stat stat_struct;
  int result;
  const char *cptr;
  size_t new_name_len;

  result = -1;

  /* Get the full path of the directory to be renamed */
  (void)GetPath(de_ptr, from_path);

/* ARCHIVE MODE HANDLER */
#ifdef HAVE_LIBARCHIVE
  if (ctx->active->vol->vol_stats.log_mode == ARCHIVE_MODE) {
    if (!(ctx->active->vol->vol_stats.archive_capabilities &
          ARCHIVE_CAP_RENAME))
      return -1;
    if (ctx->hook_draw_spinner)
      ctx->hook_draw_spinner(ctx);
    if (Archive_RenameEntry(ctx->active->vol->vol_stats.log_path, from_path,
                            new_name, ArchiveUICallback, ctx) == 0) {
      return 0;
    }
    return -1;
  }
#endif

  /* Find the parent directory by locating the last separator. */
  cptr = strrchr(from_path, FILE_SEPARATOR_CHAR);

  if (!cptr) {
    return -1;
  }

  if (cptr == from_path) {
    parent_path[0] = FILE_SEPARATOR_CHAR;
    parent_path[1] = '\0';
  } else {
    size_t parent_len = (size_t)(cptr - from_path);
    if (parent_len >= sizeof(parent_path)) {
      return -1;
    }
    memcpy(parent_path, from_path, parent_len);
    parent_path[parent_len] = '\0';
  }

  if (Path_Join(to_path, sizeof(to_path), parent_path, new_name) != 0) {
    return -1;
  }

  if (access(from_path, W_OK)) {
    return -1;
  }

  if (!RenameDirEntry(to_path, from_path)) {
    /* Rename successful */
    /*--------------------*/

    if (STAT_(to_path, &stat_struct)) {
      return -1;
    }

    /* FIX: Added +1 to allocation for null terminator */
    new_name_len = strlen(new_name);
    den_ptr = (DirEntry *)xmalloc(sizeof(DirEntry) + new_name_len + 1);

    (void)memcpy(den_ptr, de_ptr, sizeof(DirEntry));

    (void)memcpy(den_ptr->name, new_name, new_name_len + 1);

    (void)memcpy(&den_ptr->stat_struct, &stat_struct, sizeof(stat_struct));

    /* Link structure */
    /*---------------------*/

    if (den_ptr->prev)
      den_ptr->prev->next = den_ptr;
    if (den_ptr->next)
      den_ptr->next->prev = den_ptr;

    /* Subtree */
    /*---------*/

    for (sde_ptr = den_ptr->sub_tree; sde_ptr; sde_ptr = sde_ptr->next)
      sde_ptr->up_tree = den_ptr;

    /* Files */
    /*-------*/

    for (fe_ptr = den_ptr->file; fe_ptr; fe_ptr = fe_ptr->next)
      fe_ptr->dir_entry = den_ptr;

    /* Uptree */
    /*--------*/

    for (ude_ptr = den_ptr->up_tree; ude_ptr; ude_ptr = ude_ptr->next)
      if (ude_ptr->sub_tree == de_ptr)
        ude_ptr->sub_tree = den_ptr;

    /* Free old structure */
    /*-------------------------*/

    free(de_ptr);

    /* Warning: de_ptr is invalid from now on !!! */
    /*--------------------------------------------*/

    result = 0;
  }

  return (result);
}

int RenameFile(ViewContext *ctx, FileEntry *fe_ptr, const char *new_name,
               FileEntry **new_fe_ptr) {
  DirEntry *de_ptr;
  FileEntry *fen_ptr;
  char from_path[PATH_LENGTH + 1];
  char to_path[PATH_LENGTH + 1];
  char parent_path[PATH_LENGTH + 1];
  struct stat stat_struct;
  int result;
  size_t new_name_len;

  result = -1;

  *new_fe_ptr = fe_ptr;

  de_ptr = fe_ptr->dir_entry;

  (void)GetFileNamePath(fe_ptr, from_path);

/* ARCHIVE MODE HANDLER */
#ifdef HAVE_LIBARCHIVE
  if (ctx->active->vol->vol_stats.log_mode == ARCHIVE_MODE) {
    if (!(ctx->active->vol->vol_stats.archive_capabilities &
          ARCHIVE_CAP_RENAME))
      return -1;
    if (ctx->hook_draw_spinner)
      ctx->hook_draw_spinner(ctx);
    if (Archive_RenameEntry(ctx->active->vol->vol_stats.log_path, from_path,
                            new_name, ArchiveUICallback, ctx) == 0) {
      return 0;
    }
    return -1;
  }
#endif

  /* Construct destination path with normalized join semantics. */
  (void)GetPath(de_ptr, parent_path);
  if (Path_Join(to_path, sizeof(to_path), parent_path, new_name) != 0) {
    return -1;
  }

  if (access(from_path, W_OK)) {
    return -1;
  }

  if (!RenameFileEntry(to_path, from_path)) {
    /* Rename successful */
    /*--------------------*/

    if (STAT_(to_path, &stat_struct)) {
      return -1;
    }

    /* FIX: Added +1 to allocation for null terminator */
    new_name_len = strlen(new_name);
    fen_ptr = (FileEntry *)xmalloc(sizeof(FileEntry) + new_name_len + 1);

    (void)memcpy(fen_ptr, fe_ptr, sizeof(FileEntry));

    (void)memcpy(fen_ptr->name, new_name, new_name_len + 1);

    (void)memcpy(&fen_ptr->stat_struct, &stat_struct, sizeof(stat_struct));

    /* Link structure */
    /*---------------------*/

    if (fen_ptr->prev)
      fen_ptr->prev->next = fen_ptr;
    if (fen_ptr->next)
      fen_ptr->next->prev = fen_ptr;
    if (fen_ptr->dir_entry->file == fe_ptr)
      fen_ptr->dir_entry->file = fen_ptr;

    /* Free old structure */
    /*-------------------------*/

    free(fe_ptr);

    /* Warning: fe_ptr is invalid from now on !!! */
    /*--------------------------------------------*/

    result = 0;

    *new_fe_ptr = fen_ptr;
  }

  return (result);
}


static int RenameDirEntry(const char *to_path, const char *from_path) {
  if (!strcmp(to_path, from_path)) {
    return (0);
  }

  /* rename() keeps file and directory renames on the same POSIX path. */
  if (rename(from_path, to_path)) {
    return (-1);
  }

  return (0);
}

static int RenameFileEntry(const char *to_path, const char *from_path) {
  if (!strcmp(to_path, from_path)) {
    return (-1);
  }

  /*
   * Modernized: Use rename() instead of link()/unlink().
   * rename() is atomic and safer.
   */
  if (rename(from_path, to_path)) {
    return (-1);
  }

  return (0);
}

int RenameTaggedFiles(ViewContext *ctx, FileEntry *fe_ptr,
                      WalkingPackage *walking_package) {
  int result = -1;
  char new_name[PATH_LENGTH + 1];

  if (BuildFilename(fe_ptr->name,
                    walking_package->function_data.rename.new_name,
                    new_name) == 0) {
    if (*new_name == '\0') {
      return -1;
    } else {
      result = RenameFile(ctx, fe_ptr, new_name, &walking_package->new_fe_ptr);
    }
  }
  return (result);
}
