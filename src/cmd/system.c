/***************************************************************************
 *
 * src/cmd/system.c
 * System Call
 *
 ***************************************************************************/

#include "ytnova_cmd.h"
#include "ytnova_fs.h"
#include "ytnova_runtime_launch.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


static int WriteDetachedLaunchError(int fd, int error_code) {
  return write(fd, &error_code, sizeof(error_code)) == (ssize_t)sizeof(error_code)
             ? 0
             : -1;
}

int LaunchDetachedCommand(ViewContext *ctx, const char *command_line,
                          const char *working_directory, Statistic *s) {
  int status_pipe[2] = {-1, -1};
  int child_error = 0;
  ssize_t read_count;
  pid_t child_pid;
  int child_status;

  (void)ctx;

  if (command_line == NULL || *command_line == '\0' || s == NULL) {
    errno = EINVAL;
    return -1;
  }
  if (pipe(status_pipe) != 0)
    return -1;
  if (fcntl(status_pipe[1], F_SETFD, FD_CLOEXEC) == -1) {
    child_error = errno;
    close(status_pipe[0]);
    close(status_pipe[1]);
    errno = child_error;
    return -1;
  }

  child_pid = fork();
  if (child_pid == -1) {
    child_error = errno;
    close(status_pipe[0]);
    close(status_pipe[1]);
    errno = child_error;
    return -1;
  }

  if (child_pid == 0) {
    int null_fd;
    pid_t grandchild_pid;

    close(status_pipe[0]);

    if (setsid() == -1) {
      child_error = errno;
      (void)WriteDetachedLaunchError(status_pipe[1], child_error);
      _exit(1);
    }

    grandchild_pid = fork();
    if (grandchild_pid == -1) {
      child_error = errno;
      (void)WriteDetachedLaunchError(status_pipe[1], child_error);
      _exit(1);
    }
    if (grandchild_pid > 0)
      _exit(0);

    null_fd = open("/dev/null", O_RDWR);
    if (null_fd == -1) {
      child_error = errno;
      (void)WriteDetachedLaunchError(status_pipe[1], child_error);
      _exit(1);
    }
    (void)RuntimeLaunchExecShellChild(command_line, working_directory, null_fd,
                                      null_fd, null_fd, FALSE);
    child_error = errno;
    (void)WriteDetachedLaunchError(status_pipe[1], child_error);
    _exit(1);
  }

  close(status_pipe[1]);
  if (RuntimeLaunchWait(child_pid, &child_status) != 0) {
    child_error = errno;
    close(status_pipe[0]);
    errno = child_error;
    return -1;
  }

  read_count = read(status_pipe[0], &child_error, sizeof(child_error));
  close(status_pipe[0]);
  if (read_count > 0) {
    errno = child_error;
    return -1;
  }
  if (read_count == -1)
    return -1;

  (void)GetAvailBytes(&s->disk_space, s);
  return 0;
}

int SilentSystemCallEx(ViewContext *ctx, const char *command_line, BOOL enable_clock, Statistic *s) {
  int command_status;
  int result;

  /* Hier ist die einzige Stelle, in der Kommandos aufgerufen werden! */

  if (ctx->hook_suspend_clock)
    ctx->hook_suspend_clock(ctx);

  if (RuntimeLaunchRunShell(command_line, NULL, -1, -1, -1, FALSE,
                            &command_status) != 0)
    result = -1;
  else
    result = command_status;

  /* Restore terminal settings. If enable_clock is TRUE, InitClock will
     implicitly call refresh() and restore the curses display later.
     If enable_clock is FALSE, the caller (QuerySystemCall) is responsible
     for the display/pause. */

  if (enable_clock && ctx->hook_init_clock)
    ctx->hook_init_clock(ctx); /* Re-initializes timer AND calls refresh/restores
                                * curses mode
                                */

  (void)GetAvailBytes(&s->disk_space, s);
  return (result);
}

int SilentSystemCall(ViewContext *ctx, const char *command_line, Statistic *s) {
  return (SilentSystemCallEx(ctx, command_line, TRUE, s));
}
