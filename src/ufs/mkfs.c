#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "ufs.h"

#define BITMAP_SET(blkno, bitmap)                                              \
    *(char *)((bitmap) + ((blkno) / 8)) |= (1 << ((blkno) % 8))

int main(int argc, char const *argv[]) {

    const char *fs_fname = argv[1];
    unsigned fs_nbytes = atoi(argv[2]);

    /**
     * 1. Calculate block numbers
     */
    struct ufs_superblock sb = {.s_magic = {'u', 'f', 's', '\0'}};

    sb.s_blocks = fs_nbytes / UFS_BLOCK_SIZE;
    sb.s_bitmap_blocks = fs_nbytes / (UFS_BLOCK_SIZE * UFS_BLOCK_SIZE * 8) + 1;

    unsigned rest_blocks = sb.s_blocks - 2 - sb.s_bitmap_blocks;
    sb.s_inode_blocks = rest_blocks / 10 + 1;
    sb.s_data_blocks = rest_blocks - sb.s_inode_blocks;

    // Check block numbers
    if (sb.s_blocks <= sb.s_bitmap_blocks || sb.s_blocks <= sb.s_inode_blocks ||
        sb.s_blocks <= sb.s_data_blocks || sb.s_data_blocks == 0 ||
        sb.s_blocks !=
            (sb.s_bitmap_blocks + sb.s_inode_blocks + sb.s_data_blocks + 2)) {
        printf("file size too small:\n");

        printf("s_blocks: %u\ns_bitmap_blocks: %u\ns_inode_blocks: "
               "%u\ns_data_blocks: %u\n",
               sb.s_blocks, sb.s_bitmap_blocks, sb.s_inode_blocks,
               sb.s_data_blocks);
        return -1;
    }

    /**
     * 2. Fill data in fs file
     */
    int fd = open(fs_fname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        perror("failed open fs file:");
        return -1;
    }

    if (ftruncate(fd, fs_nbytes)) {
        perror("failed truncate fs file:");
        return -1;
    }

    void *buf =
        mmap(NULL, fs_nbytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buf == MAP_FAILED) {
        perror("failed mmap:");
        return -1;
    }
    close(fd);
    memset(buf, 0,
           UFS_BLOCK_SIZE * (2 + sb.s_bitmap_blocks + sb.s_inode_blocks));

    memcpy(buf + UFS_BLOCK_SIZE, &sb, sizeof(sb));

    void *bitmap = buf + 2 * UFS_BLOCK_SIZE;
    unsigned blocks_in_use = 2 + sb.s_bitmap_blocks;

    for (unsigned i = 0; i < blocks_in_use; ++i) {
        BITMAP_SET(i, bitmap);
    }

    printf("mkfs.ufs done!\n");
    return 0;
}
