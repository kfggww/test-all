#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define PGSIZE 4096
#define RANGE_BETWEEN(c, a, b) ((unsigned long)(c) >= (unsigned long)(a) && (unsigned long)(c) < (unsigned long)(b))
#define OFFSET_FROM(base, offset) ((unsigned long)(base) + (unsigned long)(offset))
#define HALF_OF(s) (((unsigned long)(s)) / 2)
#define ALLOC_BLOCK()                                                             \
    ({                                                                            \
        struct buddy_block *__block = (struct buddy_block *)buddy_block_buf_mark; \
        buddy_block_buf_mark = (void *)(__block + 1);                             \
        __block;                                                                  \
    })

#define buddy_list_for_earch(pos, head) \
    for (struct buddy_block *pos = (head)->next; pos != (head); pos = pos->next)

enum buddy_block_state
{
    BLK_FREE,
    BLK_USED,
    BLK_SPLIT,
};

struct buddy_block
{
    void *start;
    size_t size;
    enum buddy_block_state state;
    struct buddy_block *next;
    struct buddy_block *prev;
    struct buddy_block *parent;
    struct buddy_block *left;
    struct buddy_block *right;
};

static struct buddy_block buddy_block_head;
static void *buddy_block_buf;
static void *buddy_block_buf_mark;

static void buddy_block_add_prev(struct buddy_block *head, struct buddy_block *node)
{
    node->next = head;
    node->prev = head->prev;
    head->prev->next = node;
    head->prev = node;
}

static void buddy_split_block(struct buddy_block *root)
{
    struct buddy_block *left = ALLOC_BLOCK();
    struct buddy_block *right = ALLOC_BLOCK();
    // make sure buddy blocks buf uses no more than one page
    assert((unsigned long)buddy_block_buf_mark - (unsigned long)buddy_block_buf < PGSIZE);

    root->state = BLK_SPLIT;
    root->left = left;
    root->right = right;

    left->next = left->prev = left;
    left->parent = root;
    left->left = left->right = NULL;
    left->state = BLK_FREE;
    left->start = root->start;
    left->size = root->size / 2;

    right->next = right->prev = right;
    right->parent = root;
    right->left = right->right = NULL;
    right->state = BLK_FREE;
    right->start = root->start + root->size / 2;
    right->size = root->size / 2;
}

static void *buddy_block_alloc(struct buddy_block *root, size_t size)
{
    if (root == NULL)
        return NULL;

    void *buf = NULL;
    switch (root->state)
    {
    case BLK_USED:
        return NULL;
    case BLK_SPLIT:
        buf = buddy_block_alloc(root->left, size);
        if (buf != NULL)
            return buf;
        return buddy_block_alloc(root->right, size);
    case BLK_FREE:
        if (2 * size > root->size)
        {
            root->state = BLK_USED;
            return root->start;
        }
        else
        {
            buddy_split_block(root);
            buf = buddy_block_alloc(root->left, size);
            if (buf != NULL)
                return buf;
            return buddy_block_alloc(root->right, size);
        }
    default:
        return buf;
    }
}

void *buddy_alloc(size_t size)
{
    if (size > PGSIZE)
        return NULL;

    void *alloc_buf = NULL;

    buddy_list_for_earch(pos, &buddy_block_head)
    {
        alloc_buf = buddy_block_alloc(pos, size);
        if (alloc_buf != NULL)
            return alloc_buf;
    }

    struct buddy_block *root = ALLOC_BLOCK();
    buddy_block_add_prev(&buddy_block_head, root);

    root->start = malloc(PGSIZE);
    assert(root->start != NULL);
    root->state = BLK_FREE;
    root->size = PGSIZE;
    root->parent = NULL;
    root->left = root->right = NULL;

    return buddy_block_alloc(root, size);
}

static void buddy_blkcpy_top2free(struct buddy_block *blk_free)
{
    struct buddy_block *blk_top = (struct buddy_block *)buddy_block_buf_mark - 1;

    *blk_free = *blk_top;
    if (blk_top == blk_top->parent->left)
    {
        blk_top->parent->left = blk_free;
    }
    else
    {
        blk_top->parent->right = blk_free;
    }

    buddy_block_buf_mark = (void *)blk_top;
}

static int buddy_block_free(struct buddy_block *root, void *p)
{
    if (root == NULL)
        return -1;

    int res = -1;
    switch (root->state)
    {
    case BLK_FREE:
        return -1;
    case BLK_USED:
        if (root->start != p)
            return -1;
        else
        {
            root->state = BLK_FREE;
            return 0;
        }
    case BLK_SPLIT:
        if (p < (void *)((unsigned long)root->start + HALF_OF(root->size)))
            res = buddy_block_free(root->left, p);
        else
            res = buddy_block_free(root->right, p);
        break;
    default:
        return -1;
    }

    if (res)
        return res;

    if (root->left != NULL && root->left->state == BLK_FREE && root->right != NULL && root->right->state == BLK_FREE)
    {
        buddy_blkcpy_top2free(root->left > root->right ? root->left : root->right);
        buddy_blkcpy_top2free(root->left < root->right ? root->left : root->right);
        root->left = root->right = NULL;
        root->state = BLK_FREE;
    }

    return 0;
}

int buddy_free(void *p)
{
    int res = -1;
    buddy_list_for_earch(pos, &buddy_block_head)
    {
        if (RANGE_BETWEEN(p, pos->start, OFFSET_FROM(pos->start, PGSIZE)))
        {
            res = buddy_block_free(pos, p);
            break;
        }
    }
    return res;
}

static void buddy_block_info(struct buddy_block *root)
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
    buddy_list_for_earch(pos, &buddy_block_head)
    {
        buddy_block_info(pos);
    }
}

void buddy_init()
{
    // buddy_block_head act as a guard
    buddy_block_head.next = buddy_block_head.prev = &buddy_block_head;
    buddy_block_head.state = BLK_USED;
    buddy_block_head.start = NULL;
    buddy_block_head.size = 0;
    buddy_block_head.parent = NULL;
    buddy_block_head.left = buddy_block_head.right = NULL;

    // allocate one page size of memory to store buddy blocks
    // we assume that one page is enough for simplicity
    buddy_block_buf = malloc(PGSIZE);
    buddy_block_buf_mark = buddy_block_buf;
    assert(buddy_block_buf != NULL);
}

void buddy_fini()
{
    buddy_list_for_earch(root, &buddy_block_head)
    {
        assert(root != NULL && root->start != NULL && root->state == BLK_FREE);
        free(root->start);
    }

    free(buddy_block_buf);
}