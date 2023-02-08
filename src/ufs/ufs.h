#ifndef UFS_H
#define UFS_H

#define UFS_BLOCK_SIZE 512

#define UFS_NDIR_BLOCK 12
#define UFS_NIND_BLOCK 1

struct ufs_superblock {
    unsigned s_magic[4];
    unsigned s_nblock;
    unsigned s_nbitmap_block;
    unsigned s_ninodes_block;
    unsigned s_ndata_block;
};

struct ufs_inode {
    unsigned i_type;
    unsigned i_no;
    unsigned i_filesz;
    unsigned i_addrs[UFS_NDIR_BLOCK];
    unsigned i_addr_ind;
};

#endif