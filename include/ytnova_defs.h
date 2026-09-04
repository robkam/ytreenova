/***************************************************************************
 *
 * ytnova_defs.h
 * Global data structures and type definitions
 *
 ***************************************************************************/
#ifndef YTNOVA_DEFS_H
#define YTNOVA_DEFS_H

typedef struct _ViewContext ViewContext;

/* Large File Support must be defined before system headers */
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

#include <ctype.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include "ytnova_debug.h"

#ifdef XCURSES
#include <xcurses.h>
#define HAVE_CURSES 1
#endif

#if !defined(HAVE_CURSES)
#include <curses.h>
#include <term.h>
#endif

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <regex.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#ifdef HAVE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>
#endif

#ifdef WITH_UTF8
#include <wchar.h>
#endif

#include "uthash.h"

/* --- Macros & Constants --- */

#if !defined(WIN32) && !defined(__DJGPP__)
#include <sys/statfs.h>
#include <sys/statvfs.h>

#define STATFS(a, b, c, d) statvfs(a, b)
#endif

#if defined(S_IFLNK)
#define STAT_(a, b) lstat(a, b)
#else
#define STAT_(a, b) stat(a, b)
#endif

#define MINIMUM(a, b) (((a) < (b)) ? (a) : (b))
#define MAXIMUM(a, b) (((a) > (b)) ? (a) : (b))

#define UI_INPUT_PADDING 2

#define MAX_MODES 4
#define DISK_MODE 0
#define LL_FILE_MODE 1
#define ARCHIVE_MODE 2
#define USER_MODE 3

/* Win32 / DJGPP compat macros (keep them here as they affect system
 * calls/types) */
#ifdef WIN32
#define S_IREAD S_IRUSR
#define S_IWRITE S_IWUSR
#define S_IEXEC S_IXUSR
#define popen _popen
#define pclose _pclose
#define sys_errlist _sys_errlist
#define echochar(ch)                                                           \
  {                                                                            \
    addch(ch);                                                                 \
    refresh();                                                                 \
  }
#define putp(str) puts(str)
#define vidattr(attr)
#define typeahead(file)
#endif

#ifdef __DJGPP__
#define putp(str) puts(str)
#define vidattr(attr)
#define typeahead(file)
#endif

#ifndef KEY_END
#define KEY_END KEY_EOL
#endif

/* S_IS* Macros */
#ifndef S_ISREG
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#endif
#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISCHR
#define S_ISCHR(mode) (((mode) & S_IFMT) == S_IFCHR)
#endif
#ifndef S_ISBLK
#define S_ISBLK(mode) (((mode) & S_IFMT) == S_IFBLK)
#endif
#ifndef S_ISFIFO
#define S_ISFIFO(mode) (((mode) & S_IFMT) == S_IFFIFO)
#endif
#ifndef S_ISLNK
#ifdef S_IFLNK
#define S_ISLNK(mode) (((mode) & S_IFMT) == S_IFLNK)
#else
#define S_ISLNK(mode) FALSE
#endif
#endif
#ifndef S_ISSOCK
#ifdef S_IFSOCK
#define S_ISSOCK(mode) (((mode) & S_IFMT) == S_IFSOCK)
#else
#define S_ISSOCK(mode) FALSE
#endif
#endif

#define VI_KEY_UP 'k'
#define VI_KEY_DOWN 'j'
#define VI_KEY_RIGHT 'l'
#define VI_KEY_LEFT 'h'
#define VI_KEY_NPAGE ('D' & 0x1F)
#define VI_KEY_PPAGE ('U' & 0x1F)

#define OWNER_NAME_MAX 64
#define GROUP_NAME_MAX 64
#define DISPLAY_OWNER_NAME_MAX 12
#define DISPLAY_GROUP_NAME_MAX 12

/* ACS Fallbacks */
#ifndef ACS_ULCORNER
#define ACS_ULCORNER '+'
#endif
#ifndef ACS_URCORNER
#define ACS_URCORNER '+'
#endif
#ifndef ACS_LLCORNER
#define ACS_LLCORNER '+'
#endif
#ifndef ACS_LRCORNER
#define ACS_LRCORNER '+'
#endif
#ifndef ACS_VLINE
#define ACS_VLINE '|'
#endif
#ifndef ACS_HLINE
#define ACS_HLINE '-'
#endif
#ifndef ACS_RTEE
#define ACS_RTEE '+'
#endif
#ifndef ACS_LTEE
#define ACS_LTEE '+'
#endif
#ifndef ACS_BTEE
#define ACS_BTEE '+'
#endif
#ifndef ACS_TTEE
#define ACS_TTEE '+'
#endif
#ifndef ACS_BLOCK
#define ACS_BLOCK '?'
#endif
#ifndef ACS_LARROW
#define ACS_LARROW '<'
#endif

#define PATH_LENGTH 4096
#define FILE_SPEC_LENGTH 256
#define DISK_NAME_LENGTH (12 + 1)
#define COMMAND_LINE_LENGTH 4096
#define COMMAND_PRESENTATION_OVERRIDES_MAX 64
#define COMMAND_PRESENTATION_ACTION_ID_LENGTH 32
#define COMMAND_PRESET_ID_LENGTH 32
#define COMMAND_PRESENTATION_SHOWN_LENGTH 16
#define COMMAND_PRESENTATION_LABEL_LENGTH 64

/*
 * Message length covers two full paths plus context (for operations such as
 * copy/move error reporting).
 */
#define MESSAGE_LENGTH ((PATH_LENGTH * 2) + 256)

#define FILE_SEPARATOR_CHAR '/'
#define FILE_SEPARATOR_STRING "/"
#define TAGGED_SYMBOL '*'
#define PROFILE_CONFIG_HOME_PATH ".config/ytnova/ytnova.conf"
#define PROFILE_CONFIG_HOME_PARENT ".config"
#define PROFILE_CONFIG_HOME_DIR ".config/ytnova"
#define PROFILE_FILENAME ".ytnova"
#define THEME_CONFIG_HOME_PATH ".config/ytnova/themes.conf"
#define THEME_FILENAME ".ytnova.themes"
#define COMMANDS_CONFIG_HOME_PATH ".config/ytnova/commands.conf"
#define COMMANDS_FILENAME ".ytnova.commands"
#define APPLICATIONS_CONFIG_HOME_PATH ".config/ytnova/applications.conf"
#define APPLICATIONS_FILENAME ".ytnova.applications"
#ifndef PACKAGED_COMMANDS_PATH
#define PACKAGED_COMMANDS_PATH "/usr/local/share/ytnova/ytnova.commands"
#endif
#ifndef PACKAGED_COMMAND_PRESET_DIR
#define PACKAGED_COMMAND_PRESET_DIR "/usr/local/share/ytnova/commands"
#endif
#ifndef PACKAGED_APPLICATIONS_PATH
#define PACKAGED_APPLICATIONS_PATH "/usr/local/share/ytnova/ytnova.applications"
#endif
#ifndef PACKAGED_THEME_PATH
#define PACKAGED_THEME_PATH "/usr/local/share/ytnova/ytnova.themes"
#endif
#define HISTORY_STATE_HOME_ENV "XDG_STATE_HOME"
#define HISTORY_STATE_HOME_PATH "ytnova/ytnova.hst"
#define HISTORY_STATE_HOME_FALLBACK ".local/state/ytnova/ytnova.hst"
#define HISTORY_LEGACY_FILENAME ".ytnova-hst"
#define CLOCK_INTERVAL 1
#define DEFAULT_FILE_SPEC "*"

#define SORT_BY_NAME 1
#define SORT_BY_MOD_TIME 2
#define SORT_BY_CHG_TIME 3
#define SORT_BY_ACC_TIME 4
#define SORT_BY_SIZE 5
#define SORT_BY_OWNER 6
#define SORT_BY_GROUP 7
#define SORT_BY_EXTENSION 8
#define SORT_ASC 10
#define SORT_DSC 20
#define SORT_CASE 40
#define SORT_ICASE 80

#define ERR_TO_NULL " 2> /dev/null"
#define ERR_TO_STDOUT " 2>&1 "

/* Auto-refresh configuration modes */
#define REFRESH_WATCHER 1
#define REFRESH_ON_NAV 2
#define REFRESH_ON_ENTER 4

/* View return codes */
#define VIEW_EXIT 0
#define VIEW_NEXT 1
#define VIEW_PREV 2

/* File list presentation modes */
#define MODE_1 0
#define MODE_2 1
#define MODE_3 2
#define MODE_4 3
#define MODE_5 4

/* FileInfo file-window overlay modes */
#define FILEINFO_OVERLAY_NONE 0
#define FILEINFO_OVERLAY_RICH 1
#define FILEINFO_OVERLAY_SUMMARY 2
#define FILEINFO_OVERLAY_GIT 3

#define BOOL unsigned char
#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif
#define LF 10
#define ESC 27
#define LOG_ESC '.'
#define CR 13
#define QUICK_BAUD_RATE 9600

/* Enums */

typedef enum UISemanticRolePair {
  FILE_COLOR_PAIR_UNASSIGNED = 0,
  UI_ROLE_DYNAMIC_TEXT = 1,
  UI_ROLE_STATIC_TEXT,
  UI_ROLE_KEYBIND,
  UI_ROLE_FOOTER,
  UI_ROLE_HELP,
  UI_ROLE_HELP_FOOTER,
  UI_ROLE_HELP_HEADING,
  UI_ROLE_HELP_TOPIC,
  UI_ROLE_HELP_ATTENTION,
  UI_ROLE_HELP_ALERT,
  UI_ROLE_HELP_KEYBIND,
  UI_ROLE_HELP_LINK,
  UI_ROLE_HELP_LINK_SELECTION,
  UI_ROLE_HELP_BOX_LINES,
  UI_ROLE_PICKER,
  UI_ROLE_PICKER_SELECTION,
  UI_ROLE_SELECTION,
  UI_ROLE_BOX_LINES,
  UI_ROLE_TREE_LINES,
  UI_ROLE_MARGIN,
  UI_ROLE_DIALOG,
  UI_ROLE_INFO,
  UI_ROLE_WARNING,
  UI_ROLE_ERROR,
  UI_ROLE_SEARCH_HIT,
  UI_ROLE_DISABLED,
  NUM_UI_COLOR_PAIRS
} UISemanticRolePair;

enum ConfigSurface {
  CONFIG_SURFACE_PROFILE = 0,
  CONFIG_SURFACE_THEME,
  CONFIG_SURFACE_COMMANDS,
  CONFIG_SURFACE_APPLICATIONS
};
typedef enum ConfigSurface ConfigSurface;

#define UI_VIEWER_FRAME_PAIR NUM_UI_COLOR_PAIRS
#define UI_KEYBIND_BASE_PAIR (UI_VIEWER_FRAME_PAIR + 1)
#define UI_HELP_KEYBIND_BASE_PAIR (UI_KEYBIND_BASE_PAIR + (NUM_UI_COLOR_PAIRS - 1))
#define F_COLOR_PAIR_BASE (UI_HELP_KEYBIND_BASE_PAIR + (NUM_UI_COLOR_PAIRS - 1))

enum HistoryType {
  HST_GENERAL = 0,
  HST_LOG,
  HST_EXEC,
  HST_PIPE,
  HST_FILTER,
  HST_SEARCH,
  HST_FILE,
  HST_PATH,
  HST_ID,
  HST_CHANGE_MODUS,
  HST_PRINT_FRAME
};

typedef enum {
  ACTION_NONE = 0,
  ACTION_MOVE_UP,
  ACTION_MOVE_DOWN,
  ACTION_MOVE_SIBLING_NEXT,
  ACTION_MOVE_SIBLING_PREV,
  ACTION_MOVE_LEFT,
  ACTION_MOVE_RIGHT,
  ACTION_PAGE_UP,
  ACTION_PAGE_DOWN,
  ACTION_HOME,
  ACTION_END,
  ACTION_TREE_EXPAND,
  ACTION_TREE_COLLAPSE,
  ACTION_TREE_EXPAND_ALL,
  ACTION_ENTER,
  ACTION_ESCAPE,
  ACTION_LOG,
  ACTION_QUIT,
  ACTION_QUIT_DIR,
  ACTION_TAG,
  ACTION_UNTAG,
  ACTION_TAG_ALL,
  ACTION_UNTAG_ALL,
  ACTION_TAG_REST,
  ACTION_UNTAG_REST,
  ACTION_FILTER,
  ACTION_TOGGLE_MODE,
  ACTION_REFRESH,
  ACTION_RESIZE,
  ACTION_VOL_MENU,
  ACTION_VOL_PREV,
  ACTION_VOL_NEXT,
  ACTION_CMD_A,
  ACTION_CMD_B,
  ACTION_CMD_C,
  ACTION_CMD_D,
  ACTION_CMD_E,
  ACTION_CMD_G,
  ACTION_CMD_H,
  ACTION_CMD_I,
  ACTION_CMD_M,
  ACTION_CMD_O,
  ACTION_CMD_P,
  ACTION_CMD_R,
  ACTION_CMD_S,
  ACTION_CMD_V,
  ACTION_CMD_X,
  ACTION_CMD_Y,
  ACTION_CMD_PRINT,
  ACTION_TOGGLE_HIDDEN,
  ACTION_TOGGLE_COMPACT,
  ACTION_CMD_MKFILE,
  ACTION_CMD_TAGGED_A,
  ACTION_CMD_TAGGED_C,
  ACTION_CMD_TAGGED_D,
  ACTION_CMD_TAGGED_G,
  ACTION_CMD_TAGGED_M,
  ACTION_CMD_TAGGED_O,
  ACTION_CMD_TAGGED_P,
  ACTION_CMD_TAGGED_R,
  ACTION_CMD_TAGGED_S,
  ACTION_CMD_TAGGED_V,
  ACTION_CMD_TAGGED_X,
  ACTION_CMD_TAGGED_Y,
  ACTION_CMD_TAGGED_PRINT,
  ACTION_LIST_JUMP,
  ACTION_TO_DIR,
  ACTION_TOGGLE_TAGGED_MODE,
  ACTION_TOGGLE_STATS,
  ACTION_ASTERISK,
  ACTION_INVERT,
  ACTION_SPLIT_SCREEN,
  ACTION_SWITCH_PANEL,
  ACTION_VIEW_PREVIEW,
  ACTION_PREVIEW_SCROLL_UP,
  ACTION_PREVIEW_SCROLL_DOWN,
  ACTION_PREVIEW_HOME,
  ACTION_PREVIEW_END,
  ACTION_PREVIEW_PAGE_UP,
  ACTION_PREVIEW_PAGE_DOWN,
  ACTION_COMPARE_FILE,
  ACTION_COMPARE_DIR,
  ACTION_COMPARE_TREE,
  ACTION_HELP,
  ACTION_EDIT_CONFIG,
  ACTION_FILEINFO_1,
  ACTION_FILEINFO_2,
  ACTION_FILEINFO_3,
  ACTION_FILEINFO_4,
  ACTION_FILEINFO_5,
  ACTION_FILEINFO_6,
  ACTION_FILEINFO_7,
  ACTION_FILEINFO_8,
  ACTION_FILEINFO_9,
  ACTION_FILEINFO_0,
  ACTION_USER_CMD
} YtreeNovaAction;

typedef enum { FOCUS_TREE, FOCUS_FILE } ViewFocus;

typedef enum {
  COMPARE_FLOW_FILE = 0,
  COMPARE_FLOW_DIRECTORY,
  COMPARE_FLOW_LOGGED_TREE
} CompareFlowType;

typedef enum {
  COMPARE_BASIS_NONE = 0,
  COMPARE_BASIS_SIZE,
  COMPARE_BASIS_DATE,
  COMPARE_BASIS_SIZE_AND_DATE,
  COMPARE_BASIS_HASH
} CompareBasis;

typedef enum {
  COMPARE_TAG_NONE = 0,
  COMPARE_TAG_DIFFERENT,
  COMPARE_TAG_MATCH,
  COMPARE_TAG_NEWER,
  COMPARE_TAG_OLDER,
  COMPARE_TAG_UNIQUE,
  COMPARE_TAG_TYPE_MISMATCH,
  COMPARE_TAG_ERROR
} CompareTagResult;

typedef enum {
  COMPARE_MENU_CANCEL = 0,
  COMPARE_MENU_DIRECTORY_ONLY,
  COMPARE_MENU_DIRECTORY_PLUS_TREE,
  COMPARE_MENU_EXTERNAL_DIRECTORY,
  COMPARE_MENU_EXTERNAL_TREE
} CompareMenuChoice;

typedef struct {
  CompareFlowType flow_type;
  CompareBasis basis;
  CompareTagResult tag_result;
  BOOL used_split_default_target;
  char source_path[PATH_LENGTH + 1];
  char target_path[PATH_LENGTH + 1];
} CompareRequest;

typedef enum {
  PRINT_FORMAT_RAW = 0,
  PRINT_FORMAT_FRAMED,
  PRINT_FORMAT_PAGEBREAK
} PrintFormat;

typedef enum {
  PRINT_DESTINATION_FILE = 0,
  PRINT_DESTINATION_COMMAND
} PrintDestination;

typedef struct {
  PrintFormat format;
  PrintDestination destination;
  int lines_per_page;
  int margin;
  char print_to[PATH_LENGTH + 1];
  char frame_separator[32];
} PrintConfig;

/* Structs */

typedef struct {
  int dir_win_y;
  int dir_win_x;
  int dir_win_height;
  int dir_win_width;

  int small_file_win_y;
  int small_file_win_x;
  int small_file_win_height;
  int small_file_win_width;

  int big_file_win_y;
  int big_file_win_x;
  int big_file_win_height;
  int big_file_win_width;

  int preview_win_y;
  int preview_win_x;
  int preview_win_height;
  int preview_win_width;

  int stats_width;
  int main_win_width;

  int stats_y_filter_val;
  int stats_y_vol_sep;
  int stats_y_vol_info;
  int stats_y_vstat_sep;
  int stats_y_vstat_val;
  int stats_y_dstat_sep;
  int stats_y_dstat_val;
  int stats_y_attr_sep;
  int stats_y_attr_val;

  int header_y;
  int message_y;
  int prompt_y;
  int status_y;
  int bottom_border_y;
} YtreeNovaLayout;

#ifdef HAVE_LIBARCHIVE
#define AR_KEEP 0
#define AR_SKIP 1
#define AR_ABORT -1
typedef int (*ArchiveProgressCallback)(int status, const char *msg,
                                       void *user_data);
typedef int (*RewriteCallback)(struct archive *r, struct archive *w,
                               struct archive_entry *entry, void *user_data);
#else
typedef int (*ArchiveProgressCallback)(int status, const char *msg,
                                       void *user_data);
#endif

/* UI Callback Status Codes (for ArchiveProgressCallback) */
#define ARCHIVE_STATUS_PROGRESS 1
#define ARCHIVE_STATUS_ERROR 2
#define ARCHIVE_STATUS_WARNING 3
#define ARCHIVE_CB_CONTINUE 0
#define ARCHIVE_CB_ABORT -1

/* Sort methods */
#define TAGSYMBOL_VIEWNAME "tag"
#define FILENAME_VIEWNAME "fnm"
#define ATTRIBUTE_VIEWNAME "atr"
#define LINKCOUNT_VIEWNAME "lct"
#define FILESIZE_VIEWNAME "fsz"
#define MODTIME_VIEWNAME "mot"
#define SYMLINK_VIEWNAME "lnm"
#define UID_VIEWNAME "uid"
#define GID_VIEWNAME "gid"
#define INODE_VIEWNAME "ino"
#define ACCTIME_VIEWNAME "act"
#define SCTIME_VIEWNAME "sct"

/* Compression methods */
#define FREEZE_COMPRESS 1
#define COMPRESS_COMPRESS 3
#define GZIP_COMPRESS 5
#define BZIP_COMPRESS 6
#define LZIP_COMPRESS 21
#define ZSTD_COMPRESS 22

/* UI Message Macros (Global Messaging API) */
extern int UI_Error(ViewContext *ctx, const char *module, int line,
                    const char *fmt, ...);
extern int UI_Warning(ViewContext *ctx, const char *fmt, ...);
extern int UI_Message(ViewContext *ctx, const char *fmt, ...);
extern int UI_Notice(ViewContext *ctx, const char *fmt, ...);

#define ERROR(ctx, ...) UI_Error(ctx, __FILE__, __LINE__, __VA_ARGS__)
#define WARNING(ctx, ...) UI_Warning(ctx, __VA_ARGS__)
#define MESSAGE(ctx, ...) UI_Message(ctx, __VA_ARGS__)
#define NOTICE(ctx, ...) UI_Notice(ctx, __VA_ARGS__)

/* memory_utils.c */
extern void *xmalloc(size_t size);
extern void *xcalloc(size_t nmemb, size_t size);
extern void *xrealloc(void *ptr, size_t size);
extern char *xstrdup(const char *s);

/* string_utils.c */
extern int BuildFilename(char *in_filename, char *pattern, char *out_filename);
extern int String_Replace(char *dest, size_t dest_size, const char *src,
                          const char *token, const char *replacement);
extern BOOL String_HasNonWhitespace(const char *text);
extern void String_GetCommandDisplayName(const char *command_template,
                                         char *command_name,
                                         size_t command_name_size);

/* Input Debugging */

typedef struct {
  const char *name;
  int id;
  int fg;
  int bg;
} UIColor;

extern volatile sig_atomic_t ytnova_shutdown_flag;

typedef struct _file_color_rule {
  char *pattern;
  int fg;
  int bg;
  int pair_id;
  struct _file_color_rule *next;
} FileColorRule;

typedef struct _file_entry {
  struct _file_entry *next;
  struct _file_entry *prev;
  struct _dir_entry *dir_entry;
  struct stat stat_struct;
  BOOL tagged;
  BOOL matching;
  char name[];
} FileEntry;

typedef struct _dir_entry {
  struct _file_entry *file;
  struct _dir_entry *next;
  struct _dir_entry *prev;
  struct _dir_entry *sub_tree;
  struct _dir_entry *up_tree;
  long long total_bytes;
  long long matching_bytes;
  long long tagged_bytes;
  unsigned int total_files;
  unsigned int matching_files;
  unsigned int tagged_files;
  int cursor_pos;
  int start_file;
  struct stat stat_struct;
  BOOL access_denied;
  BOOL global_flag;
  BOOL global_all_volumes;
  BOOL tagged_flag;
  BOOL only_tagged;
  BOOL not_scanned;
  BOOL unlogged_flag;
  BOOL big_window;
  BOOL log_flag;
  char name[];
} DirEntry;

typedef struct {
  unsigned long indent;
  DirEntry *dir_entry;
  unsigned short level;
} DirEntryList;

typedef struct {
  FileEntry *file;
} FileEntryList;

typedef struct {
  FileEntry *file;
  char code[3];
} PanelGitStatusEntry;

typedef struct {
  DirEntry *tree;
  long long disk_space;
  long long disk_capacity;
  long long disk_total_files;
  long long disk_total_bytes;
  long long disk_matching_files;
  long long disk_matching_bytes;
  long long disk_tagged_files;
  long long disk_tagged_bytes;
  unsigned int disk_total_directories;
  int kind_of_sort;
  int log_mode;
  unsigned int archive_capabilities;
  char log_path[PATH_LENGTH + 1];
  char path[PATH_LENGTH + 1];
  char file_spec[FILE_SPEC_LENGTH + 1];
  char disk_name[DISK_NAME_LENGTH + 1];
} Statistic;

struct Volume {
  int id;
  unsigned int volume_generation;
  int saved_focus;       /* Derived restore mirror of the last focused panel mode. */
  Statistic vol_stats;
  Statistic vol_disk_stats;
  DirEntryList *dir_entry_list;
  size_t dir_entry_list_capacity;
  int total_dirs;
  UT_hash_handle hh;
};

typedef union {
  struct {
    char new_mode[12];
  } change_mode;
  struct {
    unsigned new_owner_id;
  } change_owner;
  struct {
    unsigned new_group_id;
  } change_group;
  struct {
    char *command;
  } execute;
  struct {
    Statistic *statistic_ptr;
    DirEntry *dest_dir_entry;
    char *to_file;
    char *to_path;
    BOOL path_copy;
    BOOL confirm;
    int dir_create_mode;
    int overwrite_mode;
    void *conflict_cb; /* ConflictCallback from ytnova_cmd.h */
    void *choice_cb;   /* ChoiceCallback from ytnova_cmd.h */
  } copy;
  struct {
    char *new_name;
    BOOL confirm;
  } rename;
  struct {
    DirEntry *dest_dir_entry;
    char *to_file;
    char *to_path;
    BOOL confirm;
    int dir_create_mode;
    int overwrite_mode;
    void *conflict_cb; /* ConflictCallback from ytnova_cmd.h */
    void *choice_cb;   /* ChoiceCallback from ytnova_cmd.h */
  } mv;
  struct {
    FILE *pipe_file;
  } pipe_cmd;
  struct {
    FILE *zipfile;
    int method;
  } compress_cmd;
  struct {
    Statistic *statistic_ptr;
    int auto_override;
    void *choice_cb;
  } del;
} FunctionData;

typedef struct {
  FileEntry *new_fe_ptr;
  FunctionData function_data;
} WalkingPackage;

typedef struct _PathList {
  char *path;
  struct _PathList *next;
} PathList;

typedef struct _panel_volume_file_state {
  int volume_id;
  int saved_file_start;        /* Per-panel file viewport snapshot for this volume. */
  int saved_file_cursor;       /* Per-panel file cursor snapshot for this volume. */
  unsigned int saved_panel_generation;
  unsigned int saved_volume_generation;
  unsigned int saved_tree_panel_generation;
  unsigned int saved_tree_volume_generation;
  ViewFocus saved_focus;       /* Restored panel focus shape for this volume. */
  BOOL saved_big_file_view;    /* Restored panel file-window shape for this volume. */
  BOOL has_saved_tree_selection;
  BOOL has_saved_tree_top;
  char saved_tree_selected_dir_path[PATH_LENGTH + 1];
  char saved_tree_top_dir_path[PATH_LENGTH + 1];
  char saved_file_dir_path[PATH_LENGTH + 1];
  char saved_file_selection_name[PATH_LENGTH + 1];
  char saved_file_selection_dir_path[PATH_LENGTH + 1];
  struct _panel_volume_file_state *next;
} PanelVolumeFileState;

typedef struct _ArchiveExpandedEntry {
  char *source_path;
  char *archive_path;
  struct _ArchiveExpandedEntry *next;
} ArchiveExpandedEntry;

typedef struct {
  PathList *original_source_list;
  ArchiveExpandedEntry *expanded_file_list;
} ArchivePayload;

typedef struct {
#ifdef YTNOVA_TUI
  WINDOW *pan_dir_window;
  WINDOW *pan_small_file_window;
  WINDOW *pan_big_file_window;
  WINDOW *pan_file_window;
#else
  void *pan_dir_window;
  void *pan_small_file_window;
  void *pan_big_file_window;
  void *pan_file_window;
#endif
  struct Volume *vol;
  FileEntryList *file_entry_list;
  size_t file_entry_list_capacity;
  unsigned int file_count;
  int dir_x, dir_y, dir_w, dir_h;
  int small_file_x, small_file_y, small_file_w, small_file_h;
  int big_file_x, big_file_y, big_file_w, big_file_h;
  int stats_x, stats_width;
  int cursor_pos;
  int disp_begin_pos;
  char tree_viewport_top_dir_path[2][PATH_LENGTH + 1];
  int start_file;
  int file_cursor_pos;
  DirEntry *file_dir_entry;
  struct _PathList *tagged_paths;
  PanelVolumeFileState *volume_file_state;
  char file_selection_name[PATH_LENGTH + 1];
  char file_selection_dir_path[PATH_LENGTH + 1];
  BOOL saved_big_file_view; /* Panel-local file-window shape snapshot. */
  int dir_mode;
  int file_mode;
  int fileinfo_overlay_mode;
  int fixed_col_width;
  int max_column;
  int current_dir_entry;
  unsigned int panel_generation;
  ViewFocus saved_focus;
  unsigned int max_visual_filename_len;
  unsigned int max_visual_linkname_len;
  unsigned int max_visual_userview_len;
  PanelGitStatusEntry *git_status_entries;
  unsigned int git_status_entry_count;
  FileEntry *git_status_first_file;
  FileEntry *git_status_last_file;
  char git_status_dir_path[PATH_LENGTH + 1];
  BOOL human_size_units;
  BOOL git_status_is_worktree;
  BOOL reverse_sort;
  BOOL show_symlink_targets;
  BOOL show_stats;
  BOOL hide_dot_files; /* Panel-local visibility. */
} YtreeNovaPanel;

typedef struct _history {
  char *hst;
  int type;
  int pinned;
  struct _history *next;
  struct _history *prev;
} History;

typedef struct {
  int start_y;
  int start_x;
  int height;
  int width;
  int header_y;
  int message_y;
  int prompt_y;
  int status_y;
} ViewerGeometry;

typedef struct {
  int wlines;
  int wcols;
  int bytes;
  WINDOW *view;
  WINDOW *border;
  BOOL resize_done;
  BOOL inhex;
  BOOL inedit;
  BOOL hexoffset;
} ViewerState;

typedef struct {
  BOOL active;        /* Progress display currently shown */
  char operation[32]; /* "COPYING", "MOVING", etc */
  char source_path[PATH_LENGTH + 1];
  char dest_path[PATH_LENGTH + 1]; /* Empty string for delete/scan ops */

  /* Metrics */
  long long bytes_total; /* 0 if unknown */
  long long bytes_done;
  unsigned int items_total; /* File/directory count (0 if N/A) */
  unsigned int items_done;

  /* ETA Calculation */
  time_t start_time;
  time_t last_render_time;
  double bytes_per_sec; /* Rolling average transfer rate */
  int eta_seconds;      /* Estimated time remaining */

  /* Cancellation */
  BOOL cancel_requested; /* Set by Esc key handler */
} ProgressContext;

typedef struct {
  int (*confirm_quit)(ViewContext *ctx, const char *msg, const char *choices);
  int (*save_history)(ViewContext *ctx, const char *path_for_history);
  void (*close_watcher)(ViewContext *ctx);
  void (*cleanup_volume_tree)(ViewContext *ctx);
  void (*suspend_clock)(ViewContext *ctx);
  void (*shutdown_terminal)(ViewContext *ctx);
} CoreQuitOps;

typedef struct {
  int (*read_profile)(ViewContext *ctx, const char *filename);
  int (*load_commands)(ViewContext *ctx);
  int (*load_startup_theme)(ViewContext *ctx);
  int (*load_theme)(ViewContext *ctx);
  int (*read_history)(ViewContext *ctx, const char *filename);
  char *(*get_profile_value)(const ViewContext *ctx, const char *name);
  BOOL (*has_user_action)(const ViewContext *ctx);
  void (*start_colors)(ViewContext *ctx);
  void (*dialog_init)(void);
  void (*reinit_color_pairs)(ViewContext *ctx);
  void (*set_panel_file_mode)(ViewContext *ctx, YtreeNovaPanel *panel,
                              int new_file_mode);
  void (*wbkgd_set)(const ViewContext *ctx, WINDOW *win, chtype c);
  int (*ui_notice)(ViewContext *ctx, const char *msg);
  void (*parse_color_string)(const char *color_str, int *fg, int *bg);
  BOOL (*parse_color_string_strict)(const char *color_str, int *fg, int *bg);
  void (*update_ui_color)(const char *name, int fg, int bg);
  void *(*capture_ui_colors)(void);
  void (*restore_ui_colors)(void *snapshot);
  void (*free_ui_colors)(void *snapshot);
  void (*add_file_color_rule)(ViewContext *ctx, const char *pattern, int fg,
                              int bg);
  void (*bind_runtime_hooks)(ViewContext *ctx);
} CoreInitOps;

typedef struct {
  int (*init)(ViewContext *ctx, const char *configuration_file,
              const char *history_file);
  void (*set_profile_value)(const ViewContext *ctx, char *name,
                            const char *value);
  int (*log_disk)(ViewContext *ctx, YtreeNovaPanel *panel, char *path);
  int (*set_filter)(const char *filter_spec, Statistic *s);
  void (*recalculate_sys_stats)(ViewContext *ctx, Statistic *s);
  int (*handle_dir_window)(ViewContext *ctx, const DirEntry *start_dir_entry);
  void (*suspend_clock)(ViewContext *ctx);
  void (*shutdown_curses)(ViewContext *ctx);
  void (*volume_free_all)(ViewContext *ctx);
} CoreMainOps;

typedef void (*ScanProgressCallback)(ViewContext *ctx, void *user_data);
typedef void (*CoreScanProgressCallback)(ViewContext *ctx, void *user_data);

typedef struct {
  char action_id[COMMAND_PRESENTATION_ACTION_ID_LENGTH];
  char shown[COMMAND_PRESENTATION_SHOWN_LENGTH];
  char label[COMMAND_PRESENTATION_LABEL_LENGTH];
} CommandPresentationOverride;

typedef struct {
  int (*get_disk_parameter)(char *path, char *volume_name,
                            long long *avail_bytes, long long *capacity,
                            Statistic *s);
  int (*read_tree)(ViewContext *ctx, DirEntry *dir_entry, char *path, int depth,
                   Statistic *s, CoreScanProgressCallback cb, void *cb_data);
  int (*read_tree_from_archive)(ViewContext *ctx, DirEntry **dir_entry_ptr,
                                const char *filename, Statistic *s,
                                CoreScanProgressCallback cb, void *cb_data);
  void (*delete_tree)(DirEntry *tree);
  void (*watcher_init)(ViewContext *ctx);
} CoreStorageOps;

extern int UI_CoreQuitConfirm(ViewContext *ctx, const char *msg,
                              const char *choices);
extern int UI_CoreQuitSaveHistory(ViewContext *ctx,
                                  const char *path_for_history);
extern void UI_CoreQuitCloseWatcher(ViewContext *ctx);
extern void UI_CoreQuitCleanupVolumeTree(ViewContext *ctx);
extern void UI_CoreQuitSuspendClock(ViewContext *ctx);
extern void UI_CoreQuitShutdownTerminal(ViewContext *ctx);
extern void CoreInitOps_RegisterCmdConfig(CoreInitOps *ops);
extern void CoreInitOps_RegisterCmdCommands(CoreInitOps *ops);
extern void CoreInitOps_RegisterCmdProfile(CoreInitOps *ops);
extern void CoreInitOps_RegisterCmdTheme(CoreInitOps *ops);
extern void CoreInitOps_RegisterUIRuntime(CoreInitOps *ops);
extern void CoreMainOps_Register(ViewContext *ctx);
extern void CoreStorageOps_Register(ViewContext *ctx);
extern void CoreWatcherOps_Register(ViewContext *ctx);
extern int ConfigPaths_EnsureHomeDirectory(const char *home);
extern int ConfigPaths_ResolvePreferredPath(ConfigSurface surface, char *path,
                                            size_t path_size);
extern int ConfigPaths_ResolveLegacyPath(ConfigSurface surface, char *path,
                                         size_t path_size,
                                         BOOL allow_cwd_fallback);
extern int ConfigPaths_ResolveBootstrapPath(ConfigSurface surface, char *path,
                                            size_t path_size,
                                            BOOL allow_cwd_fallback);
extern int ConfigPaths_IsPreferredPath(ConfigSurface surface,
                                       const char *path);
extern int ConfigPaths_IsLegacyPath(ConfigSurface surface, const char *path);
extern int ConfigPaths_ResolveActiveEditPath(const ViewContext *ctx,
                                             ConfigSurface surface, char *path,
                                             size_t path_size);
extern int ConfigPaths_ResolveLoadedOrBootstrapPath(const ViewContext *ctx,
                                                    ConfigSurface surface,
                                                    char *path,
                                                    size_t path_size,
                                                    BOOL allow_cwd_fallback);
typedef int (*AtomicFileWriteCallback)(FILE *fp, void *user_data);
extern int AtomicFileWrite(const char *path, AtomicFileWriteCallback writer,
                           void *user_data);

extern void UI_Dialog_Init(void);
extern char *GetProfileValue(const ViewContext *ctx, const char *name);
extern BOOL IsUserActionDefined(const ViewContext *ctx);
extern int ResolvePreferredHistoryPath(char *path, size_t path_size);
extern int ResolveLegacyHistoryPath(char *path, size_t path_size);
extern int ScanSubTree(ViewContext *ctx, DirEntry *dir_entry, Statistic *s);
extern int RemoveFile(ViewContext *ctx, FileEntry *fe_ptr, Statistic *s);
extern int MakePath(const ViewContext *ctx, DirEntry *tree, char *dir_path,
                    DirEntry **dest_dir_entry);
extern int ReadProfile(ViewContext *ctx, const char *filename);
extern void FreeProfileRuntimeData(ViewContext *ctx);
extern int ReadHistory(ViewContext *ctx, const char *Filename);
extern void SetPanelFileMode(ViewContext *ctx, YtreeNovaPanel *p,
                             int new_file_mode);
extern void InitClock(ViewContext *ctx);
extern struct Volume *Volume_Create(ViewContext *ctx);
extern void SetKindOfSort(int kind_of_sort, Statistic *s);

typedef struct _ViewContext {
  SCREEN *curses_screen;
  WINDOW *ctx_dir_window;
  WINDOW *ctx_small_file_window;
  WINDOW *ctx_big_file_window;
  WINDOW *ctx_file_window;
  WINDOW *ctx_preview_window;
  WINDOW *ctx_border_window;
  WINDOW *ctx_path_window;
  WINDOW *ctx_time_window;
  WINDOW *ctx_menu_window;
  WINDOW *ctx_error_window;
  WINDOW *ctx_history_window;
  WINDOW *ctx_matches_window;
  WINDOW *ctx_f2_window;

  YtreeNovaPanel *left;
  YtreeNovaPanel *right;
  YtreeNovaPanel *active;
  YtreeNovaLayout layout;

  int view_mode;
  int dir_mode;
  BOOL show_stats;
  BOOL preview_mode;
  BOOL clock_print_time;
  int fixed_col_width;
  int refresh_mode;
  ViewFocus preview_entry_focus;
  YtreeNovaPanel *preview_return_panel;
  ViewFocus preview_return_focus;

  ViewerState viewer;

  /* Animation State */
  BOOL anim_is_initialized;
  void *anim_stars;
  int spin_counter;

  /* Color State */
  BOOL color_enabled;

  int animation_method;
  char number_seperator;
  BOOL is_split_screen;
  char global_search_term[256];
  int user_umask;
  BOOL resize_request;
  int cached_lines; /* Last known LINES for resize detection */
  int cached_cols;  /* Last known COLS for resize detection */
  BOOL bypass_small_window;
  BOOL highlight_full_line;
  BOOL status_line_error_pending;
  char status_line_error_text[PATH_LENGTH + 1];
  BOOL status_line_notice_pending;
  char status_line_notice_text[PATH_LENGTH + 1];
  char *initial_directory;
  char configuration_file_path[PATH_LENGTH + 1];
  BOOL configuration_file_path_is_explicit;
  char history_file_path[PATH_LENGTH + 1];
  char commands_file_path[PATH_LENGTH + 1];
  char theme_file_path[PATH_LENGTH + 1];
  CommandPresentationOverride
      dir_command_presentations[COMMAND_PRESENTATION_OVERRIDES_MAX];
  size_t dir_command_presentation_count;
  CommandPresentationOverride
      archive_dir_command_presentations[COMMAND_PRESENTATION_OVERRIDES_MAX];
  size_t archive_dir_command_presentation_count;
  CommandPresentationOverride
      file_command_presentations[COMMAND_PRESENTATION_OVERRIDES_MAX];
  size_t file_command_presentation_count;
  CommandPresentationOverride
      archive_file_command_presentations[COMMAND_PRESENTATION_OVERRIDES_MAX];
  size_t archive_file_command_presentation_count;
  char *confirm_quit;
  void *file_color_rules_head;

  /* ctrl_file state */
  int ctrl_file_max_disp_files;
  int ctrl_file_x_step;
  int ctrl_file_my_x_step;
  int ctrl_file_hide_right;

  unsigned ctrl_file_max_visual_filename_len;
  unsigned ctrl_file_max_visual_linkname_len;
  unsigned ctrl_file_max_visual_userview_len;
  unsigned ctrl_file_global_max_visual_filename_len;
  unsigned ctrl_file_global_max_visual_linkname_len;

  long ctrl_file_preview_line_offset;
  int ctrl_file_saved_fixed_width;

  char ctrl_file_to_dir[PATH_LENGTH + 1];
  char ctrl_file_to_path[PATH_LENGTH + 1];
  char ctrl_file_to_file[PATH_LENGTH + 1];

  /* Boundary inversion hooks */
  void (*hook_parse_color)(const char *color_str, int *fg, int *bg);
  void (*hook_update_ui_color)(const char *name, int fg, int bg);
  void (*hook_add_file_color_rule)(ViewContext *ctx, const char *pattern,
                                   int fg, int bg);
  char *(*hook_get_profile_value)(const ViewContext *ctx, const char *name);
  BOOL (*hook_has_user_action)(const ViewContext *ctx);
  int (*hook_scan_subtree)(ViewContext *ctx, DirEntry *dir_entry, Statistic *s);
  int (*hook_remove_file)(ViewContext *ctx, FileEntry *fe_ptr, Statistic *s);
  int (*hook_make_path)(const ViewContext *ctx, DirEntry *tree, char *dir_path,
                        DirEntry **dest_dir_entry);
  BOOL (*hook_key_pressed)(void);
  BOOL (*hook_escape_key_pressed)(void);
  int (*hook_input_choice)(ViewContext *ctx, const char *msg,
                           const char *choices);
  void (*hook_quit)(ViewContext *ctx);
  int (*hook_ui_message)(ViewContext *ctx, const char *fmt, ...);
  int (*hook_ui_notice)(ViewContext *ctx, const char *fmt, ...);
  void (*hook_draw_spinner)(ViewContext *ctx);
  void (*hook_clock_handler)(ViewContext *ctx, int sig);
  void (*hook_draw_animation_step)(ViewContext *ctx, WINDOW *win);
  void (*hook_display_disk_statistic)(ViewContext *ctx, const Statistic *s);
  void (*hook_display_avail_bytes)(ViewContext *ctx, const Statistic *s);
  void (*hook_display_menu)(ViewContext *ctx);
  void (*hook_build_dir_entry_list)(ViewContext *ctx, struct Volume *vol,
                                    int *index_ptr);
  void (*hook_display_tree)(ViewContext *ctx, struct Volume *vol, WINDOW *win,
                            int start_entry_no, int hilight_no, BOOL is_active);
  void (*hook_switch_to_big_file_window)(ViewContext *ctx);
  void (*hook_init_animation)(ViewContext *ctx);
  void (*hook_refresh_window)(WINDOW *win);
  void (*hook_stop_animation)(ViewContext *ctx);
  void (*hook_switch_to_small_file_window)(ViewContext *ctx);
  void (*hook_clear_help)(ViewContext *ctx);
  int (*hook_mv_add_str)(int y, int x, char *str);
  int (*hook_read_string)(ViewContext *ctx, YtreeNovaPanel *panel,
                          const char *prompt, char *buffer, int max_len,
                          int history_type);
  void (*hook_recreate_windows)(ViewContext *ctx);
  void (*hook_hit_return_to_continue)(void);
  void (*hook_recalculate_sys_stats)(ViewContext *ctx, Statistic *s);
  void (*hook_suspend_clock)(ViewContext *ctx);
  void (*hook_init_clock)(ViewContext *ctx);
  void (*hook_clear_prompt_line)(ViewContext *ctx);
  int (*hook_refresh_ui)(void);
  CoreInitOps core_init_ops;
  CoreStorageOps core_storage_ops;
  CoreQuitOps core_quit_ops;
  CoreMainOps core_main_ops;

  /* profile.c state */
  void *profile_data;  /* Pointer to the profile array */
  void *viewer_list;   /* Pointer to head of Viewer list */
  void *dirmenu_list;  /* Pointer to head of Dirmenu list */
  void *archive_dirmenu_list;  /* Pointer to head of archive Dirmenu list */
  void *filemenu_list; /* Pointer to head of Filemenu list */
  void *archive_filemenu_list; /* Pointer to head of archive Filemenu list */

  /* history.c state */
  int total_hist;
  int cursor_pos;
  int disp_begin_pos;
  History *history_head;
  History **history_view_list;
  int history_view_count;
  /* volume.c state */
  struct Volume *volumes_head;
  int volume_serial;

  /* watcher.c state */
  int inotify_fd;
  int current_wd;
  char current_watch_path[PATH_LENGTH + 1];

  /* tabcompl.c state */
  char **tab_mtchs;
  int tab_total_matches;
  int tab_cursor_pos;
  int tab_disp_begin_pos;

  /* progress.c state */
  ProgressContext progress;

} ViewContext;

#endif /* YTNOVA_DEFS_H */
