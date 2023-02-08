#ifndef UFS_H
#define UFS_H

#define UFS_BLOCK_SIZE 512

#define UFS_NDIR_BLOCK 12
#define UFS_NIND_BLOCK 1
#define UFS_NADDR_BLOCK (UFS_NDIR_BLOCK + UFS_NIND_BLOCK)

struct ufs_superblock {
    unsigned s_magic[4];
    unsigned s_blocks;
    unsigned s_bitmap_blocks;
    unsigned s_inode_blocks;
    unsigned s_data_blocks;
};

struct ufs_inode {
    unsigned i_type;
    unsigned i_no;
    unsigned i_filesz;
    unsigned i_addr[UFS_NADDR_BLOCK];
};

#endif