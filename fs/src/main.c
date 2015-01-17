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

#include "vdev.h"
#include "fs.h"




static struct fuse_operations fs_operations = {
    .getattr    = fs_getattr,
    .readdir    = fs_readdir,

    .write      = fs_write,
    .open       = fs_open,
    .read       = fs_read,

    .access     = fs_access,

    .mknod      = fs_mknod,
    //.unlink     = fs_unlink,

    .rename     = fs_rename,                           
    .chmod      = fs_chmod,                           
    .truncate   = fs_truncate, 
};

int main(int argc, char *argv[])
{
    int res;
    fs_init();
    
    res = fuse_main(argc, argv, &fs_operations, NULL); 
    
    free(vdev);
    return res;
}
