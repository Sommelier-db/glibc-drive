/* Synchronize a file's in-core state with storage device Linux
   implementation.
   Copyright (C) 2017-2022 Free Software Foundation, Inc.
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

#include <unistd.h>
#include <sysdep-cancel.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <drive_common.h>


/* Make all changes done to FD actually appear on disk.  */
int
fsync (int fd)
{

#if DRIVE_EXT
  if(fd_drivepath_table[fd] != NULL){
    struct stat statbuf;
    char *drivepath = fd_drivepath_table[fd];
    fd_drivepath_table[fd] = NULL;
    fstat(fd, &statbuf);
    if(! S_ISREG(statbuf.st_mode) || ! (fcntl(fd, F_GETFL) & (O_WRONLY|O_RDWR))){
      free(drivepath);
      __set_errno(EINVAL);
      return -1;
    }
    char *realpath = (char *)malloc(strlen(drivepath) * 2 + 1);
    hexpath(realpath, drivepath);
    int rfd = SYSCALL_CANCEL (openat, drive_base_dirfd, realpath, O_RDONLY);
    if(rfd == -1){
      if(drive_trace) fprintf(stderr, "fsync: failed to reopen %s:(%s)\n", realpath, drivepath);
      free(realpath);
      free(drivepath);
      return -1;
    }
    free(realpath);

    fstat(rfd, &statbuf);
    // void *mapaddr = mmap(NULL, statbuf.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
    char *databuf = (char *)malloc(statbuf.st_size);
    read(rfd, databuf, statbuf.st_size);
    if(modifyFile(httpclient, userinfo, drivepath, databuf, statbuf.st_size) != 1){
      if(drive_trace) fprintf(stderr, "fsync: failed to modify %s\n", drivepath);
      return -1;
    }
    if(drive_trace) fprintf(stderr, "fsync: modify contents of %s size: %ld\n", drivepath, statbuf.st_size);
    // munmap(mapaddr, statbuf.st_size);
    free(databuf);
    free(drivepath);
    return 0;
  }
#endif

  return SYSCALL_CANCEL (fsync, fd);

}
libc_hidden_def (fsync)
