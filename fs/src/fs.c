#include "fs.h"

int fs_getattr(const char *path, struct stat *stbuf)
{
    //printf("fs_getattr:path = %s\n", path);
    inode_t * inode;

    memset(stbuf, 0, sizeof(struct stat));
    if( (inode = getInodeByPath(path)) != NULL) {
        stbuf->st_mode = inode->mode;
        stbuf->st_nlink = inode->nLinks;
        stbuf->st_size = inode->size;
    } else 
        return -ENOENT;
       
    return 0;
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
    (void) fi;  
    //printf("~~Write:path = %s, size = %d offset %d ", path, size, offset);

    inode_t * inode;
    if((inode = getInodeByPath(path)) == NULL) return -ENOENT;
    void* dataBlock;
    if ((dataBlock = getDataBlock(inode->blocks[0])) == NULL) return -ENOENT;
    
    strcpy(dataBlock + offset, buf);
    inode->size += size ;

    
    return size;
}

int fs_access(const char *path, int functMode)
{
    //printf("access: path %s \n",path);
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
    //printf("Open: path %s ",path);
    if(getInodeByPath(path) == NULL) return -ENOENT;
    //printf("+\n" );
    return 0;
}

int fs_read(const char *path, char *buf, size_t size, off_t offset, fuseFInfo_t *fi)
{
    size_t len;
    (void) fi;

    //printf("read: path %s size %d offs %d\n", path, size, offset);


    inode_t * inode;
    if((inode = getInodeByPath(path)) == NULL) return -ENOENT;
    void* dataBlock = getDataBlock(inode->blocks[0]);
    if(dataBlock == NULL) return -ENOENT;


    //printf("%d\n", inode->size);
    len = strlen(dataBlock);
    if(offset < len) {
        if (offset + size > len) size = len - offset;
        memcpy(buf, dataBlock + offset, size);
        //memcpy(buf, dataBlock + offset, size);

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
    inode_t * inode;
    
    char * dir = getDirname(from);
    if((inode = getInodeByPath(dir)) == NULL) {
        free(dir);
        return -ENOENT;
    }
    char * oldName = getBasename(from);
    dirEntry_t * de = getDirEntry(getDataBlock(inode->blocks[0]), oldName, strlen(oldName));
    if(de == NULL) {
        free(dir);
        free(oldName);
        return -ENOENT;
    }
    char * newName = getBasename(to);

    strcpy(de->name, newName);

    free(dir);
    free(oldName);
    free(newName);
    return 0;
}

int fs_chmod(const char *path, mode_t newMode)
{
    inode_t * inode;
    //printf("fs_chmod: path %s \n",path);
    if((inode = getInodeByPath(path)) == NULL) return -ENOENT;
    inode->mode = newMode;
    return 0;
}

int fs_truncate(const char *path, off_t size)
{
    inode_t * inode;
    //printf("fs_truncate: path %s \n",path);
    if((inode = getInodeByPath(path)) == NULL) return -ENOENT;
    void* dataBlock = getDataBlock(inode->blocks[0]);
    if(dataBlock == NULL) return -ENOENT;

    inode->size = 0;
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

uint64_t getAvailable(int mode);
void tagAvailable(int mode, uint64_t id);

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
    inode->blocks[0] = 2;//sb->firstBlock;
    inode->size = sb->dataBlockSize;

    dirEntry_t* de = (dirEntry_t *) getDataBlock(inode->blocks[0]);
    de->inode = 1;
    strcpy(de->name, "hello");    

    inode = getInode(1);
    inode->mode = S_IFREG | 0644;
    inode->nLinks = 1;
    char helloStr[] = "Hello world!";
    inode->size = strlen(helloStr);
    inode->blocks[0] = 3; 
    void* memBlock = getDataBlock(inode->blocks[0]);
    memcpy(memBlock, helloStr, strlen(helloStr));    


    //void * bitmapBlock = getDataBlock(0)
    byte_t * b = (byte_t *) getDataBlock(0);
    tagAvailable(AVAILABLE_INODE, 0);
    tagAvailable(AVAILABLE_INODE, 3);
    //*b |= 1;
    //*b |= 1 << 1;
    printf("%d\n", *b);
    printf("%d\n", getAvailable(AVAILABLE_INODE));
    exit(0);
}

void tagAvailable(int mode, uint64_t id)
{
    uint8_t nbit = id % 8;
    uint64_t nbyte = id / 8;
    byte_t * bitmap;

    switch(mode) {
        case AVAILABLE_INODE:
            bitmap = (byte_t *) (getDataBlock(0) + nbyte);
            break;
        case AVAILABLE_BLOCK:
            bitmap = (byte_t *) (getDataBlock(1) + nbyte);
            break;     
    }
    *bitmap |= 1 << nbit;
}

void initInode(int mode, uint64_t id)
{
}

uint64_t getAvailable(int mode) 
{
    void * bitmapBlock;
    byte_t * b;
    int bitNum, byteNum;

    switch(mode) {
        case AVAILABLE_INODE:
            bitmapBlock = getDataBlock(0);
            break;
        case AVAILABLE_BLOCK:
            bitmapBlock = getDataBlock(1);
            break;     
    }
    b = (byte_t *) bitmapBlock;

    for(byteNum = 0; byteNum < sb->dataBlockSize; byteNum++) {
        for(bitNum = 0; bitNum < 8; bitNum++) {
           
            if(! (*b & (1<<bitNum)) ) return byteNum * 8 + bitNum; 
        }
    }
    return 0;
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

inode_t* getInode(uint64_t id)
{
    return (inode_t*)(vdev + sizeof(superblock_t) + id * sizeof(inode_t));
}

void* getDataBlock(uint64_t id)
{
    return vdev + sb->firstBlock + id * sb->dataBlockSize;
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
