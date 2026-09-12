/***************************************************************************
 * src/ui/key_engine.c
 * Input Handling Utilities
 *
 * Contains low-level input helpers. The main string input logic has moved
 * to input_line.c (UI_ReadString).
 ***************************************************************************/

#include "watcher.h"
#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_focus.h"
#include "ytnova_appstate_render.h"
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <ctype.h>
#include <curses.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

#ifdef READLINE_SUPPORT
#include <readline/tilde.h>
#endif

/* External declarations for Signal Safety enforcement */

/* Wrapper function to satisfy tputs(..., int (*putc_func)(int)) signature */
/* It writes the character 'c' to standard output. */
static int term_putc(int c) { return fputc(c, stdout); }

#define CONFLICT_SIZE_SCALE_THRESHOLD 999.5
#define CONFLICT_SIZE_MAX_UNIT_INDEX 5
#define ESC_SEQUENCE_TIMEOUT_MS 100

static int NormalizeChoiceKey(int c) {
  if (c >= 0 && c <= UCHAR_MAX && islower((unsigned char)c))
    return toupper((unsigned char)c);
  return c;
}

static void FormatConflictSize(long long value, char *buffer,
                               size_t buffer_size) {
  double scaled = (double)value;
  int unit_index = 0;
  static const char *units[] = {"B", "K", "M", "G", "T", "P"};

  if (!buffer || buffer_size == 0)
    return;

  if (value < 0) {
    (void)snprintf(buffer, buffer_size, "?");
    return;
  }

  while (scaled >= CONFLICT_SIZE_SCALE_THRESHOLD &&
         unit_index < CONFLICT_SIZE_MAX_UNIT_INDEX) {
    scaled /= 1024.0;
    unit_index++;
  }

  if (unit_index == 0)
    (void)snprintf(buffer, buffer_size, "%lld%s", value, units[unit_index]);
  else
    (void)snprintf(buffer, buffer_size, "%.1f%s", scaled, units[unit_index]);
}

static void FormatConflictTime(time_t when, char *buffer, size_t buffer_size) {
  struct tm tm_buf;
  const struct tm *tm_ptr;

  if (!buffer || buffer_size == 0)
    return;

#if defined(_POSIX_THREAD_SAFE_FUNCTIONS)
  tm_ptr = localtime_r(&when, &tm_buf);
#else
  tm_ptr = localtime(&when);
  if (tm_ptr)
    tm_buf = *tm_ptr;
  tm_ptr = tm_ptr ? &tm_buf : NULL;
#endif

  if (!tm_ptr ||
      strftime(buffer, buffer_size, "%Y-%m-%d %H:%M", tm_ptr) == 0) {
    (void)snprintf(buffer, buffer_size, "?");
  }
}

static const char *ConflictTimeRelation(const struct stat *src_stat,
                                        const struct stat *dst_stat) {
  if (!src_stat || !dst_stat)
    return "time unknown";
  if (dst_stat->st_mtime > src_stat->st_mtime)
    return "dst newer";
  if (dst_stat->st_mtime < src_stat->st_mtime)
    return "dst older";
  return "same time";
}

static const char *ConflictSizeRelation(const struct stat *src_stat,
                                        const struct stat *dst_stat) {
  if (!src_stat || !dst_stat)
    return "size unknown";
  if (dst_stat->st_size > src_stat->st_size)
    return "dst bigger";
  if (dst_stat->st_size < src_stat->st_size)
    return "dst smaller";
  return "same size";
}

static BOOL BuildConflictDetail(const char *src_path, const char *dst_path,
                                char *buffer, size_t buffer_size) {
  struct stat src_stat;
  struct stat dst_stat;
  char src_size[24];
  char dst_size[24];
  char src_time[20];
  char dst_time[20];
  int written;

  if (!src_path || !dst_path || !buffer || buffer_size == 0)
    return FALSE;

  if (lstat(src_path, &src_stat) != 0 || lstat(dst_path, &dst_stat) != 0)
    return FALSE;

  FormatConflictSize((long long)src_stat.st_size, src_size, sizeof(src_size));
  FormatConflictSize((long long)dst_stat.st_size, dst_size, sizeof(dst_size));
  FormatConflictTime(src_stat.st_mtime, src_time, sizeof(src_time));
  FormatConflictTime(dst_stat.st_mtime, dst_time, sizeof(dst_time));

  written = snprintf(buffer, buffer_size, "src %s %s | dst %s %s | %s, %s",
                     src_size, src_time, dst_size, dst_time,
                     ConflictSizeRelation(&src_stat, &dst_stat),
                     ConflictTimeRelation(&src_stat, &dst_stat));
  return written >= 0 && (size_t)written < buffer_size;
}

char *StrLeft(const char *str, size_t visible_count) {
  char *result;
  size_t len;
  int left_bytes;

#ifdef WITH_UTF8
  mbstate_t state;
  const char *s, *s_start;
  size_t pos = 0;
#endif

  if (visible_count == 0)
    return (xstrdup(""));

  len = StrVisualLength(str);
  if (visible_count >= len)
    return (xstrdup(str));

#ifdef WITH_UTF8

  s_start = s = str;

  while (*s) {

    wchar_t wc;
    size_t sz;
    int width;

    s_start = s;
    memset(&state, 0, sizeof(state));
    sz = mbrtowc(&wc, s, MB_CUR_MAX, &state);

    if (sz == (size_t)-1 || sz == (size_t)-2 || sz == 0) {
      s++;
      width = 1;
    } else {
      s += sz;
      width = wcwidth(wc);
      if (width < 0)
        width = 1;
    }

    if (pos + (size_t)width > visible_count)
      break;

    pos += width;
  }

  left_bytes = s_start - str;

#else
  left_bytes = visible_count;
#endif

  result = xmalloc(left_bytes + 1);
  memcpy(result, str, left_bytes);
  result[left_bytes] = '\0';
  return (result);
}

char *StrRight(const char *str, size_t visible_count) {
  char *result;
  size_t visual_len;

#ifdef WITH_UTF8
  int left_bytes;
  mbstate_t state;
  const char *s, *s_start;
  int pos_start = 0;
#endif

  if (visible_count == 0)
    return (xstrdup(""));

  visual_len = StrVisualLength(str);
  if (visual_len <= visible_count)
    return (xstrdup(str));

#ifdef WITH_UTF8

  s_start = s = str;

  while (*s) {

    wchar_t wc;
    size_t sz;
    int width;

    s_start = s;
    memset(&state, 0, sizeof(state));
    sz = mbrtowc(&wc, s, MB_CUR_MAX, &state);

    if (sz == (size_t)-1 || sz == (size_t)-2 || sz == 0) {
      s++;
      width = 1;
    } else {
      s += sz;
      width = wcwidth(wc);
      if (width < 0)
        width = 1;
    }

    if ((visual_len - (pos_start + width)) < visible_count)
      break;

    pos_start += width;
  }

  left_bytes = s_start - str;

  result = xstrdup(&str[left_bytes]);

#else
  result = xstrdup(&str[visual_len - visible_count]);
#endif

  return (result);
}

int StrVisualLength(const char *str) {
  int len;

#ifdef WITH_UTF8

  int pos = 0;
  mbstate_t state;
  const char *s = str;

  while (*s) {
    wchar_t wc;
    int width;

    memset(&state, 0, sizeof(state));
    size_t sz = mbrtowc(&wc, s, MB_CUR_MAX, &state);

    if (sz == (size_t)-1 || sz == (size_t)-2 || sz == 0) {
      s++;
      width = 1;
    } else {
      s += sz;
      width = wcwidth(wc);
      if (width < 0)
        width = 1;
    }
    pos += width;
  }
  len = pos;

#else
  len = strlen(str);
#endif

  return len;
}

/* returns byte position for visual position */
int VisualPositionToBytePosition(const char *str, int visual_pos) {

#ifdef WITH_UTF8

  mbstate_t state;
  const char *s;
  int pos = 0;

  s = str;

  while (*s) {

    wchar_t wc;
    size_t sz;
    int width;

    const char *s_start = s;
    memset(&state, 0, sizeof(state));
    sz = mbrtowc(&wc, s, MB_CUR_MAX, &state);

    if (sz == (size_t)-1 || sz == (size_t)-2 || sz == 0) {
      s++;
      width = 1;
    } else {
      s += sz;
      width = wcwidth(wc);
      if (width < 0)
        width = 1;
    }

    if (pos + width > visual_pos)
      return (s_start - str);

    pos += width;
  }

  return (s - str);

#else
  return visual_pos;
#endif
}

int InputChoice(ViewContext *ctx, const char *msg, const char *term) {
  int c;

  if (!AppStateValidatedDispatchSurface("surface.menu-modal-completion"))
    return ERR;
  if (!AppStateValidatedDispatchSurface("surface.modal-completion-event"))
    return ERR;
  if (!AppStateValidatedEvent("event.modal-completion"))
    return ERR;

  ClearHelp(ctx);

  curs_set(1);
  leaveok(ctx->ctx_border_window, FALSE);
  mvwhline(ctx->ctx_border_window, ctx->layout.prompt_y, 1, ' ', COLS - 2);
  PrintMenuOptions(ctx->ctx_border_window, ctx->layout.prompt_y, 1, (char *)msg,
                   UI_ROLE_STATIC_TEXT, UI_ROLE_KEYBIND);
  wnoutrefresh(ctx->ctx_border_window);
  doupdate();
  do {
    c = WGetch(ctx, ctx->ctx_border_window);
    if (c == ESC)
      break;
    if (c >= 0)
      c = NormalizeChoiceKey(c);
  } while (c != -1 && !strchr(term, c));

  mvwaddstr(ctx->ctx_border_window, ctx->layout.prompt_y, 1, " ");
  mvwhline(ctx->ctx_border_window, ctx->layout.prompt_y, 1, ' ', COLS - 2);
  wnoutrefresh(ctx->ctx_border_window);
  leaveok(ctx->ctx_border_window, TRUE);
  curs_set(0);
  doupdate();

  return (c);
}

int InputChoiceWithHelp(ViewContext *ctx, const char *msg, const char *term,
                        int (*help_callback)(ViewContext *, void *),
                        void *help_data) {
  static const UICommandStripCommand help_commands[] = {
      {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("choice-prompt.commands", "help"),
       "F1", NULL, "choice-prompt.commands"},
      {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("choice-prompt.commands", "cancel"),
       "Esc", NULL, "choice-prompt.commands"}};
  int c;

  if (!AppStateValidatedDispatchSurface("surface.menu-modal-completion"))
    return ERR;
  if (!AppStateValidatedDispatchSurface("surface.modal-completion-event"))
    return ERR;
  if (!AppStateValidatedEvent("event.modal-completion"))
    return ERR;

  ClearHelp(ctx);

  curs_set(1);
  leaveok(ctx->ctx_border_window, FALSE);
  mvwhline(ctx->ctx_border_window, ctx->layout.prompt_y, 1, ' ', COLS - 2);
  mvwhline(ctx->ctx_border_window, ctx->layout.status_y, 1, ' ', COLS - 2);
  PrintMenuOptions(ctx->ctx_border_window, ctx->layout.prompt_y, 1, (char *)msg,
                   UI_ROLE_STATIC_TEXT, UI_ROLE_KEYBIND);
  if (help_callback != NULL) {
    UI_RenderAdaptiveCommandStrip(
        ctx->ctx_border_window, ctx->layout.status_y, 1, help_commands,
        sizeof(help_commands) / sizeof(help_commands[0]), UI_ROLE_STATIC_TEXT,
        UI_ROLE_KEYBIND);
  }
  wnoutrefresh(ctx->ctx_border_window);
  doupdate();
  do {
    c = WGetch(ctx, ctx->ctx_border_window);
    if (c == KEY_F(1) && help_callback != NULL) {
      curs_set(0);
      (void)help_callback(ctx, help_data);
      curs_set(1);
      mvwhline(ctx->ctx_border_window, ctx->layout.prompt_y, 1, ' ', COLS - 2);
      mvwhline(ctx->ctx_border_window, ctx->layout.status_y, 1, ' ', COLS - 2);
      PrintMenuOptions(ctx->ctx_border_window, ctx->layout.prompt_y, 1,
                       (char *)msg, UI_ROLE_STATIC_TEXT, UI_ROLE_KEYBIND);
      UI_RenderAdaptiveCommandStrip(
          ctx->ctx_border_window, ctx->layout.status_y, 1, help_commands,
          sizeof(help_commands) / sizeof(help_commands[0]), UI_ROLE_STATIC_TEXT,
          UI_ROLE_KEYBIND);
      touchwin(ctx->ctx_border_window);
      wnoutrefresh(ctx->ctx_border_window);
      doupdate();
      continue;
    }
    if (c == ESC)
      break;
    if (c >= 0)
      c = NormalizeChoiceKey(c);
  } while (c != -1 && !strchr(term, c));

  mvwhline(ctx->ctx_border_window, ctx->layout.prompt_y, 1, ' ', COLS - 2);
  mvwhline(ctx->ctx_border_window, ctx->layout.status_y, 1, ' ', COLS - 2);
  wnoutrefresh(ctx->ctx_border_window);
  leaveok(ctx->ctx_border_window, TRUE);
  curs_set(0);
  doupdate();

  return (c);
}

static int InputChoiceWithDetail(ViewContext *ctx, const char *msg,
                                 const char *detail, const char *term) {
  int c;

  if (!AppStateValidatedDispatchSurface("surface.menu-modal-completion"))
    return ERR;
  if (!AppStateValidatedDispatchSurface("surface.modal-completion-event"))
    return ERR;
  if (!AppStateValidatedEvent("event.modal-completion"))
    return ERR;

  ClearHelp(ctx);

  curs_set(1);
  leaveok(ctx->ctx_border_window, FALSE);
  mvwhline(ctx->ctx_border_window, ctx->layout.prompt_y, 1, ' ', COLS - 2);
  mvwhline(ctx->ctx_border_window, ctx->layout.status_y, 1, ' ', COLS - 2);
  PrintMenuOptions(ctx->ctx_border_window, ctx->layout.prompt_y, 1, (char *)msg,
                   UI_ROLE_STATIC_TEXT, UI_ROLE_KEYBIND);
  if (detail && detail[0] != '\0')
    Print(ctx->ctx_border_window, ctx->layout.status_y, 1, (char *)detail,
          UI_ROLE_STATIC_TEXT);
  wnoutrefresh(ctx->ctx_border_window);
  doupdate();
  do {
    c = WGetch(ctx, ctx->ctx_border_window);
    if (c == ESC)
      break;
    if (c >= 0)
      c = NormalizeChoiceKey(c);
  } while (c != -1 && !strchr(term, c));

  mvwhline(ctx->ctx_border_window, ctx->layout.prompt_y, 1, ' ', COLS - 2);
  mvwhline(ctx->ctx_border_window, ctx->layout.status_y, 1, ' ', COLS - 2);
  wnoutrefresh(ctx->ctx_border_window);
  leaveok(ctx->ctx_border_window, TRUE);
  curs_set(0);
  doupdate();

  return (c);
}

static BOOL ChoiceTermsContainKey(const char *term, int key) {
  const char *p;

  if (!term)
    return FALSE;

  for (p = term; *p; p++) {
    if (NormalizeChoiceKey((unsigned char)*p) == key)
      return TRUE;
  }
  return FALSE;
}

static BOOL ChoiceTokenMatchesTerms(const char *begin, const char *end,
                                    const char *term) {
  BOOL has_choice = FALSE;
  const char *p;

  if (!begin || !end || !term || begin >= end)
    return FALSE;

  for (p = begin; p < end; p++) {
    unsigned char ch = (unsigned char)*p;
    int normalized;

    if (!isalpha(ch))
      continue;

    normalized = NormalizeChoiceKey(ch);
    if (!ChoiceTermsContainKey(term, normalized))
      return FALSE;
    has_choice = TRUE;
  }
  return has_choice;
}

static void PrintChoiceLiteral(WINDOW *win, int row, int col, const char *msg,
                               const char *term) {
  const char *choice_begin = NULL;
  const char *choice_end = NULL;
  const char *open_paren;
  const char *p;
  chtype current_attr = (chtype)-1;
  chtype key_attr;
  chtype normal_attr;
  int max_x;

  if (!win || !msg)
    return;

  max_x = getmaxx(win);
  for (open_paren = strchr(msg, '('); open_paren != NULL;
       open_paren = strchr(open_paren + 1, '(')) {
    const char *close_paren = strchr(open_paren + 1, ')');
    if (close_paren != NULL &&
        ChoiceTokenMatchesTerms(open_paren + 1, close_paren, term)) {
      choice_begin = open_paren + 1;
      choice_end = close_paren;
      break;
    }
  }

#ifdef COLOR_SUPPORT
  normal_attr = COLOR_PAIR(UI_ROLE_STATIC_TEXT);
  key_attr = UIKeybindAttrForBase(UI_ROLE_KEYBIND, UI_ROLE_STATIC_TEXT);
#else
  normal_attr = A_NORMAL;
  key_attr = A_BOLD;
#endif

  for (p = msg; *p && col < max_x; p++) {
    unsigned char raw_ch = (unsigned char)*p;
    chtype rendered_ch = (raw_ch < 32 && raw_ch != 0) ? ACS_BLOCK : raw_ch;
    chtype attr = (choice_begin != NULL && p >= choice_begin && p < choice_end)
                      ? key_attr
                      : normal_attr;
    if (attr != current_attr) {
      wattrset(win, attr);
      current_attr = attr;
    }
    mvwaddch(win, row, col++, rendered_ch);
  }
  wattrset(win, 0);
}

int InputChoiceLiteral(ViewContext *ctx, const char *msg, const char *term) {
  int c;

  if (!AppStateValidatedDispatchSurface("surface.menu-modal-completion"))
    return ERR;
  if (!AppStateValidatedDispatchSurface("surface.modal-completion-event"))
    return ERR;
  if (!AppStateValidatedEvent("event.modal-completion"))
    return ERR;

  ClearHelp(ctx);

  curs_set(1);
  leaveok(ctx->ctx_border_window, FALSE);
  mvwhline(ctx->ctx_border_window, ctx->layout.prompt_y, 1, ' ', COLS - 2);
  PrintChoiceLiteral(ctx->ctx_border_window, ctx->layout.prompt_y, 1, msg,
                     term);
  wnoutrefresh(ctx->ctx_border_window);
  doupdate();
  do {
    c = WGetch(ctx, ctx->ctx_border_window);
    if (c == ESC)
      break;
    if (c >= 0)
      c = NormalizeChoiceKey(c);
  } while (c != -1 && !strchr(term, c));

  mvwaddstr(ctx->ctx_border_window, ctx->layout.prompt_y, 1, " ");
  mvwhline(ctx->ctx_border_window, ctx->layout.prompt_y, 1, ' ', COLS - 2);
  wnoutrefresh(ctx->ctx_border_window);
  leaveok(ctx->ctx_border_window, TRUE);
  curs_set(0);
  doupdate();

  return (c);
}

int InputChoiceCommandStrip(ViewContext *ctx,
                            const UICommandStripCommand *commands,
                            size_t command_count, const char *term) {
  int c;

  if (!AppStateValidatedDispatchSurface("surface.menu-modal-completion"))
    return ERR;
  if (!AppStateValidatedDispatchSurface("surface.modal-completion-event"))
    return ERR;
  if (!AppStateValidatedEvent("event.modal-completion"))
    return ERR;

  ClearHelp(ctx);

  curs_set(1);
  leaveok(ctx->ctx_border_window, FALSE);
  mvwhline(ctx->ctx_border_window, ctx->layout.prompt_y, 1, ' ', COLS - 2);
  UI_RenderAdaptiveCommandStrip(ctx->ctx_border_window, ctx->layout.prompt_y, 1,
                                commands, command_count, UI_ROLE_STATIC_TEXT,
                                UI_ROLE_KEYBIND);
  wnoutrefresh(ctx->ctx_border_window);
  doupdate();
  do {
    c = WGetch(ctx, ctx->ctx_border_window);
    if (c == ESC)
      break;
    if (c >= 0)
      c = NormalizeChoiceKey(c);
  } while (c != -1 && !strchr(term, c));

  mvwaddstr(ctx->ctx_border_window, ctx->layout.prompt_y, 1, " ");
  mvwhline(ctx->ctx_border_window, ctx->layout.prompt_y, 1, ' ', COLS - 2);
  wnoutrefresh(ctx->ctx_border_window);
  leaveok(ctx->ctx_border_window, TRUE);
  curs_set(0);
  doupdate();

  return (c);
}

void HitReturnToContinue(void) {
#if !defined(XCURSES)
  char *te;

  char *tgetstr(const char *, char **);
  int tputs(const char *, int, int (*)(int));

  curs_set(1);

  vidattr(A_REVERSE);

  putp("[Hit return to continue]");
  (void)fflush(stdout);

  (void)getchar();

  te = tgetstr("me", NULL);
  if (te != NULL) {
    tputs(te, 1, term_putc);
  } else {
    putp("\033[0m");
  }
  (void)fflush(stdout);

#endif

  curs_set(0);
  doupdate();
}

BOOL KeyPressed() {
  BOOL pressed = FALSE;

  nodelay(stdscr, TRUE);
  int c = wgetch(stdscr);
  if (c != ERR) {
    pressed = TRUE;
    ungetch(c);
    DEBUG_KEYSTROKE_LOG("KeyPressed() saw: %3d ('%c') - UNGETTING", c,
                        (c >= 32 && c <= 126) ? c : '.');
  }
  nodelay(stdscr, FALSE);

  return (pressed);
}

BOOL EscapeKeyPressed(void) {
  int c;
  BOOL pressed = FALSE;

  nodelay(stdscr, TRUE);
  if ((c = wgetch(stdscr)) != ERR) {
    DEBUG_KEYSTROKE_LOG("EscapeKeyPressed() saw: %3d ('%c')", c,
                        (c >= 32 && c <= 126) ? c : '.');

    if (c == ESC) {
      pressed = TRUE;
    } else {
      ungetch(c);
    }
  }
  nodelay(stdscr, FALSE);

  return (pressed);
}

int ViKey(int ch) {
  switch (ch) {
  case VI_KEY_UP:
    ch = KEY_UP;
    break;
  case VI_KEY_DOWN:
    ch = KEY_DOWN;
    break;
  case VI_KEY_RIGHT:
    ch = KEY_RIGHT;
    break;
  case VI_KEY_LEFT:
    ch = KEY_LEFT;
    break;
  case VI_KEY_PPAGE:
    ch = KEY_PPAGE;
    break;
  case VI_KEY_NPAGE:
    ch = KEY_NPAGE;
    break;
  }
  return (ch);
}

BOOL IsViKeysEnabled(const ViewContext *ctx) {
  if (!ctx || !ctx->profile_data)
    return FALSE;
  return (strtol(GetProfileValue(ctx, "VI_KEYS"), NULL, 0)) ? TRUE : FALSE;
}

static const DirEntry *GetActiveFileDirEntry(const ViewContext *ctx) {
  if (!ctx || !ctx->active)
    return NULL;
  return ctx->active->file_dir_entry;
}

static BOOL IsGlobalAllVolumesFileView(const ViewContext *ctx) {
  const DirEntry *file_dir_entry = GetActiveFileDirEntry(ctx);
  ViewFocus active_focus = AppStateResolveActivePanelFocus(ctx);

  if (!ctx || active_focus != FOCUS_FILE || !file_dir_entry)
    return FALSE;

  return (file_dir_entry->global_flag && file_dir_entry->global_all_volumes)
             ? TRUE
             : FALSE;
}

int NormalizeViKey(const ViewContext *ctx, int ch) {
  if (IsViKeysEnabled(ctx))
    return ViKey(ch);
  return ch;
}

YtreeNovaAction GetKeyAction(const ViewContext *ctx, int ch) {
  BOOL vi_keys_enabled = IsViKeysEnabled(ctx);
  ViewFocus active_focus = AppStateResolveActivePanelFocus(ctx);
  if (!AppStateValidatedDispatchSurface("surface.key-decode-input-dispatch"))
    return ACTION_NONE;

  if (vi_keys_enabled)
    ch = ViKey(ch);

  switch (ch) {
  case KEY_UP:
    return AppStateValidatedKeyAction(ACTION_MOVE_UP);
  case KEY_DOWN:
    return AppStateValidatedKeyAction(ACTION_MOVE_DOWN);
  case KEY_LEFT:
    return AppStateValidatedKeyAction(ACTION_MOVE_LEFT);
  case KEY_RIGHT:
    return AppStateValidatedKeyAction(ACTION_MOVE_RIGHT);
  case KEY_PPAGE:
    return AppStateValidatedKeyAction(ACTION_PAGE_UP);
  case KEY_NPAGE:
    return AppStateValidatedKeyAction(ACTION_PAGE_DOWN);
  case KEY_HOME:
    return AppStateValidatedKeyAction(ACTION_HOME);
  case KEY_END:
    return AppStateValidatedKeyAction(ACTION_END);

  case '\t':
    return AppStateValidatedKeyAction(
        (ctx && ctx->is_split_screen) ? ACTION_SWITCH_PANEL
                                      : ACTION_MOVE_SIBLING_NEXT);
  case '*':
    return AppStateValidatedKeyAction(ACTION_ASTERISK);
  case 'i':
  case 'I':
    return AppStateValidatedKeyAction(ACTION_INVERT);
  case KEY_BTAB:
    return AppStateValidatedKeyAction(ACTION_MOVE_SIBLING_PREV);
  case '-':
    return AppStateValidatedKeyAction(ACTION_TREE_COLLAPSE);
  case '+':
    return AppStateValidatedKeyAction(ACTION_TREE_EXPAND_ALL);
  case '/':
    return AppStateValidatedKeyAction(ACTION_LIST_JUMP);
  case '\\':
    return AppStateValidatedKeyAction(ACTION_TO_DIR);

  case CR:
  case LF:
    return AppStateValidatedKeyAction(ACTION_ENTER);
  case ESC:
    return AppStateValidatedKeyAction(ACTION_ESCAPE);
  case 'l':
  case 'L':
    return AppStateValidatedKeyAction(ACTION_LOG);
  case 'q':
  case 'Q':
    return AppStateValidatedKeyAction(ACTION_QUIT);
  case 0x11:
    return AppStateValidatedKeyAction(ACTION_QUIT_DIR);
  case 't':
  case 'T':
    return AppStateValidatedKeyAction(ACTION_TAG);
  case 'u':
    return AppStateValidatedKeyAction(ACTION_UNTAG);
  case 'U':
    /* In vi-key mode, Ctrl-U is reserved for page-up navigation.
     * Use uppercase U as the file-window "untag all" command key.
     */
    if (vi_keys_enabled && ctx && active_focus == FOCUS_FILE)
      return AppStateValidatedKeyAction(ACTION_UNTAG_ALL);
    return AppStateValidatedKeyAction(ACTION_UNTAG);
  case 0x14: /* Ctrl+T */
    return AppStateValidatedKeyAction(ACTION_TAG_ALL);
  case 0x15: /* Ctrl+U */
    return AppStateValidatedKeyAction(ACTION_UNTAG_ALL);
  case ';':
    return AppStateValidatedKeyAction(ACTION_TAG_REST);
  case ':':
    return AppStateValidatedKeyAction(ACTION_UNTAG_REST);
  case 'f':
  case 'F':
    return AppStateValidatedKeyAction(ACTION_FILTER);
  case '1':
    return AppStateValidatedKeyAction(ACTION_FILEINFO_1);
  case '2':
    return AppStateValidatedKeyAction(ACTION_FILEINFO_2);
  case '3':
    return AppStateValidatedKeyAction(ACTION_FILEINFO_3);
  case '4':
    return AppStateValidatedKeyAction(ACTION_FILEINFO_4);
  case '5':
    return AppStateValidatedKeyAction(ACTION_FILEINFO_5);
  case '6':
    return AppStateValidatedKeyAction(ACTION_FILEINFO_6);
  case '7':
    return AppStateValidatedKeyAction(ACTION_FILEINFO_7);
  case '8':
    return AppStateValidatedKeyAction(ACTION_FILEINFO_8);
  case '9':
    return AppStateValidatedKeyAction(ACTION_FILEINFO_9);
  case '0':
    return AppStateValidatedKeyAction(ACTION_FILEINFO_0);
  case 0x0C:
    return AppStateValidatedKeyAction(ACTION_REFRESH);
  case KEY_RESIZE:
    return AppStateValidatedKeyAction(ACTION_RESIZE);

  case 'k': /* Note: lowercase 'k' is KEY_UP when VI_KEYS profile is enabled */
  case 'K':
    return AppStateValidatedKeyAction(ACTION_VOL_MENU);
  case ',':
  case '<':
    return AppStateValidatedKeyAction(ACTION_VOL_PREV);
  case '.':
  case '>':
    return AppStateValidatedKeyAction(ACTION_VOL_NEXT);

  case 'a':
  case 'A':
    return AppStateValidatedKeyAction(ACTION_CMD_A);
  case 'c':
  case 'C':
    return AppStateValidatedKeyAction(ACTION_CMD_C);
  case 'd':
    return AppStateValidatedKeyAction(ACTION_CMD_D);
  case 'D':
    /* In vi-key mode, Ctrl-D is reserved for page-down navigation.
     * Use uppercase D as the file-window "delete tagged" command key.
     */
    if (vi_keys_enabled && ctx && active_focus == FOCUS_FILE)
      return AppStateValidatedKeyAction(ACTION_CMD_TAGGED_D);
    return AppStateValidatedKeyAction(ACTION_CMD_D);
  case KEY_DC:
    return AppStateValidatedKeyAction(ACTION_CMD_D);
  case 'e':
  case 'E':
    return AppStateValidatedKeyAction(ACTION_CMD_E);
  case 'g':
  case 'G':
    if (IsGlobalAllVolumesFileView(ctx))
      return AppStateValidatedKeyAction(ACTION_NONE);
    return AppStateValidatedKeyAction(ACTION_CMD_G);
  case 'h':
  case 'H':
    return AppStateValidatedKeyAction(ACTION_CMD_H);
  case 'm':
  case 'M':
    return AppStateValidatedKeyAction(ACTION_CMD_M);
  case 'n':
  case 'N':
    return AppStateValidatedKeyAction(ACTION_CMD_MKFILE);
  case 'o':
  case 'O':
    return AppStateValidatedKeyAction(ACTION_CMD_PRINT);
  case 'p':
  case 'P':
    return AppStateValidatedKeyAction(ACTION_CMD_P);
  case 'r':
  case 'R':
    return AppStateValidatedKeyAction(ACTION_CMD_R);
  case 's':
  case 'S':
    return AppStateValidatedKeyAction(ACTION_CMD_S);
  case 'v':
  case 'V':
    return AppStateValidatedKeyAction(ACTION_CMD_V);
  case 'x':
  case 'X':
    return AppStateValidatedKeyAction(ACTION_CMD_X);
  case 'y':
  case 'Y':
    return AppStateValidatedKeyAction(ACTION_CMD_Y);
  case 'z':
  case 'Z':
    return AppStateValidatedKeyAction(ACTION_CMD_I);
  case '`':
    return AppStateValidatedKeyAction(ACTION_TOGGLE_HIDDEN);

  case 0x01:
    return AppStateValidatedKeyAction(ACTION_CMD_TAGGED_A);
  case 0x03:
    return AppStateValidatedKeyAction(ACTION_CMD_TAGGED_C);
  case 0x0B:
    return AppStateValidatedKeyAction(ACTION_CMD_TAGGED_C);
  case 0x04:
    return AppStateValidatedKeyAction(ACTION_CMD_TAGGED_D);
  case 0x07:
    return AppStateValidatedKeyAction(ACTION_NONE);
  case 0x0E:
    if (ctx && ctx->preview_mode)
      return AppStateValidatedKeyAction(ACTION_PREVIEW_SCROLL_DOWN);
    return AppStateValidatedKeyAction(ACTION_CMD_TAGGED_M);
  case 0x0F:
    return AppStateValidatedKeyAction(ACTION_CMD_TAGGED_PRINT);
  case 0x10:
    if (ctx && ctx->preview_mode)
      return AppStateValidatedKeyAction(ACTION_PREVIEW_SCROLL_UP);
    return AppStateValidatedKeyAction(ACTION_CMD_TAGGED_P);
  case 0x12:
    return AppStateValidatedKeyAction(ACTION_CMD_TAGGED_R);
  case 0x13:
    return AppStateValidatedKeyAction(ACTION_CMD_TAGGED_S);
  case 0x16:
    return AppStateValidatedKeyAction(ACTION_CMD_TAGGED_V);
  case 0x18:
    return AppStateValidatedKeyAction(ACTION_CMD_TAGGED_X);
  case 0x19:
    return AppStateValidatedKeyAction(ACTION_CMD_TAGGED_Y);
  case 0x1A:
    return AppStateValidatedKeyAction(ACTION_CMD_I);

#ifdef KEY_F
  case KEY_F(1):
    return AppStateValidatedKeyAction(ACTION_HELP);
  case KEY_F(8):
    return AppStateValidatedKeyAction(ACTION_SPLIT_SCREEN);
  case KEY_F(7):
    return AppStateValidatedKeyAction(ACTION_VIEW_PREVIEW);
  case KEY_F(6):
    return AppStateValidatedKeyAction(ACTION_TOGGLE_STATS);
  case KEY_F(5):
    return AppStateValidatedKeyAction(ACTION_REFRESH);
  case KEY_F(10):
    return AppStateValidatedKeyAction(ACTION_EDIT_CONFIG);
#endif
  case 'j':
    if (!vi_keys_enabled && ctx) {
      if (active_focus == FOCUS_TREE)
        return AppStateValidatedKeyAction(ACTION_COMPARE_DIR);
      if (active_focus == FOCUS_FILE)
        return AppStateValidatedKeyAction(ACTION_COMPARE_FILE);
    }
    return AppStateValidatedKeyAction(ACTION_NONE);
  case 'J':
    if (ctx) {
      if (active_focus == FOCUS_TREE)
        return AppStateValidatedKeyAction(ACTION_COMPARE_DIR);
      if (active_focus == FOCUS_FILE)
        return AppStateValidatedKeyAction(ACTION_COMPARE_FILE);
    }
    return AppStateValidatedKeyAction(ACTION_NONE);

#ifdef KEY_SF
  case KEY_SF:
    return AppStateValidatedKeyAction(ACTION_PREVIEW_SCROLL_DOWN);
#endif
#ifdef KEY_SR
  case KEY_SR:
    return AppStateValidatedKeyAction(ACTION_PREVIEW_SCROLL_UP);
#endif
#ifdef KEY_SHOME
  case KEY_SHOME:
    return AppStateValidatedKeyAction(ACTION_PREVIEW_HOME);
#endif
#ifdef KEY_SEND
  case KEY_SEND:
    return AppStateValidatedKeyAction(ACTION_PREVIEW_END);
#endif
#ifdef KEY_SPREVIOUS
  case KEY_SPREVIOUS:
    return AppStateValidatedKeyAction(ACTION_PREVIEW_PAGE_UP);
#endif
#ifdef KEY_SNEXT
  case KEY_SNEXT:
    return AppStateValidatedKeyAction(ACTION_PREVIEW_PAGE_DOWN);
#endif

  default:
    return AppStateValidatedKeyAction(ACTION_NONE);
  }
}

static int NormalizeEscSequenceForWindow(WINDOW *win, int ch) {
  int seq1;
  int seq2;

  if (ch != ESC)
    return ch;

  if (win == NULL)
    win = stdscr;

  wtimeout(win, ESC_SEQUENCE_TIMEOUT_MS);
  seq1 = wgetch(win);
  if (seq1 == ERR) {
    wtimeout(win, -1);
    return ESC;
  }

  if (seq1 != '[' && seq1 != 'O') {
    ungetch(seq1);
    wtimeout(win, -1);
    return ESC;
  }

  seq2 = wgetch(win);
  if (seq2 == ERR) {
    ungetch(seq1);
    wtimeout(win, -1);
    return ESC;
  }

  switch (seq2) {
  case 'A':
    ch = KEY_UP;
    break;
  case 'B':
    ch = KEY_DOWN;
    break;
  case 'C':
    ch = KEY_RIGHT;
    break;
  case 'D':
    ch = KEY_LEFT;
    break;
  case 'H':
    ch = KEY_HOME;
    break;
  case 'F':
    ch = KEY_END;
    break;
  case '1':
  case '4':
  case '7':
  case '8': {
    int seq3 = wgetch(win);
    if (seq3 == '~') {
      ch = (seq2 == '1' || seq2 == '7') ? KEY_HOME : KEY_END;
    } else {
      if (seq3 != ERR)
        ungetch(seq3);
      ungetch(seq2);
      ungetch(seq1);
      ch = ESC;
    }
    break;
  }
  default:
    ungetch(seq2);
    ungetch(seq1);
    ch = ESC;
    break;
  }

  wtimeout(win, -1);
  return ch;
}

int WGetch(ViewContext *ctx, WINDOW *win) {
  int c;

  c = wgetch(win);

  if (ctx && ctx->status_line_error_pending && c != ERR) {
#ifdef KEY_RESIZE
    if (c != KEY_RESIZE)
      UI_ClearStatusLineError(ctx);
#else
    UI_ClearStatusLineError(ctx);
#endif
  }
  if (ctx && ctx->status_line_notice_pending && c != ERR) {
#ifdef KEY_RESIZE
    if (c != KEY_RESIZE)
      UI_ClearStatusLineNotice(ctx);
#else
    UI_ClearStatusLineNotice(ctx);
#endif
  }

#ifdef KEY_RESIZE
  if (c == KEY_RESIZE) {
    if (!AppStateValidatedDispatchSurface("surface.resize-signal-handling"))
      return ERR;
    if (ctx)
      (void)AppStateMarkResizeRequest(ctx);
    c = -1;
  }
#endif

  return NormalizeEscSequenceForWindow(win, c);
}

int Getch(ViewContext *ctx) { return WGetch(ctx, stdscr); }

static int NormalizeEscSequence(int ch) {
  return NormalizeEscSequenceForWindow(stdscr, ch);
}

int GetEventOrKey(ViewContext *ctx) {
  int ch;
  int w_fd = Watcher_GetFD(ctx);
  fd_set fds;
  struct timeval tv;

  if (ctx && ctx->resize_request) {
    if (!AppStateValidatedDispatchSurface("surface.resize-signal-handling"))
      return ERR;
    if (!AppStateValidatedEvent("event.terminal-resize-signal"))
      return ERR;
    return KEY_RESIZE;
  }

  /* Before the select loop, check the shutdown flag */
  if (ytnova_shutdown_flag)
    return 'q';

  /* Check if input is already available to avoid select delay */
  nodelay(stdscr, TRUE);
  ch = WGetch(ctx, stdscr);
  nodelay(stdscr, FALSE);

  if (ch != ERR) {
    return NormalizeEscSequence(ch);
  }

  if (ctx && ctx->resize_request) {
    if (!AppStateValidatedDispatchSurface("surface.resize-signal-handling"))
      return ERR;
    if (!AppStateValidatedEvent("event.terminal-resize-signal"))
      return ERR;
    return KEY_RESIZE;
  }

  while (1) {
    int max_fd;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    max_fd = STDIN_FILENO;

    if (w_fd >= 0) {
      FD_SET(w_fd, &fds);
      if (w_fd > max_fd)
        max_fd = w_fd;
    }

    /* Setup timeout for 500ms for clock update */
    tv.tv_sec = 0;
    tv.tv_usec = 500000;

    int result = select(max_fd + 1, &fds, NULL, NULL, &tv);

    if (result == 0) {
      /* Timeout: Update Clock */
      if (ctx) {
        ClockHandler(ctx, 0);
        doupdate();
      }
      continue;
    }

    if (result == -1) {
      if (errno == EINTR) {
        if (ytnova_shutdown_flag)
          return 'q';

        nodelay(stdscr, TRUE);
        ch = WGetch(ctx, stdscr);
        nodelay(stdscr, FALSE);
        if (ch != ERR)
          return NormalizeEscSequence(ch);
        if (ctx && ctx->resize_request) {
          if (!AppStateValidatedDispatchSurface(
                  "surface.resize-signal-handling"))
            return ERR;
          if (!AppStateValidatedEvent("event.terminal-resize-signal"))
            return ERR;
          return KEY_RESIZE;
        }

        continue;
      }
      return -1;
    }

    if (ctx && (ctx->refresh_mode & REFRESH_WATCHER) && w_fd >= 0 &&
        FD_ISSET(w_fd, &fds)) {
      if (Watcher_ProcessEvents(ctx)) {
        if (!AppStateValidatedDispatchSurface(
                "surface.watcher-live-refresh"))
          return ERR;
        if (!AppStateValidatedEvent("event.watcher-live-refresh"))
          return ERR;
        return KEY_F(5);
      }
    }

    if (FD_ISSET(STDIN_FILENO, &fds)) {
      /* Input available, perform WGetch */
      return NormalizeEscSequence(WGetch(ctx, stdscr));
    }
  }
}

int UI_AskConflict(ViewContext *ctx, const char *src_path, const char *dst_path,
                   int *mode_flags) {
  char msg[1024];
  char detail[128];
  int c;

  if (mode_flags && *mode_flags == CONFLICT_ALL)
    return CONFLICT_ALL;

  snprintf(msg, sizeof(msg), "Overwrite %.300s? (Y)es/(N)o/(A)ll/(Q)uit",
           dst_path);

  /* Allow Y, N, A, Q, and ESC (27) */
  if (BuildConflictDetail(src_path, dst_path, detail, sizeof(detail)))
    c = InputChoiceWithDetail(ctx, msg, detail, "YNAQ\033");
  else
    c = InputChoice(ctx, msg, "YNAQ\033");

  if (c == 'Y')
    return CONFLICT_OVERWRITE;
  if (c == 'N')
    return CONFLICT_SKIP;
  if (c == 'A') {
    if (mode_flags)
      *mode_flags = CONFLICT_ALL;
    return CONFLICT_ALL;
  }
  if (c == 'Q' || c == 27)
    return CONFLICT_ABORT;

  return CONFLICT_ABORT;
}
