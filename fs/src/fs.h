#ifndef FS_RMFCDDLE
#define FS_RMFCDDLE

#define FUSE_USE_VERSION 26 

#include <stdio.h>         
#include <stdlib.h>         
#include <string.h>  

#include <errno.h>         
#include <fcntl.h>  /* manipulate file descriptor */
#include <sys/types.h>  /* mode_t */
#include <unistd.h> /* access funcion mode macro */
#include <stdint.h>

#include <fuse.h>          

#include "vdev.h"
#include "helpers.h"


#define MAX_FILES_COUNT 20
#define MAX_FPATH_LENGTH 500


#define MAX_NAME_LEN    255
#define INODES_VEC sizeof(superblock_t)
#define DATA_BLOCK_SIZE 1024
#define DIR_ENTR_IN_BLOCK ((sb->dataBlockSize / sizeof(dirEntry_t)) - 1)
#define AVAILABLE_INODE 0
#define AVAILABLE_BLOCK 1

typedef enum {BLOCK_OP, INODE_OP} operand_t;
typedef enum {true = 1, false = 0} boolean;

typedef char content_t;
typedef struct fuse_file_info fuseFInfo_t;
typedef fuse_fill_dir_t filler_t;
typedef unsigned char byte_t;

typedef struct FileInfo {
    char  path[500];
    mode_t mode; 
    content_t* content;
    uint64_t nLinks;
    long size;
    int isUsed;
} fileInfo_t;

typedef struct Superblock {
    uint32_t fsSize;
    uint32_t nInodes;
    uint32_t nBlocks;
    uint32_t firstBlock;
    uint32_t dataBlockSize;
} superblock_t;

typedef struct Inode {
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


fileInfo_t inodes[MAX_FILES_COUNT];
//void* vdev; 
superblock_t* sb;

void fs_init();

int fs_getattr(const char *path, struct stat *stbuf);
int fs_mknod(const char *path, mode_t mode, dev_t rdev);
int fs_unlink(const char *path);
int fs_readdir(const char *path, void *buf, filler_t filler, off_t offset, fuseFInfo_t *fi);
int fs_open(const char *path, fuseFInfo_t *fi);
int fs_read(const char *path, char *buf, size_t size, off_t offset, fuseFInfo_t *fi);
int fs_rename(const char *from, const char *to);
int fs_chmod(const char *path, mode_t newMode);
int fs_truncate(const char *path, off_t size);
int fs_access(const char *path, int functMode);
int fs_write(const char *path, const char *buf, size_t size,off_t offset, fuseFInfo_t *fi);

dirEntry_t* getDirEntry(void * block, const char* name, uint16_t nameLen);

long getUnusedNodeIndex();

void initNode(long index, const char *path, mode_t mode);
inode_t* getInodeByPath(const char *path);

inode_t* getInode(uint64_t id);
void* getDataBlock(uint64_t id);


uint64_t getUnused(operand_t operand);
void setUsed(operand_t operand, uint64_t id, uint8_t val);

uint8_t addDirEntry(const char * dir, const char* name, uint64_t inodeId);
uint8_t createFile(const char * path, mode_t mode);
uint64_t makeInode(mode_t mode);

#endif /* end of include guard: FS_RMFCDDLE */
