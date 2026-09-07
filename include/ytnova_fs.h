/***************************************************************************
 *
 * ytnova_fs.h
 * Filesystem interaction and archive handling prototypes
 *
 ***************************************************************************/
#ifndef YTNOVA_FS_H
#define YTNOVA_FS_H

#include "ytnova_defs.h"

#define UNSUPPORTED_FORMAT_ERROR -2

#define ARCHIVE_CAP_BROWSE (1U << 0)
#define ARCHIVE_CAP_COPY_OUT (1U << 1)
#define ARCHIVE_CAP_ADD (1U << 2)
#define ARCHIVE_CAP_DELETE (1U << 3)
#define ARCHIVE_CAP_RENAME (1U << 4)
#define ARCHIVE_CAP_MOVE (1U << 5)

/* Define the progress callback type for Scans */
typedef void (*ScanProgressCallback)(ViewContext *ctx, void *user_data);

/* volume.c */
extern struct Volume *Volume_Create(ViewContext *ctx);
extern void Volume_Delete(ViewContext *ctx, struct Volume *vol);
extern void Volume_FreeAll(ViewContext *ctx);
extern struct Volume *Volume_GetByPath(ViewContext *ctx, const char *path);
extern struct Volume *Volume_Load(ViewContext *ctx, const char *path,
                                  struct Volume *reuse_vol,
                                  ScanProgressCallback cb, void *cb_user_data);
extern void SetKindOfSort(int kind_of_sort, Statistic *s);

/* path_utils.c */
extern const char *GetExtension(const char *filename);
extern char *GetFileNamePath(FileEntry *file_entry, char *buffer);
extern char *GetPath(DirEntry *dir_entry, char *buffer);
extern char *GetRealFileNamePath(FileEntry *file_entry, char *buffer,
                                 int view_mode);
extern void Fnsplit(char *path, char *dir, char *name);
/* On normalization failure (e.g. component stack overflow), out_path is set to
 * an empty string and errno is set.
 */
extern void NormPath(char *in_path, char *out_path);
extern int Path_Join(char *dest, size_t size, const char *dir,
                     const char *leaf);
extern const char *Path_LeafName(const char *path);
extern BOOL Path_ShellQuote(const char *src, char *dst, size_t dst_size);
extern BOOL Path_CommandInit(char *command_line, size_t command_size,
                             size_t *command_len, const char *prefix);
extern BOOL Path_CommandAppendLiteral(char *command_line, size_t command_size,
                                      size_t *command_len,
                                      const char *literal);
extern BOOL Path_CommandAppendQuotedArg(char *command_line, size_t command_size,
                                        size_t *command_len, const char *arg);
extern BOOL Path_BuildUserActionCommand(const char *command_template,
                                        const char *path, char *command_line,
                                        size_t command_size);
extern int Path_BuildCommandLine(const char *command_template,
                                 const char *append_path, const char *token1,
                                 const char *value1, const char *token2,
                                 const char *value2, char *command_line,
                                 size_t command_line_size);
extern int Path_BuildCompareCommandLine(const char *command_template,
                                        const char *source_path,
                                        const char *target_path,
                                        char *command_line,
                                        size_t command_line_size);
extern BOOL Path_BuildTempTemplate(char *dest, size_t size,
                                   const char *name_prefix);
extern BOOL Path_CreateTempFile(char *dest, size_t size,
                                const char *name_prefix, BOOL unlink_after_open,
                                int *fd_out);

/* archive.c */
extern int Archive_ValidateInternalPath(const char *path, char *canonical_path,
                                        size_t canonical_size);
extern unsigned int Archive_ProbeCapabilities(const char *archive_path);
extern int ExtractArchiveEntry(const char *archive_path, const char *entry_path,
                               int out_fd, ArchiveProgressCallback cb,
                               void *user_data);
extern int ExtractArchiveNode(const char *archive_path, const char *entry_path,
                              const char *dest_path, ArchiveProgressCallback cb,
                              void *user_data);
extern int InsertArchiveFileEntry(ViewContext *ctx, DirEntry *tree, char *path,
                                  const struct stat *stat, Statistic *s);
extern int TryInsertArchiveDirEntry(ViewContext *ctx, DirEntry *tree, const char *dir,
                                    const struct stat *stat, Statistic *s);
extern void MinimizeArchiveTree(DirEntry **tree_ptr, Statistic *s);

#ifdef HAVE_LIBARCHIVE
extern int Archive_Rewrite(char *archive_path, RewriteCallback rw_cb,
                           void *rw_data, ArchiveProgressCallback cb,
                           void *user_data);
extern int Archive_CreateFromPaths(const char *dest_path,
                                   const char *const *source_paths,
                                   const char *const *archive_paths,
                                   size_t source_count);
extern int Archive_DeleteEntry(char *archive_path, char *file_path,
                               ArchiveProgressCallback cb, void *user_data);
extern int Archive_AddFile(char *archive_path, const char *src_path,
                           const char *dest_name,
                           BOOL is_dir, ArchiveProgressCallback cb,
                           void *user_data);
extern int Archive_RenameEntry(char *archive_path, char *old_path,
                               const char *new_name, ArchiveProgressCallback cb,
                               void *user_data);
extern int Archive_DeleteTree(char *archive_path, const char *tree_path,
                              ArchiveProgressCallback cb, void *user_data);
extern int Archive_AddTree(char *archive_path, const char *src_path,
                           const char *dest_path, ArchiveProgressCallback cb,
                           void *user_data);
extern int ExtractArchiveTree(const char *archive_path, const char *tree_path,
                              const char *dest_path, ArchiveProgressCallback cb,
                              void *user_data);
#endif

/* readarchive.c */
extern int ReadTreeFromArchive(ViewContext *ctx, DirEntry **dir_entry_ptr,
                               const char *filename, Statistic *s,
                               ScanProgressCallback cb, void *cb_data);

/* freesp.c */
extern int GetAvailBytes(long long *avail_bytes, Statistic *s);
extern int GetDiskParameter(char *path, char *volume_name,
                            long long *avail_bytes, long long *capacity,
                            Statistic *s);

/* readtree.c */
extern int ReadTree(ViewContext *ctx, DirEntry *dir_entry, char *path,
                    int depth, Statistic *s, ScanProgressCallback cb,
                    void *cb_data);
extern void UnReadTree(ViewContext *ctx, DirEntry *dir_entry, Statistic *s);
extern int RescanDir(ViewContext *ctx, DirEntry *dir_entry, int depth,
                     Statistic *s, ScanProgressCallback cb, void *cb_data);

/* path_utils.c */
extern char *GetPath(DirEntry *dir_entry, char *buffer);
extern void Fnsplit(char *path, char *dir, char *name);

/* tree_utils.c */
extern void DeleteTree(DirEntry *tree);
extern int GetDirEntry(const ViewContext *ctx, DirEntry *tree,
                       DirEntry *current_dir_entry, const char *dir_path,
                       DirEntry **dir_entry, char *to_path);
extern int GetFileEntry(DirEntry *de_ptr, const char *file_name,
                        FileEntry **file_entry);
extern void SaveTreeState(DirEntry *root, PathList **expanded,
                          PathList **tagged);
extern void RestoreTreeState(ViewContext *ctx, DirEntry *root,
                             PathList **expanded, PathList *tagged,
                             Statistic *s);
void ApplyFilterToTree(DirEntry *dir_entry, Statistic *s);
extern void FreePathList(PathList *list);

/* filter_core.c */
extern BOOL FsMatchFilter(FileEntry *fe, const Statistic *s);
extern void FsApplyFilter(DirEntry *dir_entry, const Statistic *s);

#endif /* YTNOVA_FS_H */
