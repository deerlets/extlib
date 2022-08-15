#ifndef __APIX_H
#define __APIX_H

#include <stddef.h>
#include <listx.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IPC_MODE_PIPE = 0,
    IPC_MODE_SHM,
    IPC_MODE_SHM_MEMFD,
    IPC_MODE_SHM_FTOK,
    IPC_MODE_SERIAL,
    IPC_MODE_UNIX,
    IPC_MODE_TCP,
    IPC_MODE_UDP,
    IPC_MODE_HTTP,
} ipc_mode_t;

/*
 * apinode
 */

struct apinode;

struct apinode *apinode_new();
void apinode_destroy(struct apinode *node);
int apinode_poll(struct apinode *node, int timeout);

/*
 * apicore
 */

struct apicore;

struct apicore *apicore_new();
void apicore_destroy(struct apicore *core);
int apicore_poll(struct apicore *core, int timeout);

int apicore_open(struct apicore *core, const char *name, const void *addr);
int apicore_close(struct apicore *core, int fd);
int apicore_send(struct apicore *core, int fd, const void *buf, size_t len);
int apicore_recv(struct apicore *core, int fd, void *buf, size_t size);

/*
 * apisink: sink means ipc channel
 */

struct apisink;

typedef struct apisink_ops {
    int (*open)(struct apisink *sink, const void *addr);
    int (*close)(struct apisink *sink, int fd);
    int (*send)(struct apisink *sink, int fd, const void *buf, size_t len);
    int (*recv)(struct apisink *sink, int fd, void *buf, size_t size);
    int (*poll)(struct apisink *sink);
} apisink_ops_t;

struct apisink *apisink_new(const char *name, apisink_ops_t ops);
void apisink_destroy(struct apisink *sink);

int apicore_register(struct apicore *core, struct apisink *sink);
void apicore_unregister(struct apicore *core, struct apisink *sink);

#ifdef __cplusplus
}
#endif
#endif
