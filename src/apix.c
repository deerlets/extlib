#include "apix.h"
#include "apix-inl.h"
#include "autobufx.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "listx.h"
#include "logx.h"
#include "srrpx.h"
#include "stddefx.h"

static void parse_packet(struct apicore *core, struct sinkfd *sinkfd)
{
    struct srrp_packet pac = {0};

    while (autobuf_used(sinkfd->rxbuf)) {
        int nr = srrp_read_one_packet(
            autobuf_read_pos(sinkfd->rxbuf), autobuf_used(sinkfd->rxbuf), &pac);
        if (nr == -1) {
            LOG_INFO("%s", autobuf_read_pos(sinkfd->rxbuf));
            autobuf_read_advance(
                sinkfd->rxbuf, strlen(autobuf_read_pos(sinkfd->rxbuf)) + 1);
            continue;
        }
        autobuf_read_advance(sinkfd->rxbuf, nr);

        if (pac.leader == SRRP_REQUEST_LEADER) {
            struct api_request *req = malloc(sizeof(*req));
            memset(req, 0, sizeof(*req));
            req->sinkfd = sinkfd;
            strcpy(req->header, pac.header);
            req->content = strdup(pac.data);
            INIT_LIST_HEAD(&req->node);
            list_add(&req->node, &core->requests);
        } else if (pac.leader == SRRP_RESPONSE_LEADER) {
            struct api_response *resp = malloc(sizeof(*resp));
            memset(resp, 0, sizeof(*resp));
            resp->sinkfd = sinkfd;
            strcpy(resp->header, pac.header);
            resp->content = strdup(pac.data);
            INIT_LIST_HEAD(&resp->node);
            list_add(&resp->node, &core->requests);
        }
    }
}

struct apicore *apicore_new()
{
    struct apicore *core = malloc(sizeof(struct apicore));
    INIT_LIST_HEAD(&core->requests);
    INIT_LIST_HEAD(&core->responses);
    INIT_LIST_HEAD(&core->services);
    INIT_LIST_HEAD(&core->sinkfds);
    INIT_LIST_HEAD(&core->sinks);
    return core;
}

void apicore_destroy(struct apicore *core)
{
    {
        struct api_request *pos, *n;
        list_for_each_entry_safe(pos, n, &core->requests, node) {
            list_del_init(&pos->node);
            free(pos);
        }
    }

    {
        struct api_response *pos, *n;
        list_for_each_entry_safe(pos, n, &core->responses, node) {
            list_del_init(&pos->node);
            free(pos);
        }
    }

    {
        struct api_service *pos, *n;
        list_for_each_entry_safe(pos, n, &core->services, node) {
            list_del_init(&pos->node);
            free(pos);
        }
    }

    {
        struct sinkfd *pos, *n;
        list_for_each_entry_safe(pos, n, &core->sinkfds, node_core) {
            list_del_init(&pos->node_core);
            free(pos);
        }
    }

    {
        struct apisink *pos, *n;
        list_for_each_entry_safe(pos, n, &core->sinks, node) {
            apicore_del_sink(core, pos);
            apisink_fini(pos);
        }
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
            parse_packet(core, pos_fd);
            assert(pos_fd->sink->ops.send);
            pos_fd->sink->ops.send(pos_fd->sink, pos_fd->fd, "alive", 5);
        }
    }

    struct api_request *pos_req;
    list_for_each_entry(pos_req, &core->requests, node) {
        // TODO
        LOG_INFO("%s%s", pos_req->header, pos_req->content);
    }

    struct api_response *pos_resp;
    list_for_each_entry(pos_resp, &core->responses, node) {
        // TODO
        LOG_INFO("%s%s", pos_resp->header, pos_resp->content);
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
    UNUSED(core);
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
