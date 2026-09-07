/***************************************************************************
 *
 * src/ui/stats.c
 * Statistics Module - Modernized Boxed Layout
 * Refactored to share attribute display logic between files and directories.
 * Responsive layout update.
 *
 ***************************************************************************/

#include "ytnova_appstate_volume.h"
#include "ytnova_cmd.h"
#include "ytnova_fs.h"
#include "ytnova_panel_anchor.h"
#include "ytnova_ui.h"

#include <curses.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  const YtreeNovaPanel *panel;
  int width;
  int x;
  int left_border;
  int right_border;
} StatsProjection;

#define STAT_W (projection->width)
#define STAT_X (projection->x)
#define L_BORDER (projection->left_border)
#define R_BORDER (projection->right_border)
#define INNER_W (projection->width - 2)

/* Y-Coordinates (Dynamic) */
#define Y_TOP 1

/* Prototypes */
static void RecalcLayout(ViewContext *ctx);
static void FormatNumber(const ViewContext *ctx, char *buf, size_t size,
                         long long val);
static void FormatCompactCount(char *buf, size_t size, long long val);
static void FormatShortSize(char *buf, size_t size, long long val);
static void FormatDisplaySize(char *buf, size_t size, long long val);
static void SetStatsBaseColor(ViewContext *ctx);
static void SetStatsStaticColor(ViewContext *ctx);
static void SetStatsDynamicColor(ViewContext *ctx);
static void SetStatsBorderColor(ViewContext *ctx);
static BOOL ResolveStatsProjection(const ViewContext *ctx,
                                   const YtreeNovaPanel *panel,
                                   StatsProjection *projection);
static void DrawBoxFrame(ViewContext *ctx, const StatsProjection *projection);
static void DrawSeparator(ViewContext *ctx, const StatsProjection *projection,
                          int y, const char *title);
static void PrintStatRow(ViewContext *ctx, const StatsProjection *projection,
                         int y, const char *label,
                         long long count, long long bytes);
static void PrintStatsDynamicLine(ViewContext *ctx,
                                  const StatsProjection *projection, int y,
                                  const char *value);
static void PrintStatsLabelValue(ViewContext *ctx,
                                 const StatsProjection *projection, int y,
                                 const char *label,
                                 const char *value);
static void DescribeDirViewState(const YtreeNovaPanel *panel, char *buf,
                                 size_t size);
static void DescribeFileViewState(const YtreeNovaPanel *panel, char *buf,
                                  size_t size);
static void DrawAttributes(ViewContext *ctx, const StatsProjection *projection,
                           const char *name,
                           const struct stat *s, const FileEntry *fe);
static void RecalcDir(BOOL hide_dot_files, DirEntry *d, Statistic *s);

/* ************************************************************************* */
/*                           LOGIC FUNCTIONS                                 */
/* ************************************************************************* */

static void RecalcLayout(ViewContext *ctx) {
  if (LINES < 26) {
    /* Compact Mode for small terminals (e.g. 24 lines) */
    ctx->layout.stats_y_filter_val = 2;
    ctx->layout.stats_y_vol_sep = 0;   /* Hidden */
    ctx->layout.stats_y_vol_info = 3;  /* 3, 4, 5 */
    ctx->layout.stats_y_vstat_sep = 0; /* Hidden */
    ctx->layout.stats_y_vstat_val = 6; /* 6, 7, 8 */
    ctx->layout.stats_y_dstat_sep = 0; /* Hidden */
    ctx->layout.stats_y_dstat_val = 9; /* 9, 10, 11, 12, 13 */
    ctx->layout.stats_y_attr_sep = 0;  /* Hidden */
    ctx->layout.stats_y_attr_val = 14; /* 14, 15, 16, 17, 18 */
    /* Total used: 2 to 18. 19 is spacer. 20 is border. Fits in 20 (LINES=24 ->
     * ctx->layout.bottom_border_y=20) */
  } else {
    /* Standard Spacious Mode */
    ctx->layout.stats_y_filter_val = 2;
    ctx->layout.stats_y_vol_sep = 3;
    ctx->layout.stats_y_vol_info = 4;
    ctx->layout.stats_y_vstat_sep = 7;
    ctx->layout.stats_y_vstat_val = 8;
    ctx->layout.stats_y_dstat_sep = 11;
    ctx->layout.stats_y_dstat_val = 12;
    ctx->layout.stats_y_attr_sep = 17;
    ctx->layout.stats_y_attr_val = 18;
  }
}

static BOOL ResolveStatsProjection(const ViewContext *ctx,
                                   const YtreeNovaPanel *panel,
                                   StatsProjection *projection) {
  if (!ctx || !projection || !ctx->ctx_border_window)
    return FALSE;

  projection->panel = panel ? panel : ctx->active;
  if (ctx->is_split_screen) {
    if (!projection->panel || !projection->panel->show_stats ||
        projection->panel->stats_width <= 0)
      return FALSE;
    projection->width = projection->panel->stats_width;
    projection->x = projection->panel->stats_x;
    projection->left_border = projection->x;
    projection->right_border = projection->x + projection->width;
  } else {
    if (ctx->layout.stats_width <= 0)
      return FALSE;
    projection->width = ctx->layout.stats_width;
    projection->x = COLS - projection->width;
    projection->left_border = projection->x - 1;
    projection->right_border = COLS - 1;
  }

  return projection->width > 2 && projection->left_border >= 0 &&
         projection->right_border < COLS;
}

static void RecalcDir(BOOL hide_dot_files, DirEntry *d, Statistic *s) {
  FileEntry *f;
  DirEntry *sub;
  unsigned int total_files;
  unsigned int tagged_files;
  long long total_bytes;
  long long tagged_bytes;

  /* Apply current filter to this directory */
  ApplyFilter(d, s);

  total_files = 0;
  total_bytes = 0;
  tagged_files = d->tagged_files;
  tagged_bytes = d->tagged_bytes;
  /* matching_files/bytes already updated by ApplyFilter, but we sum them
   * globally below */

  for (f = d->file; f; f = f->next) {
    if (hide_dot_files && f->name[0] == '.')
      continue;

    total_files++;
    total_bytes += f->stat_struct.st_size;

    if (f->tagged) {
      tagged_files++;
      tagged_bytes += f->stat_struct.st_size;
    }
  }
  if (!AppStateCommitDirEntryTotalPayload(d, total_files, total_bytes))
    return;
  if (!AppStateCommitDirEntryTaggedPayload(d, tagged_files, tagged_bytes))
    return;

  sub = d->sub_tree;
  while (sub) {
    RecalcDir(hide_dot_files, sub, s);
    s->disk_total_directories++;
    sub = sub->next;
  }

  s->disk_total_files += d->total_files;
  s->disk_total_bytes += d->total_bytes;
  s->disk_matching_files += d->matching_files;
  s->disk_matching_bytes += d->matching_bytes;
  s->disk_tagged_files += d->tagged_files;
  s->disk_tagged_bytes += d->tagged_bytes;
}

void RecalculateSysStats(ViewContext *ctx, Statistic *s) {
  BOOL hide_dot_files = (ctx && ctx->active && ctx->active->hide_dot_files);

  s->disk_total_files = 0;
  s->disk_total_bytes = 0;
  s->disk_matching_files = 0;
  s->disk_matching_bytes = 0;
  s->disk_tagged_files = 0;
  s->disk_tagged_bytes = 0;
  s->disk_total_directories = 0;

  if (s->tree) {
    s->disk_total_directories++;
    RecalcDir(hide_dot_files, s->tree, s);
  }
}

/* ************************************************************************* */
/*                           DISPLAY HELPERS                                 */
/* ************************************************************************* */

static void FormatNumber(const ViewContext *ctx, char *buf, size_t size,
                         long long val) {
  char temp[64];
  int len, i, j, commacount;

  snprintf(temp, sizeof(temp), "%lld", val);
  len = strlen(temp);
  commacount = (len - 1) / 3;

  if (len + commacount >= (int)size) {
    snprintf(buf, size, "%lld", val);
    return;
  }

  j = len + commacount;
  buf[j] = '\0';

  for (i = len - 1; i >= 0; i--) {
    buf[--j] = temp[i];
    if (i > 0 && (len - i) % 3 == 0) {
      buf[--j] = ctx->number_seperator;
    }
  }
}

static void FormatCompactCount(char *buf, size_t size, long long val) {
  double d = (double)val;
  int i = 0;

  if (val < 0) {
    snprintf(buf, size, "Err");
    return;
  }

  while (d >= 999.5 && i < 5) {
    d /= 1000.0;
    i++;
  }

  if (i == 0) {
    snprintf(buf, size, "%lld", val);
  } else {
    static const char *const compact_units[] = {"", "K", "M", "G", "T", "P"};
    snprintf(buf, size, "%.1f%s", d, compact_units[i]);
  }
}

static void FormatShortSize(char *buf, size_t size, long long val) {
  double d = (double)val;
  const char *units[] = {"B", "K", "M", "G", "T", "P"};
  int i = 0;

  /* Handle negative values gracefully (though they shouldn't happen) */
  if (val < 0) {
    snprintf(buf, size, "Err");
    return;
  }

  while (d >= 999.5 &&
         i < 5) { /* threshold slightly < 1000 to avoid "1000K" -> "1.0M" */
    d /= 1024.0;
    i++;
  }

  if (i == 0) {
    /* Bytes: max "999B" (4 chars) */
    snprintf(buf, size, "%lld%s", val, units[i]);
  } else {
    /* Units: "1.2M", "100G" */
    /* Use %.1f for < 10, %.0f for >= 10 to save space?
       Standard: just ensure it fits.
       "999.9G" is 6 chars. "1000T" is 5 chars. Safe. */
    snprintf(buf, size, "%.1f%s", d, units[i]);
  }
}

static void FormatDisplaySize(char *buf, size_t size, long long val) {
  FormatShortSize(buf, size, val);
}

static void SetStatsBaseColor(ViewContext *ctx) {
  if (!ctx || !ctx->ctx_border_window)
    return;
#ifdef COLOR_SUPPORT
  wattrset(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));
#else
  wattrset(ctx->ctx_border_window, A_NORMAL);
#endif
}

static void SetStatsStaticColor(ViewContext *ctx) {
  if (!ctx || !ctx->ctx_border_window)
    return;
#ifdef COLOR_SUPPORT
  wattrset(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_STATIC_TEXT));
#else
  wattrset(ctx->ctx_border_window, A_BOLD);
#endif
}

static void SetStatsDynamicColor(ViewContext *ctx) {
  if (!ctx || !ctx->ctx_border_window)
    return;
#ifdef COLOR_SUPPORT
  wattrset(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));
#else
  wattrset(ctx->ctx_border_window, A_NORMAL);
#endif
}

static void SetStatsBorderColor(ViewContext *ctx) {
  if (!ctx || !ctx->ctx_border_window)
    return;
#ifdef COLOR_SUPPORT
  wattrset(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_BOX_LINES));
#else
  wattrset(ctx->ctx_border_window, A_NORMAL);
#endif
}

static void DrawBoxFrame(ViewContext *ctx, const StatsProjection *projection) {
  int y;
  int sep_y;
  const YtreeNovaPanel *panel;
  BOOL shared_split_border = FALSE;
  BOOL right_big_file = FALSE;

  if (!ctx || !projection || !ctx->ctx_border_window)
    return;

  sep_y = ctx->layout.dir_win_y + ctx->layout.dir_win_height;
  panel = projection->panel;

  if (ctx->is_split_screen && panel == ctx->left && ctx->right) {
    shared_split_border = TRUE;
    right_big_file =
        ctx->right->pan_file_window == ctx->right->pan_big_file_window;
  }

  SetStatsBorderColor(ctx);
  wattron(ctx->ctx_border_window, A_ALTCHARSET);

  /* --- Top Border with embedded " FILTER " --- */
  {
    const char *title = " FILTER ";
    int hline_len = R_BORDER - L_BORDER - 1;
    int t_len = strlen(title);
    int left_len = (hline_len - t_len) / 2;
    int right_len = hline_len - t_len - left_len;
    int x = L_BORDER + 1;

    /* Left HLINE */
    mvwhline(ctx->ctx_border_window, Y_TOP, x, ACS_HLINE, left_len);
    x += left_len;

    wattroff(ctx->ctx_border_window, A_ALTCHARSET);
    SetStatsStaticColor(ctx);
    mvwaddstr(ctx->ctx_border_window, Y_TOP, x, title);
    SetStatsBorderColor(ctx);
    wattron(ctx->ctx_border_window, A_ALTCHARSET);
    x += t_len;

    /* Right HLINE */
    mvwhline(ctx->ctx_border_window, Y_TOP, x, ACS_HLINE, right_len);
  }

  mvwaddch(ctx->ctx_border_window, Y_TOP, R_BORDER, ACS_URCORNER);

  /* --- Bottom Border --- */
  mvwaddch(ctx->ctx_border_window, ctx->layout.bottom_border_y, L_BORDER,
           ACS_LLCORNER);
  mvwhline(ctx->ctx_border_window, ctx->layout.bottom_border_y, L_BORDER + 1,
           ACS_HLINE, R_BORDER - L_BORDER - 1);
  mvwaddch(ctx->ctx_border_window, ctx->layout.bottom_border_y, R_BORDER,
           shared_split_border ? ACS_BTEE : ACS_LRCORNER);

  /* --- Vertical Lines --- */
  for (y = Y_TOP + 1; y < ctx->layout.bottom_border_y; y++) {
    mvwaddch(ctx->ctx_border_window, y, R_BORDER, ACS_VLINE);
    mvwaddch(ctx->ctx_border_window, y, L_BORDER, ACS_VLINE);
  }

  /* --- Junctions --- */
  mvwaddch(ctx->ctx_border_window, Y_TOP, L_BORDER,
           ACS_TTEE); /* Connects to Path bar in main win */

  /* Handle File Window artifact */
  if (panel && panel->pan_file_window == panel->pan_big_file_window) {
    mvwaddch(ctx->ctx_border_window, sep_y, L_BORDER, ACS_VLINE);
  } else {
    mvwaddch(ctx->ctx_border_window, sep_y, L_BORDER, ACS_RTEE);
  }
  if (shared_split_border) {
    mvwaddch(ctx->ctx_border_window, sep_y, R_BORDER,
             right_big_file ? ACS_VLINE : ACS_LTEE);
  }
  mvwaddch(ctx->ctx_border_window, ctx->layout.bottom_border_y, L_BORDER,
           ACS_BTEE);

  wattroff(ctx->ctx_border_window, A_ALTCHARSET);
  SetStatsBaseColor(ctx);
}

static void DrawSeparator(ViewContext *ctx, const StatsProjection *projection,
                          int y, const char *title) {
  int text_len = title ? strlen(title) : 0;
  int total_inner_width = R_BORDER - L_BORDER - 1;

  if (y <= 0)
    return;

  SetStatsBorderColor(ctx);
  wattron(ctx->ctx_border_window, A_ALTCHARSET);

  /* Side Junctions */
  mvwaddch(ctx->ctx_border_window, y, L_BORDER, ACS_LTEE);
  mvwaddch(ctx->ctx_border_window, y, R_BORDER, ACS_RTEE);

  if (title && text_len > 0) {
    int pad = 2; /* 1 space each side */

    if (total_inner_width >= text_len + pad) {
      int left_hline_len;
      int title_content_start_x;
      int rem = total_inner_width - (text_len + pad);
      left_hline_len = rem / 2;
      title_content_start_x = L_BORDER + 1 + left_hline_len;

      /* Left Line */
      mvwhline(ctx->ctx_border_window, y, L_BORDER + 1, ACS_HLINE,
               left_hline_len);

      wattroff(ctx->ctx_border_window, A_ALTCHARSET);
      SetStatsStaticColor(ctx);
      mvwaddstr(ctx->ctx_border_window, y, title_content_start_x, " ");
      waddstr(ctx->ctx_border_window, title);
      waddstr(ctx->ctx_border_window, " ");
      SetStatsBorderColor(ctx);
      wattron(ctx->ctx_border_window, A_ALTCHARSET);

      /* Right Line */
      mvwhline(ctx->ctx_border_window, y,
               title_content_start_x + text_len + pad, ACS_HLINE,
               total_inner_width - left_hline_len - text_len - pad);
    } else {
      SetStatsStaticColor(ctx);
      mvwaddnstr(ctx->ctx_border_window, y, L_BORDER + 1, title,
                 total_inner_width);
      SetStatsBorderColor(ctx);
    }
  } else {
    /* Pure line */
    mvwhline(ctx->ctx_border_window, y, L_BORDER + 1, ACS_HLINE,
             total_inner_width);
  }
  wattroff(ctx->ctx_border_window, A_ALTCHARSET);
  SetStatsBaseColor(ctx);
}

static void PrintStatRow(ViewContext *ctx, const StatsProjection *projection,
                         int y, const char *label,
                         long long count, long long bytes) {
  char count_buf[32];
  char size_buf[32];
  char value_buf[80];
  int value_width;

  if (y >= ctx->layout.bottom_border_y)
    return;

  value_width = INNER_W - 5;
  if (value_width < 1)
    return;

  FormatNumber(ctx, count_buf, sizeof(count_buf), count);
  FormatDisplaySize(size_buf, sizeof(size_buf), bytes);
  snprintf(value_buf, sizeof(value_buf), "%s %s", count_buf, size_buf);
  if ((int)strlen(value_buf) > value_width) {
    snprintf(count_buf, sizeof(count_buf), "%lld", count);
    snprintf(value_buf, sizeof(value_buf), "%s %s", count_buf, size_buf);
  }
  if ((int)strlen(value_buf) > value_width) {
    FormatCompactCount(count_buf, sizeof(count_buf), count);
    snprintf(value_buf, sizeof(value_buf), "%s %s", count_buf, size_buf);
  }

  SetStatsBaseColor(ctx);
  mvwhline(ctx->ctx_border_window, y, STAT_X + 1, ' ', INNER_W);
  SetStatsStaticColor(ctx);
  mvwprintw(ctx->ctx_border_window, y, STAT_X + 1, "%-4s ", label);
  SetStatsDynamicColor(ctx);
  mvwprintw(ctx->ctx_border_window, y, STAT_X + 6, "%*.*s", value_width,
            value_width, value_buf);
  SetStatsBaseColor(ctx);
}

static void PrintStatsDynamicLine(ViewContext *ctx,
                                  const StatsProjection *projection, int y,
                                  const char *value) {
  char clipped[256];

  if (y >= ctx->layout.bottom_border_y)
    return;

  CutPathname(clipped, (char *)value, INNER_W);
  SetStatsBaseColor(ctx);
  mvwhline(ctx->ctx_border_window, y, STAT_X + 1, ' ', INNER_W);
  SetStatsDynamicColor(ctx);
  mvwprintw(ctx->ctx_border_window, y, STAT_X + 1, "%-*s", INNER_W, clipped);
  SetStatsBaseColor(ctx);
}

static void PrintStatsLabelValue(ViewContext *ctx,
                                 const StatsProjection *projection, int y,
                                 const char *label,
                                 const char *value) {
  int label_len;
  int value_width;

  if (y >= ctx->layout.bottom_border_y)
    return;

  label_len = (int)strlen(label);
  value_width = INNER_W - label_len;
  if (value_width < 0)
    value_width = 0;

  SetStatsBaseColor(ctx);
  mvwhline(ctx->ctx_border_window, y, STAT_X + 1, ' ', INNER_W);
  SetStatsStaticColor(ctx);
  mvwprintw(ctx->ctx_border_window, y, STAT_X + 1, "%s", label);
  SetStatsDynamicColor(ctx);
  if (value_width > 0)
    mvwprintw(ctx->ctx_border_window, y, STAT_X + 1 + label_len, "%-*.*s",
              value_width, value_width, value);
  SetStatsBaseColor(ctx);
}

static const char *BaseViewName(int mode) {
  switch (mode) {
  case MODE_1:
    return "Attributes";
  case MODE_2:
    return "Owner";
  case MODE_4:
    return "Times";
  case MODE_5:
    return "Custom";
  case MODE_3:
  default:
    return "Name";
  }
}

static const char *PrimaryFileInfoName(const YtreeNovaPanel *panel) {
  if (!panel)
    return "Name";

  if (panel->fixed_col_width != 0)
    return "Compact";

  switch (panel->fileinfo_overlay_mode) {
  case FILEINFO_OVERLAY_RICH:
    return "Mini preview";
  case FILEINFO_OVERLAY_SUMMARY:
    return "File";
  case FILEINFO_OVERLAY_GIT:
    return "Git";
  default:
    break;
  }

  return BaseViewName(panel->file_mode);
}

static void DescribeDirViewState(const YtreeNovaPanel *panel, char *buf,
                                 size_t size) {
  if (!buf || size == 0)
    return;

  buf[0] = '\0';
  if (panel &&
      (panel->fileinfo_overlay_mode != FILEINFO_OVERLAY_NONE ||
       panel->fixed_col_width != 0)) {
    snprintf(buf, size, "%s", PrimaryFileInfoName(panel));
    return;
  }
  snprintf(buf, size, "%s", BaseViewName(panel ? panel->dir_mode : MODE_3));
}

static void DescribeFileViewState(const YtreeNovaPanel *panel, char *buf,
                                  size_t size) {
  if (!buf || size == 0)
    return;

  buf[0] = '\0';
  snprintf(buf, size, "%s", PrimaryFileInfoName(panel));
}

static void DrawAttributes(ViewContext *ctx, const StatsProjection *projection,
                           const char *name,
                           const struct stat *s, const FileEntry *fe) {
  char buf[128];
  char num_buf[32];
  char time_buf[20];

  if (!name || !s)
    return;

  DrawSeparator(ctx, projection, ctx->layout.stats_y_attr_sep, "ATTRIBUTES");

  (void)fe;
  PrintStatsDynamicLine(ctx, projection, ctx->layout.stats_y_attr_val, name);

  FormatDisplaySize(num_buf, sizeof(num_buf), s->st_size);
  PrintStatsLabelValue(ctx, projection, ctx->layout.stats_y_attr_val + 1,
                       "Size: ", num_buf);

  GetAttributes(s->st_mode, buf);
  PrintStatsLabelValue(ctx, projection, ctx->layout.stats_y_attr_val + 2,
                       "Attr: ", buf);

  {
    const char *owner = GetDisplayPasswdName(s->st_uid);
    const char *group = GetDisplayGroupName(s->st_gid);
    char owner_buf[32];
    char grp_buf[32];
    if (!owner) {
      snprintf(owner_buf, sizeof(owner_buf), "%d", s->st_uid);
      owner = owner_buf;
    }
    if (!group) {
      snprintf(grp_buf, sizeof(grp_buf), "%d", s->st_gid);
      group = grp_buf;
    }

    char full_own[64];
    snprintf(full_own, sizeof(full_own), "%s:%s", owner, group);
    CutName(buf, full_own, INNER_W - 6); /* "Own : " is 6 chars */
    PrintStatsLabelValue(ctx, projection, ctx->layout.stats_y_attr_val + 3,
                         "Own : ", buf);
  }

  CTime(s->st_mtime, time_buf);
  PrintStatsLabelValue(ctx, projection, ctx->layout.stats_y_attr_val + 4,
                       "Mod : ", time_buf);
}

/* ************************************************************************* */
/*                           DISPLAY FUNCTIONS */
/* ************************************************************************* */

static void DisplayDiskNameProjected(ViewContext *ctx,
                                     const StatsProjection *projection,
                                     const Statistic *s) {
  char buf[128];
  char path_buf[PATH_LENGTH + 1];
  int total_volumes = HASH_COUNT(ctx->volumes_head);
  int current_index = 0;

  if (!s)
    return;

  RecalcLayout(ctx);

  if (ctx->volumes_head) {
    struct Volume *vol_iter, *tmp;
    int i = 1;
    HASH_ITER(hh, ctx->volumes_head, vol_iter, tmp) {
      if (&vol_iter->vol_stats == s) {
        current_index = i;
        break;
      }
      i++;
    }
  }
  if (current_index == 0 && total_volumes > 0)
    current_index = 1;

  SetStatsBaseColor(ctx);
  DrawBoxFrame(ctx, projection);

  CutName(buf, s->file_spec, INNER_W);
  SetStatsBaseColor(ctx);
  mvwhline(ctx->ctx_border_window, ctx->layout.stats_y_filter_val, STAT_X + 1,
           ' ', INNER_W);
  SetStatsDynamicColor(ctx);
  {
    int pad = (INNER_W - strlen(buf)) / 2;
    mvwprintw(ctx->ctx_border_window, ctx->layout.stats_y_filter_val,
              STAT_X + 1, "%*s%-*s", pad, "", INNER_W - pad, buf);
  }
  SetStatsBaseColor(ctx);

  snprintf(buf, sizeof(buf), "VOLUME %d/%d", current_index, total_volumes);
  DrawSeparator(ctx, projection, ctx->layout.stats_y_vol_sep, buf);

  if (ctx->view_mode == ARCHIVE_MODE)
    strncpy(path_buf, s->log_path, PATH_LENGTH);
  else
    strncpy(path_buf, s->path, PATH_LENGTH);
  path_buf[PATH_LENGTH] = '\0';

  PrintStatsDynamicLine(ctx, projection, ctx->layout.stats_y_vol_info,
                        path_buf);

  {
    char fs_buf[64];

    if (ctx->view_mode == ARCHIVE_MODE)
      snprintf(fs_buf, sizeof(fs_buf), "ARCHIVE");
    else
      snprintf(fs_buf, sizeof(fs_buf), "%s", s->disk_name);
    CutName(buf, fs_buf, INNER_W - 4);
    PrintStatsLabelValue(ctx, projection, ctx->layout.stats_y_vol_info + 1,
                         "FS: ", buf);
  }

  if (ctx->view_mode == ARCHIVE_MODE) {
    unsigned int caps = s->archive_capabilities;

    snprintf(buf, sizeof(buf), "%s%s%s%s%s%s",
             (caps & ARCHIVE_CAP_BROWSE) ? "browse " : "",
             (caps & ARCHIVE_CAP_COPY_OUT) ? "copy " : "",
             (caps & ARCHIVE_CAP_ADD) ? "add " : "",
             (caps & ARCHIVE_CAP_DELETE) ? "delete " : "",
             (caps & ARCHIVE_CAP_RENAME) ? "rename " : "",
             (caps & ARCHIVE_CAP_MOVE) ? "move" : "");
    CutName(buf, buf, INNER_W - 6);
    PrintStatsLabelValue(ctx, projection, ctx->layout.stats_y_vol_info + 2,
                         "Ops: ", buf);
  } else {
    char fs_buf[64];
    char size_buf[32];
    int free_percent = -1;

    FormatDisplaySize(size_buf, sizeof(size_buf), s->disk_space);
    if (s->disk_capacity > 0) {
      double percent = ((double)s->disk_space * 100.0) / (double)s->disk_capacity;
      if (percent < 0.0)
        percent = 0.0;
      if (percent > 100.0)
        percent = 100.0;
      free_percent = (int)(percent + 0.5);
    }
    if (free_percent >= 0)
      snprintf(fs_buf, sizeof(fs_buf), "%s (%d%%)", size_buf, free_percent);
    else
      snprintf(fs_buf, sizeof(fs_buf), "%s", size_buf);
    PrintStatsLabelValue(ctx, projection, ctx->layout.stats_y_vol_info + 2,
                         "Free: ", fs_buf);
  }
}

static void DisplayDiskStatisticProjected(ViewContext *ctx,
                                          const StatsProjection *projection,
                                          const Statistic *s) {
  if (!s)
    return;

  DisplayDiskNameProjected(ctx, projection, s);
  DrawSeparator(ctx, projection, ctx->layout.stats_y_vstat_sep,
                "VOLUME STATS");

  PrintStatRow(ctx, projection, ctx->layout.stats_y_vstat_val, "Tot:",
               s->disk_total_files, s->disk_total_bytes);
  PrintStatRow(ctx, projection, ctx->layout.stats_y_vstat_val + 1, "Mat:",
               s->disk_matching_files, s->disk_matching_bytes);
  PrintStatRow(ctx, projection, ctx->layout.stats_y_vstat_val + 2, "Tag:",
               s->disk_tagged_files, s->disk_tagged_bytes);
}

static void DisplayDirStatisticProjected(ViewContext *ctx,
                                         const StatsProjection *projection,
                                         const DirEntry *de,
                                         const char *title,
                                         const Statistic *s) {
  if (!de)
    return;

  /* Use provided title, or fallback to default logic */
  if (title) {
    DrawSeparator(ctx, projection, ctx->layout.stats_y_dstat_sep, title);
  } else if (de->global_flag) {
    DrawSeparator(ctx, projection, ctx->layout.stats_y_dstat_sep, "SHOW ALL");
  } else {
    if (ctx->view_mode == ARCHIVE_MODE) {
      DrawSeparator(ctx, projection, ctx->layout.stats_y_dstat_sep, "ARCHIVE");
    } else {
      DrawSeparator(ctx, projection, ctx->layout.stats_y_dstat_sep,
                    "CURRENT DIR");
    }
  }

  PrintStatsDynamicLine(ctx, projection, ctx->layout.stats_y_dstat_val,
                        de->name);
  {
    char view_buf[64];
    DescribeDirViewState(projection->panel, view_buf, sizeof(view_buf));
    PrintStatsLabelValue(ctx, projection, ctx->layout.stats_y_dstat_val + 1,
                         "View: ", view_buf);
  }

  if (de->global_flag) {
    /* In Show All mode, display global totals */
    PrintStatRow(ctx, projection, ctx->layout.stats_y_dstat_val + 2, "Tot:",
                 s->disk_total_files, s->disk_total_bytes);
    PrintStatRow(ctx, projection, ctx->layout.stats_y_dstat_val + 3, "Mat:",
                 s->disk_matching_files, s->disk_matching_bytes);
  } else {
    /* In Normal mode, display current directory totals */
    PrintStatRow(ctx, projection, ctx->layout.stats_y_dstat_val + 2, "Tot:",
                 de->total_files, de->total_bytes);
    PrintStatRow(ctx, projection, ctx->layout.stats_y_dstat_val + 3, "Mat:",
                 de->matching_files, de->matching_bytes);
  }

  /* Tag count always shows global disk total in Show All mode, but we use the
   * disk stats directly if global_flag is set. */
  if (de->global_flag) {
    PrintStatRow(ctx, projection, ctx->layout.stats_y_dstat_val + 4, "Tag:",
                 s->disk_tagged_files, s->disk_tagged_bytes);
  } else {
    PrintStatRow(ctx, projection, ctx->layout.stats_y_dstat_val + 4, "Tag:",
                 de->tagged_files, de->tagged_bytes);
  }
}

/*
 * DisplayFileStatistic
 * Shows individual file information in the "CURRENT DIR" statistics area
 * when the user is navigating files (small or big window mode).
 */
static void DisplayFileStatisticProjected(ViewContext *ctx,
                                          const StatsProjection *projection,
                                          const FileEntry *fe,
                                          const Statistic *s) {
  char size_buf[32];
  char time_buf[20];

  if (!fe)
    return;

  DrawSeparator(ctx, projection, ctx->layout.stats_y_dstat_sep,
                "CURRENT FILE");

  PrintStatsDynamicLine(ctx, projection, ctx->layout.stats_y_dstat_val,
                        fe->name);
  {
    char view_buf[64];
    DescribeFileViewState(projection->panel, view_buf, sizeof(view_buf));
    PrintStatsLabelValue(ctx, projection, ctx->layout.stats_y_dstat_val + 1,
                         "View: ", view_buf);
  }

  FormatDisplaySize(size_buf, sizeof(size_buf), fe->stat_struct.st_size);
  PrintStatsLabelValue(ctx, projection, ctx->layout.stats_y_dstat_val + 2,
                       "Size: ", size_buf);

  {
    char attr_buf[16];
    GetAttributes(fe->stat_struct.st_mode, attr_buf);
    PrintStatsLabelValue(ctx, projection, ctx->layout.stats_y_dstat_val + 3,
                         "Perm: ", attr_buf);
  }

  CTime(fe->stat_struct.st_mtime, time_buf);
  PrintStatsLabelValue(ctx, projection, ctx->layout.stats_y_dstat_val + 4,
                       "Mod : ", time_buf);
}

static void DisplayFileParameterProjected(ViewContext *ctx,
                                          const StatsProjection *projection,
                                          FileEntry *fe) {
  if (fe) {
    DrawAttributes(ctx, projection, fe->name, &fe->stat_struct, fe);
  }
}

static void DisplayDirParameterProjected(ViewContext *ctx,
                                         const StatsProjection *projection,
                                         DirEntry *de) {
  if (de)
    DrawAttributes(ctx, projection, de->name, &de->stat_struct, NULL);
}

static BOOL ResolveDefaultStatsProjection(const ViewContext *ctx,
                                          StatsProjection *projection) {
  return ResolveStatsProjection(ctx,
                                ctx && ctx->is_split_screen ? ctx->active : NULL,
                                projection);
}

void DisplayPanelStatistics(ViewContext *ctx, YtreeNovaPanel *panel) {
  StatsProjection projection;
  const Statistic *s;
  DirEntry *de;
  FileEntry *fe = NULL;
  int y;

  if (!ctx || !panel || !panel->vol ||
      !ResolveStatsProjection(ctx, panel, &projection))
    return;

  s = &panel->vol->vol_stats;
  /* Split separator redraws can leave a horizontal rule in an otherwise
   * untouched statistics interior. Clear the projected strip before filling
   * it so the panel frame remains self-contained. */
  for (y = Y_TOP + 1; y < ctx->layout.bottom_border_y; y++)
    mvwhline(ctx->ctx_border_window, y, projection.left_border + 1, ' ',
             projection.right_border - projection.left_border - 1);
  de = GetPanelDirEntry(panel);
  if (panel->saved_focus == FOCUS_FILE &&
      panel->file_selection_dir_path[0] != '\0') {
    DirEntry *file_dir = ResolvePanelAnchorTarget(
        panel, panel->vol, panel->file_selection_dir_path);
    if (file_dir)
      de = file_dir;
  }

  DisplayDiskStatisticProjected(ctx, &projection, s);
  if (!de)
    return;

  if (panel->saved_focus == FOCUS_FILE && panel->file_entry_list &&
      panel->file_count > 0) {
    int file_index = panel->start_file + panel->file_cursor_pos;
    if (file_index >= 0 && (unsigned int)file_index < panel->file_count)
      fe = panel->file_entry_list[file_index].file;
  }

  if (fe) {
    DisplayFileStatisticProjected(ctx, &projection, fe, s);
    DisplayFileParameterProjected(ctx, &projection, fe);
  } else {
    DisplayDirStatisticProjected(ctx, &projection, de,
                                 de->global_flag ? "SHOW ALL" : NULL, s);
    DisplayDirParameterProjected(ctx, &projection, de);
  }
}

void DisplayDiskName(ViewContext *ctx, const Statistic *s) {
  StatsProjection projection;
  if (ResolveDefaultStatsProjection(ctx, &projection))
    DisplayDiskNameProjected(ctx, &projection, s);
}

void DisplayAvailBytes(ViewContext *ctx, const Statistic *s) {
  DisplayDiskStatistic(ctx, s);
}

void DisplayFilter(ViewContext *ctx, const Statistic *s) {
  DisplayDiskStatistic(ctx, s);
}

void DisplayDiskStatistic(ViewContext *ctx, const Statistic *s) {
  StatsProjection projection;
  if (ResolveDefaultStatsProjection(ctx, &projection))
    DisplayDiskStatisticProjected(ctx, &projection, s);
}

void DisplayDirStatistic(ViewContext *ctx, const DirEntry *de,
                         const char *title, const Statistic *s) {
  StatsProjection projection;
  if (ResolveDefaultStatsProjection(ctx, &projection))
    DisplayDirStatisticProjected(ctx, &projection, de, title, s);
}

void DisplayFileStatistic(ViewContext *ctx, const FileEntry *fe,
                          const Statistic *s) {
  StatsProjection projection;
  if (ResolveDefaultStatsProjection(ctx, &projection))
    DisplayFileStatisticProjected(ctx, &projection, fe, s);
}

void DisplayFileParameter(ViewContext *ctx, FileEntry *fe) {
  StatsProjection projection;
  if (ResolveDefaultStatsProjection(ctx, &projection))
    DisplayFileParameterProjected(ctx, &projection, fe);
}

/* ************************************************************************* */
/*                           COMPATIBILITY WRAPPERS                          */
/* ************************************************************************* */

void DisplayDiskTagged(ViewContext *ctx, const Statistic *s) {
  DisplayDiskStatistic(ctx, s);
}

void DisplayDirTagged(ViewContext *ctx, const DirEntry *de,
                      const Statistic *s) {
  DisplayDirStatistic(ctx, de, NULL, s);
}

void DisplayDirParameter(ViewContext *ctx, DirEntry *de) {
  StatsProjection projection;
  if (ResolveDefaultStatsProjection(ctx, &projection))
    DisplayDirParameterProjected(ctx, &projection, de);
}

void DisplayGlobalFileParameter(ViewContext *ctx, FileEntry *fe) {
  DisplayFileParameter(ctx, fe);
}
