
#ifndef _DRIVE_COMMON_H

#define _DRIVE_COMMON_H

#define DRIVE_EXT 1

#include <sys/types.h>
#include <stdint.h>

typedef struct CHttpClient {
  char *base_url;
  char *region_name;
} CHttpClient;

typedef struct CUserInfo {
  uint64_t id;
  char *data_sk;
  char *keyword_sk;
} CUserInfo;

typedef struct CPublicKeys {
  char *data_pk;
  char *keyword_pk;
} CPublicKeys;

typedef struct CContentsData {
  int is_file;
  size_t num_readable_users;
  size_t num_writeable_users;
  uint64_t *readable_user_path_ids;
  uint64_t *writeable_user_path_ids;
  uint8_t *file_bytes_ptr;
  size_t file_bytes_len;
} CContentsData;

typedef struct CPathVec {
  char **ptr;
  size_t len;
} CPathVec;

extern void *crypto_handler;
extern int (*addDirectory)(struct CHttpClient, struct CUserInfo, const char *);
extern int (*addFile)(struct CHttpClient, struct CUserInfo, const char *, const char *, size_t);
extern int (*addReadPermission)(struct CHttpClient, struct CUserInfo, const char *, uint64_t);

extern void (*freeContentsData)(struct CContentsData);
extern void (*freePathVec)(struct CPathVec);
extern void (*freePublicKeys)(struct CPublicKeys);
extern void (*freeUserInfo)(struct CUserInfo);

extern struct CPathVec (*getChildrenPathes)(struct CHttpClient, struct CUserInfo, const char *);
extern char *(*getFilePathWithId)(struct CHttpClient, struct CUserInfo, uint64_t);
extern struct CPublicKeys (*getPublicKeys)(struct CHttpClient, uint64_t);
extern int (*isExistFilepath)(struct CHttpClient, struct CUserInfo, const char *);
extern int (*modifyFile)(struct CHttpClient, struct CUserInfo, const char *, const char *, size_t);
extern struct CContentsData (*openFilepath)(struct CHttpClient, struct CUserInfo, const char *);
extern struct CUserInfo (*registerUser)(struct CHttpClient, const char *);
extern int (*searchDescendantPathes)(struct CHttpClient, struct CUserInfo, const char *, char **);

extern char *data_sk;
extern char *data_pk;
extern char *keyword_sk;
extern char *keyword_pk;

extern char *drive_base_dir;
extern int drive_base_dirfd;
extern char *drive_prefix;
extern size_t drive_prefix_len;
extern char *fd_drivepath_table[256];
extern int drive_loaded;
extern int drive_trace;

extern struct CHttpClient httpclient;
extern struct CUserInfo userinfo;
extern struct CPublicKeys pubkeys;

extern size_t hexpath(char *dst, const char *path);
extern char* fd_to_drivepath(int, const char *);

#endif