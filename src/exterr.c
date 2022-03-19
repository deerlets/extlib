#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "extlist.h"
#include "exterr.h"

#define NR_SERVICE 919
#define tag_hash_fn(tag) (tag % NR_SERVICE)

struct exterr {
    int __errno;
    const char *errmsg;
    struct hlist_node hnode;
    struct list_head node;
};

struct hlist_head err_hlist[NR_SERVICE];
LIST_HEAD(err_list);

static unsigned int calc_tag(const void *buf, size_t len)
{
    unsigned int retval = ~0;
    char *tag = (char *)&retval;

    for (int i = 0; i < len; i++)
        tag[i % 4] ^= *((unsigned char *)buf + i);

    return retval;
}

static struct exterr *find_exterr(int __errno)
{
    char buffer[32] = {0};
    snprintf(buffer, sizeof(buffer), "%d", __errno);
    unsigned int tag = calc_tag(buffer, strlen(buffer));

    struct hlist_head *head = &err_hlist[tag_hash_fn(tag)];
    struct exterr *pos;
    hlist_for_each_entry(pos, head, hnode) {
        if (pos->__errno == __errno)
            return pos;
    }

    return NULL;
}

void exterr_unregister_all()
{
    struct exterr *pos, *n;
    list_for_each_entry_safe(pos, n, &err_list, node) {
        hlist_del_init(&pos->hnode);
        list_del_init(&pos->node);
        free(pos);
    }
}

int exterr_register(int __errno, const char *errmsg)
{
    if (find_exterr(__errno))
        return -1;

    struct exterr *err = (struct exterr *)malloc(sizeof(*err));
    err->__errno = __errno;
    err->errmsg = errmsg;
    INIT_HLIST_NODE(&err->hnode);
    INIT_LIST_HEAD(&err->node);

    char buffer[32] = {0};
    snprintf(buffer, sizeof(buffer), "%d", __errno);
    unsigned int tag = calc_tag(buffer, strlen(buffer));
    struct hlist_head *head = &err_hlist[tag_hash_fn(tag)];
    hlist_add_head(&err->hnode, head);
    list_add(&err->node, &err_list);
    return 0;
}

const char *ext_strerr(int __errno)
{
    struct exterr *err = find_exterr(__errno);
    if (err) return err->errmsg;
    return "Unknown errno";
}
