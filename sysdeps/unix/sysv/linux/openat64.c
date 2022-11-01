/* Linux openat syscall implementation, LFS.
   Copyright (C) 2007-2022 Free Software Foundation, Inc.
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

#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <alloca.h>
#include <sysdep-cancel.h>
#include <drive_common.h>

/* Open FILE with access OFLAG.  Interpret relative paths relative to
   the directory associated with FD.  If OFLAG includes O_CREAT or
   O_TMPFILE, a fourth argument is the file protection.  */
int
__libc_openat64 (int fd, const char *file, int oflag, ...)
{
  mode_t mode = 0;
  if (__OPEN_NEEDS_MODE (oflag))
    {
      va_list arg;
      va_start (arg, oflag);
      mode = va_arg (arg, mode_t);
      va_end (arg);
    }

#if DRIVE_EXT
  char *drivepath;

  if((drivepath = fd_to_drivepath(fd, file)) != NULL){
    struct CContentsData contents;
    int isexist;
    if((isexist = isExistFilepath(httpclient, userinfo, drivepath)) == -1){
      __set_errno(EINVAL);
      goto handle_error;
    }
    if(drive_trace) fprintf(stderr, "openat: %s %sexists\n", drivepath, isexist ? "":"does not ");
    if((oflag & O_WRONLY) | (oflag & O_RDWR)){
      if(!isexist){
        // not exist
        if(oflag & O_CREAT){
          if(addFile(httpclient, userinfo, drivepath, "", 0) == 0){
            __set_errno(EACCES);
            goto handle_error;
          }
          else if(drive_trace)
            fprintf(stderr, "openat: %s is created\n", drivepath);
        }
        else{
          // not exist and O_CREAT is not set
          __set_errno(EACCES);
          goto handle_error;
        }
      }else{
        // exist
        if(oflag & O_CREAT && oflag & O_EXCL){
          __set_errno(EEXIST);
          goto handle_error;
        }
      }
    }
    else{
      // O_RDONLY
      if(!isexist){
        __set_errno(ENOENT);
        goto handle_error;
      }
    }
    if(isexist && ~oflag & O_TRUNC){
      // if already exists
      contents = openFilepath(httpclient, userinfo, drivepath);
      if(contents.is_file == 0){
        int dirfd =  SYSCALL_CANCEL(open, drive_base_dir, O_RDONLY | O_DIRECTORY, mode);
        fd_drivepath_table[dirfd] = drivepath;
        freeContentsData(contents);
        return dirfd;
      }
      else if(oflag & O_DIRECTORY){
        freeContentsData(contents);
        __set_errno(ENOTDIR);
        goto handle_error;
      }
      if(drive_trace) fprintf(stderr, "openat: get contents of %s\n", drivepath);
    }
    char *realpath = (char *)alloca(strlen(drivepath) * 2 + 1);
    hexpath(realpath, drivepath);
    int fd;

    if(isexist && ~oflag & O_TRUNC){
      fd = SYSCALL_CANCEL(openat, drive_base_dirfd, realpath, O_CREAT|O_WRONLY|O_TRUNC, 0644);
      if(fd == -1){
        if(drive_trace) fprintf(stderr, "openat: failed to open %s\n", realpath);
        freeContentsData(contents);
        __set_errno(EACCES);
        goto handle_error;
      }
      write(fd, contents.file_bytes_ptr, contents.file_bytes_len);
      __close(fd);
      freeContentsData(contents);
    }

    fd = SYSCALL_CANCEL (openat, drive_base_dirfd, realpath, oflag | O_LARGEFILE, mode);

    if(fd >= 0) fd_drivepath_table[fd] = drivepath;
    else free(drivepath);
  
    return fd;

    handle_error:
    free(drivepath);
    return -1;
  }
#endif

  return SYSCALL_CANCEL (openat, fd, file, oflag | O_LARGEFILE, mode);
}

strong_alias (__libc_openat64, __openat64)
libc_hidden_weak (__openat64)
weak_alias (__libc_openat64, openat64)

#ifdef __OFF_T_MATCHES_OFF64_T
strong_alias (__libc_openat64, __openat)
libc_hidden_weak (__openat)
weak_alias (__libc_openat64, openat)
#endif
