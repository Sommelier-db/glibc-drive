/* Linux statx implementation.
   Copyright (C) 2018-2022 Free Software Foundation, Inc.
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

#include <errno.h>
#include <sys/stat.h>
#include <sysdep.h>
#include <drive_common.h>
#include <string.h>
#include "statx_generic.c"

int
statx (int fd, const char *path, int flags,
       unsigned int mask, struct statx *buf)
{
  if(drive_loaded && (fd == AT_FDCWD && strncmp(drive_prefix, path, drive_prefix_len) == 0)){
    const char *drivepath = path + drive_prefix_len;
    if(isExistFilepath(httpclient, userinfo, drivepath)){
      struct CContentsData contents;
      contents =  openFilepath(httpclient, userinfo, drivepath);
      if(contents.is_file){
        buf->stx_mode = S_IFREG;
      }
      else{
        buf->stx_mode = S_IFDIR;
      }
      buf->stx_mode |= S_IRUSR;
      for(int i=0;i<contents.num_writeable_users; i++){
        if(contents.writeable_user_path_ids[i] == userinfo.id) buf->stx_mode |= S_IWUSR;
      }
      buf->stx_uid = buf->stx_gid = 0;
      buf->stx_blksize = contents.file_bytes_len;
      freeContentsData(contents);
      return 0;
    }
    else{
      return -1;
    }
  }
  int ret = INLINE_SYSCALL_CALL (statx, fd, path, flags, mask, buf);
#ifdef __ASSUME_STATX
  return ret;
#else
  if (ret == 0 || errno != ENOSYS)
    /* Preserve non-error/non-ENOSYS return values.  */
    return ret;
  else
    return statx_generic (fd, path, flags, mask, buf);
#endif
}
