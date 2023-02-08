#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "ufs.h"

int main(int argc, char const *argv[]) {

    const char *fs_fname = argv[1];
    unsigned fs_nbytes = atoi(argv[2]);

    /*Calculate block numbers*/
    struct ufs_superblock sb = {.s_magic = {'u', 'f', 's', '\0'}};

    sb.s_nblock = fs_nbytes / UFS_BLOCK_SIZE;
    sb.s_nbitmap_block = fs_nbytes / (UFS_BLOCK_SIZE * UFS_BLOCK_SIZE * 8) + 1;

    unsigned nblock_rest = sb.s_nbitmap_block - 2 - sb.s_nbitmap_block;
    sb.s_ninodes_block = nblock_rest / 10 + 1;
    sb.s_ndata_block = nblock_rest - sb.s_ninodes_block;

    /*Fill data in fs file*/
    int fd = open(fs_fname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        perror("failed open fs file:");
        return -1;
    }

    return 0;
}
