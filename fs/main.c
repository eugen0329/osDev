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
typedef enum  {false, true} bool;

typedef struct {
    char  path[500];
    mode_t mode; 
    content_t* content;
    long nlinks;
    long size;
    int isUsed;
} fileInfo_t;

char *fs_str;
char *fs_path;
mode_t   mode;

static fileInfo_t inodes[MAX_FILES_COUNT];

static int fs_getattr(const char *path, struct stat *stbuf);
static int fs_mknod(const char *path, mode_t mode, dev_t rdev);
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

int findFileIndex(const char * path);
void sendNotification(const char * name, const char * text);
long getUnusedNodeIndex();

void initNode(long index, const char *path, const char * content, mode_t mode)
{
    inodes[index].mode = mode;
    strcpy(inodes[index].path, path);
    inodes[index].content = (content_t *) malloc(500);
    strcpy(inodes[index].content, content);
    inodes[index].nlinks = 1;
    inodes[index].size = strlen(inodes[index].content);
    inodes[index].isUsed = 1;    
}

static struct fuse_operations fs_operations = {
    .getattr    = fs_getattr,
    .readdir    = fs_readdir,

    .open       = fs_open,
    .read       = fs_read,
    .write      = fs_write,
    .mknod      = fs_mknod,

    .access     = fs_access,

    .rename     = fs_rename,                           
    .chmod      = fs_chmod,                           
    .truncate   = fs_truncate, 
};

int main(int argc, char *argv[])
{
    memset(inodes, 0, sizeof(fileInfo_t) * MAX_FILES_COUNT);

    inodes[0].mode = S_IFREG | 0644;
    strcpy(inodes[0].path, "/hello");
    inodes[0].content = (content_t *) malloc(500);
    strcpy(inodes[0].content, "Hello World!\n");
    inodes[0].nlinks = 1;
    inodes[0].size = strlen(inodes[0].content);
    inodes[0].isUsed = 1;

    sendNotification("Warning", "asd");

    fuse_main(argc, argv, &fs_operations, NULL); 


    return 0;
}

static int fs_mknod(const char *path, mode_t mode, dev_t rdev)
{
    int res;
    int index;

    if (S_ISREG(mode)) {
        if((index = getUnusedNodeIndex())== -1) return -ENOMEM;
        initNode(index, path, "\0", mode);

    } else if (S_ISFIFO(mode)) {
        sendNotification("Can't create named pipe", "(function is not implemented)");
        res = -1;
    } else {
        sendNotification("Can't create device special file", "(function is not implemented)");
        res = -1;
    }
        

    return 0;
}

int fs_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;
    int index;

    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if((index = findFileIndex(path)) != -1) {
        stbuf->st_mode = inodes[index].mode;
        stbuf->st_nlink = inodes[index].nlinks;
        stbuf->st_size = inodes[index].size;
    } else {
        res = -ENOENT;
    }

    return res;
}



int fs_write(const char *path, const char *buf, size_t size,
             off_t offset, struct fuse_file_info *fi)
{
    int res = 0;
    (void) fi;  
    int index;
    if((index = findFileIndex(path)) == -1)
        return -ENOENT;

    printf("(%s %d)\n\n\n\n\n\n", inodes[index].content, size);
    strcpy(inodes[index].content + offset, buf);
    inodes[index].size = size;
    return res;
}

int fs_access(const char *path, int functMode)
{
    //int res;

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
    if (findFileIndex(path) == -1)
        return -ENOENT;

    return 0;
}



int fs_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi)
{
    size_t len;
    (void) fi;
    int index;
    if((index = findFileIndex(path)) == -1)
        return -ENOENT;

    len = strlen(inodes[index].content);
    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, inodes[index].content + offset, size);
    } else
        size = 0;

    return size;
}

int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
             off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;
    int i;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    for(i = 0; i < MAX_FILES_COUNT; i++) {
        if(strlen(inodes[i].path) != 0) filler(buf, inodes[i].path + 1, NULL, 0);
    }

    return 0;
}


int fs_rename(const char *from, const char *to)
{
    int index;
    if((index = findFileIndex(from)) == -1)
        return -ENOENT;

    strcpy(inodes[index].path, to);
    return 0;
}

int fs_chmod(const char *path, mode_t newMode)
{
    int index;
    if((index = findFileIndex(path)) == -1)
        return -ENOENT;

    inodes[index].mode = newMode;
    return 0;
}

int fs_truncate(const char *path, off_t size)
{
    int index;
    if((index = findFileIndex(path)) == -1)
        return -ENOENT;
    inodes[index].content[0] = '\0';
    inodes[index].size = 0;
    return 0;
}

int findFileIndex(const char * path)
{
    int i;
    for(i = 0; i < MAX_FILES_COUNT; i++) {
        if(inodes[i].path && ! strncmp(inodes[i].path, path, MAX_FPATH_LENGTH)) return i;
    }
    return -1;
}

void sendNotification(const char * name, const char * text)
{
    static char notifyIcon[] = "dialog-information";
    notify_init (name);
    NotifyNotification * notif = notify_notification_new (name, text, notifyIcon);
    notify_notification_show (notif, NULL);
    g_object_unref(G_OBJECT(notif));
    notify_uninit();
}

long getUnusedNodeIndex()
{
    int i;
    for(i = 0; i < MAX_FILES_COUNT; i++) {
        if(! inodes[i].isUsed) return i;
    }
    return -1;
}