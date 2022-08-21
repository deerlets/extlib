#ifndef __APIX_INL_H
#define __APIX_INL_H

#include <stdint.h>
#include "apix.h"
#include "listx.h"
#include "autobufx.h"

#define APISINK_NAME_SIZE 64
#define SINKFD_ADDR_SIZE 64
#define API_HEADER_SIZE 256
#define API_TOPIC_SUBSCRIBE_MAX 32

#define API_REQUEST_ST_NONE 0
#define API_REQUEST_ST_WAIT_RESPONSE 1

#define API_REQUEST_TIMEOUT 3000 /*ms*/

#ifdef __cplusplus
extern "C" {
#endif

/*
 * apisink
 */

struct apisink;

typedef struct apisink_ops {
    int (*open)(struct apisink *sink, const char *addr);
    int (*close)(struct apisink *sink, int fd);
    int (*ioctl)(struct apisink *sink, int fd, unsigned int cmd, unsigned long arg);
    int (*send)(struct apisink *sink, int fd, const void *buf, size_t len);
    int (*recv)(struct apisink *sink, int fd, void *buf, size_t size);
    int (*poll)(struct apisink *sink, int timeout);
} apisink_ops_t;

struct apisink {
    char name[APISINK_NAME_SIZE]; // identify
    apisink_ops_t ops;
    struct apicore *core;
    struct list_head sinkfds;
    struct list_head node;
};

void apisink_init(struct apisink *sink, const char *name, apisink_ops_t ops);
void apisink_fini(struct apisink *sink);

int apicore_add_sink(struct apicore *core, struct apisink *sink);
void apicore_del_sink(struct apicore *core, struct apisink *sink);

/*
 * sinkfd
 */

struct sinkfd {
    int fd;
    int listen;
    char addr[SINKFD_ADDR_SIZE];
    autobuf_t *txbuf;
    autobuf_t *rxbuf;
    struct apisink *sink;
    struct list_head node_sink;
    struct list_head node_core;
};

struct sinkfd *sinkfd_new();
void sinkfd_destroy();

struct sinkfd *find_sinkfd_in_apicore(struct apicore *core, int fd);
struct sinkfd *find_sinkfd_in_apisink(struct apisink *sink, int fd);

/*
 * api_request
 * api_response
 * api_service
 * api_topic
 */

struct api_request {
    void *raw;
    size_t raw_len;
    int state;
    uint64_t ts_create;
    uint64_t ts_send;

    struct sinkfd *sinkfd;
    uint16_t crc16;
    char leader;
    char header[API_HEADER_SIZE];
    char *content; // dynamic alloc, need free
    struct list_head node;
};

struct api_response {
    void *raw;
    size_t raw_len;

    struct sinkfd *sinkfd;
    uint16_t crc16_req;
    char leader;
    char header[API_HEADER_SIZE];
    char *content; // dynamic alloc, need free
    struct list_head node;
};

struct api_service {
    char header[API_HEADER_SIZE];
    uint64_t ts_alive;
    struct sinkfd *sinkfd;
    struct list_head node;
};

struct api_topic_msg {
    void *raw;
    size_t raw_len;

    struct sinkfd *sinkfd;
    char leader;
    char header[API_HEADER_SIZE];
    char *content; // dynamic alloc, need free
    struct list_head node;
};

struct api_topic {
    char header[API_HEADER_SIZE];
    int fds[API_TOPIC_SUBSCRIBE_MAX];
    int nfds;
    struct list_head node;
};

#define api_request_delete(req) \
{ \
    list_del(&req->node); \
    free(req->raw); \
    free(req->content); \
    free(req); \
}

#define api_response_delete(resp) \
{ \
    list_del(&resp->node); \
    free(resp->raw); \
    free(resp->content); \
    free(resp); \
}

#define api_topic_msg_delete(tmsg) \
{ \
    list_del(&tmsg->node); \
    free(tmsg->raw); \
    free(tmsg->content); \
    free(tmsg); \
}

/*
 * apicore
 */

struct apicore {
    struct list_head requests;
    struct list_head responses;
    struct list_head services;
    struct list_head topic_msgs;
    struct list_head topics;
    struct list_head sinkfds;
    struct list_head sinks;
};

#ifdef __cplusplus
}
#endif
#endif
