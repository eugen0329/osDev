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
#include <libgen.h> /* basename, dirname */


#include <libnotify/notify.h>
#include <fuse.h>          


#define MAX_FILES_COUNT 20
#define MAX_FPATH_LENGTH 500


#define FS_SIZE         100000000
#define MAX_NAME_LEN    255
#define INODES_VEC sizeof(superblock_t)
#define DATA_BLOCK_SIZE 1024
#define DIR_ENTR_IN_BLOCK ((sb->dataBlockSize / sizeof(dirEntry_t)) - 1)

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

    uint32_t dataBlockSize;
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
dirEntry_t* getDirEntry(void * block, const char* name, uint16_t nameLen);
int getCharIndex(const char * str, char target);
int rgetCharIndex(const char * str, char target);

void initNode(long index, const char *path, mode_t mode);
inode_t* getInodeByPath(const char *path);
//dirEntry_t* getDirEntry(void * block, const char* name, uint16_t nameLen);

inode_t* getInode(uint64_t id);
void* getDataBlock(uint64_t id);

static struct fuse_operations fs_operations = {
    .getattr    = fs_getattr,
    .readdir    = fs_readdir,

    .open       = fs_open,
    .read       = fs_read,

    .access     = fs_access,

    .write      = fs_write,

    //.mknod      = fs_mknod,
    //.unlink     = fs_unlink,

    .rename     = fs_rename,                           
    //.chmod      = fs_chmod,                           
    //.truncate   = fs_truncate, 

};

void test();

/*typedef struct { */
    /*uint32_t inode;*/
    /*char name[MAX_NAME_LEN + 1];*/
/*} dirEntry_t;*/


char * getBasename(const char * path);
char * getDirname(const char * path);


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

    fuse_main(argc, argv, &fs_operations, NULL); 
    free(vdev);

    return 0;
}



int fs_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;
    //int index;
    inode_t * inode;

    memset(stbuf, 0, sizeof(struct stat));
   
    if( (inode = getInodeByPath(path)) != NULL) {
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


    inode_t * inode;
    if((inode = getInodeByPath(path)) == NULL) return -ENOENT;
    void* dataBlock = getDataBlock(inode->blocks[0]);

    memcpy(dataBlock + offset, buf, strlen(buf));
    inode->size = size;
    //strcpy(inodes[index].content + offset, buf);
    //inodes[index].size = size;
    return res;
}

int fs_access(const char *path, int functMode)
{
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
    if(getInodeByPath(path) == NULL) return -ENOENT;
    return 0;
}

int fs_read(const char *path, char *buf, size_t size, off_t offset, fuseFInfo_t *fi)
{
    size_t len;
    (void) fi;

    inode_t * inode;
    if((inode = getInodeByPath(path)) == NULL) return -ENOENT;
    void* dataBlock = getDataBlock(inode->blocks[0]);

    len = inode->size;
    if(offset < len) {
        if (offset + size > len) size = len - offset;
        memcpy(buf, dataBlock + offset, size);
    } else
        size = 0;

    return size;
}

int fs_readdir(const char *path, void *buf, filler_t filler, off_t offset, fuseFInfo_t *fi)
{
    (void) offset;
    (void) fi;
    int i;
    int lim = DIR_ENTR_IN_BLOCK;

    inode_t * inode = getInodeByPath(path);
    dirEntry_t * de = (dirEntry_t *) getDataBlock(inode->blocks[0]);

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    for(i = 0; i < lim; i++) {
        if(de[i].inode != 0 ) filler(buf, de[i].name, NULL, 0);
    }

    return 0;
}


int fs_rename(const char *from, const char *to)
{
    int index;
    if((index = findFileIndex(from)) == -1)
        return -ENOENT;

    char * dir = getDirname(from);
    char * oldName = getBasename(from);
    char * newName = getBasename(to);

    printf("%s %s %s\n", dir, oldName, newName);
    printf("rename %s %s \n", from, to);
    inode_t * inode;
    if((inode = getInodeByPath(dir)) == NULL) return -ENOENT;
    dirEntry_t * de = getDirEntry(getDataBlock(inode->blocks[0]), oldName, strlen(oldName));
    if(de == NULL) return -ENOENT;
    printf("%s\n", de->name);
    strcpy(de->name, newName);
    printf("%s\n", de->name);

    free(dir);
    free(oldName);
    free(newName);
    return 0;
}

//- rgetCharIndex(from)

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

int rgetCharIndex(const char * str, char target)
{
    const char *ptr = strrchr(str, target);
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
    sb->dataBlockSize = DATA_BLOCK_SIZE;
    sb->firstBlock = sizeof(superblock_t) + sizeof(inode_t) * sb->nInodes;
    //printf("superblock size = %d, inode size = %d, memory block size = %d, dirEnt size = %d\n\n\n\n\n", sizeof(superblock_t), sizeof(inode_t), sb->dataBlockSize, sizeof(dirEntry_t));

    inode_t * inode = (inode_t *) (vdev + sizeof(superblock_t));
    inode->mode = S_IFDIR | 0755;
    inode->nLinks = 2;
    inode->blocks[0] = 0;//sb->firstBlock;
    inode->size = sb->dataBlockSize;

    dirEntry_t* de = (dirEntry_t *) getDataBlock(inode->blocks[0]);
    de->inode = 1;
    strcpy(de->name, "hello");    

    inode = getInode(1);
    inode->mode = S_IFREG | 0644;
    inode->nLinks = 1;
    char helloStr[] = "Hello world!";
    inode->size = strlen(helloStr);
    inode->blocks[0] = 1; 
    void* memBlock = getDataBlock(inode->blocks[0]);
    memcpy(memBlock, helloStr, strlen(helloStr));    
}

void test()
{
    char * data = (char *) malloc(500);
    inode_t* inode ;//= getInodeByPath("/");
    if((inode = getInodeByPath("/hello")) == NULL) exit(1);
    memcpy(data, getDataBlock(1) , inode->size);
    data[inode->size] = '\0';
    //printf("size %d, mode %x, nlinks %d, data \"%s\"\n", inode->size, inode->mode, inode->nLinks, data); 
    exit(1);     
}

inode_t* getInode(uint64_t id)
{
    return (inode_t*)(vdev + sizeof(superblock_t) + id * sizeof(inode_t));
}

void* getDataBlock(uint64_t id)
{
    return vdev + sb->firstBlock + id * sb->dataBlockSize;
}

inode_t* getInodeByPath(const char *path)
{
    inode_t* inode = getInode(0);
    int nameStart = 1;
    int nameEnd = 1;

    if(! strcmp(path, "/")) return inode;

    while(1) {
        if( (nameEnd = getCharIndex(path + nameStart, '/')) == -1) nameEnd = strlen(path);
        dirEntry_t * de = getDirEntry(getDataBlock(inode->blocks[0]), path + nameStart, nameEnd - nameStart);
        inode = (de == NULL) ? NULL : getInode(de->inode);
        if(de == NULL || path[nameEnd] == '\0') break;
        nameStart = nameEnd;        
    }

    return inode;
}

dirEntry_t* getDirEntry(void * block, const char* name, uint16_t nameLen)
{
    
    int i;
    int lim = (sb->dataBlockSize / sizeof(dirEntry_t)) - 1;
    dirEntry_t* de = (dirEntry_t *) block;

    for(i = 0; i < lim; i++) {
        if(strlen(de[i].name) == nameLen && ! strncmp(de[i].name, name, nameLen)) {
            return de;
        }
    }
    
    return NULL;
}

char * getBasename(const char * path)
{
    int start = rgetCharIndex(path, '/') + 1;
    int len = strlen(path) - start + 1;

    if(len == 0) return NULL;
    char * base = (char *) malloc(len);
    strncpy(base, path + start, len);
    base[len - 1] = '\0';
    return base; 
}

char * getDirname(const char * path)
{
    int len;
    len  = rgetCharIndex(path, '/') + 2;
    printf("len%d\n", len);
    char * dir = (char *) malloc(strlen(path));
    strcpy(dir, path);
    dirname(dir);
    return dir;     
}