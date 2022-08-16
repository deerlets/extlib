#include "apix.h"
#include "apix-inl.h"
#include "autobufx.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <listx.h>
#include <logx.h>

struct apicore *apicore_new()
{
    struct apicore *core = malloc(sizeof(struct apicore));
    INIT_LIST_HEAD(&core->sinkfds);
    INIT_LIST_HEAD(&core->sinks);
    return core;
}

void apicore_destroy(struct apicore *core)
{
    struct sinkfd *pos, *n;
    list_for_each_entry_safe(pos, n, &core->sinkfds, node_core) {
        list_del_init(&pos->node_core);
        free(pos);
    }

    struct apisink *pos_sink, *n_sink;
    list_for_each_entry_safe(pos_sink, n_sink, &core->sinks, node) {
        apicore_del_sink(core, pos_sink);
        apisink_fini(pos_sink);
    }

    free(core);
}

int apicore_poll(struct apicore *core, int timeout)
{
    int cnt = 0;
    struct list_head *pos;
    list_for_each(pos, &core->sinks)
        cnt++;

    struct apisink *pos_sink;
    list_for_each_entry(pos_sink, &core->sinks, node) {
        if (pos_sink->ops.poll(pos_sink, timeout / cnt) != 0) {
            LOG_ERROR("%s", strerror(errno));
        }
    }

    struct sinkfd *pos_fd;
    list_for_each_entry(pos_fd, &core->sinkfds, node_core) {
        if (autobuf_used(pos_fd->rxbuf)) {
            LOG_INFO("%s", autobuf_read_pos(pos_fd->rxbuf));
            autobuf_read_head(pos_fd->rxbuf, autobuf_used(pos_fd->rxbuf));
            assert(pos_fd->sink->ops.send);
            pos_fd->sink->ops.send(pos_fd->sink, pos_fd->fd, "alive", 5);
        }
    }

    return 0;
}

int apicore_open(struct apicore *core, const char *name, const void *addr)
{
    struct apisink *pos;
    list_for_each_entry(pos, &core->sinks, node) {
        if (strcmp(pos->name, name) == 0) {
            assert(pos->ops.open);
            return pos->ops.open(pos, addr);
        }
    }
    return -1;
}

int apicore_close(struct apicore *core, int fd)
{
    struct sinkfd *sinkfd = to_sinkfd(&core->sinkfds, fd);
    if (sinkfd == NULL)
        return -1;
    if (sinkfd->sink && sinkfd->sink->ops.close)
        sinkfd->sink->ops.close(sinkfd->sink, fd);
    return 0;
}

void apisink_init(struct apisink *sink, const char *name, apisink_ops_t ops)
{
    assert(strlen(name) < APISINK_NAME_SIZE);
    INIT_LIST_HEAD(&sink->sinkfds);
    INIT_LIST_HEAD(&sink->node);
    snprintf(sink->name, sizeof(sink->name), "%s", name);
    sink->ops = ops;
    sink->core = NULL;
}

void apisink_fini(struct apisink *sink)
{
    struct sinkfd *pos, *n;
    list_for_each_entry_safe(pos, n, &sink->sinkfds, node_sink)
        sinkfd_destroy(pos);

    if (sink->core)
        apicore_del_sink(sink->core, sink);
}

int apicore_add_sink(struct apicore *core, struct apisink *sink)
{
    struct apisink *pos;
    list_for_each_entry(pos, &core->sinks, node) {
        if (strcmp(sink->name, pos->name) == 0)
            return -1;
    }

    list_add(&core->sinks, &sink->node);
    sink->core = core;
    return 0;
}

void apicore_del_sink(struct apicore *core, struct apisink *sink)
{
    list_del_init(&sink->node);
    sink->core = NULL;
}

struct sinkfd *sinkfd_new()
{
    struct sinkfd *sinkfd = malloc(sizeof(struct sinkfd));
    sinkfd->fd = 0;
    sinkfd->listen = 0;
    sinkfd->txbuf = autobuf_new(0);
    sinkfd->rxbuf = autobuf_new(0);
    sinkfd->sink = NULL;
    INIT_LIST_HEAD(&sinkfd->node_sink);
    INIT_LIST_HEAD(&sinkfd->node_core);
    return sinkfd;
}

void sinkfd_destroy(struct sinkfd *sinkfd)
{
    sinkfd->fd = 0;
    autobuf_delete(sinkfd->txbuf);
    autobuf_delete(sinkfd->rxbuf);
    sinkfd->sink = NULL;
    list_del_init(&sinkfd->node_sink);
    list_del_init(&sinkfd->node_core);
    free(sinkfd);
}

struct sinkfd *to_sinkfd(struct list_head *sinkfds, int fd)
{
    struct sinkfd *pos, *n;
    list_for_each_entry_safe(pos, n, sinkfds, node_core) {
        if (pos->fd == fd)
            return pos;
    }
    return NULL;
}
