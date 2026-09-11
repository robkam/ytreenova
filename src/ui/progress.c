/***************************************************************************
 *
 * src/ui/progress.c
 * Universal progress/ETA display system
 *
 ***************************************************************************/

#include "ytnova_ui.h"
#include "ytnova_dialog.h"

#include <limits.h>
#include <string.h>
#include <time.h>

#define PROGRESS_PULSE_WIDTH 5
#define PROGRESS_PROMOTION_SECONDS 1
#define PROGRESS_RENDER_INTERVAL_SECONDS 0.1
#define PROGRESS_WINDOW_MAX_WIDTH 78
#define PROGRESS_WINDOW_MIN_WIDTH 32
#define PROGRESS_BAR_HEIGHT 5
#define PROGRESS_FOOTER_ROWS 3
#define PROGRESS_MIN_TERMINAL_WIDTH 8
#define PROGRESS_ELLIPSIS_WIDTH 4
#define PROGRESS_PERCENT_SCALE 100.0
#define PROGRESS_COMPLETE_FRACTION 1.0
#define PROGRESS_BAR_MAX_WIDTH 56
#define NANOSECONDS_PER_SECOND 1000000000.0

static double ProgressMonotonicSeconds(void) {
  struct timespec now;

  if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
    return 0.0;
  return (double)now.tv_sec + (double)now.tv_nsec / NANOSECONDS_PER_SECOND;
}

static double ProgressElapsedSince(double start, double now) {
  double elapsed = now - start;

  return elapsed > 0.0 ? elapsed : 0.0;
}

static double ProgressCompletionFraction(const ProgressContext *progress) {
  double completed = 0.0;
  unsigned int dimensions = 0;

  if (!progress)
    return 0.0;
  if (progress->bytes_total > 0) {
    completed += progress->bytes_done >= progress->bytes_total
                     ? 1.0
                     : (double)progress->bytes_done /
                           (double)progress->bytes_total;
    dimensions++;
  }
  if (progress->items_total > 0) {
    completed += progress->items_done >= progress->items_total
                     ? 1.0
                     : (double)progress->items_done /
                           (double)progress->items_total;
    dimensions++;
  }
  return dimensions > 0 ? completed / (double)dimensions : 0.0;
}

static void ProgressCloseWindow(ViewContext *ctx) {
  WINDOW *window;

  if (!ctx || !ctx->progress.window)
    return;
  window = ctx->progress.window;
  ctx->progress.window = NULL;
  UI_Dialog_Close(ctx, window);
}

static void ProgressClearFooterSpinner(ViewContext *ctx) {
  WINDOW *window;
  int height;
  int width;

  if (!ctx)
    return;
  window = ctx->ctx_menu_window ? ctx->ctx_menu_window : stdscr;
  getmaxyx(window, height, width);
  if (height < 2 || width < 2)
    return;
  mvwaddch(window, height - 2, width - 2, ' ');
  wrefresh(window);
}

static int ProgressWindowWidth(void) {
  int width = COLS - 4;

  if (width > PROGRESS_WINDOW_MAX_WIDTH)
    width = PROGRESS_WINDOW_MAX_WIDTH;
  if (width < PROGRESS_WINDOW_MIN_WIDTH && COLS >= PROGRESS_WINDOW_MIN_WIDTH)
    width = PROGRESS_WINDOW_MIN_WIDTH;
  return width;
}

static WINDOW *ProgressEnsureWindow(ViewContext *ctx, int height) {
  WINDOW *window;
  int available_height;
  int width;
  int x;
  int y;

  if (!ctx || COLS < PROGRESS_MIN_TERMINAL_WIDTH ||
      LINES < height + PROGRESS_FOOTER_ROWS)
    return NULL;
  width = ProgressWindowWidth();
  if (width < PROGRESS_MIN_TERMINAL_WIDTH)
    return NULL;
  available_height = LINES - PROGRESS_FOOTER_ROWS;
  x = MAXIMUM(0, (COLS - width) / 2);
  y = MAXIMUM(0, (available_height - height) / 2);

  window = ctx->progress.window;
  if (window) {
    int current_height;
    int current_width;

    getmaxyx(window, current_height, current_width);
    if (current_height == height && current_width == width) {
      (void)mvwin(window, y, x);
      return window;
    }
    ProgressCloseWindow(ctx);
  }

  window = newwin(height, width, y, x);
  if (!window)
    return NULL;
  if (UI_Dialog_Push(window, UI_TIER_POPOVER) != 0) {
    delwin(window);
    return NULL;
  }
  ctx->progress.window = window;
  keypad(window, TRUE);
  WbkgdSet(ctx, window, COLOR_PAIR(UI_ROLE_PICKER));
  return window;
}

static void ProgressPrintTruncated(WINDOW *window, int row, int width,
                                   const char *text) {
  char buffer[PATH_LENGTH + 96];

  if (!window || !text || width <= 0)
    return;
  snprintf(buffer, sizeof(buffer), "%s", text);
  if ((int)strlen(buffer) > width) {
    if (width >= PROGRESS_ELLIPSIS_WIDTH) {
      buffer[width - 3] = '.';
      buffer[width - 2] = '.';
      buffer[width - 1] = '.';
    }
    buffer[width] = '\0';
  }
  mvwaddnstr(window, row, 2, buffer, width);
}

void Progress_Start(ViewContext *ctx, const char *operation,
                    const char *source_path, const char *dest_path,
                    long long bytes_total, unsigned int items_total) {
  if (!ctx)
    return;

  if (ctx->progress.window)
    ProgressCloseWindow(ctx);
  memset(&ctx->progress, 0, sizeof(ctx->progress));
  ctx->progress.active = TRUE;
  snprintf(ctx->progress.operation, sizeof(ctx->progress.operation), "%s",
           operation ? operation : "WORKING");
  snprintf(ctx->progress.source_path, sizeof(ctx->progress.source_path), "%s",
           source_path ? source_path : "");
  snprintf(ctx->progress.dest_path, sizeof(ctx->progress.dest_path), "%s",
           dest_path ? dest_path : "");
  ctx->progress.bytes_total = bytes_total > 0 ? bytes_total : 0;
  ctx->progress.items_total = items_total;
  ctx->progress.start_monotonic_seconds = ProgressMonotonicSeconds();
  ctx->progress.last_render_monotonic_seconds =
      ctx->progress.start_monotonic_seconds;
  DrawSpinner(ctx);
}

BOOL Progress_ShouldRender(ViewContext *ctx) {
  double now;

  if (!ctx)
    return FALSE;
  now = ProgressMonotonicSeconds();
  if (ProgressElapsedSince(ctx->progress.last_render_monotonic_seconds, now) <
      PROGRESS_RENDER_INTERVAL_SECONDS)
    return FALSE;
  ctx->progress.last_render_monotonic_seconds = now;
  return TRUE;
}

BOOL Progress_Update(ViewContext *ctx, long long bytes_done,
                     unsigned int items_done) {
  double completed;
  double elapsed;
  double now;

  if (!ctx || !ctx->progress.active)
    return TRUE;
  ctx->progress.bytes_done = bytes_done;
  ctx->progress.items_done = items_done;
  now = ProgressMonotonicSeconds();
  elapsed = ProgressElapsedSince(ctx->progress.start_monotonic_seconds, now);
  if (elapsed > 0.0 && bytes_done > 0)
    ctx->progress.bytes_per_sec = (double)bytes_done / elapsed;
  if (elapsed > 0.0 && items_done > 0)
    ctx->progress.items_per_sec = (double)items_done / elapsed;
  completed = ProgressCompletionFraction(&ctx->progress);
  if (elapsed > 0.0 && completed > 0.0)
    ctx->progress.eta_seconds = completed >= PROGRESS_COMPLETE_FRACTION
                                    ? 0
                                    : (int)(elapsed * (1.0 - completed) /
                                            completed);
  if (Progress_ShouldRender(ctx))
    DrawSpinner(ctx);
  if (EscapeKeyPressed(ctx)) {
    ctx->progress.cancel_requested = TRUE;
    (void)snprintf(ctx->progress.error_message,
                   sizeof(ctx->progress.error_message), "%s",
                   "Operation Interrupted");
    return FALSE;
  }
  return TRUE;
}

void Progress_SetTotals(ViewContext *ctx, long long bytes_total,
                        unsigned int items_total) {
  if (!ctx || !ctx->progress.active)
    return;

  ctx->progress.bytes_total = bytes_total > 0 ? bytes_total : 0;
  ctx->progress.items_total = items_total;
  ctx->progress.bytes_done = 0;
  ctx->progress.items_done = 0;
  ctx->progress.bytes_per_sec = 0.0;
  ctx->progress.items_per_sec = 0.0;
  ctx->progress.eta_seconds = 0;
}

BOOL Progress_Advance(ViewContext *ctx, long long bytes_delta,
                      unsigned int items_delta) {
  long long bytes_done;
  unsigned int items_done;

  if (!ctx || !ctx->progress.active)
    return TRUE;
  bytes_done = ctx->progress.bytes_done;
  items_done = ctx->progress.items_done;
  if (bytes_delta > 0) {
    if (bytes_done > LLONG_MAX - bytes_delta)
      bytes_done = LLONG_MAX;
    else
      bytes_done += bytes_delta;
  }
  if (items_delta > UINT_MAX - items_done)
    items_done = UINT_MAX;
  else
    items_done += items_delta;
  if (ctx->progress.bytes_total > 0 &&
      bytes_done > ctx->progress.bytes_total)
    bytes_done = ctx->progress.bytes_total;
  if (ctx->progress.items_total > 0 &&
      items_done > ctx->progress.items_total)
    items_done = ctx->progress.items_total;
  return Progress_Update(ctx, bytes_done, items_done);
}

void Progress_Finish(ViewContext *ctx) {
  if (!ctx)
    return;
  ctx->progress.active = FALSE;
  ctx->progress.promoted = FALSE;
  ProgressCloseWindow(ctx);
  ProgressClearFooterSpinner(ctx);
  if (ctx->progress.error_message[0])
    UI_ShowStatusLineError(ctx, "%s", ctx->progress.error_message);
}

void Progress_Render(ViewContext *ctx) {
  const ProgressContext *p;
  WINDOW *window;
  BOOL byte_progress;
  BOOL item_progress;
  char eta_str[32];
  char line1[(PATH_LENGTH * 2) + 96];
  char speed_str[32];
  char stats[160];
  char time_str[32];
  int bar_width;
  int content_width;
  int elapsed_sec;
  int filled_width;
  int i;
  int percentage;
  int pulse_cycle;
  int pulse_start;
  int pulse_width;
  int window_height;
  int window_width;
  double now;
  double elapsed;
  double completed;

  if (!ctx || !ctx->progress.active)
    return;
  p = &ctx->progress;
  if (p->dest_path[0])
    snprintf(line1, sizeof(line1), "%s: %s TO: %s", p->operation,
             p->source_path, p->dest_path);
  else if (p->source_path[0])
    snprintf(line1, sizeof(line1), "%s: %s", p->operation, p->source_path);
  else
    snprintf(line1, sizeof(line1), "%s", p->operation);

  now = ProgressMonotonicSeconds();
  elapsed = ProgressElapsedSince(p->start_monotonic_seconds, now);
  elapsed_sec = (int)elapsed;
  if (!ctx->progress.promoted && elapsed < PROGRESS_PROMOTION_SECONDS)
    return;
  ctx->progress.promoted = TRUE;
  window_height = PROGRESS_BAR_HEIGHT;
  window = ProgressEnsureWindow(ctx, window_height);
  if (!window)
    return;
  getmaxyx(window, window_height, window_width);
  content_width = window_width - 4;
  werase(window);
  wattron(window, COLOR_PAIR(UI_ROLE_PICKER) | A_ALTCHARSET);
  wborder(window, 0, 0, 0, 0, 0, 0, 0, 0);
  wattroff(window, A_ALTCHARSET);
  ProgressPrintTruncated(window, 1, content_width, line1);

  byte_progress = p->bytes_total > 0;
  item_progress = !byte_progress && p->items_total > 0;
  completed = ProgressCompletionFraction(p);
  if (byte_progress || item_progress) {
    percentage = (int)(completed * PROGRESS_PERCENT_SCALE);
  } else {
    percentage = -1;
  }
  if (percentage > 100)
    percentage = 100;
  if (percentage == 100)
    percentage = 99;
  if (percentage == 0 && completed > 0.0)
    percentage = 1;
  String_FormatCompactDuration(elapsed_sec, time_str, sizeof(time_str));
  if ((byte_progress || item_progress) && completed > 0.0)
    String_FormatCompactDuration(p->eta_seconds, eta_str, sizeof(eta_str));
  else
    snprintf(eta_str, sizeof(eta_str), "--");
  if (p->bytes_per_sec > 0.0)
    snprintf(speed_str, sizeof(speed_str), "%.2f MB/s",
             p->bytes_per_sec / (1024.0 * 1024.0));
  else if (p->items_per_sec > 0.0)
    snprintf(speed_str, sizeof(speed_str), "%.2f item/s", p->items_per_sec);
  else
    snprintf(speed_str, sizeof(speed_str), "--");

  bar_width = content_width - 2;
  if (bar_width > PROGRESS_BAR_MAX_WIDTH)
    bar_width = PROGRESS_BAR_MAX_WIDTH;
  if (bar_width < 1)
    bar_width = 1;
  filled_width = percentage >= 0 ? (percentage * bar_width) / 100 : 0;
  if (percentage > 0 && filled_width == 0)
    filled_width = 1;
  pulse_width = bar_width < PROGRESS_PULSE_WIDTH ? 1 : PROGRESS_PULSE_WIDTH;
  pulse_cycle = 2 * (bar_width - pulse_width);
  pulse_start = 0;
  if (percentage < 0 && pulse_cycle > 0) {
    pulse_start = (int)(p->items_done % (unsigned int)pulse_cycle);
    if (pulse_start > bar_width - pulse_width)
      pulse_start = pulse_cycle - pulse_start;
  }

  wmove(window, 2, 2);
  waddch(window, '[');
  for (i = 0; i < bar_width; ++i) {
    if ((percentage >= 0 && i < filled_width) ||
        (percentage < 0 && i >= pulse_start &&
         i < pulse_start + pulse_width))
      waddch(window, ACS_BLOCK);
    else
      waddch(window, ' ');
  }
  waddch(window, ']');

  if (byte_progress)
    snprintf(stats, sizeof(stats),
             "Progress: %d%% Elapsed: %s Left: %s Rate: %s", percentage,
             time_str, eta_str, speed_str);
  else if (item_progress)
    snprintf(stats, sizeof(stats),
             "Progress: %d%% Elapsed: %s Left: %s Rate: %s", percentage,
             time_str, eta_str, speed_str);
  else
    snprintf(stats, sizeof(stats),
             "Progress: -- Elapsed: %s Left: -- Work: %u", time_str,
             p->items_done);
  ProgressPrintTruncated(window, 3, content_width, stats);
  wrefresh(window);
}
