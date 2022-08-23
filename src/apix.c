#include "apix-inl.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include "stddefx.h"
#include "apix.h"
#include "apix-service.h"
#include "crc16.h"
#include "list.h"
#include "atbuf.h"
#include "log.h"
#include "srrp.h"
#include "json.h"

static void parse_packet(struct apibus *bus, struct sinkfd *sinkfd)
{
    while (atbuf_used(sinkfd->rxbuf)) {
        struct srrp_packet *pac = srrp_read_one_packet(atbuf_read_pos(sinkfd->rxbuf));
        if (pac == NULL) {
            if (time(0) < sinkfd->ts_poll_recv.tv_sec + PARSE_PACKET_TIMEOUT / 1000)
                break;

            LOG_WARN("parse packet failed: %s", atbuf_read_pos(sinkfd->rxbuf));
            int offset = srrp_next_packet_offset(atbuf_read_pos(sinkfd->rxbuf));
            if (offset == -1 || offset == 0)
                atbuf_read_advance(sinkfd->rxbuf, atbuf_used(sinkfd->rxbuf));
            else {
                assert((size_t)offset < atbuf_used(sinkfd->rxbuf));
                atbuf_read_advance(sinkfd->rxbuf, offset);
            }
            break;
        }

        if (pac->leader == SRRP_REQUEST_LEADER) {
            struct api_request *req = malloc(sizeof(*req));
            memset(req, 0, sizeof(*req));
            req->pac = pac;
            req->state = API_REQUEST_ST_NONE;
            req->ts_create = time(0);
            req->ts_send = 0;
            req->fd = sinkfd->fd;
            req->crc16 = crc16(pac->header, pac->header_len);
            req->crc16 = crc16_crc(req->crc16, pac->data, pac->data_len);
            INIT_LIST_HEAD(&req->node);
            list_add(&req->node, &bus->requests);
        } else if (pac->leader == SRRP_RESPONSE_LEADER) {
            struct api_response *resp = malloc(sizeof(*resp));
            memset(resp, 0, sizeof(*resp));
            resp->pac = pac;
            resp->fd = sinkfd->fd;
            INIT_LIST_HEAD(&resp->node);
            list_add(&resp->node, &bus->responses);
        } else if (pac->leader == SRRP_SUBSCRIBE_LEADER ||
                   pac->leader == SRRP_UNSUBSCRIBE_LEADER ||
                   pac->leader == SRRP_PUBLISH_LEADER) {
            struct api_topic_msg *tmsg = malloc(sizeof(*tmsg));
            memset(tmsg, 0, sizeof(*tmsg));
            tmsg->pac = pac;
            tmsg->sinkfd = sinkfd;
            INIT_LIST_HEAD(&tmsg->node);
            list_add(&tmsg->node, &bus->topic_msgs);
        }

        atbuf_read_advance(sinkfd->rxbuf, pac->len);
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

static void clear_unalive_serivce(struct apibus *bus)
{
    struct api_service *pos, *n;
    list_for_each_entry_safe(pos, n, &bus->services, node) {
        if (time(0) > pos->ts_alive + APIBUS_SERVICE_ALIVE_TIMEOUT) {
            LOG_DEBUG("clear unalive service: %s", pos->header);
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
    struct apibus *bus, const char *content, int fd)
{
    struct json_object *jo = json_object_new(content);
    char header[256] = {0};
    int rc = json_get_string(jo, "/header", header, sizeof(header));
    json_object_delete(jo);
    if (rc != 0) {
        apibus_send(bus, fd, "FAILED", 6);
        return;
    }

    struct api_service *serv = find_service(
        &bus->services, header, strlen(header));

    if (serv != NULL) {
        apibus_send(bus, fd, "DUP SERVICE", 11);
        return;
    }

    serv = malloc(sizeof(*serv));
    memset(serv, 0, sizeof(*serv));
    snprintf(serv->header, sizeof(serv->header), "%s", header);
    serv->ts_alive = time(0);
    serv->sinkfd = find_sinkfd_in_apibus(bus, fd);
    INIT_LIST_HEAD(&serv->node);
    list_add(&serv->node, &bus->services);

    apibus_send(bus, fd, "OK", 2);
}

static void service_del_handler(
    struct apibus *bus, const char *content, int fd)
{
    struct json_object *jo = json_object_new(content);
    char header[256] = {0};
    int rc = json_get_string(jo, "/header", header, sizeof(header));
    json_object_delete(jo);
    if (rc != 0) {
        apibus_send(bus, fd, "FAILED", 6);
        return;
    }

    struct api_service *serv = find_service(
        &bus->services, header, strlen(header));

    if (serv == NULL || serv->sinkfd->fd != fd) {
        apibus_send(bus, fd, "SERVICE NOT FOUND", 17);
    } else {
        list_del(&serv->node);
        free(serv);
        apibus_send(bus, fd, "OK", 2);
    }
}

static void service_alive_handler(
    struct apibus *bus, const char *content, int fd)
{
    struct json_object *jo = json_object_new(content);
    char header[256] = {0};
    int rc = json_get_string(jo, "/header", header, sizeof(header));
    json_object_delete(jo);
    if (rc != 0) {
        apibus_send(bus, fd, "FAILED", 6);
        return;
    }

    struct api_service *serv = find_service(
        &bus->services, header, strlen(header));

    if (serv == NULL || serv->sinkfd->fd != fd) {
        apibus_send(bus, fd, "SERVICE NOT FOUND", 17);
    } else {
        serv->ts_alive = time(0);
        apibus_send(bus, fd, "OK", 2);
    }
}

static void topic_sub_handler(struct apibus *bus, struct api_topic_msg *tmsg)
{
    struct api_topic *topic = NULL;
    struct api_topic *pos;
    list_for_each_entry(pos, &bus->topics, node) {
        if (strcmp(topic->header, tmsg->pac->header) == 0) {
            topic = pos;
            break;
        }
    }
    if (topic == NULL) {
        topic = malloc(sizeof(*topic));
        memset(topic, 0, sizeof(*topic));
        snprintf(topic->header, sizeof(topic->header), "%s", tmsg->pac->header);
        INIT_LIST_HEAD(&topic->node);
        list_add(&topic->node, &bus->topics);
    }
    assert(topic);
    topic->fds[topic->nfds] = tmsg->sinkfd->fd;
    topic->nfds++;

    apibus_send(bus, tmsg->sinkfd->fd, "Sub OK", 6);
}

static void topic_unsub_handler(struct apibus *bus, struct api_topic_msg *tmsg)
{
    struct api_topic *topic = NULL;
    list_for_each_entry(topic, &bus->topics, node) {
        if (strcmp(topic->header, tmsg->pac->header) == 0) {
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

    apibus_send(bus, tmsg->sinkfd->fd, "Unsub OK", 8);
}

static void topic_pub_handler(struct apibus *bus, struct api_topic_msg *tmsg)
{
    struct api_topic *topic = find_topic(
        &bus->topics, tmsg->pac->header, tmsg->pac->header_len);
    if (topic) {
        for (int i = 0; i < topic->nfds; i++)
            apibus_send(bus, topic->fds[i], tmsg->pac->raw, tmsg->pac->len);
    } else {
        // do nothing, just drop this msg
        LOG_DEBUG("drop @: %s%s", tmsg->pac->header, tmsg->pac->data);
    }
}

struct apibus *apibus_new()
{
    struct apibus *bus = malloc(sizeof(*bus));
    bzero(bus, sizeof(*bus));
    INIT_LIST_HEAD(&bus->requests);
    INIT_LIST_HEAD(&bus->responses);
    INIT_LIST_HEAD(&bus->services);
    INIT_LIST_HEAD(&bus->topic_msgs);
    INIT_LIST_HEAD(&bus->topics);
    INIT_LIST_HEAD(&bus->sinkfds);
    INIT_LIST_HEAD(&bus->sinks);
    return bus;
}

void apibus_destroy(struct apibus *bus)
{
    {
        struct api_request *pos, *n;
        list_for_each_entry_safe(pos, n, &bus->requests, node)
            api_request_delete(pos);
    }

    {
        struct api_response *pos, *n;
        list_for_each_entry_safe(pos, n, &bus->responses, node)
            api_response_delete(pos);
    }

    {
        struct api_service *pos, *n;
        list_for_each_entry_safe(pos, n, &bus->services, node) {
            list_del_init(&pos->node);
            free(pos);
        }
    }

    {
        struct api_topic_msg *pos, *n;
        list_for_each_entry_safe(pos, n, &bus->topic_msgs, node)
            api_topic_msg_delete(pos);
    }

    {
        struct api_topic *pos, *n;
        list_for_each_entry_safe(pos, n, &bus->topics, node) {
            list_del_init(&pos->node);
            free(pos);
        }
    }

    {
        struct sinkfd *pos, *n;
        list_for_each_entry_safe(pos, n, &bus->sinkfds, node_bus)
            sinkfd_destroy(pos);
    }

    {
        struct apisink *pos, *n;
        list_for_each_entry_safe(pos, n, &bus->sinks, node) {
            apibus_del_sink(bus, pos);
            apisink_fini(pos);
        }
    }

    free(bus);
}

static void handle_request(struct apibus *bus)
{
    struct api_request *pos, *n;
    list_for_each_entry_safe(pos, n, &bus->requests, node) {
        if (pos->state == API_REQUEST_ST_WAIT_RESPONSE) {
            if (time(0) < pos->ts_send + API_REQUEST_TIMEOUT / 1000)
                continue;
            apibus_send(bus, pos->fd, "request timeout", 15);
            LOG_DEBUG("request timeout: %s", pos->pac->raw);
            api_request_delete(pos);
            continue;
        }

        LOG_INFO("poll >: %.4x:%s?%s", pos->pac->reqid, pos->pac->header, pos->pac->data);
        if (strcmp(pos->pac->header, APIBUS_SERVICE_ADD) == 0) {
            service_add_handler(bus, pos->pac->data, pos->fd);
            api_request_delete(pos);
        } else if (strcmp(pos->pac->header, APIBUS_SERVICE_DEL) == 0) {
            service_del_handler(bus, pos->pac->data, pos->fd);
            api_request_delete(pos);
        } else if (strcmp(pos->pac->header, APIBUS_SERVICE_ALIVE) == 0) {
            service_alive_handler(bus, pos->pac->data, pos->fd);
            api_request_delete(pos);
        } else {
            struct api_service *serv = find_service(
                &bus->services, pos->pac->header, strlen(pos->pac->header));
            if (serv) {
                // change reqid
                struct srrp_packet *pac = srrp_write_request(
                    pos->fd, pos->pac->header, pos->pac->data);

                assert(serv->sinkfd->sink->ops.send);
                serv->sinkfd->sink->ops.send(
                    serv->sinkfd->sink, serv->sinkfd->fd,
                    pac->raw, pos->pac->len);
                pos->state = API_REQUEST_ST_WAIT_RESPONSE;
                pos->ts_send = time(0);

                srrp_free(pac);
            } else {
                apibus_send(bus, pos->fd, "SERVICE NOT FOUND", 17);
                api_request_delete(pos);
            }
        }
    }
}

static void handle_response(struct apibus *bus)
{
    struct api_response *pos, *n;
    list_for_each_entry_safe(pos, n, &bus->responses, node) {
        LOG_INFO("poll <: %.4x:%s?%s", pos->pac->reqid, pos->pac->header, pos->pac->data);
        struct api_request *pos_req, *n_req;
        list_for_each_entry_safe(pos_req, n_req, &bus->requests, node) {
            if (pos_req->crc16 == pos->pac->reqcrc16 &&
                strcmp(pos_req->pac->header, pos->pac->header) == 0 &&
                pos_req->fd == pos->pac->reqid) {
                // restore reqid
                struct srrp_packet *pac = srrp_write_response(
                    pos_req->pac->reqid, pos->pac->reqcrc16,
                    pos->pac->header, pos->pac->data);

                apibus_send(bus, pos_req->fd, pac->raw, pos->pac->len);
                api_request_delete(pos_req);

                srrp_free(pac);
                break;
            }
        }

        struct api_service *serv = find_service(
            &bus->services, pos->pac->header, strlen(pos->pac->header));
        if (serv) serv->ts_alive = time(0);
        api_response_delete(pos);
    }
}

static void handle_topic_msg(struct apibus *bus)
{
    struct api_topic_msg *pos, *n;
    list_for_each_entry_safe(pos, n, &bus->topic_msgs, node) {
        if (pos->pac->leader == SRRP_SUBSCRIBE_LEADER) {
            topic_sub_handler(bus, pos);
            LOG_INFO("poll #: %s?%s", pos->pac->header, pos->pac->data);
        } else if (pos->pac->leader == SRRP_UNSUBSCRIBE_LEADER) {
            topic_unsub_handler(bus, pos);
            LOG_INFO("poll %: %s?%s", pos->pac->header, pos->pac->data);
        } else {
            topic_pub_handler(bus, pos);
            LOG_INFO("poll @: %s?%s", pos->pac->header, pos->pac->data);
        }
        api_topic_msg_delete(pos);
    }
}

int apibus_poll(struct apibus *bus)
{
    bus->poll_cnt = 0;
    gettimeofday(&bus->poll_ts, NULL);

    // poll each sink
    struct apisink *pos_sink;
    list_for_each_entry(pos_sink, &bus->sinks, node) {
        if (pos_sink->ops.poll(pos_sink) != 0) {
            LOG_ERROR("%s", strerror(errno));
        }
    }

    // parse each sinkfds
    struct sinkfd *pos_fd;
    list_for_each_entry(pos_fd, &bus->sinkfds, node_bus) {
        if (timercmp(&bus->poll_ts, &pos_fd->ts_poll_recv, <))
            bus->poll_cnt++;
        if (atbuf_used(pos_fd->rxbuf)) {
            parse_packet(bus, pos_fd);
        }
    }

    LOG_DEBUG("poll_cnt: %d", bus->poll_cnt);
    if (bus->poll_cnt == 0) {
        if (bus->idle_usec != APIBUS_IDLE_MAX) {
            bus->idle_usec += APIBUS_IDLE_MAX / 10;
            if (bus->idle_usec > APIBUS_IDLE_MAX)
                bus->idle_usec = APIBUS_IDLE_MAX;
        }
        usleep(bus->idle_usec);
    } else {
        bus->idle_usec = APIBUS_IDLE_MAX / 10;
    }

    // hander each msg
    handle_request(bus);
    handle_response(bus);
    handle_topic_msg(bus);

    // clear service which is not alive
    clear_unalive_serivce(bus);

    return 0;
}

int apibus_open(struct apibus *bus, const char *name, const char *addr)
{
    struct apisink *pos;
    list_for_each_entry(pos, &bus->sinks, node) {
        if (strcmp(pos->name, name) == 0) {
            assert(pos->ops.open);
            return pos->ops.open(pos, addr);
        }
    }
    return -1;
}

int apibus_close(struct apibus *bus, int fd)
{
    struct sinkfd *sinkfd = find_sinkfd_in_apibus(bus, fd);
    if (sinkfd == NULL)
        return -1;
    if (sinkfd->sink && sinkfd->sink->ops.close)
        sinkfd->sink->ops.close(sinkfd->sink, fd);
    return 0;
}

int apibus_ioctl(struct apibus *bus, int fd, unsigned int cmd, unsigned long arg)
{
    struct sinkfd *sinkfd = find_sinkfd_in_apibus(bus, fd);
    if (sinkfd == NULL)
        return -1;
    if (sinkfd->sink == NULL || sinkfd->sink->ops.ioctl == NULL)
        return -1;
    return sinkfd->sink->ops.ioctl(sinkfd->sink, fd, cmd, arg);
}

int apibus_send(struct apibus *bus, int fd, const void *buf, size_t len)
{
    struct sinkfd *sinkfd = find_sinkfd_in_apibus(bus, fd);
    if (sinkfd == NULL)
        return -1;
    if (sinkfd->sink == NULL || sinkfd->sink->ops.send == NULL)
        return -1;
    return sinkfd->sink->ops.send(sinkfd->sink, fd, buf, len);
}

int apibus_recv(struct apibus *bus, int fd, void *buf, size_t size)
{
    struct sinkfd *sinkfd = find_sinkfd_in_apibus(bus, fd);
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
    sink->bus = NULL;
}

void apisink_fini(struct apisink *sink)
{
    struct sinkfd *pos, *n;
    list_for_each_entry_safe(pos, n, &sink->sinkfds, node_sink)
        sinkfd_destroy(pos);

    if (sink->bus)
        apibus_del_sink(sink->bus, sink);
}

int apibus_add_sink(struct apibus *bus, struct apisink *sink)
{
    struct apisink *pos;
    list_for_each_entry(pos, &bus->sinks, node) {
        if (strcmp(sink->name, pos->name) == 0)
            return -1;
    }

    list_add(&sink->node, &bus->sinks);
    sink->bus = bus;
    return 0;
}

void apibus_del_sink(struct apibus *bus, struct apisink *sink)
{
    UNUSED(bus);
    list_del_init(&sink->node);
    sink->bus = NULL;
}

struct sinkfd *sinkfd_new()
{
    struct sinkfd *sinkfd = malloc(sizeof(struct sinkfd));
    memset(sinkfd, 0, sizeof(*sinkfd));
    sinkfd->fd = 0;
    sinkfd->listen = 0;
    sinkfd->txbuf = atbuf_new(0);
    sinkfd->rxbuf = atbuf_new(0);
    sinkfd->sink = NULL;
    INIT_LIST_HEAD(&sinkfd->node_sink);
    INIT_LIST_HEAD(&sinkfd->node_bus);
    return sinkfd;
}

void sinkfd_destroy(struct sinkfd *sinkfd)
{
    sinkfd->fd = 0;
    atbuf_delete(sinkfd->txbuf);
    atbuf_delete(sinkfd->rxbuf);
    sinkfd->sink = NULL;
    list_del_init(&sinkfd->node_sink);
    list_del_init(&sinkfd->node_bus);
    free(sinkfd);
}

struct sinkfd *find_sinkfd_in_apibus(struct apibus *bus, int fd)
{
    struct sinkfd *pos, *n;
    list_for_each_entry_safe(pos, n, &bus->sinkfds, node_bus) {
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
