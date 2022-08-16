#ifndef __APIX_INL_H
#define __APIX_INL_H

#include "apix.h"
#include <listx.h>
#include <autobufx.h>

#define APISINK_NAME_SIZE 64

#ifdef __cplusplus
extern "C" {
#endif

struct apinode {
};

struct apicore {
    struct list_head sinkfds;
    struct list_head sinks;
};

struct apisink {
    char name[APISINK_NAME_SIZE]; // identify
    apisink_ops_t ops;
    struct apicore *core;
    struct list_head sinkfds;
    struct list_head node;
};

void apisink_init(struct apisink *sink, const char *name, apisink_ops_t ops);
void apisink_fini(struct apisink *sink);

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
