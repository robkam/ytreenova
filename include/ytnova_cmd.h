/***************************************************************************
 *
 * ytnova_cmd.h
 * User commands and action handler prototypes
 *
 ***************************************************************************/
#ifndef YTNOVA_CMD_H
#define YTNOVA_CMD_H

#include "ytnova_defs.h"

/* Conflict Resolution Codes */
#define CONFLICT_SKIP 0
#define CONFLICT_OVERWRITE 1
#define CONFLICT_ALL 2
#define CONFLICT_ABORT -1

/* Callback Type Definition */
typedef int (*ConflictCallback)(ViewContext *ctx, const char *src_path,
                                const char *dst_path, int *mode_flags);
typedef int (*ChoiceCallback)(ViewContext *ctx, const char *prompt,
                              const char *choices);

/* attributes.c */
extern int ChangeOwnership(const char *path, uid_t new_uid, gid_t new_gid,
                           struct stat *stat_buf);
extern int ChangeDirGroup(DirEntry *de_ptr);
extern int ChangeFileGroup(ViewContext *ctx, FileEntry *fe_ptr);
extern int SetFileGroup(ViewContext *ctx, FileEntry *fe_ptr,
                        WalkingPackage *walking_package);
extern int SetFileModus(ViewContext *ctx, FileEntry *fe_ptr,
                        WalkingPackage *walking_package);
extern int SetDirModus(DirEntry *de_ptr, WalkingPackage *walking_package);
extern int GetMode(const char *modus);
extern int ChangeDirOwner(DirEntry *de_ptr);
extern int ChangeFileOwner(ViewContext *ctx, FileEntry *fe_ptr);
extern int SetFileOwner(ViewContext *ctx, FileEntry *fe_ptr,
                        WalkingPackage *walking_package);

/* copy.c */
extern int CopyFile(ViewContext *ctx, Statistic *statistic_ptr,
                    FileEntry *fe_ptr, char *to_file, DirEntry *dest_dir_entry,
                    const char *to_dir_path, BOOL path_copy,
                    int *dir_create_mode,
                    int *overwrite_mode, ConflictCallback cb,
                    ChoiceCallback choice_cb);
extern int CopyTaggedFiles(ViewContext *ctx, FileEntry *fe_ptr,
                           WalkingPackage *walking_package);
extern int GetCopyParameter(ViewContext *ctx, const char *from_file,
                            BOOL path_copy, char *to_file, char *to_dir);
extern int GetDestinationDirectoryParameter(ViewContext *ctx, char *to_dir);
extern int ResolveDestinationDirectoryPath(DirEntry *current_dir_entry,
                                           const char *dir_path,
                                           char *resolved_path);
extern int UI_EnsureCopyMoveDestinationDirectory(ViewContext *ctx,
                                                 char *dir_path, DirEntry *tree,
                                                 DirEntry **result_ptr,
                                                 int *auto_create);
extern int CopyFileContent(ViewContext *ctx, char *to_path, char *from_path,
                           const Statistic *s);

/* delete.c */
extern int DeleteFile(ViewContext *ctx, FileEntry *fe_ptr, int *auto_override,
                      Statistic *s, ChoiceCallback choice_cb);
extern int DeleteTaggedFiles(ViewContext *ctx, FileEntry *fe_ptr,
                             WalkingPackage *walking_package);
extern int RemoveFile(ViewContext *ctx, FileEntry *fe_ptr, Statistic *s);

/* edit.c */
extern int Edit(ViewContext *ctx, DirEntry *dir_entry, char *file_path);

/* execute.c */
extern int Execute(ViewContext *ctx, DirEntry *dir_entry,
                   const FileEntry *file_entry, const char *cmd_template,
                   Statistic *s, ArchiveProgressCallback cb);
extern int ExecuteCommand(ViewContext *ctx, FileEntry *fe_ptr,
                          WalkingPackage *walking_package, Statistic *s);

/* filter.c */
extern int UI_ReadFilter(ViewContext *ctx);
extern int SetFilter(const char *filter_spec, Statistic *s);
extern void ApplyFilter(DirEntry *dir_entry, const Statistic *s);
extern BOOL Match(FileEntry *fe, const Statistic *s);

/* log.c */
extern int Log(DirEntry *dir_entry, Statistic *s);
extern int SetLogFile(char *filename);
extern int LogDisk(ViewContext *ctx, YtreeNovaPanel *panel, char *path);
extern int CycleLoadedVolume(ViewContext *ctx, YtreeNovaPanel *panel,
                             int direction);
extern int GetNewLogPath(ViewContext *ctx, YtreeNovaPanel *panel, char *path);

/* mkdir.c */
extern int MakeDirectory(const ViewContext *ctx, YtreeNovaPanel *panel,
                         DirEntry *father_dir_entry, const char *dir_name,
                         Statistic *s);
extern int MakePath(const ViewContext *ctx, DirEntry *tree, char *dir_path,
                    DirEntry **dest_dir_entry);
extern int EnsureDirectoryExists(ViewContext *ctx, char *dir_path,
                                 DirEntry *tree, BOOL *created,
                                 DirEntry **result_ptr, int *auto_create,
                                 ChoiceCallback choice_cb);

/* mkfile.c */
extern int MakeFile(ViewContext *ctx, DirEntry *dir_entry, const char *name,
                    Statistic *s, int *overwrite_mode,
                    ChoiceCallback choice_cb);

/* move.c */
extern int GetMoveParameter(ViewContext *ctx, const char *from_file,
                            char *to_file, char *to_dir);
extern int MoveFile(ViewContext *ctx, FileEntry *fe_ptr, const char *to_file,
                    DirEntry *dest_dir_entry, const char *to_dir_path,
                    FileEntry **new_fe_ptr, int *dir_create_mode,
                    int *overwrite_mode, ConflictCallback cb,
                    ChoiceCallback choice_cb);
extern int MoveTaggedFiles(ViewContext *ctx, FileEntry *fe_ptr,
                           WalkingPackage *walking_package);

/* passwd.c */
extern int Passwd(void);

/* pipe.c */
extern int Pipe(ViewContext *ctx, DirEntry *dir_entry, FileEntry *file_entry,
                char *pipe_command);
extern int PipeDirectory(ViewContext *ctx, DirEntry *dir_entry,
                         char *pipe_command);
extern int PipeTaggedFiles(ViewContext *ctx, FileEntry *fe_ptr,
                           WalkingPackage *walking_package, Statistic *s);

/* print_ops.c */
typedef enum {
  PRINT_WRITE_OK = 0,
  PRINT_WRITE_NO_DESTINATION,
  PRINT_WRITE_IO_ERROR,
  PRINT_WRITE_OPEN_FAILED
} PrintWriteStatus;
extern PrintWriteStatus Cmd_WritePrintOutput(ViewContext *ctx,
                                             DirEntry *dir_entry, BOOL tagged,
                                             PrintConfig *config, int *is_pipe,
                                             char *error_target);
extern void UI_HandlePrint(ViewContext *ctx, DirEntry *dir_entry, BOOL tagged);
extern void UI_HandlePrintController(ViewContext *ctx, DirEntry *dir_entry,
                                     BOOL tagged);

/* profile.c */
typedef struct _profile_runtime_snapshot ProfileRuntimeSnapshot;
extern ProfileRuntimeSnapshot *ProfileRuntimeSnapshot_Create(ViewContext *ctx);
extern void ProfileRuntimeSnapshot_Restore(ViewContext *ctx,
                                           ProfileRuntimeSnapshot *snapshot);
extern void ProfileRuntimeSnapshot_Free(ProfileRuntimeSnapshot *snapshot);
extern void SetProfileValue(const ViewContext *ctx, char *name,
                            const char *value);
extern char *GetProfileValue(const ViewContext *ctx, const char *name);
extern char *GetUserFileAction(const ViewContext *ctx, int chkey,
                               int *pchremap);
extern char *GetUserDirAction(const ViewContext *ctx, int chkey, int *pchremap);
extern BOOL IsUserActionDefined(const ViewContext *ctx);
extern int ValidateProfileFile(ViewContext *ctx, const char *filename);
extern int ReadProfile(ViewContext *ctx, const char *filename);
extern int WriteProfileFromRuntimeState(ViewContext *ctx, const char *filename);
extern int CreateProfileFromRuntimeState(ViewContext *ctx, const char *filename);
extern void FreeProfileRuntimeData(ViewContext *ctx);
extern int Profile_SetDirUserAction(ViewContext *ctx, int chkey, int chremap,
                                    const char *cmd);
extern int Profile_SetArchiveDirUserAction(ViewContext *ctx, int chkey,
                                           int chremap, const char *cmd);
extern int Profile_SetFileUserAction(ViewContext *ctx, int chkey, int chremap,
                                     const char *cmd);
extern int Profile_SetArchiveFileUserAction(ViewContext *ctx, int chkey,
                                            int chremap, const char *cmd);
extern int Profile_SetCommandSurfaceUserAction(ViewContext *ctx,
                                               const char *context, int chkey,
                                               int chremap, const char *cmd);
extern void Profile_ClearCommandRuntime(ViewContext *ctx);
extern int ResolveCommandBindingKeyForContext(const ViewContext *ctx,
                                              const char *context,
                                              int default_key);
extern int ResolveUserActionBindingKey(const ViewContext *ctx, BOOL is_dir,
                                       int default_key);
extern int ValidateCommandsFile(const char *filename);
extern int ReadCommandsFile(ViewContext *ctx, const char *filename);
extern int LoadConfiguredCommands(ViewContext *ctx);
extern int CommandActionDefaultKeyCode(const char *context,
                                       const char *action_id);
extern int CommandKeyCodeToToken(int key_code, char *token, size_t token_size);
#ifdef COLOR_SUPPORT
extern int ReadThemeFile(ViewContext *ctx, const char *filename,
                         const char *theme_name);
#else
extern int ReadThemeFile(const ViewContext *ctx, const char *filename,
                         const char *theme_name);
#endif
extern int LoadConfiguredTheme(ViewContext *ctx);
extern int LoadStartupTheme(ViewContext *ctx);

/* history.c */
extern int ReadHistory(ViewContext *ctx, const char *filename);
extern int SaveHistory(ViewContext *ctx, const char *filename);
extern void InsHistory(ViewContext *ctx, const char *new_hst, int type);
extern void BuildHistoryViewList(ViewContext *ctx, int type);
extern char *PrepareCompletionMatches(ViewContext *ctx, char *base,
                                      int *show_dialog);
extern char *GetHistory(ViewContext *ctx, int type);
extern char *GetMatches(ViewContext *ctx, char *base);

/* passwd.c */
extern char *GetPasswdName(unsigned int uid);
extern char *GetDisplayPasswdName(unsigned int uid);
extern int GetPasswdUid(char *name);

/* group.c */
extern char *GetGroupName(unsigned int gid);
extern char *GetDisplayGroupName(unsigned int gid);
extern int GetGroupId(char *name);

/* rmdir.c */
extern int RemoveDirectory(ViewContext *ctx, DirEntry *dir_entry, Statistic *s);

/* rename.c */
extern int RenameFile(ViewContext *ctx, FileEntry *fe_ptr, const char *new_name,
                      FileEntry **new_fe_ptr);
extern int RenameDirectory(ViewContext *ctx, DirEntry *de_ptr,
                           const char *new_name);
extern int RenameTaggedFiles(ViewContext *ctx, FileEntry *fe_ptr,
                             WalkingPackage *walking_package);
extern int GetRenameParameter(ViewContext *ctx, const char *old_name,
                              char *new_name);
extern int GetPipeCommand(ViewContext *ctx, char *pipe_command);

/* sort.c */
extern void UI_HandleSort(ViewContext *ctx, DirEntry *dir_entry, Statistic *s,
                          int start_x);
extern void UI_SetKindOfSort(int kind_of_sort, Statistic *s);

/* system.c */
extern int QuerySystemCall(ViewContext *ctx, const char *command_line,
                           Statistic *s);
extern int LaunchDetachedCommand(ViewContext *ctx, const char *command_line,
                                 const char *working_directory, Statistic *s);
extern int SilentSystemCall(ViewContext *ctx, const char *command_line,
                            Statistic *s);
extern int SilentSystemCallEx(ViewContext *ctx, const char *command_line,
                              BOOL enable_clock, Statistic *s);
extern int SystemCall(ViewContext *ctx, const char *command_line, Statistic *s);

/* usermode.c */
extern int DirUserMode(ViewContext *ctx, DirEntry *dir_entry, int ch,
                       Statistic *s);
extern int FileUserMode(ViewContext *ctx, FileEntryList *file_entry_list,
                        int ch, Statistic *s);

/* view.c */
extern int InternalView(ViewContext *ctx, char *file_path,
                        const ViewerGeometry *geometry);
extern int View(ViewContext *ctx, DirEntry *dir_entry, char *file_path);
extern int ViewHex(ViewContext *ctx, char *file_path);
extern int UI_ViewTaggedFiles(ViewContext *ctx, DirEntry *dir_entry);

/* Added for utility decoupling integration */
extern int DeleteDirectory(ViewContext *ctx, DirEntry *dir_entry,
                           ChoiceCallback choice_cb);
extern int UI_ChoiceResolver(ViewContext *ctx, const char *prompt,
                             const char *choices);
extern int UI_ArchiveCallback(int status, const char *msg, void *user_data);

extern BOOL Progress_ShouldRender(ViewContext *ctx);
extern void RefreshView(ViewContext *ctx, DirEntry *dir_entry);
extern int GetCommandLine(ViewContext *ctx, char *command_line);
extern int GetTaggedCommandLine(ViewContext *ctx, char *command_line);
extern int GetSearchCommandLine(ViewContext *ctx, char *command_line,
                                char *search_pattern);

#endif /* YTNOVA_CMD_H */
