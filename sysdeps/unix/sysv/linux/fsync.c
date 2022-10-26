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
  if(fd_drivepath_table[fd] != NULL){
    char *temp = fd_drivepath_table[fd];
    char *realpath = (char *)malloc(strlen(temp) * 2 + 1);
    hexpath(realpath, temp);
    int rfd = openat(drive_base_dirfd, realpath, O_RDONLY);
    assert(rfd != -1);
    struct stat statbuf;
    fstat(rfd, &statbuf);
    // void *mapaddr = mmap(NULL, statbuf.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
    char *databuf = (char *)malloc(statbuf.st_size);
    read(rfd, databuf, statbuf.st_size);
    if(modifyFile(httpclient, userinfo, temp + drive_prefix_len, databuf, statbuf.st_size) != 1){
      fputs("failed to modify file", stderr);
    }
    // munmap(mapaddr, statbuf.st_size);
    free(databuf);
  }
  return SYSCALL_CANCEL (fsync, fd);

}
libc_hidden_def (fsync)
