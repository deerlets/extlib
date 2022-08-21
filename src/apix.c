#include "apix-inl.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "apix.h"
#include "apix-service.h"
#include "crc16x.h"
#include "stddefx.h"
#include "listx.h"
#include "autobufx.h"
#include "logx.h"
#include "srrpx.h"
#include "jsonx.h"

static void parse_packet(struct apicore *core, struct sinkfd *sinkfd)
{
    struct srrp_packet pac = {0};

    while (autobuf_used(sinkfd->rxbuf)) {
        int nr = srrp_read_one_packet(
            autobuf_read_pos(sinkfd->rxbuf), autobuf_used(sinkfd->rxbuf), &pac);
        if (nr == -1) {
            if (time(0) < sinkfd->ts_poll_recv + PARSE_PACKET_TIMEOUT / 1000)
                break;

            LOG_WARN("parse packet failed: %s", autobuf_read_pos(sinkfd->rxbuf));
            int offset = srrp_next_packet_offset(autobuf_read_pos(sinkfd->rxbuf));
            if (offset == -1 || offset == 0)
                autobuf_read_advance(sinkfd->rxbuf, autobuf_used(sinkfd->rxbuf));
            else {
                assert(offset < autobuf_used(sinkfd->rxbuf));
                autobuf_read_advance(sinkfd->rxbuf, offset);
            }
            break;
        }

        if (pac.leader == SRRP_REQUEST_LEADER) {
            struct api_request *req = malloc(sizeof(*req));
            memset(req, 0, sizeof(*req));
            req->raw = malloc(nr);
            memcpy(req->raw, autobuf_read_pos(sinkfd->rxbuf), nr);
            req->raw_len = nr;
            req->state = API_REQUEST_ST_NONE;
            req->ts_create = time(0);
            req->ts_send = 0;
            req->sinkfd = sinkfd;
            char *tmp = calloc(1, strlen(pac.header) + strlen(pac.data) + 1);
            memcpy(tmp, pac.header, strlen(pac.header));
            memcpy(tmp + strlen(pac.header), pac.data, strlen(pac.data));
            req->crc16 = crc16(tmp, strlen(tmp));
            free(tmp);
            req->leader = pac.leader;
            snprintf(req->header, sizeof(req->header), "%s", pac.header);
            req->content = strdup(pac.data);
            INIT_LIST_HEAD(&req->node);
            list_add(&req->node, &core->requests);
        } else if (pac.leader == SRRP_RESPONSE_LEADER) {
            struct api_response *resp = malloc(sizeof(*resp));
            memset(resp, 0, sizeof(*resp));
            resp->raw = malloc(nr);
            memcpy(resp->raw, autobuf_read_pos(sinkfd->rxbuf), nr);
            resp->raw_len = nr;
            resp->sinkfd = sinkfd;
            resp->crc16_req = pac.crc16_req;
            resp->leader = pac.leader;
            snprintf(resp->header, sizeof(resp->header), "%s", pac.header);
            resp->content = strdup(pac.data);
            INIT_LIST_HEAD(&resp->node);
            list_add(&resp->node, &core->responses);
        } else if (pac.leader == SRRP_SUBSCRIBE_LEADER ||
                   pac.leader == SRRP_UNSUBSCRIBE_LEADER ||
                   pac.leader == SRRP_PUBLISH_LEADER) {
            struct api_topic_msg *tmsg = malloc(sizeof(*tmsg));
            memset(tmsg, 0, sizeof(*tmsg));
            tmsg->raw = malloc(nr);
            memcpy(tmsg->raw, autobuf_read_pos(sinkfd->rxbuf), nr);
            tmsg->raw_len = nr;
            tmsg->sinkfd = sinkfd;
            tmsg->leader = pac.leader;
            snprintf(tmsg->header, sizeof(tmsg->header), "%s", pac.header);
            tmsg->content = strdup(pac.data);
            INIT_LIST_HEAD(&tmsg->node);
            list_add(&tmsg->node, &core->topic_msgs);
        }

        free(pac.header);
        free(pac.data);
        autobuf_read_advance(sinkfd->rxbuf, nr);
    }
}

static struct api_service *
find_service(struct list_head *services, const void *header, size_t len)
{
    struct api_service *pos;
    list_for_each_entry(pos, services, node) {
        if (memcmp(pos->header, header, len) == 0) {
            return pos;
        }
    }
    return NULL;
}

static struct api_topic *
find_topic(struct list_head *topics, const void *header, size_t len)
{
    struct api_topic *pos;
    list_for_each_entry(pos, topics, node) {
        if (memcmp(pos->header, header, len) == 0) {
            return pos;
        }
    }
    return NULL;
}

static void clear_unalive_serivce(struct apicore *core)
{
    struct api_service *pos, *n;
    list_for_each_entry_safe(pos, n, &core->services, node) {
        if (time(0) > pos->ts_alive + APICORE_SERVICE_ALIVE_TIMEOUT) {
            assert(pos->sinkfd->sink->ops.send);
            pos->sinkfd->sink->ops.send(
                pos->sinkfd->sink, pos->sinkfd->fd,
                pos->header, strlen(pos->header));
            list_del(&pos->node);
            free(pos);
        }
    }
}

static void service_add_handler(
    struct apicore *core, const char *content, struct sinkfd *sinkfd)
{
    struct json_object *jo = json_object_new(content);
    char header[256] = {0};
    int rc = json_get_string(jo, "/header", header, sizeof(header));
    json_object_delete(jo);
    if (rc != 0) {
        assert(sinkfd->sink->ops.send);
        sinkfd->sink->ops.send(sinkfd->sink, sinkfd->fd, "FAILED", 6);
        return;
    }

    struct api_service *serv = malloc(sizeof(*serv));
    memset(serv, 0, sizeof(*serv));
    snprintf(serv->header, sizeof(serv->header), "%s", header);
    serv->ts_alive = time(0);
    serv->sinkfd = sinkfd;
    INIT_LIST_HEAD(&serv->node);
    list_add(&serv->node, &core->services);

    assert(sinkfd->sink->ops.send);
    sinkfd->sink->ops.send(sinkfd->sink, sinkfd->fd, "OK", 2);
}

static void service_del_handler(
    struct apicore *core, const char *content, struct sinkfd *sinkfd)
{
    struct json_object *jo = json_object_new(content);
    char header[256] = {0};
    int rc = json_get_string(jo, "/header", header, sizeof(header));
    json_object_delete(jo);
    if (rc != 0) {
        assert(sinkfd->sink->ops.send);
        sinkfd->sink->ops.send(sinkfd->sink, sinkfd->fd, "FAILED", 6);
        return;
    }

    struct api_service *serv = find_service(
        &core->services, header, strlen(header));

    if (serv == NULL || serv->sinkfd != sinkfd) {
        assert(sinkfd->sink->ops.send);
        sinkfd->sink->ops.send(sinkfd->sink, sinkfd->fd, "SERVICE NOT FOUND", 17);
    } else {
        list_del(&serv->node);
        free(serv);
        assert(sinkfd->sink->ops.send);
        sinkfd->sink->ops.send(sinkfd->sink, sinkfd->fd, "OK", 2);
    }
}

static void service_alive_handler(
    struct apicore *core, const char *content, struct sinkfd *sinkfd)
{
    struct json_object *jo = json_object_new(content);
    char header[256] = {0};
    int rc = json_get_string(jo, "/header", header, sizeof(header));
    json_object_delete(jo);
    if (rc != 0) {
        assert(sinkfd->sink->ops.send);
        sinkfd->sink->ops.send(sinkfd->sink, sinkfd->fd, "FAILED", 6);
        return;
    }

    struct api_service *serv = find_service(
        &core->services, header, strlen(header));

    if (serv == NULL || serv->sinkfd != sinkfd) {
        assert(sinkfd->sink->ops.send);
        sinkfd->sink->ops.send(sinkfd->sink, sinkfd->fd, "SERVICE NOT FOUND", 17);
    } else {
        serv->ts_alive = time(0);
    }
}

static void topic_sub_handler(struct apicore *core, struct api_topic_msg *tmsg)
{
    struct api_topic *topic = NULL;
    struct api_topic *pos;
    list_for_each_entry(pos, &core->topics, node) {
        if (strcmp(topic->header, tmsg->header) == 0) {
            topic = pos;
            break;
        }
    }
    if (topic == NULL) {
        topic = malloc(sizeof(*topic));
        memset(topic, 0, sizeof(*topic));
        snprintf(topic->header, sizeof(topic->header), "%s", tmsg->header);
        INIT_LIST_HEAD(&topic->node);
        list_add(&topic->node, &core->topics);
    }
    assert(topic);
    topic->fds[topic->nfds] = tmsg->sinkfd->fd;
    topic->nfds++;

    apicore_send(core, tmsg->sinkfd->fd, "Sub OK", 6);
}

static void topic_unsub_handler(struct apicore *core, struct api_topic_msg *tmsg)
{
    struct api_topic *topic = NULL;
    list_for_each_entry(topic, &core->topics, node) {
        if (strcmp(topic->header, tmsg->header) == 0) {
            break;
        }
    }
    if (topic) {
        for (int i = 0; i < topic->nfds; i++) {
            if (topic->fds[i] == tmsg->sinkfd->fd) {
                topic->fds[i] = topic->fds[topic->nfds-1];
                topic->nfds--;
            }
        }
    }

    apicore_send(core, tmsg->sinkfd->fd, "Unsub OK", 8);
}

static void topic_pub_handler(struct apicore *core, struct api_topic_msg *tmsg)
{
    struct api_topic *topic = find_topic(
        &core->topics, tmsg->header, strlen(tmsg->header));
    if (topic) {
        for (int i = 0; i < topic->nfds; i++)
            apicore_send(core, topic->fds[i], tmsg->raw, tmsg->raw_len);
    } else {
        // do nothing, just drop this msg
        LOG_DEBUG("drop @: %s%s", tmsg->header, tmsg->content);
    }
}

struct apicore *apicore_new()
{
    struct apicore *core = malloc(sizeof(struct apicore));
    INIT_LIST_HEAD(&core->requests);
    INIT_LIST_HEAD(&core->responses);
    INIT_LIST_HEAD(&core->services);
    INIT_LIST_HEAD(&core->topic_msgs);
    INIT_LIST_HEAD(&core->topics);
    INIT_LIST_HEAD(&core->sinkfds);
    INIT_LIST_HEAD(&core->sinks);
    return core;
}

void apicore_destroy(struct apicore *core)
{
    {
        struct api_request *pos, *n;
        list_for_each_entry_safe(pos, n, &core->requests, node)
            api_request_delete(pos);
    }

    {
        struct api_response *pos, *n;
        list_for_each_entry_safe(pos, n, &core->responses, node)
            api_response_delete(pos);
    }

    {
        struct api_service *pos, *n;
        list_for_each_entry_safe(pos, n, &core->services, node) {
            list_del_init(&pos->node);
            free(pos);
        }
    }

    {
        struct api_topic_msg *pos, *n;
        list_for_each_entry_safe(pos, n, &core->topic_msgs, node)
            api_topic_msg_delete(pos);
    }

    {
        struct api_topic *pos, *n;
        list_for_each_entry_safe(pos, n, &core->topics, node) {
            list_del_init(&pos->node);
            free(pos);
        }
    }

    {
        struct sinkfd *pos, *n;
        list_for_each_entry_safe(pos, n, &core->sinkfds, node_core)
            sinkfd_destroy(pos);
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

static void handle_request(struct apicore *core)
{
    struct api_request *pos, *n;
    list_for_each_entry_safe(pos, n, &core->requests, node) {
        if (pos->state == API_REQUEST_ST_WAIT_RESPONSE) {
            if (time(0) < pos->ts_send + API_REQUEST_TIMEOUT / 1000)
                continue;
            pos->sinkfd->sink->ops.send(
                pos->sinkfd->sink, pos->sinkfd->fd,
                "request timeout", 15);
            LOG_DEBUG("request timeout: %s", pos->raw);
            api_request_delete(pos);
            continue;
        }

        LOG_INFO("poll >: %s%s", pos->header, pos->content);
        if (strcmp(pos->header, APICORE_SERVICE_ADD) == 0) {
            service_add_handler(core, pos->content, pos->sinkfd);
            api_request_delete(pos);
        } else if (strcmp(pos->header, APICORE_SERVICE_DEL) == 0) {
            service_del_handler(core, pos->content, pos->sinkfd);
            api_request_delete(pos);
        } else if (strcmp(pos->header, APICORE_SERVICE_ALIVE) == 0) {
            service_alive_handler(core, pos->content, pos->sinkfd);
            api_request_delete(pos);
        } else {
            struct api_service *serv = find_service(
                &core->services, pos->header, strlen(pos->header));
            if (serv) {
                assert(serv->sinkfd->sink->ops.send);
                serv->sinkfd->sink->ops.send(
                    serv->sinkfd->sink, serv->sinkfd->fd,
                    pos->raw, pos->raw_len);
                pos->state = API_REQUEST_ST_WAIT_RESPONSE;
                pos->ts_send = time(0);
            } else {
                pos->sinkfd->sink->ops.send(
                    pos->sinkfd->sink, pos->sinkfd->fd,
                    "SERVICE NOT FOUND", 17);
                api_request_delete(pos);
            }
        }
    }
}

static void handle_response(struct apicore *core)
{
    struct api_response *pos, *n;
    list_for_each_entry_safe(pos, n, &core->responses, node) {
        LOG_INFO("poll <: %s%s", pos->header, pos->content);
        struct api_request *pos_req, *n_req;
        list_for_each_entry_safe(pos_req, n_req, &core->requests, node) {
            if (pos_req->crc16 == pos->crc16_req &&
                strcmp(pos_req->header, pos->header) == 0) {
                pos_req->sinkfd->sink->ops.send(
                    pos_req->sinkfd->sink, pos_req->sinkfd->fd,
                    pos->raw, pos->raw_len);
                api_request_delete(pos_req);
                break;
            }
        }
        api_response_delete(pos);
    }
}

static void handle_topic_msg(struct apicore *core)
{
    struct api_topic_msg *pos, *n;
    list_for_each_entry_safe(pos, n, &core->topic_msgs, node) {
        if (pos->leader == SRRP_SUBSCRIBE_LEADER) {
            topic_sub_handler(core, pos);
            LOG_INFO("poll #: %s%s", pos->header, pos->content);
        } else if (pos->leader == SRRP_UNSUBSCRIBE_LEADER) {
            topic_unsub_handler(core, pos);
            LOG_INFO("poll %: %s%s", pos->header, pos->content);
        } else {
            topic_pub_handler(core, pos);
            LOG_INFO("poll @: %s%s", pos->header, pos->content);
        }
        api_topic_msg_delete(pos);
    }
}

int apicore_poll(struct apicore *core, int timeout)
{
    int cnt = 0;
    struct list_head *pos;
    list_for_each(pos, &core->sinks)
        cnt++;

    // poll each sink
    struct apisink *pos_sink;
    list_for_each_entry(pos_sink, &core->sinks, node) {
        if (pos_sink->ops.poll(pos_sink, timeout / cnt) != 0) {
            LOG_ERROR("%s", strerror(errno));
        }
    }

    // parse each sinkfds
    struct sinkfd *pos_fd;
    list_for_each_entry(pos_fd, &core->sinkfds, node_core) {
        if (autobuf_used(pos_fd->rxbuf)) {
            parse_packet(core, pos_fd);
        }
    }

    // hander each msg
    handle_request(core);
    handle_response(core);
    handle_topic_msg(core);

    // clear service which is not alive
    clear_unalive_serivce(core);

    return 0;
}

int apicore_open(struct apicore *core, const char *name, const char *addr)
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
    struct sinkfd *sinkfd = find_sinkfd_in_apicore(core, fd);
    if (sinkfd == NULL)
        return -1;
    if (sinkfd->sink && sinkfd->sink->ops.close)
        sinkfd->sink->ops.close(sinkfd->sink, fd);
    return 0;
}

int apicore_ioctl(struct apicore *core, int fd, unsigned int cmd, unsigned long arg)
{
    struct sinkfd *sinkfd = find_sinkfd_in_apicore(core, fd);
    if (sinkfd == NULL)
        return -1;
    if (sinkfd->sink == NULL || sinkfd->sink->ops.ioctl == NULL)
        return -1;
    return sinkfd->sink->ops.ioctl(sinkfd->sink, fd, cmd, arg);
}

int apicore_send(struct apicore *core, int fd, const void *buf, size_t len)
{
    struct sinkfd *sinkfd = find_sinkfd_in_apicore(core, fd);
    if (sinkfd == NULL)
        return -1;
    if (sinkfd->sink == NULL || sinkfd->sink->ops.send == NULL)
        return -1;
    return sinkfd->sink->ops.send(sinkfd->sink, fd, buf, len);
}

int apicore_recv(struct apicore *core, int fd, void *buf, size_t size)
{
    struct sinkfd *sinkfd = find_sinkfd_in_apicore(core, fd);
    if (sinkfd == NULL)
        return -1;
    if (sinkfd->sink == NULL || sinkfd->sink->ops.recv == NULL)
        return -1;
    return sinkfd->sink->ops.recv(sinkfd->sink, fd, buf, size);
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

    list_add(&sink->node, &core->sinks);
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
    memset(sinkfd, 0, sizeof(*sinkfd));
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

struct sinkfd *find_sinkfd_in_apicore(struct apicore *core, int fd)
{
    struct sinkfd *pos, *n;
    list_for_each_entry_safe(pos, n, &core->sinkfds, node_core) {
        if (pos->fd == fd)
            return pos;
    }
    return NULL;
}

struct sinkfd *find_sinkfd_in_apisink(struct apisink *sink, int fd)
{
    struct sinkfd *pos, *n;
    list_for_each_entry_safe(pos, n, &sink->sinkfds, node_sink) {
        if (pos->fd == fd)
            return pos;
    }
    return NULL;
}
