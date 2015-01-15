#define FUSE_USE_VERSION 26 
#pragma GCC diagnostic ignored "-Wunused-function"


#include <stdio.h>         
#include <stdlib.h>         
#include <string.h>  

#include <errno.h>         
#include <fcntl.h>  /* manipulate file descriptor */
#include <sys/types.h>  /* mode_t */
#include <unistd.h> /* access funcion mode macro */
#include <stdint.h>

#include <libnotify/notify.h>
#include <fuse.h>          


#define MAX_FILES_COUNT 20
#define MAX_FPATH_LENGTH 500


#define FS_SIZE         100000000
#define MAX_NAME_LEN    255
#define INODES_VEC sizeof(superblock_t)

typedef char content_t;
typedef struct fuse_file_info fuseFInfo_t;
typedef fuse_fill_dir_t filler_t;

typedef struct {
    char  path[500];
    mode_t mode; 
    content_t* content;
    uint64_t nLinks;
    long size;
    int isUsed;
} fileInfo_t;


typedef struct {
    uint32_t fsSize;
    uint32_t nInodes;
    uint32_t nBlocks;

    uint32_t firstBlock;

    uint32_t blockSize;
} superblock_t;

typedef struct {
    mode_t mode;
    uint64_t size;
    uint64_t nLinks;
    uint64_t nBlocks;
    uint64_t blocks[10];
} inode_t;

typedef struct {
    uint32_t inode;
    char name[MAX_NAME_LEN + 1];
} dirEntry_t;

static fileInfo_t inodes[MAX_FILES_COUNT];
static void* vdev; 
static superblock_t* sb;

static int fs_getattr(const char *path, struct stat *stbuf);
static int fs_mknod(const char *path, mode_t mode, dev_t rdev);
static int fs_unlink(const char *path);
static int fs_readdir(const char *path, void *buf, filler_t filler, off_t offset, fuseFInfo_t *fi);
static int fs_open(const char *path, fuseFInfo_t *fi);
static int fs_read(const char *path, char *buf, size_t size, off_t offset, fuseFInfo_t *fi);
static int fs_rename(const char *from, const char *to);
static int fs_chmod(const char *path, mode_t newMode);
static int fs_truncate(const char *path, off_t size);
static int fs_access(const char *path, int functMode);
static int fs_write(const char *path, const char *buf, size_t size,off_t offset, fuseFInfo_t *fi);

int findFileIndex(const char * path);
void sendNotification(const char * name, const char * text);
long getUnusedNodeIndex();
void fs_init();
inode_t* findInodeFromEntry(void * block, const char* name, uint16_t nameLen);
int getCharIndex(const char * str, char target);

void initNode(long index, const char *path, mode_t mode);
inode_t* getInode(const char *path);
inode_t* findInodeFromEntry(void * block, const char* name, uint16_t nameLen);

static struct fuse_operations fs_operations = {
    .getattr    = fs_getattr,
    .readdir    = fs_readdir,

    .open       = fs_open,
    .read       = fs_read,

    //.write      = fs_write,

    //.mknod      = fs_mknod,
    //.unlink     = fs_unlink,

    //.rename     = fs_rename,                           
    //.chmod      = fs_chmod,                           
    //.truncate   = fs_truncate, 
    //.access     = fs_access,
};

int main(int argc, char *argv[])
{
    memset(inodes, 0, sizeof(fileInfo_t) * MAX_FILES_COUNT);
    inodes[0].mode = S_IFREG | 0644;
    strcpy(inodes[0].path, "/hello");
    inodes[0].content = (content_t *) malloc(500);
    strcpy(inodes[0].content, "Hello World!\n");
    inodes[0].nLinks = 1;
    inodes[0].size = strlen(inodes[0].content);
    inodes[0].isUsed = 1;

    fs_init();


    char * data = (char *) malloc(500);
    inode_t* inode ;//= getInode("/");


    //if((inode = getInode("/.trash")) == NULL) exit(1);
    //memcpy(data, vdev + inode->blocks[0], inode->size);
    //data[inode->size] = '\0';
    //printf("size %d, mode %x, nlinks %d, data \"%s\"\n", inode->size, inode->mode, inode->nLinks, data); 
    //exit(1); 

    fuse_main(argc, argv, &fs_operations, NULL); 

    free(vdev);

    return 0;
}



inode_t* getInode(const char *path)
{
    inode_t* inode = (inode_t *)(vdev + INODES_VEC);
    int nameStart = 1;
    int nameEnd = 1;

    if(! strcmp(path, "/")) return inode;

    while(1) {
        if( (nameEnd = getCharIndex(path + nameStart, '/')) == -1) nameEnd = strlen(path);
        //printf("paht = %s,  name: start = %d, end = %d\n", path, nameStart, nameEnd);
        

        inode = findInodeFromEntry(vdev + inode->blocks[0], path + nameStart, nameEnd - nameStart);
        
        printf("+\n");
        
        if(inode == NULL || path[nameEnd] == '\0'){
            break;
        } 
        //printf("getInode:finded sizee%d\n", inode->size);
        nameStart = nameEnd;        
    }

    return inode;
}

inode_t* findInodeFromEntry(void * block, const char* name, uint16_t nameLen)
{
    dirEntry_t* de = (dirEntry_t *) block;
    int i;
    int lim = (sb->blockSize / sizeof(dirEntry_t)) - 2;

    for(i = 0; i < lim; i++) {
        printf("de = %s\n", de->name);
        if(strlen(de->name) == nameLen && ! strncmp(de->name, name, nameLen)) {
            return (inode_t *) (vdev + de->inode);
        }
        de += sizeof(dirEntry_t);
    }
    printf("-\n");
    return NULL;
}


int fs_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;
    int index;
    inode_t * inode;

    memset(stbuf, 0, sizeof(struct stat));
   
    if( (inode = getInode(path)) != NULL) {
        stbuf->st_mode = inode->mode;
        stbuf->st_nlink = inode->nLinks;
        stbuf->st_size = inode->size;
    } else {
        res = -ENOENT;
    }
       

    return res;
}

int fs_mknod(const char *path, mode_t mode, dev_t rdev)
{
    int res = 0;
    int index;

    if (S_ISREG(mode)) {
        if((index = getUnusedNodeIndex())== -1) return -ENOMEM;
        initNode(index, path, mode);
    } else if (S_ISFIFO(mode)) {
        sendNotification("Can't create named pipe", "(function is not implemented)");
        res = -1;
    } else {
        sendNotification("Can't create device special file", "(function is not implemented)");
        res = -1;
    }
        

    return res;
}

int fs_unlink(const char *path)
{
    int index = findFileIndex(path);
    if(index == -1) 
        return -ENOENT;

    inodes[index].isUsed = 0;
    free(inodes[index].content);

    return 0;
}


int fs_write(const char *path, const char *buf, size_t size, off_t offset, fuseFInfo_t *fi)
{
    int res = 0;
    (void) fi;  
    int index;
    if((index = findFileIndex(path)) == -1)
        return -ENOENT;

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

int fs_open(const char *path, fuseFInfo_t *fi)
{
    if (findFileIndex(path) == -1)
        return -ENOENT;

    return 0;
}

int fs_read(const char *path, char *buf, size_t size, off_t offset, fuseFInfo_t *fi)
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

int fs_readdir(const char *path, void *buf, filler_t filler, off_t offset, fuseFInfo_t *fi)
{
    (void) offset;
    (void) fi;
    int i;

    if (strcmp(path, "/") != 0) {
        return -ENOENT;
    }  

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

void initNode(long index, const char *path, mode_t mode)
{
    inodes[index].mode = mode;
    strcpy(inodes[index].path, path);
    inodes[index].content = (content_t *) malloc(500);
    inodes[index].nLinks = 1;
    inodes[index].size = 0;
    inodes[index].isUsed = 1;    
}

int getCharIndex(const char * str, char target)
{
    const char *ptr = strchr(str, target);
    if(ptr) return ptr - str;
    return -1;
}



void fs_init()
{
    vdev = calloc(1, FS_SIZE);
    sb = (superblock_t *) vdev;
    sb->fsSize = FS_SIZE;
    sb->nInodes = 50;
    sb->nBlocks = 100;
    sb->blockSize = 1024;
    sb->firstBlock = sizeof(superblock_t) + sizeof(inode_t) * sb->nInodes;
    printf("superblock size = %d, inode size = %d, memory block size = %d, dirEnt size = %d\n\n\n\n\n", sizeof(superblock_t), sizeof(inode_t), sb->blockSize, sizeof(dirEntry_t));


    inode_t * inode = (inode_t *) (vdev + sizeof(superblock_t));
    inode->mode = S_IFDIR | 0755;
    inode->nLinks = 2;
    inode->blocks[0] = sb->firstBlock;
    inode->size = sb->blockSize;

    dirEntry_t* de = (dirEntry_t *) (vdev + inode->blocks[0]);
    //strcpy(de->name, "/");
    //de->inode = INODES_VEC;
//
    //de += sb->blockSize;
    de->inode = INODES_VEC + sizeof(inode_t);
    strcpy(de->name, "hello");    

    inode = (inode_t *) (vdev + INODES_VEC + sizeof(inode_t));
    inode->mode = S_IFREG | 0644;
    inode->nLinks = 1;
    char helloStr[] = "Hello world!";
    inode->size = strlen(helloStr);
    inode->blocks[0] = sb->firstBlock + sb->blockSize; 
    void* memBlock = (dirEntry_t *) (vdev + inode->blocks[0]);
    memcpy(memBlock, helloStr, strlen(helloStr));    


}