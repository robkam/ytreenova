/***************************************************************************
 *
 * src/fs/freesp.c
 * Determining free disk capacity
 *
 ***************************************************************************/

#include "ytnova_defs.h"
#include <unistd.h>


int GetDiskParameter(char *path, char *volume_name, long long *avail_bytes,
                     long long *capacity, Statistic *s) {

#ifdef WIN32
  struct _diskfree_t diskspace;
#else
  /* Use struct statvfs as the modern POSIX standard */
  struct statvfs statfs_struct;
#define FS_STRUCT_IS_STATVFS
#endif

  char *p;
  const char *fname;
  int result;
  long long this_disk_space;

#ifdef WIN32
  if ((result = _getdiskfree(0, &diskspace)) == 0)
#else
  /* Use the STATFS macro defined in ytnova.h which resolves to statvfs */
  if ((result = STATFS(path, &statfs_struct, 0, 0)) == 0)
#endif /* WIN32 */
  {
    if (volume_name) {
      /* Renamed usage: s->mode -> s->log_mode */
      if (s->log_mode == DISK_MODE || s->log_mode == USER_MODE) {

#if defined(__linux__)
        /* Minimal Linux/GNU FS Type Detection */
        switch (statfs_struct.f_type) {
        case 0xEF53:
          fname = "EXT2/3/4";
          break;
        case 0x9660:
          fname = "ISOFS";
          break;
        case 0x4d44:
          fname = "DOS/FAT";
          break;
        case 0x6969:
          fname = "NFS";
          break;
        case 0x9fa0:
          fname = "PROC";
          break;
        case 0x5346544e: /* 'NTFS' magic from ntfs-3g */
        case 0x53465435:
          fname = "NTFS";
          break;
        case 0x72656d6f:
          fname = "WSLFS/9P";
          break; /* For WSL2 mounts */
        default:
          fname = "OTHER-FS";
        }
#elif defined(__APPLE__)
        /* Rely on statvfs f_fstypename */
        fname = statfs_struct.f_fstypename;
#elif defined(__NetBSD__) || defined(__OpenBSD__) || defined(__FreeBSD__)
        /* BSDs often provide f_fstypename via statvfs */
        fname = statfs_struct.f_fstypename;
#else
        /* Generic POSIX fallback (includes WSL/other Unix-likes) */
        fname = "UNIX-FS";
#endif

        (void)strncpy(volume_name, fname,
                      MINIMUM(DISK_NAME_LENGTH, strlen(fname)));
        volume_name[MINIMUM(DISK_NAME_LENGTH, strlen(fname))] = '\0';
      } else {
        if ((p = strrchr(s->log_path, FILE_SEPARATOR_CHAR)) == NULL)
          p = s->log_path;
        else
          p++;

        (void)strncpy(volume_name, p, sizeof(s->disk_name));
        volume_name[sizeof(s->disk_name)] = '\0';
      }
    } /* volume_name */

#ifdef WIN32
    *avail_bytes = (long long)diskspace.bytes_per_sector *
                   (long long)diskspace.sectors_per_cluster *
                   (long long)diskspace.avail_clusters;
    this_disk_space = 900000000L; /* for now.. */
#else
    /* Consolidated POSIX logic for disk space */
    long long bfree;

    /* Use f_bavail for non-root, f_bfree for root/fallback */
    bfree = getuid() ? statfs_struct.f_bavail : statfs_struct.f_bfree;

    if (bfree < 0L)
      bfree = 0L;

/* statvfs uses f_frsize (fragment size) */
#define FRAG_SIZE statfs_struct.f_frsize

    *avail_bytes = bfree * FRAG_SIZE;
    this_disk_space = (long long)statfs_struct.f_blocks * FRAG_SIZE;

#endif /* WIN32 */

    if (capacity) {
      *capacity = this_disk_space;
    }
  }
  return (result);
}

int GetAvailBytes(long long *avail_bytes, Statistic *s) {
  return (GetDiskParameter(s->tree->name, NULL, avail_bytes, NULL, s));
}
