#ifndef __APIX_H
#define __APIX_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

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

int /*fd*/ apicore_open(struct apicore *core, const char *name, const void *addr);
int apicore_close(struct apicore *core, int fd);

#ifdef __cplusplus
}
#endif
#endif
