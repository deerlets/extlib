#ifndef __QUEBUFX_H
#define __QUEBUFX_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QUEBUF_DEFAULT_SIZE 4096
#define QUEBUF_COLLECT_POLICY_NONE 0
#define QUEBUF_COLLECT_POLICY_LESS_SPARE 1
#define QUEBUF_COLLECT_POLICY_LARGE_GARBAGE 2

typedef struct __queue_buf quebuf_t;

quebuf_t *quebuf_new(size_t size);
void quebuf_delete(quebuf_t *self);
int quebuf_realloc(quebuf_t *self, size_t len);

char *quebuf_rawbuf_out_pos(quebuf_t *self);
char *quebuf_rawbuf_in_pos(quebuf_t *self);
size_t quebuf_size(quebuf_t *self);
size_t quebuf_garbage(quebuf_t *self);
size_t quebuf_used(quebuf_t *self);
size_t quebuf_spare(quebuf_t *self);
size_t quebuf_collect(quebuf_t *self, int policy);
size_t quebuf_offset_out_head(quebuf_t *self, size_t len);
size_t quebuf_offset_in_head(quebuf_t *self, size_t len);

size_t quebuf_peek(quebuf_t *self, void *ptr, size_t len);
size_t quebuf_read(quebuf_t *self, void *ptr, size_t len);
size_t quebuf_write(quebuf_t *self, const void *ptr, size_t len);

#ifdef __cplusplus
}
#endif
#endif
