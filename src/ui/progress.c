/***************************************************************************
 *
 * src/ui/progress.c
 * Universal progress/ETA display system
 *
 ***************************************************************************/

#include "ytnova_ui.h"
#include <time.h>
#include <string.h>

/*
 * Initialize progress context for a new operation.
 *
 * Parameters:
 *   ctx         - ViewContext pointer
 *   operation   - Operation verb (e.g., "COPYING", "MOVING")
 *   source_path - Source file/directory path
 *   dest_path   - Destination path (empty string "" for delete/scan ops)
 *   bytes_total - Total bytes to process (0 if unknown)
 *   items_total - Total items count (0 if unknown)
 */
void Progress_Start(ViewContext *ctx, const char *operation,
                   const char *source_path, const char *dest_path,
                   long long bytes_total, unsigned int items_total) {
  if (!ctx)
    return;

  ctx->progress.active = TRUE;
  strncpy(ctx->progress.operation, operation, sizeof(ctx->progress.operation) - 1);
  ctx->progress.operation[sizeof(ctx->progress.operation) - 1] = '\0';

  strncpy(ctx->progress.source_path, source_path, PATH_LENGTH);
  ctx->progress.source_path[PATH_LENGTH] = '\0';

  strncpy(ctx->progress.dest_path, dest_path, PATH_LENGTH);
  ctx->progress.dest_path[PATH_LENGTH] = '\0';

  ctx->progress.bytes_total = bytes_total;
  ctx->progress.bytes_done = 0;
  ctx->progress.items_total = items_total;
  ctx->progress.items_done = 0;

  ctx->progress.start_time = time(NULL);
  ctx->progress.last_render_time = ctx->progress.start_time;
  ctx->progress.bytes_per_sec = 0.0;
  ctx->progress.eta_seconds = 0;
  ctx->progress.cancel_requested = FALSE;
}

BOOL Progress_ShouldRender(ViewContext *ctx) {
  time_t now;

  if (!ctx)
    return FALSE;
  now = time(NULL);
  if (now <= ctx->progress.last_render_time)
    return FALSE;
  ctx->progress.last_render_time = now;
  return TRUE;
}

/*
 * Update progress metrics.
 *
 * Parameters:
 *   ctx        - ViewContext pointer
 *   bytes_done - Bytes processed so far
 *   items_done - Items processed so far (0 if N/A)
 *
 * Returns: TRUE if operation should continue, FALSE if canceled
 */
BOOL Progress_Update(ViewContext *ctx, long long bytes_done,
                    unsigned int items_done) {
  time_t now;
  double elapsed;

  if (!ctx || !ctx->progress.active)
    return TRUE;

  ctx->progress.bytes_done = bytes_done;
  ctx->progress.items_done = items_done;

  /* Calculate transfer rate and ETA */
  now = time(NULL);
  elapsed = difftime(now, ctx->progress.start_time);

  if (elapsed > 0.0) {
    ctx->progress.bytes_per_sec = (double)bytes_done / elapsed;

    if (ctx->progress.bytes_total > 0 && ctx->progress.bytes_per_sec > 0.0) {
      long long bytes_remaining = ctx->progress.bytes_total - bytes_done;
      ctx->progress.eta_seconds = (int)(bytes_remaining / ctx->progress.bytes_per_sec);
    }
  }

  /* Render progress display */
  Progress_Render(ctx);

  /* Check for Esc key cancellation */
  if (EscapeKeyPressed()) {
    ctx->progress.cancel_requested = TRUE;
    return FALSE;
  }

  return TRUE;
}

/*
 * Finalize and clear progress display.
 */
void Progress_Finish(ViewContext *ctx) {
  if (!ctx)
    return;

  ctx->progress.active = FALSE;

  /* Clear progress display area (bottom 2 lines) */
  if (ctx->ctx_menu_window) {
    werase(ctx->ctx_menu_window);
    wnoutrefresh(ctx->ctx_menu_window);
  }
  if (ctx->ctx_error_window) {
    werase(ctx->ctx_error_window);
    wnoutrefresh(ctx->ctx_error_window);
  }

  doupdate();
}

/*
 * Render the two-line progress display.
 */
void Progress_Render(ViewContext *ctx) {
  const ProgressContext *p;
  int screen_width;
  int bar_width;
  int filled_width;
  char line1[256];
  char time_str[32];
  char eta_str[32];
  char speed_str[32];
  int percentage;
  time_t now;
  int elapsed_sec;

  if (!ctx || !ctx->progress.active)
    return;

  p = &ctx->progress;

  /* Get screen width */
  screen_width = COLS;
  if (screen_width <= 0)
    screen_width = 80;

  /* === LINE 1: OPERATION: source TO: destination === */

  if (p->dest_path[0] != '\0') {
    snprintf(line1, sizeof(line1), "%s: %s TO: %s",
             p->operation, p->source_path, p->dest_path);
  } else {
    snprintf(line1, sizeof(line1), "%s: %s",
             p->operation, p->source_path);
  }

  /* Truncate line1 if too long */
  if ((int)strlen(line1) > screen_width - 2) {
    line1[screen_width - 5] = '.';
    line1[screen_width - 4] = '.';
    line1[screen_width - 3] = '.';
    line1[screen_width - 2] = '\0';
  }

  /* === LINE 2: Progress bar + stats === */

  /* Calculate percentage */
  if (p->bytes_total > 0) {
    percentage = (int)((p->bytes_done * 100) / p->bytes_total);
    if (percentage > 100)
      percentage = 100;
  } else {
    percentage = 0;
  }

  /* Format elapsed time */
  now = time(NULL);
  elapsed_sec = (int)difftime(now, p->start_time);
  snprintf(time_str, sizeof(time_str), "%02d:%02d",
           elapsed_sec / 60, elapsed_sec % 60);

  /* Format ETA */
  if (p->eta_seconds > 0 && p->bytes_total > 0) {
    snprintf(eta_str, sizeof(eta_str), "%02d:%02d",
             p->eta_seconds / 60, p->eta_seconds % 60);
  } else {
    snprintf(eta_str, sizeof(eta_str), "%s", "--:--");
  }

  /* Format speed */
  if (p->bytes_per_sec > 0.0) {
    double mb_per_sec = p->bytes_per_sec / (1024.0 * 1024.0);
    snprintf(speed_str, sizeof(speed_str), "%.1f MB/s", mb_per_sec);
  } else {
    snprintf(speed_str, sizeof(speed_str), "%s", "-- MB/s");
  }

  /* Build progress bar */
  bar_width = screen_width - 60; /* Reserve space for stats */
  if (bar_width < 10)
    bar_width = 10;
  if (bar_width > 40)
    bar_width = 40;

  filled_width = (percentage * bar_width) / 100;

  /* Render to menu_window (bottom area) */
  if (ctx->ctx_menu_window) {
    werase(ctx->ctx_menu_window);
    mvwprintw(ctx->ctx_menu_window, 0, 0, "%s", line1);

    /* Line 2 start */
    wmove(ctx->ctx_menu_window, 1, 0);
    waddch(ctx->ctx_menu_window, '[');

    /* Progress bar */
    int i;
    for (i = 0; i < bar_width; i++) {
      if (i < filled_width) {
        waddch(ctx->ctx_menu_window, ACS_BLOCK);
      } else {
        waddch(ctx->ctx_menu_window, ' ');
      }
    }
    waddch(ctx->ctx_menu_window, ']');

    /* Stats */
    wprintw(ctx->ctx_menu_window, " %3d%%  Time: %s  ETA: %s  Speed: %s",
            percentage, time_str, eta_str, speed_str);

    wnoutrefresh(ctx->ctx_menu_window);
    doupdate();
  }
}
