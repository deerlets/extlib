#ifndef __APIX_H
#define __APIX_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct apicore;

struct apicore *apicore_new();
void apicore_destroy(struct apicore *core);
int apicore_poll(struct apicore *core, int /*ms*/ timeout);

int /*fd*/ apicore_open(struct apicore *core, const char *name, const char *addr);
int apicore_close(struct apicore *core, int fd);

#ifdef __cplusplus
}
#endif
#endif
