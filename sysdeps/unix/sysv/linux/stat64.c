/* Get file status.  Linux version.
   Copyright (C) 2020-2022 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#define __stat __redirect___stat
#define stat   __redirect_stat
#include <sys/stat.h>
#include <fcntl.h>
#include <kernel_stat.h>
#include <stat_t64_cp.h>
#include <unistd.h>
#include <string.h>
#include <drive_common.h>

int
__stat64_time64 (const char *file, struct __stat64_t64 *buf)
{
  if(drive_loaded && strncmp(drive_prefix, file, drive_prefix_len) == 0){
    const char *drivepath = file + drive_prefix_len;
    if(isExistFilepath(httpclient, userinfo, drivepath)){
      struct CContentsData contents;
      contents =  openFilepath(httpclient, userinfo, drivepath);
      if(contents.is_file){
        buf->st_mode = S_IFREG;
      }
      else{
        buf->st_mode = S_IFDIR;
      }
      buf->st_mode |= S_IRUSR;
      for(int i=0;i<contents.num_writeable_users; i++){
        if(contents.writeable_user_path_ids[i] == userinfo.id) buf->st_mode |= S_IWUSR;
      }
      buf->st_uid = buf->st_gid = getuid();
      buf->st_blksize = contents.file_bytes_len;
      freeContentsData(contents);
      return 0;
    }
    else{
      return -1;
    }
  }
  return __fstatat64_time64 (AT_FDCWD, file, buf, 0);
}
#if __TIMESIZE != 64
hidden_def (__stat64_time64)

int
__stat64 (const char *file, struct stat64 *buf)
{
  struct __stat64_t64 st_t64;
  return __stat64_time64 (file, &st_t64)
	 ?: __cp_stat64_t64_stat64 (&st_t64, buf);
}
#endif

#undef __stat
#undef stat

hidden_def (__stat64)
weak_alias (__stat64, stat64)

#if XSTAT_IS_XSTAT64
strong_alias (__stat64, __stat)
weak_alias (__stat64, stat)
#endif
