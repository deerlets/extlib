#ifndef __APIX_INL_H
#define __APIX_INL_H

#include "apix.h"
#include "listx.h"
#include "autobufx.h"

#define APISINK_NAME_SIZE 64
#define API_HEADER_SIZE 256
#define API_ERRMSG_SIZE 256

#define APICORE_SERVICE_ADD "/core/service/add"
#define APICORE_SERVICE_DEL "/core/service/del"

#define API_REQUEST_ST_NONE 0
#define API_REQUEST_ST_WAIT_RESPONSE 1

#ifdef __cplusplus
extern "C" {
#endif

/*
 * apinode
 */

struct apinode {
};

/*
 * apiservice
 */

struct api_request {
    void *raw;
    size_t raw_len;
    int state;

    struct sinkfd *sinkfd;
    char header[API_HEADER_SIZE];
    char *content; // dynamic alloc, need free
    struct list_head node;
};

struct api_response {
    void *raw;
    size_t raw_len;

    struct sinkfd *sinkfd;
    char header[API_HEADER_SIZE];
    char *content; // dynamic alloc, nedd free
    struct list_head node;
};

struct api_service {
    char header[API_HEADER_SIZE];
    struct sinkfd *sinkfd;
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

/*
 * apicore
 */

struct apicore {
    struct list_head requests;
    struct list_head responses;
    struct list_head services;

    struct list_head sinkfds;
    struct list_head sinks;
};

/*
 * apisink
 */

struct apisink;

typedef struct apisink_ops {
    int (*open)(struct apisink *sink, const void *addr);
    int (*close)(struct apisink *sink, int fd);
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
    autobuf_t *txbuf;
    autobuf_t *rxbuf;
    struct apisink *sink;
    struct list_head node_sink;
    struct list_head node_core;
};

struct sinkfd *sinkfd_new();
void sinkfd_destroy();

struct sinkfd *to_sinkfd(struct list_head *sinkfds, int fd);

#ifdef __cplusplus
}
#endif
#endif
