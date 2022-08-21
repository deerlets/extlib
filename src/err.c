#include "err.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

#define NR_SERVICE 919
#define tag_hash_fn(tag) (tag % NR_SERVICE)

struct errx {
    int __errno;
    const char *errmsg;
    struct hlist_node hnode;
    struct list_head node;
};

static struct hlist_head err_hlist[NR_SERVICE];
static LIST_HEAD(err_list);

static unsigned int calc_tag(const void *buf, size_t len)
{
    unsigned int retval = ~0;
    char *tag = (char *)&retval;

    for (int i = 0; i < len; i++)
        tag[i % 4] ^= *((unsigned char *)buf + i);

    return retval;
}

static struct errx *find_errx(int __errno)
{
    char buffer[32] = {0};
    snprintf(buffer, sizeof(buffer), "%d", __errno);
    unsigned int tag = calc_tag(buffer, strlen(buffer));

    struct hlist_head *head = &err_hlist[tag_hash_fn(tag)];
    struct errx *pos;
    hlist_for_each_entry(pos, head, hnode) {
        if (pos->__errno == __errno)
            return pos;
    }

    return NULL;
}

#define ERRX_INIT_GEN(name, errmsg) errx_register(ERRX_##name, errmsg);
static __attribute__((constructor))
void errx_init() { ERRX_ERRNO_MAP(ERRX_INIT_GEN) }
#undef ERRX_INIT_GEN

static __attribute__((destructor))
void errx_fini()
{
    struct errx *pos, *n;
    list_for_each_entry_safe(pos, n, &err_list, node) {
        hlist_del_init(&pos->hnode);
        list_del_init(&pos->node);
        free(pos);
    }
}

int errx_register(int __errno, const char *errmsg)
{
    if (find_errx(__errno))
        return -1;

    struct errx *err = (struct errx *)malloc(sizeof(*err));
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

const char *errx_strerr(int __errno)
{
    struct errx *err = find_errx(__errno);
    if (err) return err->errmsg;
    return "Unknown errno";
}
