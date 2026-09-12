/***************************************************************************
 *
 * src/core/init.c
 * Application Initialization and Layout Management
 *
 * Handles ncurses startup, configuration loading (profile/history),
 * and dynamic window geometry calculation.
 *
 ***************************************************************************/

#include "ytnova_defs.h"
#include "ytnova_appstate_focus.h"
#include "ytnova_appstate_layout.h"
#include "ytnova_appstate_modal.h"
#include "ytnova_appstate_mode.h"
#include "ytnova_appstate_panel.h"
#include "ytnova_appstate_session.h"
#include "ytnova_appstate_visibility.h"
#include "ytnova_appstate_window.h"
#include "ytnova_debug.h"
#include "ytnova_i18n.h"
#include "default_profile_template.h"
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define SORT_BY_NAME 1
#define STATS_PANEL_WIDTH 24

#define F2_WINDOW_X(ctx) ((ctx)->layout.dir_win_x)
#define F2_WINDOW_Y(ctx) ((ctx)->layout.dir_win_y)
#define F2_WINDOW_WIDTH(ctx) ((ctx)->layout.dir_win_width)
#define F2_WINDOW_HEIGHT(ctx) ((ctx)->layout.dir_win_height + 1)

#define ERROR_WINDOW_WIDTH 40
#define ERROR_WINDOW_HEIGHT 10
#define ERROR_WINDOW_X ((COLS - ERROR_WINDOW_WIDTH) >> 1)
#define ERROR_WINDOW_Y ((LINES - ERROR_WINDOW_HEIGHT) >> 1)

#define HISTORY_WINDOW_X 1
#define HISTORY_WINDOW_Y 2
#define HISTORY_WINDOW_WIDTH(ctx) ((ctx)->layout.main_win_width)
#define HISTORY_WINDOW_HEIGHT (LINES - 6)

#define TIME_WINDOW_X (COLS - 20)
#define TIME_WINDOW_Y 0
#define TIME_WINDOW_WIDTH 20
#define TIME_WINDOW_HEIGHT 1

static WINDOW *Subwin(WINDOW *orig, int nlines, int ncols, int begin_y,
                      int begin_x);
static WINDOW *Newwin(int nlines, int ncols, int begin_y, int begin_x);
static void InitBoundaryHooks(ViewContext *ctx);
static void RegisterCoreInitOps(ViewContext *ctx);
static const char *CoreInitGetProfileValue(const ViewContext *ctx,
                                           const char *name);
static void CoreInitWbkgdSet(const ViewContext *ctx, WINDOW *win, chtype c);
static int CoreInitUINotice(ViewContext *ctx, const char *msg);
static void BoundaryClearPromptLine(ViewContext *ctx);
static int CoreInitLoadDefaultProfileTemplate(ViewContext *ctx);
static BOOL InitConfigurePanelRenderingDefaults(YtreeNovaPanel *panel);
static int InitPrepareRuntimeDefaults(ViewContext *ctx);
static int InitStartTerminal(ViewContext *ctx);
static void InitRestoreEditorAndPager(ViewContext *ctx);
static void InitLoadFallbackProfileTemplate(ViewContext *ctx,
                                            const char *reason);
static void InitLoadExplicitProfile(ViewContext *ctx,
                                    const char *configuration_file);
static void InitLoadDiscoveredProfile(ViewContext *ctx, char *buffer,
                                      size_t buffer_size);
static void InitLoadProfileData(ViewContext *ctx,
                                const char *configuration_file, char *buffer,
                                size_t buffer_size);
static void InitLoadHistoryData(ViewContext *ctx, const char *history_file,
                                char *buffer, size_t buffer_size);
static int InitApplyProfileDisplaySettings(ViewContext *ctx);
static int InitApplyProfileRuntimeFlags(ViewContext *ctx);
static void InitStartRuntimeServices(ViewContext *ctx);
extern int RuntimePort_MainInit(ViewContext *ctx, const char *configuration_file,
                                const char *history_file);
extern void RuntimePort_MainSetProfileValue(const ViewContext *ctx, char *name,
                                            const char *value);
extern int RuntimePort_MainLogDisk(ViewContext *ctx, YtreeNovaPanel *panel,
                                   char *path);
extern int RuntimePort_MainSetFilter(const char *filter_spec, Statistic *s);
extern void RuntimePort_MainRecalculateSysStats(ViewContext *ctx, Statistic *s);
extern int RuntimePort_MainHandleDirWindow(ViewContext *ctx,
                                           const DirEntry *start_dir_entry);
extern void RuntimePort_MainSuspendClock(ViewContext *ctx);
extern void RuntimePort_MainShutdownCurses(ViewContext *ctx);
extern void RuntimePort_MainVolumeFreeAll(ViewContext *ctx);

static int CoreInitWriteAll(int fd, const char *buf, size_t len) {
  size_t written_total = 0;

  while (written_total < len) {
    ssize_t written_now = write(fd, buf + written_total, len - written_total);
    if (written_now <= 0)
      return -1;
    written_total += (size_t)written_now;
  }

  return 0;
}

static int CoreInitLoadDefaultProfileTemplate(ViewContext *ctx) {
  char template_path[] = "/tmp/ytnova-default-profile-XXXXXX";
  int fd;
  size_t template_len;
  int result = -1;

  if (!ctx || !ctx->core_init_ops.read_profile)
    return -1;

  fd = mkstemp(template_path);
  if (fd == -1)
    return -1;

  template_len = strlen(default_profile_template);
  if (CoreInitWriteAll(fd, default_profile_template, template_len) == 0) {
    if (close(fd) == 0) {
      fd = -1;
      result = ctx->core_init_ops.read_profile(ctx, template_path);
    }
  }
  if (fd != -1)
    close(fd);
  unlink(template_path);
  return result;
}

#ifdef XCURSES
char *XCursesProgramName = "ytnova";
#endif

static void RegisterCoreInitOps(ViewContext *ctx) {
  if (ctx == NULL)
    return;
  memset(&ctx->core_init_ops, 0, sizeof(ctx->core_init_ops));
  CoreInitOps_RegisterCmdConfig(&ctx->core_init_ops);
  CoreInitOps_RegisterCmdCommands(&ctx->core_init_ops);
  CoreInitOps_RegisterCmdProfile(&ctx->core_init_ops);
  CoreInitOps_RegisterCmdTheme(&ctx->core_init_ops);
  CoreInitOps_RegisterUIRuntime(&ctx->core_init_ops);
}

static const char *CoreInitGetProfileValue(const ViewContext *ctx,
                                           const char *name) {
  if (ctx == NULL || ctx->core_init_ops.get_profile_value == NULL)
    return "";
  return ctx->core_init_ops.get_profile_value(ctx, name);
}

static void CoreInitWbkgdSet(const ViewContext *ctx, WINDOW *win, chtype c) {
  if (ctx == NULL || ctx->core_init_ops.wbkgd_set == NULL || win == NULL)
    return;
  ctx->core_init_ops.wbkgd_set(ctx, win, c);
}

static int CoreInitUINotice(ViewContext *ctx, const char *msg) {
  if (ctx == NULL || ctx->core_init_ops.ui_notice == NULL)
    return -1;
  return ctx->core_init_ops.ui_notice(ctx, msg);
}

BOOL ParseSmallWindowSkipValue(const char *value) {
  char *end_ptr;
  long parsed;

  if (!value)
    return FALSE;

  errno = 0;
  parsed = strtol(value, &end_ptr, 10);
  if (errno != 0)
    return FALSE;

  while (end_ptr && *end_ptr && isspace((unsigned char)*end_ptr))
    ++end_ptr;
  if (end_ptr && *end_ptr != '\0')
    return FALSE;

  return (parsed == 1) ? TRUE : FALSE;
}

void Layout_Recalculate(ViewContext *ctx) {
  YtreeNovaLayout layout;

  if (!ctx)
    return;

  layout = ctx->layout;

  /* Centralize UI vertical geometry */
  layout.header_y = 0;
  layout.message_y = LINES - 3;
  layout.prompt_y = LINES - 2;
  layout.status_y = LINES - 1;
  layout.bottom_border_y = LINES - 4;

  int available_height = LINES - 6;
  if (available_height < 1)
    available_height = 1;

  if (ctx->preview_mode) {
    layout.stats_width = 0;
    layout.dir_win_height = 0;

    int file_list_width = COLS * 0.20;
    if (file_list_width < 16)
      file_list_width = 16;
    if (file_list_width > COLS - 4)
      file_list_width = COLS - 4;

    layout.main_win_width = COLS - 2;

    layout.big_file_win_x = 1;
    layout.big_file_win_y = 2;
    layout.big_file_win_width = file_list_width;
    layout.big_file_win_height = available_height;

    layout.dir_win_x = 1;
    layout.dir_win_y = 2;
    layout.dir_win_width = file_list_width;
    layout.dir_win_height = 0;

    layout.small_file_win_x = 1;
    layout.small_file_win_y = 2;
    layout.small_file_win_width = file_list_width;
    layout.small_file_win_height = 0;

    layout.preview_win_x = file_list_width + 2;
    layout.preview_win_y = 2;
    layout.preview_win_width = COLS - file_list_width - 3;
    layout.preview_win_height = available_height;

    if (layout.preview_win_width < 1)
      layout.preview_win_width = 1;

    if (!AppStateCommitLayoutGeometry(ctx, &layout))
      return;

    if (ctx->active) {
      YtreeNovaPanelWindowGeometry geometry;

      geometry.dir_x = layout.dir_win_x;
      geometry.dir_y = layout.dir_win_y;
      geometry.dir_w = layout.dir_win_width;
      geometry.dir_h = layout.dir_win_height;
      geometry.small_file_x = layout.small_file_win_x;
      geometry.small_file_y = layout.small_file_win_y;
      geometry.small_file_w = layout.small_file_win_width;
      geometry.small_file_h = layout.small_file_win_height;
      geometry.big_file_x = layout.big_file_win_x;
      geometry.big_file_y = layout.big_file_win_y;
      geometry.big_file_w = layout.big_file_win_width;
      geometry.big_file_h = available_height;
      geometry.stats_x = 0;
      geometry.stats_width = 0;

      if (!AppStateCommitPanelWindowGeometry(ctx->active, &geometry))
        return;
    }

    return;
  }

  if (ctx->is_split_screen) {
    layout.stats_width = 0;
  } else {
    if (ctx->show_stats) {
      layout.stats_width = 24;
    } else {
      layout.stats_width = 0;
    }
  }

  layout.main_win_width = (layout.stats_width > 0)
                              ? (COLS - layout.stats_width - 2)
                              : (COLS - 2);

  int panel_width;
  int split_left_width = 0;
  int split_right_width = 0;
  int left_stats_width = 0;
  int right_stats_width = 0;
  if (ctx->is_split_screen) {
    /* Reserve space for separator (the -1) */
    split_left_width = (layout.main_win_width - 1) / 2;
    split_right_width = layout.main_win_width - split_left_width - 1;
    if (ctx->left && ctx->left->show_stats &&
        split_left_width > STATS_PANEL_WIDTH)
      left_stats_width = STATS_PANEL_WIDTH;
    if (ctx->right && ctx->right->show_stats &&
        split_right_width > STATS_PANEL_WIDTH)
      right_stats_width = STATS_PANEL_WIDTH;
    panel_width = split_left_width - left_stats_width;
  } else {
    panel_width = layout.main_win_width;
  }

  int dir_h = (available_height * 6) / 10;
  if (dir_h < 1)
    dir_h = 1;
  int small_file_h = available_height - dir_h - 1;
  if (small_file_h < 1)
    small_file_h = 1;

  layout.dir_win_height = dir_h;
  layout.small_file_win_height = small_file_h;

  layout.dir_win_x = 1;
  layout.dir_win_y = 2;
  layout.dir_win_width = panel_width;

  layout.small_file_win_x = 1;
  layout.small_file_win_y =
      layout.dir_win_y + dir_h + 1; /* +1 for horizontal separator */
  layout.small_file_win_width = panel_width;

  layout.big_file_win_x = 1;
  layout.big_file_win_y = 2;
  layout.big_file_win_width = panel_width;
  layout.big_file_win_height = available_height;

  if (!AppStateCommitLayoutGeometry(ctx, &layout))
    return;

  if (ctx->left) {
    YtreeNovaPanelWindowGeometry geometry;

    geometry.dir_x = layout.dir_win_x;
    geometry.dir_y = layout.dir_win_y;
    geometry.dir_w = layout.dir_win_width;
    geometry.dir_h = layout.dir_win_height;
    geometry.small_file_x = layout.small_file_win_x;
    geometry.small_file_y = layout.small_file_win_y;
    geometry.small_file_w = layout.small_file_win_width;
    geometry.small_file_h = layout.small_file_win_height;
    geometry.big_file_x = layout.big_file_win_x;
    geometry.big_file_y = layout.big_file_win_y;
    geometry.big_file_w = layout.big_file_win_width;
    geometry.big_file_h = available_height;
    geometry.stats_x = layout.dir_win_x + layout.dir_win_width;
    geometry.stats_width = left_stats_width;

    if (!AppStateCommitPanelWindowGeometry(ctx->left, &geometry))
      return;
  }

  if (ctx->right && ctx->is_split_screen) {
    int right_x = layout.dir_win_x + split_left_width + 1;
    int right_w = split_right_width - right_stats_width;
    YtreeNovaPanelWindowGeometry geometry;

    geometry.dir_x = right_x;
    geometry.dir_y = 2;
    geometry.dir_w = right_w;
    geometry.dir_h = dir_h;
    geometry.small_file_x = right_x;
    geometry.small_file_y = geometry.dir_y + dir_h + 1;
    geometry.small_file_w = right_w;
    geometry.small_file_h = small_file_h;
    geometry.big_file_x = right_x;
    geometry.big_file_y = 2;
    geometry.big_file_w = right_w;
    geometry.big_file_h = available_height;
    geometry.stats_x = right_x + right_w;
    geometry.stats_width = right_stats_width;

    if (!AppStateCommitPanelWindowGeometry(ctx->right, &geometry))
      return;
  }
}

void InitView(ViewContext *ctx) {
  DEBUG_LOG("ENTER InitView");
  memset(ctx, 0, sizeof(ViewContext));
  CoreMainOps_Register(ctx);
  ctx->viewer.inhex = TRUE;
  if (!AppStateCommitViewMode(ctx, DISK_MODE)) {
    fprintf(stderr, "InitView: failed to initialize view mode\n");
    exit(1);
  }
  if (!AppStateCommitDirectoryDisplayMode(ctx, MODE_3)) {
    fprintf(stderr, "InitView: failed to initialize directory display mode\n");
    exit(1);
  }
  if (!AppStateCommitSplitScreenLayout(ctx, FALSE)) {
    fprintf(stderr, "InitView: failed to initialize split layout\n");
    exit(1);
  }

  /* Initialize Panels */
  ctx->left = (YtreeNovaPanel *)calloc(1, sizeof(YtreeNovaPanel));
  if (ctx->left == NULL) {
    fprintf(stderr, "InitView: failed to allocate left panel\n");
    exit(1);
  }
  DEBUG_LOG("InitView: setup left panel=%p", (void *)ctx->left);
  if (!AppStateCommitPanelDirectoryDisplayMode(ctx->left, MODE_3)) {
    fprintf(stderr, "InitView: failed to initialize left panel dir mode\n");
    free(ctx->left);
    ctx->left = NULL;
    exit(1);
  }
  if (!AppStateCommitPanelFileDisplayMode(ctx->left, MODE_1)) {
    fprintf(stderr, "InitView: failed to initialize left panel file mode\n");
    free(ctx->left);
    ctx->left = NULL;
    exit(1);
  }
  if (!AppStateCommitPanelFileInfoOverlayMode(ctx->left,
                                              FILEINFO_OVERLAY_NONE) ||
      !AppStateCommitPanelFixedColumnWidth(ctx->left, 0) ||
      !AppStateCommitPanelSizeUnitMode(ctx->left, FALSE) ||
      !AppStateCommitPanelSymlinkTargetMode(ctx->left, FALSE) ||
      !AppStateCommitPanelStatsVisibility(ctx->left, FALSE)) {
    fprintf(stderr, "InitView: failed to initialize left panel fileinfo state\n");
    free(ctx->left);
    ctx->left = NULL;
    exit(1);
  }
  if (!AppStateCommitPanelFileAnchor(ctx->left, NULL)) {
    fprintf(stderr, "InitView: failed to initialize left panel file anchor\n");
    free(ctx->left);
    ctx->left = NULL;
    exit(1);
  }
  if (!AppStateCommitPanelFileViewport(ctx->left, 0, 0)) {
    fprintf(stderr, "InitView: failed to initialize left panel file viewport\n");
    free(ctx->left);
    ctx->left = NULL;
    exit(1);
  }
  if (!AppStateSeedPanelVisibilityFilter(ctx->left, FALSE)) {
    fprintf(stderr, "InitView: failed to initialize left panel visibility\n");
    free(ctx->left);
    ctx->left = NULL;
    exit(1);
  }

  ctx->right = (YtreeNovaPanel *)calloc(1, sizeof(YtreeNovaPanel));
  if (ctx->right == NULL) {
    fprintf(stderr, "InitView: failed to allocate right panel\n");
    free(ctx->left);
    ctx->left = NULL;
    exit(1);
  }
  DEBUG_LOG("InitView: setup right panel=%p", (void *)ctx->right);
  if (!AppStateCommitPanelDirectoryDisplayMode(ctx->right, MODE_3)) {
    fprintf(stderr, "InitView: failed to initialize right panel dir mode\n");
    free(ctx->right);
    free(ctx->left);
    ctx->right = NULL;
    ctx->left = NULL;
    exit(1);
  }
  if (!AppStateCommitPanelFileDisplayMode(ctx->right, MODE_1)) {
    fprintf(stderr, "InitView: failed to initialize right panel file mode\n");
    free(ctx->right);
    free(ctx->left);
    ctx->right = NULL;
    ctx->left = NULL;
    exit(1);
  }
  if (!AppStateCommitPanelFileInfoOverlayMode(ctx->right,
                                              FILEINFO_OVERLAY_NONE) ||
      !AppStateCommitPanelFixedColumnWidth(ctx->right, 0) ||
      !AppStateCommitPanelSizeUnitMode(ctx->right, FALSE) ||
      !AppStateCommitPanelSymlinkTargetMode(ctx->right, FALSE) ||
      !AppStateCommitPanelStatsVisibility(ctx->right, FALSE)) {
    fprintf(stderr, "InitView: failed to initialize right panel fileinfo state\n");
    free(ctx->right);
    free(ctx->left);
    ctx->right = NULL;
    ctx->left = NULL;
    exit(1);
  }
  if (!AppStateCommitPanelFileAnchor(ctx->right, NULL)) {
    fprintf(stderr, "InitView: failed to initialize right panel file anchor\n");
    free(ctx->right);
    free(ctx->left);
    ctx->right = NULL;
    ctx->left = NULL;
    exit(1);
  }
  if (!AppStateCommitPanelFileViewport(ctx->right, 0, 0)) {
    fprintf(stderr, "InitView: failed to initialize right panel file viewport\n");
    free(ctx->right);
    free(ctx->left);
    ctx->right = NULL;
    ctx->left = NULL;
    exit(1);
  }
  if (!AppStateSeedPanelVisibilityFilter(ctx->right, FALSE)) {
    fprintf(stderr, "InitView: failed to initialize right panel visibility\n");
    free(ctx->right);
    free(ctx->left);
    ctx->right = NULL;
    ctx->left = NULL;
    exit(1);
  }

  if (!AppStateCommitActivePanel(ctx, ctx->left) ||
      !AppStateCommitPanelFileShape(ctx->left, FALSE) ||
      !AppStateCommitPanelFileShape(ctx->right, FALSE) ||
      !AppStateCommitPanelFocus(ctx, ctx->left, FOCUS_TREE) ||
      !AppStateCommitPanelFocus(ctx, ctx->right, FOCUS_TREE)) {
    fprintf(stderr, "InitView: failed to initialize panel focus\n");
    free(ctx->right);
    free(ctx->left);
    ctx->right = NULL;
    ctx->left = NULL;
    exit(1);
  }

  DEBUG_LOG("EXIT InitView");
}

void CoreMainOps_Register(ViewContext *ctx) {
  if (ctx == NULL)
    return;

  ctx->core_main_ops.init = RuntimePort_MainInit;
  ctx->core_main_ops.set_profile_value = RuntimePort_MainSetProfileValue;
  ctx->core_main_ops.log_disk = RuntimePort_MainLogDisk;
  ctx->core_main_ops.set_filter = RuntimePort_MainSetFilter;
  ctx->core_main_ops.recalculate_sys_stats = RuntimePort_MainRecalculateSysStats;
  ctx->core_main_ops.handle_dir_window = RuntimePort_MainHandleDirWindow;
  ctx->core_main_ops.suspend_clock = RuntimePort_MainSuspendClock;
  ctx->core_main_ops.shutdown_curses = RuntimePort_MainShutdownCurses;
  ctx->core_main_ops.volume_free_all = RuntimePort_MainVolumeFreeAll;
}

static void InitBoundaryHooks(ViewContext *ctx) {
  if (ctx == NULL)
    return;
  ctx->hook_parse_color = ctx->core_init_ops.parse_color_string;
  ctx->hook_update_ui_color = ctx->core_init_ops.update_ui_color;
  ctx->hook_add_file_color_rule = ctx->core_init_ops.add_file_color_rule;
  ctx->hook_get_profile_value = ctx->core_init_ops.get_profile_value;
  ctx->hook_has_user_action = ctx->core_init_ops.has_user_action;
  if (ctx->core_init_ops.bind_runtime_hooks != NULL)
    ctx->core_init_ops.bind_runtime_hooks(ctx);
  CoreStorageOps_Register(ctx);
  CoreWatcherOps_Register(ctx);
  ctx->hook_clear_prompt_line = BoundaryClearPromptLine;
  ctx->hook_refresh_ui = doupdate;
}

static void BoundaryClearPromptLine(ViewContext *ctx) {
  int y, x;

  if (ctx == NULL || ctx->ctx_border_window == NULL)
    return;

  getyx(stdscr, y, x);
  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);
  move(y, x);
  doupdate();
}

void ReCreateWindows(ViewContext *ctx) {
  DEBUG_LOG("ENTER ReCreateWindows");
  if (ctx == NULL || ctx->left == NULL || ctx->right == NULL)
    return;
  if (ctx->active == NULL &&
      !AppStateCommitActivePanel(ctx, ctx->left))
    return;
  /* 1. Recalculate Layout based on current SplitScreen state */
  Layout_Recalculate(ctx);
  if (ctx->left == NULL || ctx->right == NULL)
    return;

  BOOL left_is_big = FALSE;
  BOOL right_is_big = FALSE;

  /* Capture INDEPENDENT Panel Mode State */
  /* If panels exist, check if they are currently zoomed */
  if (ctx->left->pan_file_window) {
    if (ctx->left->pan_file_window == ctx->left->pan_big_file_window)
      left_is_big = TRUE;
  }
  if (ctx->right->pan_file_window) {
    if (ctx->right->pan_file_window == ctx->right->pan_big_file_window)
      right_is_big = TRUE;
  }

  /* Force 'big' (zoom) mode for ActivePanel if Preview Mode is active */
  if (ctx->preview_mode) {
    if (ctx->active == ctx->left)
      left_is_big = TRUE;
    if (ctx->active == ctx->right)
      right_is_big = TRUE;
  }

  /* 2. Cleanup: Destroy ALL existing panel windows */
  if (ctx->left->pan_dir_window) {
    delwin(ctx->left->pan_dir_window);
    ctx->left->pan_dir_window = NULL;
  }
  if (ctx->left->pan_small_file_window) {
    delwin(ctx->left->pan_small_file_window);
    ctx->left->pan_small_file_window = NULL;
  }
  if (ctx->left->pan_big_file_window) {
    delwin(ctx->left->pan_big_file_window);
    ctx->left->pan_big_file_window = NULL;
  }

  if (ctx->right->pan_dir_window) {
    delwin(ctx->right->pan_dir_window);
    ctx->right->pan_dir_window = NULL;
  }
  if (ctx->right->pan_small_file_window) {
    delwin(ctx->right->pan_small_file_window);
    ctx->right->pan_small_file_window = NULL;
  }
  if (ctx->right->pan_big_file_window) {
    delwin(ctx->right->pan_big_file_window);
    ctx->right->pan_big_file_window = NULL;
  }

  /* Cleanup Preview Window */
  if (ctx->ctx_preview_window) {
    WINDOW *preview_window = ctx->ctx_preview_window;
    if (!AppStateSetPreviewWindowHandle(ctx, NULL))
      return;
    delwin(preview_window);
  }

  /* 3. Create Primary Panel Windows (Always Created) */
  YtreeNovaPanel *primary = (ctx->preview_mode) ? ctx->active : ctx->left;
  BOOL primary_big_mode = (primary == ctx->left) ? left_is_big : right_is_big;

  primary->pan_dir_window = Subwin(stdscr, primary->dir_h, primary->dir_w,
                                   primary->dir_y, primary->dir_x);
  keypad(primary->pan_dir_window, TRUE);
  scrollok(primary->pan_dir_window, TRUE);

  leaveok(primary->pan_dir_window, TRUE);
  CoreInitWbkgdSet(ctx, primary->pan_dir_window, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));

  primary->pan_small_file_window =
      Subwin(stdscr, primary->small_file_h, primary->small_file_w,
             primary->small_file_y, primary->small_file_x);
  keypad(primary->pan_small_file_window, TRUE);

  leaveok(primary->pan_small_file_window, TRUE);
  CoreInitWbkgdSet(ctx, primary->pan_small_file_window, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));

  primary->pan_big_file_window =
      Subwin(stdscr, primary->big_file_h, primary->big_file_w,
             primary->big_file_y, primary->big_file_x);
  keypad(primary->pan_big_file_window, TRUE);

  leaveok(primary->pan_big_file_window, TRUE);
  CoreInitWbkgdSet(ctx, primary->pan_big_file_window, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));

  if (!AppStateSetPanelFileWindowHandle(ctx, primary, primary_big_mode))
    return;

  /* 4. Create Right Panel Windows (Only if Split Screen and NOT Preview Mode)
   */
  if (ctx->is_split_screen && !ctx->preview_mode) {
    ctx->right->pan_dir_window =
        Subwin(stdscr, ctx->right->dir_h, ctx->right->dir_w, ctx->right->dir_y,
               ctx->right->dir_x);
    keypad(ctx->right->pan_dir_window, TRUE);
    scrollok(ctx->right->pan_dir_window, TRUE);

    leaveok(ctx->right->pan_dir_window, TRUE);
    CoreInitWbkgdSet(ctx, ctx->right->pan_dir_window, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));

    ctx->right->pan_small_file_window =
        Subwin(stdscr, ctx->right->small_file_h, ctx->right->small_file_w,
               ctx->right->small_file_y, ctx->right->small_file_x);
    keypad(ctx->right->pan_small_file_window, TRUE);

    leaveok(ctx->right->pan_small_file_window, TRUE);
    CoreInitWbkgdSet(ctx, ctx->right->pan_small_file_window, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));

    ctx->right->pan_big_file_window =
        Subwin(stdscr, ctx->right->big_file_h, ctx->right->big_file_w,
               ctx->right->big_file_y, ctx->right->big_file_x);
    keypad(ctx->right->pan_big_file_window, TRUE);

    leaveok(ctx->right->pan_big_file_window, TRUE);
    CoreInitWbkgdSet(ctx, ctx->right->pan_big_file_window, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));

    if (!AppStateSetPanelFileWindowHandle(ctx, ctx->right, right_is_big))
      return;
  }

  /* 5. Create Preview Window (If Preview Mode) */
  if (ctx->preview_mode) {
    WINDOW *preview_window =
        Newwin(ctx->layout.preview_win_height, ctx->layout.preview_win_width,
               ctx->layout.preview_win_y, ctx->layout.preview_win_x);
    if (!AppStateSetPreviewWindowHandle(ctx, preview_window)) {
      if (preview_window)
        delwin(preview_window);
      return;
    }
    if (ctx->ctx_preview_window) {
      keypad(ctx->ctx_preview_window, TRUE);

      leaveok(ctx->ctx_preview_window, TRUE);
      CoreInitWbkgdSet(ctx, ctx->ctx_preview_window, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));
    }
  }

  /* 6. Sync ctx->active Pointers */
  if (ctx->active == NULL &&
      !AppStateCommitActivePanel(ctx, ctx->left))
    return;

  if (ctx->active == ctx->left) {
    ctx->active->pan_dir_window = ctx->left->pan_dir_window;
    ctx->active->pan_small_file_window = ctx->left->pan_small_file_window;
    ctx->active->pan_big_file_window = ctx->left->pan_big_file_window;
    if (!AppStateSetPanelFileWindowHandle(ctx, ctx->active, left_is_big))
      return;
  } else if (ctx->active == ctx->right &&
             (ctx->is_split_screen || ctx->preview_mode)) {
    /* In Preview Mode, RightPanel might not have windows, but ActivePanel
     * should still point to it if it was selected. Layout_Recalculate handles
     * hiding the right-side UI, but we shouldn't force-switch focus to Left.
     */
    ctx->active->pan_dir_window = ctx->right->pan_dir_window;
    ctx->active->pan_small_file_window = ctx->right->pan_small_file_window;
    ctx->active->pan_big_file_window = ctx->right->pan_big_file_window;
    if (!AppStateSetPanelFileWindowHandle(ctx, ctx->active, right_is_big))
      return;
  } else {
    /* Fallback if something went wrong (e.g. RightPanel active but
     * SplitScreen/Preview disabled) */
    if (!AppStateCommitActivePanel(ctx, ctx->left))
      return;
    ctx->active->pan_dir_window = ctx->left->pan_dir_window;
    ctx->active->pan_small_file_window = ctx->left->pan_small_file_window;
    ctx->active->pan_big_file_window = ctx->left->pan_big_file_window;
    if (!AppStateSetPanelFileWindowHandle(ctx, ctx->active, left_is_big))
      return;
  }

  if (!AppStateSyncActiveWindowHandles(ctx))
    return;

  /* 8. Utility Windows */
  if (ctx->ctx_border_window) {
    WINDOW *border_window = ctx->ctx_border_window;
    if (!AppStateSetBorderWindowHandle(ctx, NULL))
      return;
    delwin(border_window);
  }

  {
    WINDOW *border_window = Newwin(LINES, COLS, 0, 0);
    if (!AppStateSetBorderWindowHandle(ctx, border_window)) {
      if (border_window)
        delwin(border_window);
      return;
    }
  }
  if (ctx->ctx_border_window) {
    CoreInitWbkgdSet(ctx, ctx->ctx_border_window,
                     COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));

    leaveok(ctx->ctx_border_window, TRUE);

    /* Header Path Window: subwindow of border */
    if (ctx->ctx_path_window) {
      WINDOW *path_window = ctx->ctx_path_window;
      if (!AppStateSetPathWindowHandle(ctx, NULL))
        return;
      delwin(path_window);
    }
    {
      WINDOW *path_window = Subwin(ctx->ctx_border_window, 1, COLS - 26, 0, 6);
      if (!AppStateSetPathWindowHandle(ctx, path_window)) {
        if (path_window)
          delwin(path_window);
        return;
      }
    }
    leaveok(ctx->ctx_path_window, TRUE);
  }

  if (ctx->ctx_error_window) {
    WINDOW *error_window = ctx->ctx_error_window;
    if (!AppStateSetErrorWindowHandle(ctx, NULL))
      return;
    delwin(error_window);
  }

  {
    WINDOW *error_window = Newwin(ERROR_WINDOW_HEIGHT, ERROR_WINDOW_WIDTH,
                                  ERROR_WINDOW_Y, ERROR_WINDOW_X);
    if (!AppStateSetErrorWindowHandle(ctx, error_window)) {
      if (error_window)
        delwin(error_window);
      return;
    }
  }
  CoreInitWbkgdSet(ctx, ctx->ctx_error_window, COLOR_PAIR(UI_ROLE_ERROR));

  leaveok(ctx->ctx_error_window, TRUE);

#ifdef CLOCK_SUPPORT

  if (ctx->ctx_time_window) {
    WINDOW *time_window = ctx->ctx_time_window;
    if (!AppStateSetTimeWindowHandle(ctx, NULL))
      return;
    delwin(time_window);
  }

  {
    WINDOW *time_window =
        Subwin(ctx->ctx_border_window, TIME_WINDOW_HEIGHT, TIME_WINDOW_WIDTH,
               TIME_WINDOW_Y, TIME_WINDOW_X);
    if (!AppStateSetTimeWindowHandle(ctx, time_window)) {
      if (time_window)
        delwin(time_window);
      return;
    }
  }

  scrollok(ctx->ctx_time_window, FALSE);
  leaveok(ctx->ctx_time_window, TRUE);
  CoreInitWbkgdSet(ctx, ctx->ctx_time_window, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));
  /* Keep clock redraws in the normal wnoutrefresh/doupdate pipeline.
   * Immediate refresh causes visible blinking during rapid navigation.
   */
  immedok(ctx->ctx_time_window, FALSE);
#endif

  if (ctx->ctx_history_window) {
    WINDOW *history_window = ctx->ctx_history_window;
    if (!AppStateSetHistoryWindowHandle(ctx, NULL))
      return;
    if (!AppStateSetMatchesWindowHandle(ctx, NULL))
      return;
    delwin(history_window);
  }

  {
    WINDOW *history_window =
        Newwin(HISTORY_WINDOW_HEIGHT, HISTORY_WINDOW_WIDTH(ctx),
               HISTORY_WINDOW_Y, HISTORY_WINDOW_X);
    if (!AppStateSetHistoryWindowHandle(ctx, history_window)) {
      if (history_window)
        delwin(history_window);
      return;
    }
  }
  scrollok(ctx->ctx_history_window, TRUE);

  leaveok(ctx->ctx_history_window, TRUE);
  CoreInitWbkgdSet(ctx, ctx->ctx_history_window, COLOR_PAIR(UI_ROLE_PICKER));

  if (!AppStateSetMatchesWindowHandle(ctx, ctx->ctx_history_window))
    return;

  if (ctx->ctx_menu_window) {
    WINDOW *menu_window = ctx->ctx_menu_window;
    if (!AppStateSetMenuWindowHandle(ctx, NULL))
      return;
    delwin(menu_window);
  }

  /* Menu area: Y_MESSAGE down to bottom (typically 3 lines) */
  {
    WINDOW *menu_window = Newwin(3, COLS, ctx->layout.message_y, 0);
    if (!AppStateSetMenuWindowHandle(ctx, menu_window)) {
      if (menu_window)
        delwin(menu_window);
      return;
    }
  }
  if (ctx->ctx_menu_window) {
    CoreInitWbkgdSet(ctx, ctx->ctx_menu_window, COLOR_PAIR(UI_ROLE_FOOTER));

    leaveok(ctx->ctx_menu_window, TRUE);
  }

  if (ctx->ctx_f2_window) {
    WINDOW *f2_window = ctx->ctx_f2_window;
    if (!AppStateSetF2WindowHandle(ctx, NULL))
      return;
    delwin(f2_window);
  }

  {
    WINDOW *f2_window =
        Newwin(F2_WINDOW_HEIGHT(ctx), F2_WINDOW_WIDTH(ctx), F2_WINDOW_Y(ctx),
               F2_WINDOW_X(ctx));
    if (!AppStateSetF2WindowHandle(ctx, f2_window)) {
      if (f2_window)
        delwin(f2_window);
      return;
    }
  }

  keypad(ctx->ctx_f2_window, TRUE);
  scrollok(ctx->ctx_f2_window, FALSE);

  leaveok(ctx->ctx_f2_window, TRUE);
  CoreInitWbkgdSet(ctx, ctx->ctx_f2_window, COLOR_PAIR(UI_ROLE_PICKER));
  DEBUG_LOG("EXIT ReCreateWindows");
}

static void CoreInitCreateThemedStartupWindows(ViewContext *ctx) {
  /* Pre-theme notices fall back to stderr until the final themed windows exist.
   */
  CoreInitWbkgdSet(ctx, stdscr, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));
  werase(stdscr);
  ReCreateWindows(ctx);
}

static BOOL InitConfigurePanelRenderingDefaults(YtreeNovaPanel *panel) {
  if (!AppStateCommitPanelDirectoryDisplayMode(panel, MODE_3))
    return FALSE;
  if (!AppStateCommitPanelFileDisplayMode(panel, MODE_1))
    return FALSE;
  if (!AppStateCommitPanelFileInfoOverlayMode(panel, FILEINFO_OVERLAY_NONE))
    return FALSE;
  if (!AppStateCommitPanelFixedColumnWidth(panel, 0))
    return FALSE;
  if (!AppStateCommitPanelSizeUnitMode(panel, FALSE))
    return FALSE;
  if (!AppStateCommitPanelSymlinkTargetMode(panel, FALSE))
    return FALSE;
  return AppStateCommitPanelFileMaxColumn(panel, 1);
}

static int InitPrepareRuntimeDefaults(ViewContext *ctx) {
  ctx->show_stats = TRUE;
  if (!AppStateCommitFixedColumnWidth(ctx, 0))
    return -1;
  if (!AppStateCommitRefreshMode(ctx, 0))
    return -1;
  if (!AppStateCommitPreviewMode(ctx, FALSE))
    return -1;
  if (!AppStateSetPreviewWindowHandle(ctx, NULL))
    return -1;
  if (!AppStateCommitViewMode(ctx, DISK_MODE))
    return -1;
  return 0;
}

static int InitStartTerminal(ViewContext *ctx) {
  I18n_Init();

  ctx->user_umask = umask(0);
  ctx->configuration_file_path[0] = '\0';
  ctx->configuration_file_path_is_explicit = FALSE;
  ctx->history_file_path[0] = '\0';
  ctx->commands_file_path[0] = '\0';
  setenv("ESCDELAY", "25", 1);
  ctx->curses_screen = newterm(NULL, stdout, stdin);
  if (ctx->curses_screen == NULL) {
    fprintf(stderr, "Init: failed to initialize terminal\n");
    return 1;
  }

  set_term(ctx->curses_screen);
  if (!AppStateCommitTerminalGeometryCache(ctx, LINES, COLS))
    return -1;
  Layout_Recalculate(ctx);
  if (ctx->core_init_ops.start_colors != NULL)
    ctx->core_init_ops.start_colors(ctx);
  if (ctx->core_init_ops.dialog_init != NULL)
    ctx->core_init_ops.dialog_init();

  cbreak();
  noecho();
  nonl();
  raw();
  keypad(stdscr, TRUE);
  clearok(stdscr, TRUE);
  leaveok(stdscr, FALSE);
  curs_set(0);

  if (baudrate() >= QUICK_BAUD_RATE)
    typeahead(-1);
  DEBUG_LOG("Init: typeahead done");
  return 0;
}

static void InitRestoreEditorAndPager(ViewContext *ctx) {
  const char *editor_env = getenv("EDITOR");
  const char *pager_env = getenv("PAGER");

  if (ctx->core_main_ops.set_profile_value == NULL)
    return;

  if (editor_env != NULL && *editor_env) {
    char editor_key[] = "EDITOR";

    ctx->core_main_ops.set_profile_value(ctx, editor_key, editor_env);
  }
  if (pager_env != NULL && *pager_env) {
    char pager_key[] = "PAGER";

    ctx->core_main_ops.set_profile_value(ctx, pager_key, pager_env);
  }
}

static void InitLoadFallbackProfileTemplate(ViewContext *ctx,
                                            const char *reason) {
  DEBUG_LOG("%s", reason);
  if (CoreInitLoadDefaultProfileTemplate(ctx) == 0)
    InitRestoreEditorAndPager(ctx);
}

static void InitLoadExplicitProfile(ViewContext *ctx,
                                    const char *configuration_file) {
  int read_profile_result = -1;

  ctx->configuration_file_path_is_explicit = TRUE;
  (void)snprintf(ctx->configuration_file_path,
                 sizeof(ctx->configuration_file_path), "%s",
                 configuration_file);
  DEBUG_LOG("Init: Reading profile %s", configuration_file);
  if (ctx->core_init_ops.read_profile != NULL)
    read_profile_result = ctx->core_init_ops.read_profile(ctx, configuration_file);
  if (read_profile_result != 0) {
    InitLoadFallbackProfileTemplate(
        ctx, "Init: Profile invalid or unreadable, loading built-in default "
             "profile template");
  }
}

static void InitLoadDiscoveredProfile(ViewContext *ctx, char *buffer,
                                      size_t buffer_size) {
  int read_profile_result = -1;

  if (ConfigPaths_ResolvePreferredPath(CONFIG_SURFACE_PROFILE, buffer,
                                       buffer_size) == 0) {
    DEBUG_LOG("Init: Reading profile %s", buffer);
    if (ctx->core_init_ops.read_profile != NULL)
      read_profile_result = ctx->core_init_ops.read_profile(ctx, buffer);
    if (read_profile_result == 0)
      (void)snprintf(ctx->configuration_file_path,
                     sizeof(ctx->configuration_file_path), "%s", buffer);
  }
  if (read_profile_result != 0 && read_profile_result != 1) {
    if (ConfigPaths_ResolveLegacyPath(CONFIG_SURFACE_PROFILE, buffer,
                                      buffer_size, FALSE) == 0) {
      DEBUG_LOG("Init: Reading legacy profile %s", buffer);
      if (ctx->core_init_ops.read_profile != NULL)
        read_profile_result = ctx->core_init_ops.read_profile(ctx, buffer);
      if (read_profile_result == 0)
        (void)snprintf(ctx->configuration_file_path,
                       sizeof(ctx->configuration_file_path), "%s", buffer);
    }
  }
  if (read_profile_result != 0) {
    InitLoadFallbackProfileTemplate(
        ctx, "Init: Profile missing or unreadable, loading built-in default "
             "profile template");
  }
}

static void InitLoadProfileData(ViewContext *ctx,
                                const char *configuration_file, char *buffer,
                                size_t buffer_size) {
  if (configuration_file != NULL)
    InitLoadExplicitProfile(ctx, configuration_file);
  else
    InitLoadDiscoveredProfile(ctx, buffer, buffer_size);

  DEBUG_LOG("Init: ReadProfile done");
}

static void InitLoadHistoryData(ViewContext *ctx, const char *history_file,
                                char *buffer, size_t buffer_size) {
  if (history_file != NULL) {
    (void)snprintf(ctx->history_file_path, sizeof(ctx->history_file_path), "%s",
                   history_file);
    DEBUG_LOG("Init: Reading history %s", history_file);
    if (ctx->core_init_ops.read_history != NULL)
      (void)ctx->core_init_ops.read_history(ctx, history_file);
  } else if (ResolvePreferredHistoryPath(buffer, buffer_size) == 0) {
    char legacy_history_path[PATH_LENGTH + 1];
    int read_history_result = -1;

    (void)snprintf(ctx->history_file_path, sizeof(ctx->history_file_path), "%s",
                   buffer);
    DEBUG_LOG("Init: Reading history %s", buffer);
    if (ctx->core_init_ops.read_history != NULL)
      read_history_result = ctx->core_init_ops.read_history(ctx, buffer);
    if (read_history_result == -1 && access(buffer, F_OK) != 0 &&
        ResolveLegacyHistoryPath(legacy_history_path,
                                 sizeof(legacy_history_path)) == 0) {
      DEBUG_LOG("Init: Reading legacy history %s", legacy_history_path);
      if (ctx->core_init_ops.read_history != NULL)
        (void)ctx->core_init_ops.read_history(ctx, legacy_history_path);
    }
  } else if (ResolveLegacyHistoryPath(buffer, buffer_size) == 0) {
    (void)snprintf(ctx->history_file_path, sizeof(ctx->history_file_path), "%s",
                   buffer);
    DEBUG_LOG("Init: Reading fallback history %s", buffer);
    if (ctx->core_init_ops.read_history != NULL)
      (void)ctx->core_init_ops.read_history(ctx, buffer);
  }

  DEBUG_LOG("Init: ReadHistory done");
}

static int InitApplyProfileDisplaySettings(ViewContext *ctx) {
  BOOL human_size_units =
      (strcmp(CoreInitGetProfileValue(ctx, "FILE_SIZE_UNITS"),
              "human-readable") == 0);

  if (ctx->core_init_ops.set_panel_file_mode != NULL) {
    ctx->core_init_ops.set_panel_file_mode(ctx, ctx->left, MODE_3);
    ctx->core_init_ops.set_panel_file_mode(ctx, ctx->right, MODE_3);
  }
  if (!AppStateCommitPanelDirectoryDisplayMode(ctx->left, MODE_3) ||
      !AppStateCommitPanelDirectoryDisplayMode(ctx->right, MODE_3) ||
      !AppStateCommitDirectoryDisplayMode(ctx, MODE_3))
    return -1;
  if (!AppStateCommitPanelSizeUnitMode(ctx->left, human_size_units) ||
      !AppStateCommitPanelSizeUnitMode(ctx->right, human_size_units))
    return -1;

  DEBUG_LOG("Init: SetPanelFileMode done");
  return 0;
}

static int InitApplyProfileRuntimeFlags(ViewContext *ctx) {
  BOOL hide_dot_files =
      (strtol(CoreInitGetProfileValue(ctx, "HIDEDOTFILES"), NULL, 0)) ? TRUE
                                                                      : FALSE;
  struct lconv *lc;

  SetKindOfSort(SORT_BY_NAME, &ctx->active->vol->vol_stats);
  DEBUG_LOG("Init: SetKindOfSort done");

  lc = localeconv();
  if (lc != NULL && lc->thousands_sep != NULL && *lc->thousands_sep)
    ctx->number_seperator = *lc->thousands_sep;
  else
    ctx->number_seperator = ',';
  DEBUG_LOG("Init: locale fallback done");

  if (!AppStateCommitSmallWindowBypass(
          ctx,
          ParseSmallWindowSkipValue(
              CoreInitGetProfileValue(ctx, "SMALLWINDOWSKIP"))))
    return -1;
  if (!AppStateCommitFullLineHighlight(
          ctx,
          (strtol(CoreInitGetProfileValue(ctx, "HIGHLIGHT_FULL_LINE"), NULL, 0))
              ? TRUE
              : FALSE))
    return -1;
  if (!AppStateSeedPanelVisibilityFilter(ctx->left, hide_dot_files) ||
      !AppStateSeedPanelVisibilityFilter(ctx->right, hide_dot_files))
    return -1;

  ctx->animation_method =
      strtol(CoreInitGetProfileValue(ctx, "ANIMATION"), NULL, 0);
  ctx->initial_directory = (char *)CoreInitGetProfileValue(ctx, "INITIALDIR");
  if (!AppStateCommitRefreshMode(
          ctx, strtol(CoreInitGetProfileValue(ctx, "AUTO_REFRESH"), NULL, 0)))
    return -1;

  DEBUG_LOG("Init: Profile variables done");
  return 0;
}

static void InitStartRuntimeServices(ViewContext *ctx) {
  if (ctx->hook_init_clock != NULL)
    ctx->hook_init_clock(ctx);
  DEBUG_LOG("Init: InitClock done");
  if ((ctx->refresh_mode & REFRESH_WATCHER) &&
      ctx->core_storage_ops.watcher_init != NULL)
    ctx->core_storage_ops.watcher_init(ctx);
  DEBUG_LOG("Init: Watcher_Init done");
}

void ShutdownCurses(ViewContext *ctx) {
  SCREEN *screen = (ctx != NULL) ? ctx->curses_screen : NULL;

  if (screen != NULL)
    set_term(screen);

  endwin();

  if (screen != NULL) {
    delscreen(screen);
    ctx->curses_screen = NULL;
  }
}

int Init(ViewContext *ctx, const char *configuration_file,
         const char *history_file) {
  char buffer[PATH_LENGTH + 1];
  struct Volume *initial_vol;

  InitView(ctx);
  RegisterCoreInitOps(ctx);
  InitBoundaryHooks(ctx);
  DEBUG_LOG("ENTER Init");

  if (!AppStateCommitSplitScreenLayout(ctx, FALSE))
    return -1;
  if (!AppStateCommitActivePanel(ctx, ctx->left))
    return -1;

  ctx->left->file_entry_list = NULL;
  ctx->left->file_entry_list_capacity = 0;
  ctx->left->file_count = 0;
  ctx->right->file_entry_list = NULL;
  ctx->right->file_entry_list_capacity = 0;
  ctx->right->file_count = 0;

  if (!InitConfigurePanelRenderingDefaults(ctx->left) ||
      !InitConfigurePanelRenderingDefaults(ctx->right))
    return -1;

  initial_vol = Volume_Create(ctx);
  if (initial_vol == NULL)
    return -1;
  if (!AppStateCommitActivePanel(ctx, ctx->left))
    return -1;
  if (!AppStateCommitPanelVolume(ctx->active, initial_vol))
    return -1;

  if (InitPrepareRuntimeDefaults(ctx) != 0)
    return -1;
  if (InitStartTerminal(ctx) != 0)
    return -1;

  InitLoadProfileData(ctx, configuration_file, buffer, sizeof(buffer));

  if (ctx->core_init_ops.load_commands != NULL &&
      ctx->core_init_ops.load_commands(ctx) != 0) {
    CoreInitUINotice(ctx, "LoadCommands failed*ABORT");
    exit(1);
  }
  DEBUG_LOG("Init: LoadCommands done");

  if (((ctx->core_init_ops.load_startup_theme != NULL &&
        ctx->core_init_ops.load_startup_theme(ctx) != 0) ||
       (ctx->core_init_ops.load_startup_theme == NULL &&
        ctx->core_init_ops.load_theme != NULL &&
        ctx->core_init_ops.load_theme(ctx) != 0))) {
    CoreInitUINotice(ctx, "LoadTheme failed*ABORT");
    exit(1);
  }
  DEBUG_LOG("Init: LoadTheme done");

  if (ctx->core_init_ops.reinit_color_pairs != NULL)
    ctx->core_init_ops.reinit_color_pairs(ctx);
  DEBUG_LOG("Init: ReinitColorPairs done");
  CoreInitCreateThemedStartupWindows(ctx);
  DEBUG_LOG("Init: ReCreateWindows after theme done");

  InitLoadHistoryData(ctx, history_file, buffer, sizeof(buffer));
  if (InitApplyProfileDisplaySettings(ctx) != 0)
    return -1;
  if (InitApplyProfileRuntimeFlags(ctx) != 0)
    return -1;
  InitStartRuntimeServices(ctx);

  DEBUG_LOG("EXIT Init");
  return (0);
}

static WINDOW *Subwin(WINDOW *orig, int nlines, int ncols, int begin_y,
                      int begin_x) {
  int x, y, h, w;
  WINDOW *win;

  if (nlines > LINES)
    nlines = LINES;
  if (ncols > COLS)
    ncols = COLS;

  h = MAXIMUM(nlines, 1);
  w = MAXIMUM(ncols, 1);
  x = MAXIMUM(begin_x, 0);
  y = MAXIMUM(begin_y, 0);

  if (x + w > COLS)
    x = COLS - w;
  if (y + h > LINES)
    y = LINES - h;

  win = subwin(orig, h, w, y, x);

  return (win);
}

static WINDOW *Newwin(int nlines, int ncols, int begin_y, int begin_x) {
  int x, y, h, w;
  WINDOW *win;

  if (nlines > LINES)
    nlines = LINES;
  if (ncols > COLS)
    ncols = COLS;

  h = MAXIMUM(nlines, 1);
  w = MAXIMUM(ncols, 1);
  x = MAXIMUM(begin_x, 0);
  y = MAXIMUM(begin_y, 0);

  if (x + w > COLS)
    x = COLS - w;
  if (y + h > LINES)
    y = LINES - h;

  win = newwin(h, w, y, x);
  if (win) {
    keypad(win, TRUE);
  }

  return (win);
}
