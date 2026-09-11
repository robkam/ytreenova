#include "terminal_input.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int StartCurses(ViewContext *ctx) {
  ctx->curses_screen = newterm(NULL, stdout, stdin);
  if (ctx->curses_screen == NULL)
    return -1;
  set_term(ctx->curses_screen);
  raw();
  noecho();
  keypad(stdscr, TRUE);
  return 0;
}

static void StopCurses(ViewContext *ctx) {
  SCREEN *screen = ctx->curses_screen;
  TerminalInputShutdown(ctx);
  endwin();
  if (screen != NULL)
    delscreen(screen);
  ctx->curses_screen = NULL;
}

static int RunProbe(ViewContext *ctx, int key_count) {
  int capability;
  int active;
  int index;
  int keys[256];

  if (key_count > (int)(sizeof(keys) / sizeof(keys[0])))
    return 64;

  if (TerminalInputStart(ctx) != 0)
    return 2;
  capability = (int)ctx->terminal_input_capability;
  active = ctx->terminal_input_active ? 1 : 0;
  nodelay(stdscr, TRUE);
  for (index = 0; index < key_count; ++index)
    keys[index] = TerminalInputReadKey(ctx, stdscr);
  StopCurses(ctx);

  fprintf(stderr, "CAP=%d ACTIVE=%d KEYS=", capability, active);
  for (index = 0; index < key_count; ++index) {
    fprintf(stderr, "%s%d", index == 0 ? "" : ",", keys[index]);
  }
  fputc('\n', stderr);
  return 0;
}

static int RunDecode(ViewContext *ctx, int key_count) {
  int index;
  int keys[256];

  if (key_count > (int)(sizeof(keys) / sizeof(keys[0])))
    return 64;

  ctx->terminal_input_capability = TERMINAL_INPUT_KITTY;
  ctx->terminal_input_active = TRUE;
  fputs("READY\n", stderr);
  fflush(stderr);
  for (index = 0; index < key_count; ++index)
    keys[index] = TerminalInputReadKey(ctx, stdscr);
  StopCurses(ctx);
  fprintf(stderr, "KEYS=");
  for (index = 0; index < key_count; ++index)
    fprintf(stderr, "%s%d", index == 0 ? "" : ",", keys[index]);
  fputc('\n', stderr);
  return 0;
}

static int RunLifecycle(ViewContext *ctx) {
  int initial_capability;
  int suspended;

  if (TerminalInputStart(ctx) != 0)
    return 2;
  initial_capability = (int)ctx->terminal_input_capability;
  suspended = TerminalInputSuspend(ctx) ? 1 : 0;
  fprintf(stderr, "SUSPENDED CAP=%d RESULT=%d\n", initial_capability,
          suspended);
  fflush(stderr);

  if (TerminalInputResume(ctx) != 0)
    return 3;
  fprintf(stderr, "RESUMED CAP=%d ACTIVE=%d\n",
          (int)ctx->terminal_input_capability,
          ctx->terminal_input_active ? 1 : 0);
  return 0;
}

static int RunUnread(ViewContext *ctx) {
  int key;
  int replayed_key;

  ctx->terminal_input_capability = TERMINAL_INPUT_KITTY;
  ctx->terminal_input_session_active = TRUE;
  ctx->terminal_input_active = TRUE;
  fputs("READY\n", stderr);
  fflush(stderr);
  key = TerminalInputReadKey(ctx, stdscr);
  if (TerminalInputUnreadKey(ctx, key) != 0)
    return 4;
  replayed_key = TerminalInputReadKey(ctx, stdscr);
  StopCurses(ctx);
  fprintf(stderr, "KEYS=%d,%d\n", key, replayed_key);
  return 0;
}

static int RunQueuedProbe(ViewContext *ctx) {
  int key;

  if (ungetch('x') == ERR)
    return 5;
  if (TerminalInputStart(ctx) != 0)
    return 2;
  key = TerminalInputReadKey(ctx, stdscr);
  StopCurses(ctx);
  fprintf(stderr, "CAP=%d KEY=%d\n", (int)ctx->terminal_input_capability, key);
  return 0;
}

int main(int argc, char **argv) {
  ViewContext ctx;
  int result;
  int key_count;

  if (argc != 3)
    return 64;
  key_count = atoi(argv[2]);
  if (key_count < 0)
    return 64;

  memset(&ctx, 0, sizeof(ctx));
  if (StartCurses(&ctx) != 0)
    return 1;

  if (strcmp(argv[1], "probe") == 0)
    result = RunProbe(&ctx, key_count);
  else if (strcmp(argv[1], "decode") == 0)
    result = RunDecode(&ctx, key_count);
  else if (strcmp(argv[1], "lifecycle") == 0)
    result = RunLifecycle(&ctx);
  else if (strcmp(argv[1], "unread") == 0)
    result = RunUnread(&ctx);
  else if (strcmp(argv[1], "queued-probe") == 0)
    result = RunQueuedProbe(&ctx);
  else
    result = 64;

  if (ctx.curses_screen != NULL)
    StopCurses(&ctx);
  return result;
}
