/* Linux open syscall implementation, LFS.
   Copyright (C) 1991-2022 Free Software Foundation, Inc.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sysdep-cancel.h>
#include <shlib-compat.h>
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <drive_common.h>

/* Open FILE with access OFLAG.  If O_CREAT or O_TMPFILE is in OFLAG,
   a third argument is the file protection.  */
int
__libc_open64 (const char *file, int oflag, ...)
{
  int mode = 0;

  if (__OPEN_NEEDS_MODE (oflag))
    {
      va_list arg;
      va_start (arg, oflag);
      mode = va_arg (arg, int);
      va_end (arg);
    }

  if(drive_loaded && strncmp(drive_prefix, file, drive_prefix_len) == 0){
    struct CContentsData contents;
    const char *drivepath = file + drive_prefix_len;
    int isexist;
    if((isexist = isExistFilepath(httpclient, userinfo, drivepath)) == -1){
      __set_errno(EINVAL);
      return -1;
    }
    if((oflag & O_WRONLY) | (oflag & O_RDWR)){
      if(!isexist){ // not exist
        if(oflag & O_CREAT){
          if(addFile(httpclient, userinfo, drivepath, "", 0) == 0){
            __set_errno(EACCES);
            return -1;
          }
        }
        else{ // not exist and O_CREAT is not set
          __set_errno(EACCES);
          return -1;
        }
      }
    }
    else{ // O_RDONLY
      if(!isexist){
        __set_errno(ENOENT);
        return -1;
      }
    }
    contents = openFilepath(httpclient, userinfo, drivepath);
    if(contents.is_file == 0){
      int dirfd =  open(drive_base_dir, O_RDONLY | O_DIRECTORY);
      fd_drivepath_table[dirfd] = strdup(file);
      return dirfd;
    }
    char *realpath = (char *)malloc(strlen(file) * 2 + 1);
    hexpath(realpath, file);

    int fd = openat(drive_base_dirfd, realpath, O_CREAT|O_WRONLY|O_TRUNC, 0644);
    write(fd, contents.file_bytes_ptr, contents.file_bytes_len);
    close(fd);
    freeContentsData(contents);

    fd = SYSCALL_CANCEL (openat, drive_base_dirfd, realpath, oflag | O_LARGEFILE, mode);
    free(realpath);

    if((oflag & O_WRONLY) | (oflag & O_RDWR)){
      fd_drivepath_table[fd] = strdup(file);
    }
    return fd;
  }

  return SYSCALL_CANCEL (openat, AT_FDCWD, file, oflag | O_LARGEFILE,
			 mode);
}

int
mkdir (const char *path, mode_t mode)
{
  if(drive_loaded && strncmp(drive_prefix, path, drive_prefix_len) == 0){
    const char *drivepath = path + drive_prefix_len;
    if(addDirectory(httpclient, userinfo, drivepath) == 1){
      return 0;
    }
    else{
      __set_errno(EINVAL);
      return -1;
    }
  }
  return INLINE_SYSCALL (mkdirat, 3, AT_FDCWD, path, mode);
}

strong_alias (__libc_open64, __open64)
libc_hidden_weak (__open64)
weak_alias (__libc_open64, open64)

#ifdef __OFF_T_MATCHES_OFF64_T
strong_alias (__libc_open64, __libc_open)
strong_alias (__libc_open64, __open)
libc_hidden_weak (__open)
weak_alias (__libc_open64, open)
#endif

#if OTHER_SHLIB_COMPAT (libpthread, GLIBC_2_1, GLIBC_2_2)
compat_symbol (libc, __libc_open64, open64, GLIBC_2_2);
#endif
