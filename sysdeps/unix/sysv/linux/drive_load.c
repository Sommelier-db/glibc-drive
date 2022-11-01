#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <drive_common.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define DLSYM(__DLHANDLE__,__FUNC__) (__FUNC__ = dlsym((__DLHANDLE__), #__FUNC__))

char *data_sk;
char *data_pk;
char *keyword_sk;
char *keyword_pk;

char *drive_base_dir;
int drive_base_dirfd;
char *drive_prefix;
size_t drive_prefix_len;
char *fd_drivepath_table[256];
int drive_loaded;
int drive_trace;

struct CHttpClient httpclient;
struct CUserInfo userinfo;
struct CPublicKeys pubkeys;

size_t hexpath(char *dst, const char *path);

void *crypto_handler;

int (*addDirectory)(struct CHttpClient, struct CUserInfo, const char *);
int (*addFile)(struct CHttpClient, struct CUserInfo, const char *, const char *, size_t);
int (*addReadPermission)(struct CHttpClient, struct CUserInfo, const char *, uint64_t);

void (*freeContentsData)(struct CContentsData);
void (*freePathVec)(struct CPathVec);
void (*freePublicKeys)(struct CPublicKeys);
void (*freeUserInfo)(struct CUserInfo);

struct CPathVec (*getChildrenPathes)(struct CHttpClient, struct CUserInfo, const char *);
char *(*getFilePathWithId)(struct CHttpClient, struct CUserInfo, uint64_t);
struct CPublicKeys (*getPublicKeys)(struct CHttpClient, uint64_t);
int (*isExistFilepath)(struct CHttpClient, struct CUserInfo, const char *);
int (*modifyFile)(struct CHttpClient, struct CUserInfo, const char *, const char *, size_t);
struct CContentsData (*openFilepath)(struct CHttpClient, struct CUserInfo, const char *);
struct CUserInfo (*registerUser)(struct CHttpClient, const char *);
int (*searchDescendantPathes)(struct CHttpClient, struct CUserInfo, const char *, char **);

static void __attribute__((constructor)) load_drive(void);
static void __attribute__((destructor)) finalize_drive(void);

static void load_drive(void){
  struct stat statbuf;
  char *user_id, *dataskfile, *keywordskfile, *base_url, *region_name, *base_dir;
  if((getenv("SOMMELIER_DRIVE_TRACE")) != NULL){
    drive_trace = 1;
  }
  if((user_id = getenv("SOMMELIER_DRIVE_USER_ID")) == NULL){
    if(drive_trace) fputs("SOMMELIER_DRIVE_USER_ID is not set\n", stderr);
    return;
  }
  if((dataskfile = getenv("SOMMELIER_DRIVE_DATA_SK")) == NULL){
    if(drive_trace) fputs("SOMMELIER_DRIVE_DATA_SK is not set\n", stderr);
    return;
  }
  if((keywordskfile = getenv("SOMMELIER_DRIVE_KEYWORD_SK")) == NULL){
    if(drive_trace) fputs("SOMMELIER_DRIVE_KEYWORD_SK is not set\n", stderr);
    return;
  }
  if((base_url = getenv("SOMMELIER_DRIVE_BASE_URL")) == NULL){
    if(drive_trace) fputs("SOMMELIER_DRIVE_BASE_URL is not set\n", stderr);
    return;
  }
  if((region_name = getenv("SOMMELIER_DRIVE_REGION_NAME")) == NULL){
    if(drive_trace) fputs("SOMMELIER_DRIVE_REGION_NAME is not set\n", stderr);
    return;
  }
  if((base_dir = getenv("SOMMELIER_DRIVE_BASE_DIR")) == NULL){
    if(drive_trace) fputs("SOMMELIER_DRIVE_BASE_DIR is not set\n", stderr);
    return;
  }

  if((userinfo.id = atol(user_id)) == 0){
    if(drive_trace) fputs("SOMMELIER_DRIVE_USER_ID is not valid number\n", stderr);
    return;
  }
  httpclient.base_url = (char *)malloc(strlen(base_url) + 1);
  strcpy(httpclient.base_url, base_url);

  drive_prefix_len = strlen(region_name) + 1;
  drive_prefix = (char *)malloc(drive_prefix_len + 1);
  strncpy(drive_prefix, region_name, drive_prefix_len + 1);
  drive_prefix[drive_prefix_len-1] = ':';
  httpclient.region_name = (char *)malloc(strlen(region_name) + 1);
  strcpy(httpclient.region_name, region_name);

  int fd_sk;
  if((fd_sk = __open64(dataskfile, O_RDONLY)) == -1){
    if(drive_trace) fputs("failed to open data secret key file\n", stderr);
    return;
  }
  if(fstat(fd_sk, &statbuf) == -1){
    if(drive_trace) fputs("failed to get data secret key size\n", stderr);
    return;
  }
  userinfo.data_sk = (char *)malloc(statbuf.st_size + 1);
  userinfo.data_sk[statbuf.st_size] = 0;
  read(fd_sk, userinfo.data_sk, statbuf.st_size);
  close(fd_sk);

  if((fd_sk = __open64(keywordskfile, O_RDONLY)) == -1){
    if(drive_trace) fputs("failed to open keyword secret key file\n", stderr);
    return;
  }
  if(fstat(fd_sk, &statbuf) == -1){
    if(drive_trace) fputs("failed to get keyword secret key size\n", stderr);
    return;
  }
  userinfo.keyword_sk = (char *)malloc(statbuf.st_size + 1);
  read(fd_sk, userinfo.keyword_sk, statbuf.st_size);
  userinfo.keyword_sk[statbuf.st_size] = 0;
  close(fd_sk);

  if(access(base_dir, R_OK|W_OK) != 0){
    if(drive_trace) fputs("cannot access drive base directory\n", stderr);
    return;
  }
  if((drive_base_dirfd = __open64(base_dir, O_RDONLY | O_DIRECTORY)) == -1){
    if(drive_trace) fputs("failed to access drive base directory\n", stderr);
    return;
  }
  drive_base_dir = strdup(base_dir);
  fd_drivepath_table[drive_base_dirfd] = strdup("");
  
  if((crypto_handler = dlopen("libsommelier_drive_client.so", RTLD_LAZY | RTLD_LOCAL)) != NULL){
    if(DLSYM(crypto_handler, addDirectory) &&
      DLSYM(crypto_handler, addFile) &&
      DLSYM(crypto_handler, addReadPermission) &&
      DLSYM(crypto_handler, freeContentsData) &&
      DLSYM(crypto_handler, freePathVec) &&
      DLSYM(crypto_handler, freePublicKeys) &&
      DLSYM(crypto_handler, freeUserInfo) &&
      DLSYM(crypto_handler, getChildrenPathes) &&
      DLSYM(crypto_handler, getFilePathWithId) &&
      DLSYM(crypto_handler, getPublicKeys) &&
      DLSYM(crypto_handler, isExistFilepath) &&
      DLSYM(crypto_handler, modifyFile) &&
      DLSYM(crypto_handler, openFilepath) &&
      DLSYM(crypto_handler, registerUser) &&
      DLSYM(crypto_handler, searchDescendantPathes)){
        drive_loaded = 1;
        if(drive_trace) fputs("load all functions from libsommelier_drive_client\n", stderr);
    }
  }
  if(drive_loaded != 1){
    if(drive_trace) fputs("failed to load libsommelier_drive_client\n", stderr);
  }

}

static void finalize_drive(void){
  if(httpclient.base_url) free(httpclient.base_url);
  if(httpclient.region_name) free(httpclient.region_name);
  if(userinfo.data_sk) free(userinfo.data_sk);
  if(userinfo.keyword_sk) free(userinfo.keyword_sk);
  if(drive_base_dirfd > 0) close(drive_base_dirfd);
  if(drive_base_dir) free(drive_base_dir);
  if(crypto_handler) dlclose(crypto_handler);
}

size_t hexpath(char *dst, const char *path){
  size_t i;
  for(i = 0; path[i] != 0; i++){
    sprintf(&dst[i*2], "%x", path[i]);
  }
  dst[i*2] = 0;
  return i*2;
}


char *fd_to_drivepath(int dirfd, const char *name){
  char *drivepath = NULL;
  if(!drive_loaded) return NULL;
  if(dirfd == AT_FDCWD && strncmp(drive_prefix, name, drive_prefix_len) == 0){
    // fd_to_drivepath(AT_FDCWD, "region-name:/A/B") -> /A/B
    drivepath = strdup(name + drive_prefix_len);
  }

  else if(0<=dirfd && dirfd<sizeof(fd_drivepath_table)/sizeof(fd_drivepath_table[0])
  && fd_drivepath_table[dirfd] != NULL){
    if(*name == '/'){
      // fd_to_drivepath(base_dir_fd, "/A/B") -> /A/B
      drivepath = strdup(name);
    }
    else{
      // fd_to_drivepath(base_dir_fd, "A/B") -> <base>/A/B
      size_t base_len = strlen(fd_drivepath_table[dirfd]);
      drivepath = (char*)malloc(base_len + strlen(name) + 2);
      strcpy(drivepath, fd_drivepath_table[dirfd]);
      if(*name != 0){
        drivepath[base_len] = '/';
        strcpy(drivepath + base_len + 1, name);
      }
    }
  }
  return drivepath;
}