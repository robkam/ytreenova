/***************************************************************************
 *
 * src/cmd/pipe.c
 * Redirecting file and directory contents to a command
 *
 ***************************************************************************/

#include "ytnova_cmd.h"
#include "ytnova_fs.h"
#include "ytnova_runtime_launch.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define PIPE_ARGV_MAX 64

static int ParsePipeCommand(char *command_line, char **argv, size_t argv_len) {
  size_t argc = 0;
  char *p = command_line;

  if (!command_line || !argv || argv_len < 2) {
    return -1;
  }

  while (*p) {
    while (*p == ' ' || *p == '\t') {
      p++;
    }
    if (!*p) {
      break;
    }
    if (argc + 1 >= argv_len) {
      return -1;
    }
    argv[argc++] = p;
    while (*p && *p != ' ' && *p != '\t') {
      p++;
    }
    if (*p) {
      *p = '\0';
      p++;
    }
  }

  if (argc == 0) {
    return -1;
  }

  argv[argc] = NULL;
  return 0;
}

static int OpenPipeWriter(const char *pipe_command, FILE **pipe_fp_out,
                          pid_t *child_pid_out) {
  int copied_len;
  size_t argc;
  size_t i;
  char command_line[PATH_LENGTH + 1];
  char *argv[PIPE_ARGV_MAX];
  const char *redirect_path = NULL;
  BOOL redirect_append = FALSE;

  if (!pipe_command || !pipe_fp_out || !child_pid_out) {
    return -1;
  }

  copied_len = snprintf(command_line, sizeof(command_line), "%s", pipe_command);
  if (copied_len < 0 || (size_t)copied_len >= sizeof(command_line)) {
    errno = EINVAL;
    return -1;
  }
  if (ParsePipeCommand(command_line, argv, PIPE_ARGV_MAX) != 0) {
    errno = EINVAL;
    return -1;
  }
  for (argc = 0; argc < PIPE_ARGV_MAX && argv[argc]; argc++) {
  }
  for (i = 0; i < argc; i++) {
    if (!strcmp(argv[i], ">") || !strcmp(argv[i], ">>")) {
      if (i + 1 >= argc) {
        errno = EINVAL;
        return -1;
      }
      redirect_path = argv[i + 1];
      redirect_append = (argv[i][1] == '>');
      argv[i] = NULL;
      argc = i;
      break;
    }
  }
  if (argc == 0 || !argv[0]) {
    errno = EINVAL;
    return -1;
  }

  return RuntimeLaunchStartArgvWriter(argv, NULL, redirect_path,
                                      redirect_append, pipe_fp_out,
                                      child_pid_out);
}

static int ClosePipeWriter(FILE *pipe_fp, pid_t child_pid) {
  return RuntimeLaunchCloseWriter(pipe_fp, child_pid, NULL);
}

static void RestorePipeCommandUi(ViewContext *ctx, DirEntry *dir_entry) {
  if (!ctx) {
    return;
  }

  if (ctx->hook_init_clock) {
    ctx->hook_init_clock(ctx);
  }
  touchwin(stdscr);
  wnoutrefresh(stdscr);

  if (dir_entry && ctx->active && ctx->active->vol) {
    RefreshView(ctx, dir_entry);
    return;
  }

  if (ctx->hook_refresh_ui) {
    ctx->hook_refresh_ui();
  }
}

int Pipe(ViewContext *ctx, DirEntry *dir_entry, FileEntry *file_entry,
         char *pipe_command) {
  char file_name_path[PATH_LENGTH + 1];
  int result = -1;
  FILE *pipe_fp = NULL;
  pid_t child_pid = -1;
  char path[PATH_LENGTH + 1];
  int start_dir_fd;

  (void)GetRealFileNamePath(file_entry, file_name_path, ctx->view_mode);

  /* Robustly save current working directory using a file descriptor */
  start_dir_fd = open(".", O_RDONLY);
  if (start_dir_fd == -1) {
    return -1;
  }

  if (ctx->view_mode == DISK_MODE || ctx->view_mode == USER_MODE) {
    if (chdir(GetPath(dir_entry, path))) {
      close(start_dir_fd);
      return -1;
    }
  } else { /* ARCHIVE_MODE */
    char archive_dir[PATH_LENGTH + 1];
    char *last_slash;
    int copied_len = snprintf(archive_dir, sizeof(archive_dir), "%s",
                              ctx->active->vol->vol_stats.log_path);
    if (copied_len < 0) {
      close(start_dir_fd);
      return -1;
    }
    if ((size_t)copied_len >= sizeof(archive_dir)) {
      close(start_dir_fd);
      return -1;
    }
    last_slash = strrchr(archive_dir, '/');
    if (last_slash) {
      if (last_slash ==
          archive_dir) { /* Root directory, e.g., "/archive.zip" */
        *(last_slash + 1) = '\0';
      } else {
        *last_slash = '\0';
      }
      if (chdir(archive_dir) != 0) {
        close(start_dir_fd);
        return -1;
      }
    }
  }

  /* Exit curses mode for external command */
  endwin();
  if (ctx->hook_suspend_clock)
    ctx->hook_suspend_clock(ctx);

  if (OpenPipeWriter(pipe_command, &pipe_fp, &child_pid) != 0) {
    RestorePipeCommandUi(ctx, dir_entry);

    /* Restore CWD before returning */
    if (fchdir(start_dir_fd) == -1) {
    }
    close(start_dir_fd);
    return -1;
  } else {
    if (ctx->view_mode == DISK_MODE || ctx->view_mode == USER_MODE) {
      int in_fd;
      in_fd = open(file_name_path, O_RDONLY);
      if (in_fd != -1) {
        char buffer[4096];
        ssize_t bytes_read;
        while ((bytes_read = read(in_fd, buffer, sizeof(buffer))) > 0) {
          if (fwrite(buffer, 1, bytes_read, pipe_fp) < (size_t)bytes_read) {
            break;
          }
        }
        close(in_fd);
      }
    } else {
      /* ARCHIVE_MODE */
#ifdef HAVE_LIBARCHIVE
      const char *archive = ctx->active->vol->vol_stats.log_path;
      ExtractArchiveEntry(archive, file_name_path, fileno(pipe_fp), NULL, NULL);
#endif
    }
    result = ClosePipeWriter(pipe_fp, child_pid);

    /* Wait for user to see output */
    if (ctx->hook_hit_return_to_continue)
      ctx->hook_hit_return_to_continue();
  }

  RestorePipeCommandUi(ctx, dir_entry);

  if (fchdir(start_dir_fd) == -1) {
  }
  close(start_dir_fd);

  return (result);
}

int PipeDirectory(ViewContext *ctx, DirEntry *dir_entry, char *pipe_command) {
  char path[PATH_LENGTH + 1];
  FILE *pipe_fp = NULL;
  FileEntry *fe;
  const YtreeNovaPanel *active_panel = NULL;
  BOOL hide_dot_files = FALSE;
  int result = -1;
  int start_dir_fd;
  pid_t child_pid = -1;

  if (!ctx || !dir_entry || !pipe_command)
    return -1;

  active_panel = ctx->active;
  hide_dot_files =
      (active_panel != NULL && active_panel->hide_dot_files) ? TRUE : FALSE;

  (void)GetPath(dir_entry, path);

  /* Robustly save current working directory using a file descriptor */
  start_dir_fd = open(".", O_RDONLY);
  if (start_dir_fd == -1) {
    return -1;
  }

  if (ctx->view_mode == DISK_MODE || ctx->view_mode == USER_MODE) {
    if (chdir(path)) {
      close(start_dir_fd);
      return (-1);
    }
  }

  /* Exit curses mode for external command */
  endwin();
  if (ctx->hook_suspend_clock)
    ctx->hook_suspend_clock(ctx);

  if (OpenPipeWriter(pipe_command, &pipe_fp, &child_pid) != 0) {
    RestorePipeCommandUi(ctx, dir_entry);

    /* Restore CWD */
    if (fchdir(start_dir_fd) == -1) {
    }
    close(start_dir_fd);
    return (-1);
  }

  for (fe = dir_entry->file; fe; fe = fe->next) {
    if (fe->matching) {
      if (!hide_dot_files || fe->name[0] != '.') {
        fprintf(pipe_fp, "%s\n", fe->name);
      }
    }
  }

  if (ClosePipeWriter(pipe_fp, child_pid) != 0) {
    goto PIPE_CLOSE_FAILURE;
  }

  /* Wait for user to see output */
  if (ctx->hook_hit_return_to_continue)
    ctx->hook_hit_return_to_continue();

  result = 0;

  RestorePipeCommandUi(ctx, dir_entry);

  /* Restore CWD */
  if (fchdir(start_dir_fd) == -1) {
  }
  close(start_dir_fd);

  return (result);

PIPE_CLOSE_FAILURE:
  RestorePipeCommandUi(ctx, dir_entry);

  /* Restore CWD */
  if (fchdir(start_dir_fd) == -1) {
  }
  close(start_dir_fd);
  return -1;
}


int PipeTaggedFiles(ViewContext *ctx, FileEntry *fe_ptr,
                    WalkingPackage *walking_package, Statistic *s) {
  int i, n;
  char from_path[PATH_LENGTH + 1];
  char buffer[2048];

  (void)s; /* Unused */

  walking_package->new_fe_ptr = fe_ptr; /* unchanged */

  (void)GetRealFileNamePath(fe_ptr, from_path, ctx->view_mode);
  if ((i = open(from_path, O_RDONLY)) == -1) {
    return (-1);
  }

  while ((n = read(i, buffer, sizeof(buffer))) > 0) {
    if (fwrite(buffer, n, 1,
               walking_package->function_data.pipe_cmd.pipe_file) != 1) {
      (void)close(i);
      return (-1);
    }
  }

  (void)close(i);

  return (0);
}
