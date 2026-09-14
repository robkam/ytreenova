/***************************************************************************
 *
 * src/core/sort.c
 * File-entry sorting comparators and dispatch
 *
 ***************************************************************************/

#define _GNU_SOURCE
#include "sort.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
  BOOL ascending;
  BOOL case_sensitive;
} SortContext;

extern const char *GetExtension(const char *filename);

/* Forward declarations */
static int SortByName(const void *p1, const void *p2, void *arg);
static int SortByChgTime(const void *p1, const void *p2, void *arg);
static int SortByAccTime(const void *p1, const void *p2, void *arg);
static int SortByModTime(const void *p1, const void *p2, void *arg);
static int SortBySize(const void *p1, const void *p2, void *arg);
static const char *LookupOwnerSortName(uid_t uid, char *fallback,
                                       size_t fallback_size);
static const char *LookupGroupSortName(gid_t gid, char *fallback,
                                       size_t fallback_size);
static int SortByOwner(const void *p1, const void *p2, void *arg);
static int SortByGroup(const void *p1, const void *p2, void *arg);
static int SortByExtension(const void *p1, const void *p2, void *arg);

void Panel_Sort(YtreeNovaPanel *panel, int method) {
  int aux;
  int (*compare)(const void *, const void *, void *);
  SortContext sctx;

  if (!panel || !panel->file_entry_list || panel->file_count == 0)
    return;

  sctx.ascending = (method < SORT_DSC);
  sctx.case_sensitive = FALSE;

  aux = method;
  if (aux > SORT_DSC) {
    aux -= SORT_DSC;
  } else {
    aux -= SORT_ASC;
  }

  switch (aux) {
  case SORT_BY_NAME:
    compare = SortByName;
    break;
  case SORT_BY_MOD_TIME:
    compare = SortByModTime;
    break;
  case SORT_BY_CHG_TIME:
    compare = SortByChgTime;
    break;
  case SORT_BY_ACC_TIME:
    compare = SortByAccTime;
    break;
  case SORT_BY_OWNER:
    compare = SortByOwner;
    break;
  case SORT_BY_GROUP:
    compare = SortByGroup;
    break;
  case SORT_BY_SIZE:
    compare = SortBySize;
    break;
  case SORT_BY_EXTENSION:
    compare = SortByExtension;
    break;
  default:
    compare = SortByName;
  }

  qsort_r((void *)panel->file_entry_list, panel->file_count,
          sizeof(panel->file_entry_list[0]), compare, &sctx);
}

static int SortByName(const void *p1, const void *p2, void *arg) {
  const FileEntryList *e1 = p1;
  const FileEntryList *e2 = p2;
  const SortContext *sctx = arg;

  if (sctx->case_sensitive)
    if (sctx->ascending)
      return (strcmp(e1->file->name, e2->file->name));
    else
      return (-(strcmp(e1->file->name, e2->file->name)));
  else if (sctx->ascending)
    return (strcasecmp(e1->file->name, e2->file->name));
  else
    return (-(strcasecmp(e1->file->name, e2->file->name)));
}

static int SortByExtension(const void *p1, const void *p2, void *arg) {
  const FileEntryList *e1 = p1;
  const FileEntryList *e2 = p2;
  const SortContext *sctx = arg;
  char *ext1, *ext2;
  int cmp, casecmp;

  ext1 = GetExtension(e1->file->name);
  ext2 = GetExtension(e2->file->name);
  cmp = strcmp(ext1, ext2);
  casecmp = strcasecmp(ext1, ext2);

  if (sctx->case_sensitive && !cmp)
    return SortByName(e1, e2, (void *)sctx);
  if (!sctx->case_sensitive && !casecmp)
    return SortByName(e1, e2, (void *)sctx);

  if (sctx->case_sensitive)
    if (sctx->ascending)
      return (strcmp(ext1, ext2));
    else
      return (-(strcmp(ext1, ext2)));
  else if (sctx->ascending)
    return (strcasecmp(ext1, ext2));
  else
    return (-(strcasecmp(ext1, ext2)));
}

static int SortByModTime(const void *p1, const void *p2, void *arg) {
  const FileEntryList *e1 = p1;
  const FileEntryList *e2 = p2;
  const SortContext *sctx = arg;

  if (sctx->ascending)
    return (e1->file->stat_struct.st_mtime - e2->file->stat_struct.st_mtime);
  else
    return (-(e1->file->stat_struct.st_mtime - e2->file->stat_struct.st_mtime));
}

static int SortByChgTime(const void *p1, const void *p2, void *arg) {
  const FileEntryList *e1 = p1;
  const FileEntryList *e2 = p2;
  const SortContext *sctx = arg;

  if (sctx->ascending)
    return (e1->file->stat_struct.st_ctime - e2->file->stat_struct.st_ctime);
  else
    return (-(e1->file->stat_struct.st_ctime - e2->file->stat_struct.st_ctime));
}

static int SortByAccTime(const void *p1, const void *p2, void *arg) {
  const FileEntryList *e1 = p1;
  const FileEntryList *e2 = p2;
  const SortContext *sctx = arg;

  if (sctx->ascending)
    return (e1->file->stat_struct.st_atime - e2->file->stat_struct.st_atime);
  else
    return (-(e1->file->stat_struct.st_atime - e2->file->stat_struct.st_atime));
}

static int SortBySize(const void *p1, const void *p2, void *arg) {
  const FileEntryList *e1 = p1;
  const FileEntryList *e2 = p2;
  const SortContext *sctx = arg;
  int result = 0;

  if (e1->file->stat_struct.st_size < e2->file->stat_struct.st_size)
    result = 1;
  else if (e1->file->stat_struct.st_size > e2->file->stat_struct.st_size)
    result = -1;

  if (sctx->ascending)
    result *= -1;

  return result;
}

static const char *LookupOwnerSortName(uid_t uid, char *fallback,
                                       size_t fallback_size) {
  struct passwd *pwd_ptr;

  pwd_ptr = getpwuid(uid);
  if (pwd_ptr && pwd_ptr->pw_name)
    return pwd_ptr->pw_name;

  (void)snprintf(fallback, fallback_size, "%u", (unsigned int)uid);
  return fallback;
}

static const char *LookupGroupSortName(gid_t gid, char *fallback,
                                       size_t fallback_size) {
  struct group *grp_ptr;

  grp_ptr = getgrgid(gid);
  if (grp_ptr && grp_ptr->gr_name)
    return grp_ptr->gr_name;

  (void)snprintf(fallback, fallback_size, "%u", (unsigned int)gid);
  return fallback;
}

static int SortByOwner(const void *p1, const void *p2, void *arg) {
  const FileEntryList *e1 = p1;
  const FileEntryList *e2 = p2;
  const SortContext *sctx = arg;
  const char *o1, *o2;
  char n1[32], n2[32];

  o1 = LookupOwnerSortName((uid_t)e1->file->stat_struct.st_uid, n1, sizeof(n1));
  o2 = LookupOwnerSortName((uid_t)e2->file->stat_struct.st_uid, n2, sizeof(n2));

  if (sctx->case_sensitive)
    if (sctx->ascending)
      return (strcmp(o1, o2));
    else
      return (-(strcmp(o1, o2)));
  else if (sctx->ascending)
    return (strcasecmp(o1, o2));
  else
    return (-(strcasecmp(o1, o2)));
}

static int SortByGroup(const void *p1, const void *p2, void *arg) {
  const FileEntryList *e1 = p1;
  const FileEntryList *e2 = p2;
  const SortContext *sctx = arg;
  const char *g1, *g2;
  char n1[32], n2[32];

  g1 = LookupGroupSortName((gid_t)e1->file->stat_struct.st_gid, n1, sizeof(n1));
  g2 = LookupGroupSortName((gid_t)e2->file->stat_struct.st_gid, n2, sizeof(n2));

  if (sctx->case_sensitive)
    if (sctx->ascending)
      return (strcmp(g1, g2));
    else
      return (-(strcmp(g1, g2)));
  else if (sctx->ascending)
    return (strcasecmp(g1, g2));
  else
    return (-(strcasecmp(g1, g2)));
}
