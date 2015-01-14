#define FUSE_USE_VERSION 26 

#include <stdio.h>         
#include <stdlib.h>         
#include <string.h>        
#include <errno.h>         
#include <fcntl.h>  /* manipulate file descriptor */
#include <sys/types.h>  /* mode_t */
#include <unistd.h> /* access funcion mode macro */

#include <libnotify/notify.h>
#include <fuse.h>          


#define MAX_FILES_COUNT 20
#define MAX_FPATH_LENGTH 500

typedef char content_t;

typedef struct {
    char * path;
    mode_t mode; 
    content_t* content;
    long nlinks;
    long size;
    int isFree;
} fileInfo_t;

static char *fs_str;
static char *fs_path;
static mode_t   mode;


static int fs_getattr(const char *path, struct stat *stbuf);
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
             off_t offset, struct fuse_file_info *fi);
static int fs_open(const char *path, struct fuse_file_info *fi);
static int fs_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi);

static int fs_rename(const char *from, const char *to);
static int fs_chmod(const char *path, mode_t newMode);
static int fs_truncate(const char *path, off_t size);

static int fs_access(const char *path, int functMode);

static int fs_write(const char *path, const char *buf, size_t size,
             off_t offset, struct fuse_file_info *fi);




static struct fuse_operations fs_operations = {
    .getattr    = fs_getattr,
    .readdir    = fs_readdir,

    .open       = fs_open,
    .read       = fs_read,
    .write      = fs_write,

    .access     = fs_access,

    .rename     = fs_rename,                           
    .chmod      = fs_chmod,                           
    .truncate   = fs_truncate, 
};

int main(int argc, char *argv[])
{

    fs_str = (char*) malloc(500);
    fs_path = (char*) malloc(500);
    strcpy(fs_str, "Hello World!\n");
    strcpy(fs_path, "/hello");
    mode = S_IFREG | 0644;

    fuse_main(argc, argv, &fs_operations, NULL); 
    free(fs_path);
    free(fs_str);
    return 0;
    //return fuse_main(argc, argv, &fs_oper, NULL);
}

int fs_write(const char *path, const char *buf, size_t size,
             off_t offset, struct fuse_file_info *fi)
{
    int res = 0;
    (void) fi;
    strcpy(fs_str + offset, buf);
    return res;
}





int fs_access(const char *path, int functMode)
{
    int res;
    switch(functMode) {
        case R_OK:
            break;
        case W_OK:
            break;
        case X_OK:
            break;
        case F_OK:
            break;
        default:
            break;
    }

    return 0;
}

int fs_open(const char *path, struct fuse_file_info *fi)
{
    if (strcmp(path, fs_path) != 0)
        return -ENOENT;

    //if ((fi->flags & 3) != O_RDONLY)
    //    return -EACCES;

    return 0;
}

int fs_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;

    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if (strcmp(path, fs_path) == 0) {
        stbuf->st_mode = mode;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(fs_str);
    } else
        res = -ENOENT;

    return res;
}

int fs_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi)
{
    size_t len;
    (void) fi;
    if(strcmp(path, fs_path) != 0)
        return -ENOENT;

    len = strlen(fs_str);
    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, fs_str + offset, size);
    } else
        size = 0;

    return size;
}

int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
             off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    filler(buf, fs_path + 1, NULL, 0);

    return 0;
}


int fs_rename(const char *from, const char *to)
{
    strcpy(fs_path, to);
    return 0;
}

int fs_chmod(const char *path, mode_t newMode)
{
    mode = newMode;
    return 0;
}

int fs_truncate(const char *path, off_t size)
{
    fs_str[0] = '\0';
    fprintf(stderr, "\n\n\n\n\n\n\n\n\n");
    return 0;
}