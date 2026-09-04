/***************************************************************************
 *
 * ytnova_ui.h
 * User Interface (Ncurses) rendering and input handling
 *
 ***************************************************************************/

#ifndef YTNOVA_UI_H
#define YTNOVA_UI_H

#include "ytnova_defs.h"
#include "ytnova_dialog.h"
#include "ytnova_i18n.h"

#ifdef WITH_UTF8
/* In UTF-8 mode, let ncurses handle bytes directly. */
#ifndef PRINT
#define PRINT(ch) ((unsigned char)(ch) < 32 && (ch) != 0 ? ACS_BLOCK : (ch))
#endif
#else
#ifndef PRINT
#define PRINT(ch)                                                              \
  (iscntrl(ch) && (((unsigned char)(ch)) < ' ')) ? (ACS_BLOCK)                 \
                                                 : ((unsigned char)(ch))
#endif
#endif

#ifndef ERROR_WINDOW_WIDTH
#define ERROR_WINDOW_WIDTH 40
#endif
#ifndef ERROR_WINDOW_HEIGHT
#define ERROR_WINDOW_HEIGHT 10
#endif
#ifndef ERROR_WINDOW_X
#define ERROR_WINDOW_X ((COLS - ERROR_WINDOW_WIDTH) >> 1)
#endif
#ifndef ERROR_WINDOW_Y
#define ERROR_WINDOW_Y ((LINES - ERROR_WINDOW_HEIGHT) >> 1)
#endif

/* Standard UI Vertical Layout */
#ifndef Y_HEADER
#define Y_HEADER(ctx) ((ctx)->layout.header_y)
#endif
#ifndef Y_PROMPT
#define Y_PROMPT(ctx) ((ctx)->layout.prompt_y)
#endif
#ifndef Y_STATUS
#define Y_STATUS(ctx) ((ctx)->layout.status_y)
#endif
#ifndef Y_MESSAGE
#define Y_MESSAGE(ctx) ((ctx)->layout.message_y)
#endif

/* animate.c */
extern void InitAnimation(ViewContext *ctx);
extern void StopAnimation(ViewContext *ctx);
extern void DrawAnimationStep(ViewContext *ctx, WINDOW *win);
extern void DrawSpinner(ViewContext *ctx);

/* init.c */
extern int Init(ViewContext *ctx, const char *configuration_file,
                const char *history_file);
extern void Layout_Recalculate(ViewContext *ctx);
extern void ReCreateWindows(ViewContext *ctx);
extern void ShutdownCurses(ViewContext *ctx);

/* clock.c */
extern void ClockHandler(ViewContext *ctx, int sig);
extern void InitClock(ViewContext *ctx);
extern void SuspendClock(ViewContext *ctx);

/* color.c */
#ifdef COLOR_SUPPORT
typedef struct _ui_color_snapshot UIColorSnapshot;
extern UIColorSnapshot *UIColorSnapshot_Create(void);
extern void UIColorSnapshot_Restore(UIColorSnapshot *snapshot);
extern void UIColorSnapshot_Free(UIColorSnapshot *snapshot);
extern void StartColors(ViewContext *ctx);
extern void ReinitColorPairs(ViewContext *ctx);
extern void WbkgdSet(const ViewContext *ctx, WINDOW *w, chtype c);
extern void ParseColorString(const char *color_str, int *fg, int *bg);
extern BOOL ParseColorStringStrict(const char *color_str, int *fg, int *bg);
extern void UpdateUIColor(const char *name, int fg, int bg);
extern chtype UISelectionAttrForBase(const ViewContext *ctx, int base_role);
extern chtype UIKeybindAttrForBase(int overlay_role, int base_role);
extern void AddFileColorRule(ViewContext *ctx, const char *pattern, int fg,
                             int bg);
extern int GetFileTypeColor(const ViewContext *ctx, const FileEntry *fe_ptr);

#else
#define StartColors(ctx) ;
#define ReinitColorPairs(ctx) ;
#define WbkgdSet(ctx, a, b) ;
#define ParseColorString(color_str, fg, bg)                                 \
  ((void)(color_str), (void)(fg), (void)(bg))
#define ParseColorStringStrict(color_str, fg, bg)                           \
  ((void)(color_str), (void)(fg), (void)(bg), FALSE)
#define UpdateUIColor(name, fg, bg) ((void)(name), (void)(fg), (void)(bg))
#define UISelectionAttrForBase(ctx, base_role)                              \
  ((void)(ctx), (void)(base_role), A_REVERSE)
#define UIKeybindAttrForBase(overlay_role, base_role)                       \
  ((void)(overlay_role), (void)(base_role), A_BOLD)
#define AddFileColorRule(ctx, pattern, fg, bg)                               \
  ((void)(ctx), (void)(pattern), (void)(fg), (void)(bg))
#define GetFileTypeColor(ctx, fe_ptr)                                        \
  ((void)(ctx), (void)(fe_ptr), UI_ROLE_DYNAMIC_TEXT)
#endif

/* dirwin.c */
extern int HandleDirWindow(ViewContext *ctx, const DirEntry *start_dir_entry);
extern int KeyF2Get(ViewContext *ctx, YtreeNovaPanel *panel, char *path);
extern int RefreshDirWindow(ViewContext *ctx, YtreeNovaPanel *p);
extern int ScanSubTree(ViewContext *ctx, DirEntry *dir_entry, Statistic *s);
extern void ToggleDotFiles(ViewContext *ctx, YtreeNovaPanel *p);
extern BOOL HandleDirMakeFile(ViewContext *ctx, DirEntry *dir_entry);
extern void HandleDirMakeDirectory(ViewContext *ctx, DirEntry *dir_entry,
                                   Statistic *s);
extern int ArchiveDirectoryTransferProgress(int status, const char *message,
                                            void *user_data);
extern void ArchiveDirectoryTransferRemoveTemporary(const char *path);
typedef enum {
  ARCHIVE_DIRECTORY_COPY,
  ARCHIVE_DIRECTORY_MOVE
} ArchiveDirectoryTransferMode;

extern void ArchiveDirectoryTransfer(ViewContext *ctx, DirEntry **dir_entry_ptr,
                                     ArchiveDirectoryTransferMode mode,
                                     const char *src_path,
                                     const char *dest_dir_path,
                                     const char *dest_path);
extern int FilesystemDirectoryTransferToArchive(
    ViewContext *ctx, ArchiveDirectoryTransferMode mode, const char *src_path,
    const char *dest_dir_path, const char *dest_path);
extern DirEntry *HandleDirDeleteDirectory(ViewContext *ctx,
                                          DirEntry *dir_entry);
extern DirEntry *HandleDirRenameDirectory(ViewContext *ctx,
                                          DirEntry *dir_entry);
extern DirEntry *GetSelectedDirEntry(const ViewContext *ctx, struct Volume *vol);
extern DirEntry *GetPanelDirEntry(YtreeNovaPanel *p);
extern int GetPanelVisibleSelectionIndex(const YtreeNovaPanel *p);
extern void BuildDirEntryList(ViewContext *ctx, struct Volume *vol,
                              int *index_ptr);
extern BOOL PanelDirIsVisible(const YtreeNovaPanel *panel, const DirEntry *dir_entry);
extern int PanelFindNextVisibleDirIndex(const YtreeNovaPanel *panel, int start_idx,
                                        int direction);
extern int PanelFindFirstVisibleDirIndex(const YtreeNovaPanel *panel);
extern int PanelFindLastVisibleDirIndex(const YtreeNovaPanel *panel);
extern BOOL PanelComputeViewportPosition(const YtreeNovaPanel *panel,
                                         int target_idx, int height,
                                         int *begin_io, int *cursor_io);
extern void FreeDirEntryList(ViewContext *ctx);
extern void FreeVolumeCache(struct Volume *vol);
extern DirEntry *RefreshTreeSafe(ViewContext *ctx, YtreeNovaPanel *p,
                                 DirEntry *entry);
extern void DirOps_ReloadPanelFileAnchorIfMissing(ViewContext *ctx,
                                                  YtreeNovaPanel *panel,
                                                  DirEntry *dir_entry);
extern DirEntry *RestorePanelFileSelection(ViewContext *ctx, DirEntry *dir_entry,
                                           YtreeNovaPanel *panel);
extern void PanelTags_Clear(YtreeNovaPanel *panel);
extern void PanelTags_Copy(YtreeNovaPanel *dst, const YtreeNovaPanel *src);
extern void PanelTags_PruneUnderDir(YtreeNovaPanel *panel, DirEntry *dir_entry);
extern BOOL PanelTags_FileIsTagged(const YtreeNovaPanel *panel,
                                   FileEntry *file_entry);
extern void PanelTags_RecordFileState(YtreeNovaPanel *panel, FileEntry *file_entry,
                                      BOOL tagged);
extern void PanelTags_ApplyToTree(ViewContext *ctx, YtreeNovaPanel *panel);
extern void PanelTags_Restore(ViewContext *ctx, YtreeNovaPanel *panel);
extern BOOL DirOps_SelectVisibleDirAndRefresh(ViewContext *ctx,
                                              YtreeNovaPanel *panel,
                                              const DirEntry *target,
                                              DirEntry **dir_entry_ptr);
extern DirEntry *DirOps_FindDirEntryByPath(const ViewContext *ctx,
                                           const char *dir_path);
extern DirEntry *DirOps_ResolveCopyMoveRefreshAnchor(ViewContext *ctx,
                                                     const char *src_path,
                                                     const char *dest_dir_path,
                                                     DirEntry *fallback);

/* render_dir.c */
extern void DisplayTree(ViewContext *ctx, struct Volume *vol, WINDOW *win,
                        int start_entry_no, int hilight_no, BOOL is_active);
extern void PrintDirEntry(ViewContext *ctx, struct Volume *vol, WINDOW *win,
                          int entry_no, int y, unsigned char hilight,
                          BOOL is_active);
extern void SetDirMode(ViewContext *ctx, int new_mode);
extern void SelectDirMode(ViewContext *ctx, int selection);
extern void RotateDirMode(ViewContext *ctx);

/* display.c */
extern void ClearHelp(ViewContext *ctx);
extern void DisplayDirHelp(ViewContext *ctx, const DirEntry *dir_entry);
extern void DisplayFileHelp(ViewContext *ctx, const DirEntry *dir_entry);
extern void DisplayMenu(ViewContext *ctx);
extern void MapF2Window(ViewContext *ctx);
extern void RefreshWindow(WINDOW *win);
extern void SwitchToBigFileWindow(ViewContext *ctx);
extern void SwitchToSmallFileWindow(ViewContext *ctx);
extern void UnmapF2Window(ViewContext *ctx);
extern void DisplayHeaderPath(ViewContext *ctx, const char *path);
extern void RenderInactivePanel(ViewContext *ctx, YtreeNovaPanel *panel);
extern void RefreshView(ViewContext *ctx, DirEntry *dir_entry);
extern void DisplayPreviewHelp(ViewContext *ctx);
extern void DisplayHistoryHelp(ViewContext *ctx);
extern int UI_ShowIntegratedHelp(ViewContext *ctx, const DirEntry *dir_entry);
extern int UI_ShowHistoryHelpPopup(ViewContext *ctx);

/* display_utils.c */
extern int AddStr(char *str);
extern int BuildUserFileEntry(FileEntry *fe_ptr, int filename_width,
                              int linkname_width, BOOL tagged, char *template,
                              int linelen, char *line);
extern char *CTime(time_t f_time, char *buffer);
extern char *CutFilename(char *dest, const char *src, unsigned int max_len);
extern char *CutName(char *dest, const char *src, unsigned int max_len);
extern char *CutPathname(char *dest, const char *src, unsigned int max_len);
extern char *FormFilename(char *dest, char *src, unsigned int max_len);
extern char *GetAttributes(unsigned short mode, char *buffer);
extern void GetMaxYX(WINDOW *win, int *height, int *width);
extern int GetVisualUserFileEntryLength(int max_visual_filename_len,
                                        int max_visual_linkname_len,
                                        char *template);
extern int MvAddStr(int y, int x, char *str);
extern int MvWAddStr(WINDOW *win, int y, int x, char *str);
extern void Print(WINDOW *, int, int, char *, int);
extern void PrintLine(WINDOW *win, int y, int x, const char *line, int len);
extern void PrintMenuOptions(WINDOW *, int, int, char *, int, int);
extern void PrintOptions(WINDOW *, int, int, char *);
extern void PrintSpecialString(WINDOW *win, int y, int x, char *str, int color);
typedef enum {
  UI_COMMAND_LAYOUT_MNEMONIC,
  UI_COMMAND_LAYOUT_KEY_PREFIX,
  UI_COMMAND_LAYOUT_ALT_MNEMONIC,
  UI_COMMAND_LAYOUT_CTRL_MNEMONIC,
  UI_COMMAND_LAYOUT_LABEL_FIRST
} UICommandStripLayout;
typedef struct {
  UICommandStripLayout layout;
  const char *label;
  const char *primary_key;
  const char *secondary_key;
  const char *translation_context;
} UICommandStripCommand;
typedef enum {
  UI_HELP_POPUP_TEXT,
  UI_HELP_POPUP_COMMAND_STRIP,
  UI_HELP_POPUP_LINK_TEXT
} UIHelpPopupRowKind;
typedef enum {
  UI_HELP_POPUP_SPAN_TERM,
  UI_HELP_POPUP_SPAN_ATTENTION,
  UI_HELP_POPUP_SPAN_LINK
} UIHelpPopupSpanKind;
typedef struct {
  size_t start;
  size_t length;
  size_t link_index;
  UIHelpPopupSpanKind kind;
} UIHelpPopupSpan;
typedef struct {
  UIHelpPopupRowKind kind;
  const char *prefix;
  const char *text;
  const UICommandStripCommand *commands;
  const UIHelpPopupSpan *spans;
  size_t command_count;
  size_t span_count;
  size_t selected_link_index;
  BOOL selected;
  BOOL compact_with_previous;
} UIHelpPopupRow;
typedef struct {
  const char *canonical_label;
  const char *display_label;
} UIHelpLabelOverride;
typedef struct {
  const UICommandStripCommand *commands;
  size_t command_count;
  size_t link_command_count;
  size_t active_command_index;
  int (*key_handler)(ViewContext *, int, void *);
  int (*active_row_handler)(const void *);
  void (*viewport_handler)(void *, int, int, int);
  int initial_visible_row;
  int initial_scroll_line;
  int *final_scroll_line;
  void *key_data;
} UIHelpPopupFooterSpec;
extern int UI_CommandStripVisualLength(const UICommandStripCommand *commands,
                                       size_t command_count);
extern int UI_FormatCommandStripEntryText(const UICommandStripCommand *command,
                                          char *buf, size_t buf_size);
extern int UI_RenderCommandStripEntry(WINDOW *win, int y, int x,
                                      const UICommandStripCommand *command,
                                      int ncolor, int hcolor);
extern void UI_RenderCommandStrip(WINDOW *win, int y, int x,
                                  const UICommandStripCommand *commands,
                                  size_t command_count, int ncolor,
                                  int hcolor);
extern int UI_RenderAdaptiveCommandStrip(WINDOW *win, int y, int x,
                                         const UICommandStripCommand *commands,
                                         size_t command_count, int ncolor,
                                         int hcolor);
extern int
UI_ShowHelpPopupWithFooter(ViewContext *ctx, const char *title,
                           const UIHelpPopupRow *rows, size_t row_count,
                           const UIHelpPopupFooterSpec *footer_spec);
extern int UI_ShowHelpPopup(ViewContext *ctx, const char *title,
                            const UIHelpPopupRow *rows, size_t row_count);
extern int UI_ShowHelpPopupDismissAnyKey(ViewContext *ctx, const char *title,
                                         const UIHelpPopupRow *rows,
                                         size_t row_count);
extern int WAddStr(WINDOW *win, char *str);
extern int WAttrAddStr(WINDOW *win, int attr, char *str);

/* error.c */
extern void AboutBox(ViewContext *ctx);
extern void UnmapNoticeWindow(ViewContext *ctx);
extern void UI_Beep(ViewContext *ctx, BOOL critical);
extern void UI_ShowStatusLineError(ViewContext *ctx, const char *fmt, ...);
extern void UI_RenderStatusLineError(ViewContext *ctx);
extern void UI_ClearStatusLineError(ViewContext *ctx);
extern void UI_ShowStatusLineNotice(ViewContext *ctx, const char *fmt, ...);
extern void UI_RenderStatusLineNotice(ViewContext *ctx);
extern void UI_ClearStatusLineNotice(ViewContext *ctx);

/* filewin.c / ctrl_file.c / ctrl_file_ops.c */
extern void FreeFileEntryList(YtreeNovaPanel *panel);
extern void InvalidateVolumePanels(ViewContext *ctx, const struct Volume *vol);
extern void BuildFileEntryList(ViewContext *ctx, YtreeNovaPanel *panel);
extern void DisplayFileWindow(ViewContext *ctx, YtreeNovaPanel *panel,
                              const DirEntry *dir_entry);
extern int HandleFileWindow(ViewContext *ctx, DirEntry *dir_entry);
extern DirEntry *RefreshFileView(ViewContext *ctx, DirEntry *dir_entry);
extern void UI_RefreshSyncPanels(ViewContext *ctx, DirEntry *dir_entry);
extern void UI_RenderFilePanel(ViewContext *ctx, const DirEntry *dir_entry,
                               int start_x);
extern BOOL handle_file_window_command_action(
    ViewContext *ctx, YtreeNovaAction action, DirEntry **dir_entry_ptr,
    BOOL *need_dsp_help_ptr, BOOL *maybe_change_x_step_ptr, Statistic *s);
extern BOOL handle_file_window_misc_dispatch_action(
    ViewContext *ctx, YtreeNovaAction action, DirEntry **dir_entry_ptr,
    YtreeNovaAction *loop_action_ptr, int *unput_char_ptr,
    const int *start_x_ptr,
    BOOL *need_dsp_help_ptr, BOOL *maybe_change_x_step_ptr, Statistic *s,
    long *preview_line_offset_ptr,
    void (*update_preview)(ViewContext *, const DirEntry *));
extern BOOL handle_file_window_preview_action(
    ViewContext *ctx, YtreeNovaAction action, DirEntry **dir_entry_ptr,
    YtreeNovaAction *loop_action_ptr, Statistic **stats_ptr,
    struct Volume **start_vol_ptr, BOOL *need_dsp_help_ptr,
    long *preview_line_offset_ptr, int *saved_fixed_width_ptr,
    void (*update_preview)(ViewContext *, const DirEntry *));
extern BOOL handle_file_window_navigation_action(
    ViewContext *ctx, YtreeNovaAction action, DirEntry *dir_entry, int *start_x_ptr,
    BOOL *need_dsp_help_ptr, long *preview_line_offset_ptr,
    void (*update_preview)(ViewContext *, const DirEntry *),
    void (*list_jump)(ViewContext *, DirEntry *, char *));
extern void CapturePanelSelectionAnchor(ViewContext *ctx, YtreeNovaPanel *panel,
                                       const DirEntry *dir_entry);
extern BOOL RebindActiveFilePanelSelection(YtreeNovaPanel *panel,
                                           DirEntry **dir_entry_io);
extern BOOL handle_file_window_volume_action(ViewContext *ctx,
                                             YtreeNovaAction action,
                                             const struct Volume *start_vol,
                                             int *unput_char_ptr,
                                             BOOL *return_esc_ptr);
extern BOOL handle_tag_file_action(ViewContext *ctx, int action,
                                   DirEntry *dir_entry, int *unput_char_ptr,
                                   BOOL *need_dsp_help_ptr, int start_x,
                                   Statistic *s, BOOL *maybe_change_x_step_ptr);

/* render_file.c */
extern void SetPanelFileMode(ViewContext *ctx, YtreeNovaPanel *p,
                             int new_file_mode);
extern int GetPanelFileMode(const YtreeNovaPanel *p);
extern int ResolveCompactFileWidth(const ViewContext *ctx,
                                   const YtreeNovaPanel *panel);
extern void SelectPanelFileMode(ViewContext *ctx, YtreeNovaPanel *p,
                                int selection);
extern void RotatePanelFileMode(ViewContext *ctx, YtreeNovaPanel *p);
extern int GetPanelMaxColumn(const YtreeNovaPanel *p);
extern void SetFileRenderingMetrics(YtreeNovaPanel *p, unsigned max_filename,
                                    unsigned max_linkname,
                                    unsigned max_userview);
extern void SetRenderSortOrder(YtreeNovaPanel *p, BOOL reverse);
extern void DisplayFiles(ViewContext *ctx, YtreeNovaPanel *panel,
                         const DirEntry *de_ptr, int start_file_no,
                         int hilight_no, int start_x, WINDOW *win);
extern void PrintFileEntry(ViewContext *ctx, YtreeNovaPanel *panel, int entry_no,
                           int y, int x, unsigned char hilight, int start_x,
                           WINDOW *win);

/* fileinfo_band.c */
extern int FileInfoActionSelection(YtreeNovaAction action);
extern BOOL FileInfoHandleDirAction(ViewContext *ctx, YtreeNovaAction action,
                                    DirEntry *dir_entry, const Statistic *s);
extern BOOL FileInfoHandleFileAction(
    ViewContext *ctx, YtreeNovaAction action, DirEntry *dir_entry,
    const Statistic *s, int start_x, long *preview_line_offset_ptr,
    void (*update_preview)(ViewContext *, const DirEntry *));

/* fileinfo_git.c */
extern void FileInfoGitInvalidate(YtreeNovaPanel *panel);
extern BOOL FileInfoGitRefresh(ViewContext *ctx, YtreeNovaPanel *panel,
                               const DirEntry *dir_entry);
extern void FileInfoGitDescribe(const YtreeNovaPanel *panel,
                                const FileEntry *file_entry, char *buffer,
                                size_t buffer_size);

/* key_engine.c */
extern int Getch(ViewContext *ctx);
extern void HitReturnToContinue(void);
extern int InputChoice(ViewContext *ctx, const char *msg, const char *term);
extern int InputChoiceWithHelp(ViewContext *ctx, const char *msg,
                               const char *term,
                               int (*help_callback)(ViewContext *, void *),
                               void *help_data);
extern int InputChoiceLiteral(ViewContext *ctx, const char *msg,
                              const char *term);
extern int InputChoiceCommandStrip(ViewContext *ctx,
                                   const UICommandStripCommand *commands,
                                   size_t command_count, const char *term);
extern int UI_AskConflict(ViewContext *ctx, const char *src_path,
                          const char *dst_path, int *mode_flags);

/* input_line.c */
typedef BOOL (*UIPromptActionHandler)(ViewContext *ctx, YtreeNovaPanel *panel,
                                      int ch, const char *buffer, int max_len,
                                      int *cursor_pos, void *action_data);
typedef struct {
  const UICommandStripCommand *hints_override;
  size_t hints_override_count;
  int (*help_callback)(ViewContext *, void *);
  void *help_data;
  UIPromptActionHandler action_handler;
  void *action_data;
  BOOL suppress_final_refresh;
  BOOL cursor_at_start;
} UIPromptOptions;
extern int UI_ReadString(ViewContext *ctx, YtreeNovaPanel *panel,
                         const char *prompt, char *buffer, int max_len,
                         int history_type);
extern int UI_ReadStringWithPromptOptions(
    ViewContext *ctx, YtreeNovaPanel *panel, const char *prompt, char *buffer,
    int max_len, int history_type, const UIPromptOptions *options);
extern int UI_ReadStringWithHelp(ViewContext *ctx, YtreeNovaPanel *panel,
                                 const char *prompt, char *buffer, int max_len,
                                 int history_type,
                                 const UICommandStripCommand *hints_override,
                                 size_t hints_override_count,
                                 int (*help_callback)(ViewContext *, void *),
                                 void *help_data);
extern int UI_ShowGeneratedContextHelp(ViewContext *ctx, const char *context_id,
                                       const UIHelpPopupRow *prefix_rows,
                                       size_t prefix_row_count);
extern int UI_ShowGeneratedContextHelpWithOverrides(
    ViewContext *ctx, const char *context_id, const UIHelpPopupRow *prefix_rows,
    size_t prefix_row_count, const UIHelpLabelOverride *label_overrides,
    size_t label_override_count);
extern int UI_ShowGeneratedContextHelpCallback(ViewContext *ctx,
                                               void *help_data);

extern BOOL KeyPressed(void);
extern BOOL EscapeKeyPressed(void);
extern char *StrLeft(const char *str, size_t visible_count);
extern char *StrRight(const char *str, size_t visible_count);
extern int StrVisualLength(const char *str);
extern BOOL IsViKeysEnabled(const ViewContext *ctx);
extern int ViKey(int ch);
extern int NormalizeViKey(const ViewContext *ctx, int ch);
extern int VisualPositionToBytePosition(const char *str, int visual_pos);
extern int WGetch(ViewContext *ctx, WINDOW *win);
extern YtreeNovaAction GetKeyAction(const ViewContext *ctx, int ch);
extern int GetEventOrKey(ViewContext *ctx);

/* stats.c */
extern void DisplayAvailBytes(ViewContext *ctx, const Statistic *s);
extern void DisplayDirParameter(ViewContext *ctx, DirEntry *de);
extern void DisplayDirStatistic(ViewContext *ctx, const DirEntry *de,
                                const char *title, const Statistic *s);
extern void DisplayDirTagged(ViewContext *ctx, const DirEntry *de,
                             const Statistic *s);
extern void DisplayDiskName(ViewContext *ctx, const Statistic *s);
extern void DisplayDiskStatistic(ViewContext *ctx, const Statistic *s);
extern void DisplayDiskTagged(ViewContext *ctx, const Statistic *s);
extern void DisplayFileParameter(ViewContext *ctx, FileEntry *fe);
extern void DisplayFileStatistic(ViewContext *ctx, const FileEntry *fe,
                                 const Statistic *s);
extern void DisplayPanelStatistics(ViewContext *ctx, YtreeNovaPanel *panel);
extern void UpdateStatsPanel(ViewContext *ctx, DirEntry *dir_entry,
                             const Statistic *s);
extern void DisplayFilter(ViewContext *ctx, const Statistic *s);
extern void DisplayGlobalFileParameter(ViewContext *ctx, FileEntry *fe);
extern void RecalculateSysStats(ViewContext *ctx, Statistic *s);

typedef enum {
  DIR_WINDOW_DISPATCH_UNHANDLED = 0,
  DIR_WINDOW_DISPATCH_HANDLED,
  DIR_WINDOW_DISPATCH_CONTINUE,
  DIR_WINDOW_DISPATCH_RETURN_ESC
} DirWindowDispatchResult;

/* ctrl_dir.c / dir_tags.c */
extern void HandleShowAll(ViewContext *ctx, BOOL tagged_only, BOOL all_volumes,
                          DirEntry *dir_entry, BOOL *need_dsp_help, int *ch,
                          YtreeNovaPanel *p);
extern BOOL HandleDirTagActions(ViewContext *ctx, int action,
                                DirEntry **dir_entry_ptr, BOOL *need_dsp_help,
                                int *ch);
extern void SyncActivePanelWindows(ViewContext *ctx);
extern DirEntry *ResolveActiveDirEntry(ViewContext *ctx, const Statistic *s);
extern void RefreshVolumeSwitchViews(ViewContext *ctx, DirEntry *dir_entry,
                                     const Statistic *s);
extern DirWindowDispatchResult
HandleDirWindowPanelAction(ViewContext *ctx, YtreeNovaAction action,
                           DirEntry **dir_entry_ptr, Statistic **s_ptr,
                           const struct Volume **start_vol_ptr,
                           BOOL *need_dsp_help_ptr, int *ch_ptr,
                           const int *unput_char_ptr);
extern DirWindowDispatchResult
HandleDirWindowEnterAction(ViewContext *ctx, DirEntry **dir_entry_ptr,
                           Statistic **s_ptr,
                           const struct Volume **start_vol_ptr,
                           BOOL *need_dsp_help_ptr, int *ch_ptr,
                           const int *unput_char_ptr, YtreeNovaAction *action_ptr);
extern DirWindowDispatchResult
HandleDirWindowVolumeAction(ViewContext *ctx, YtreeNovaAction action,
                            DirEntry **dir_entry_ptr, Statistic **s_ptr,
                            const struct Volume *start_vol,
                            BOOL *need_dsp_help_ptr);
extern DirWindowDispatchResult
HandleDirWindowLogAction(ViewContext *ctx, DirEntry **dir_entry_ptr,
                         Statistic **s_ptr, const struct Volume *start_vol,
                         BOOL *need_dsp_help_ptr, char *new_log_path,
                         size_t new_log_path_size);

/* ui_edit_config.c */
extern BOOL ParseSmallWindowSkipValue(const char *value);
extern void UI_OpenConfigProfile(ViewContext *ctx, DirEntry *dir_entry);
extern void UI_EditCommandsCatalog(ViewContext *ctx, DirEntry *dir_entry);
extern void UI_EditApplicationsCatalog(ViewContext *ctx, DirEntry *dir_entry);

/* dir_nav.c */
extern void DirNav_Movedown(ViewContext *ctx, DirEntry **dir_entry,
                            YtreeNovaPanel *p);
extern void DirNav_Moveup(ViewContext *ctx, DirEntry **dir_entry,
                          YtreeNovaPanel *p);
extern void DirNav_Movenpage(ViewContext *ctx, DirEntry **dir_entry,
                             YtreeNovaPanel *p);
extern void DirNav_Moveppage(ViewContext *ctx, DirEntry **dir_entry,
                             YtreeNovaPanel *p);
extern void DirNav_MoveEnd(ViewContext *ctx, DirEntry **dir_entry,
                           YtreeNovaPanel *p);
extern void DirNav_MoveHome(ViewContext *ctx, DirEntry **dir_entry,
                            YtreeNovaPanel *p);
extern const DirEntry *DirNav_FindVisibleSibling(const YtreeNovaPanel *panel,
                                                 const DirEntry *dir_entry,
                                                 int direction);

/* file_nav.c */
extern void FileNav_MoveDown(ViewContext *ctx, DirEntry *dir_entry,
                             int start_x);
extern void FileNav_MoveUp(ViewContext *ctx, DirEntry *dir_entry, int start_x);
extern void FileNav_MoveRight(ViewContext *ctx, DirEntry *dir_entry,
                              int *start_x);
extern void FileNav_MoveLeft(ViewContext *ctx, DirEntry *dir_entry,
                             int *start_x);
extern void FileNav_PageDown(ViewContext *ctx, DirEntry *dir_entry,
                             int start_x);
extern void FileNav_PageUp(ViewContext *ctx, DirEntry *dir_entry, int start_x);
extern void FileNav_RereadWindowSize(ViewContext *ctx, DirEntry *dir_entry);
extern void FileNav_SyncGridMetrics(ViewContext *ctx);
extern int FileNav_GetMaxDispFiles(const ViewContext *ctx);
extern int FileNav_GetXStep(const ViewContext *ctx);
extern void FileNav_UpdateHeaderPath(ViewContext *ctx, DirEntry *dir_entry);

/* ui_nav.c */
extern void Nav_MoveDown(int *cursor, int *offset, int total_items,
                         int page_height, int step);
extern void Nav_MoveUp(int *cursor, int *offset);
extern void Nav_PageDown(int *cursor, int *offset, int total_items,
                         int page_height);
extern void Nav_PageUp(int *cursor, int *offset, int page_height);
extern void Nav_Home(int *cursor, int *offset);
extern void Nav_End(int *cursor, int *offset, int total_items, int page_height);

/* vol_menu.c */
extern int SelectLoadedVolume(ViewContext *ctx, int *return_key);

/* application_menu.c */
extern int UI_OpenApplicationsMenu(ViewContext *ctx);

/* view_internal.c */
extern int InternalView(ViewContext *ctx, char *file_path,
                        const ViewerGeometry *geom);

/* view_preview.c */
extern void RenderFilePreview(ViewContext *ctx, WINDOW *win, char *filename,
                              long *line_offset_ptr, int col_offset);
extern void RenderArchivePreview(ViewContext *ctx, WINDOW *win,
                                 const char *archive_path,
                                 const char *internal_path,
                                 long *line_offset_ptr);

/* quit.c */
extern void Quit(ViewContext *ctx);
extern void QuitTo(ViewContext *ctx, DirEntry *dir_entry);

/* interactions.c */
/* Date-change scopes (only fields POSIX allows us to set). */
#define DATE_SCOPE_ACCESS 1
#define DATE_SCOPE_MODIFY 2

extern int UI_PromptAttributeAction(ViewContext *ctx, BOOL tagged,
                                    BOOL allow_date);
extern int UI_ParseModeInput(const char *input, char *out_mode,
                             char *preview_mode);
extern int UI_GetDateChangeSpec(ViewContext *ctx, time_t *new_time,
                                int *scope_mask);
extern int ChangeFileModus(ViewContext *ctx, FileEntry *fe_ptr);
extern int ChangeDirModus(ViewContext *ctx, DirEntry *de_ptr);
extern int ChangeFileDate(ViewContext *ctx, FileEntry *fe_ptr);
extern int ChangeDirDate(ViewContext *ctx, DirEntry *de_ptr);
extern int HandleDirOwnership(ViewContext *ctx, DirEntry *de_ptr,
                              BOOL change_owner, unsigned char change_group);
extern int HandleFileOwnership(ViewContext *ctx, FileEntry *fe_ptr,
                               BOOL change_owner, unsigned char change_group);
extern int GetNewOwner(ViewContext *ctx, int st_uid);
extern int GetNewGroup(ViewContext *ctx, int st_gid);
extern int ChangeFileOrDirOwnership(ViewContext *ctx, const char *path,
                                    struct stat *stat_buf, BOOL change_owner,
                                    BOOL change_group);
extern int UI_ArchiveCallback(int status, const char *msg, void *user_data);
extern int recursive_mkdir(char *path);
extern int recursive_rmdir(const char *path);
extern int UI_BuildArchivePayloadFromPaths(const char *const *source_paths,
                                           size_t source_count,
                                           BOOL recursive_directories,
                                           ArchivePayload *payload);
extern int UI_GatherArchivePayload(ViewContext *ctx, DirEntry *selected_dir,
                                   FileEntry *selected_file,
                                   ArchivePayload *payload);
extern void UI_FreeArchivePayload(ArchivePayload *payload);
extern int UI_CreateArchiveFromPayload(ViewContext *ctx,
                                       const ArchivePayload *payload);
extern int UI_BuildFileCompareRequest(ViewContext *ctx, FileEntry *source_file,
                                      CompareRequest *request);
extern int UI_BuildDirectoryCompareRequest(ViewContext *ctx,
                                           DirEntry *source_dir,
                                           CompareRequest *request,
                                           BOOL *launch_external);
extern const char *UI_CompareFlowTypeName(CompareFlowType flow_type);
extern const char *UI_CompareBasisName(CompareBasis basis);
extern const char *UI_CompareTagResultName(CompareTagResult tag_result);
extern const char *UI_GetCompareHelperCommand(const ViewContext *ctx,
                                              CompareFlowType flow_type);
extern int UI_ConflictResolverWrapper(ViewContext *ctx, const char *src_path,
                                      const char *dst_path, int *mode_flags);
extern void DirCompare_RunInternalDirectory(ViewContext *ctx,
                                            DirEntry *source_dir,
                                            const CompareRequest *request);
extern void DirCompare_RunInternalLoggedTree(ViewContext *ctx,
                                             const CompareRequest *request);
extern void DirCompare_LaunchExternal(ViewContext *ctx,
                                      const CompareRequest *request);

/* file_compare.c */
extern void FileCompare_LaunchExternal(ViewContext *ctx,
                                       FileEntry *source_file);

/* file_tags.c */
extern void FileTags_WalkTaggedFiles(ViewContext *ctx, int start_file,
                                     int cursor_pos,
                                     int (*fkt)(ViewContext *, FileEntry *,
                                                WalkingPackage *),
                                     WalkingPackage *walking_package);
extern BOOL FileTags_IsMatchingTaggedFiles(ViewContext *ctx);
extern int FileTags_UI_DeleteTaggedFiles(ViewContext *ctx, int max_disp_files,
                                         Statistic *s);
extern void FileTags_SilentWalkTaggedFiles(
    ViewContext *ctx,
    int (*fkt)(ViewContext *, FileEntry *, WalkingPackage *, Statistic *),
    WalkingPackage *walking_package);
extern void FileTags_SilentTagWalkTaggedFiles(
    ViewContext *ctx,
    int (*fkt)(ViewContext *, FileEntry *, WalkingPackage *, Statistic *),
    WalkingPackage *walking_package);
extern void FileTags_HandleInvertTags(ViewContext *ctx, DirEntry *dir_entry,
                                      Statistic *s);

/* file_list.c */
extern void FileList_RemoveFileEntry(ViewContext *ctx, int entry_no);
extern void FileList_ChangeFileEntry(ViewContext *ctx);

/* progress.c */
extern void Progress_Start(ViewContext *ctx, const char *operation,
                           const char *source_path, const char *dest_path,
                           long long bytes_total, unsigned int items_total);
extern BOOL Progress_ShouldRender(ViewContext *ctx);
extern BOOL Progress_Update(ViewContext *ctx, long long bytes_done,
                            unsigned int items_done);
extern void Progress_Finish(ViewContext *ctx);
extern void Progress_Render(ViewContext *ctx);

#endif /* YTNOVA_UI_H */
