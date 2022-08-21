#ifndef __APIX_H
#define __APIX_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct apicore;

struct apicore *apicore_new();
void apicore_destroy(struct apicore *core);
int apicore_poll(struct apicore *core);

int /*fd*/ apicore_open(struct apicore *core, const char *name, const char *addr);
int apicore_close(struct apicore *core, int fd);
int apicore_ioctl(struct apicore *core, int fd, unsigned int cmd, unsigned long arg);
int apicore_send(struct apicore *core, int fd, const void *buf, size_t len);
int apicore_recv(struct apicore *core, int fd, void *buf, size_t size);

#ifdef __cplusplus
}
#endif
#endif
