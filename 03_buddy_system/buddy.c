#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define PGSIZE 4096

enum vm_block_state
{
    BLK_FREE,
    BLK_USED,
    BLK_SPLIT,
};

struct buddy_vm_block
{
    void *start;
    size_t size;
    enum vm_block_state state;
    struct buddy_vm_block *next;
    struct buddy_vm_block *prev;
    struct buddy_vm_block *left;
    struct buddy_vm_block *right;
};

static struct buddy_vm_block vmblk_head;
static void *buddy_buf;
static void *buddy_buf_mark;

void buddy_split_block(struct buddy_vm_block *root)
{
    struct buddy_vm_block *left = (struct buddy_vm_block *)buddy_buf_mark;
    buddy_buf_mark = (void *)((unsigned long)buddy_buf_mark + sizeof(struct buddy_vm_block));
    struct buddy_vm_block *right = (struct buddy_vm_block *)buddy_buf_mark;
    buddy_buf_mark = (void *)((unsigned long)buddy_buf_mark + sizeof(struct buddy_vm_block));

    root->state = BLK_SPLIT;
    root->left = left;
    root->right = right;

    left->next = left->prev = left;
    left->left = left->right = NULL;
    left->state = BLK_FREE;
    left->start = root->start;
    left->size = root->size / 2;

    right->next = right->prev = right;
    right->left = right->right = NULL;
    right->state = BLK_FREE;
    right->start = root->start + root->size / 2;
    right->size = root->size / 2;
}

void *buddy_alloc1(struct buddy_vm_block *root, size_t size)
{

    void *buf = NULL;
    switch (root->state)
    {
    case BLK_USED:
        return NULL;
    case BLK_SPLIT:
        buf = buddy_alloc1(root->left, size);
        if (buf != NULL)
            return buf;
        return buddy_alloc1(root->right, size);
    case BLK_FREE:
        if (2 * size > root->size)
        {
            root->state = BLK_USED;
            return root->start;
        }
        else
        {
            buddy_split_block(root);
            buf = buddy_alloc1(root->left, size);
            if (buf != NULL)
                return buf;
            return buddy_alloc1(root->right, size);
        }
    default:
        return buf;
    }
}

void list_add_prev(struct buddy_vm_block *head, struct buddy_vm_block *node)
{
    node->next = head;
    node->prev = head->prev;
    head->prev->next = node;
    head->prev = node;
}

void buddy_init()
{
    vmblk_head.next = vmblk_head.prev = &vmblk_head;
    vmblk_head.state = BLK_USED;
    vmblk_head.start = NULL;
    vmblk_head.size = 0;
    vmblk_head.left = vmblk_head.right = NULL;

    buddy_buf = malloc(PGSIZE);
    buddy_buf_mark = buddy_buf;
    assert(buddy_buf != NULL);
}

void buddy_fini()
{
}

void *buddy_alloc(size_t size)
{
    if (size > PGSIZE)
        return NULL;

    void *alloc_buf = NULL;

    for (struct buddy_vm_block *pos = vmblk_head.next; pos != &vmblk_head; pos = pos->next)
    {
        alloc_buf = buddy_alloc1(pos, size);
        if (alloc_buf != NULL)
            return alloc_buf;
    }

    struct buddy_vm_block *new_root = (struct buddy_vm_block *)buddy_buf_mark;
    buddy_buf_mark = (void *)((unsigned long)buddy_buf_mark + sizeof(struct buddy_vm_block));
    list_add_prev(&vmblk_head, new_root);

    new_root->start = malloc(PGSIZE);
    assert(new_root->start != NULL);
    new_root->state = BLK_FREE;
    new_root->size = PGSIZE;
    new_root->left = new_root->right = NULL;

    return buddy_alloc1(new_root, size);
}

int buddy_free(void *p)
{

    return 0;
}

void buddy_block_info(struct buddy_vm_block *root)
{
    if (root == NULL)
        return;

    const char *states[] = {"Free", "Used", "Split"};
    printf("{start: %p, size: %ld, state: %s}\n", root->start, root->size, states[root->state]);

    buddy_block_info(root->left == root ? NULL : root->left);
    buddy_block_info(root->right == root ? NULL : root->right);
}

void buddy_info()
{
    for (struct buddy_vm_block *pos = vmblk_head.next; pos != &vmblk_head; pos = pos->next)
    {
        buddy_block_info(pos);
    }
}