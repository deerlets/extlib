#ifndef __QUEBUFX_H
#define __QUEBUFX_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QUEBUF_DEFAULT_SIZE 4096

typedef struct __queue_buf quebuf_t;

quebuf_t *quebuf_new(size_t size);
void quebuf_delete(quebuf_t *self);

size_t quebuf_size(quebuf_t *self);
size_t quebuf_used(quebuf_t *self);
size_t quebuf_spare(quebuf_t *self);

void quebuf_in_head(quebuf_t *self, size_t len);
void quebuf_out_head(quebuf_t *self, size_t len);

size_t quebuf_write(quebuf_t *self, const void *ptr, size_t len);
size_t quebuf_peek(quebuf_t *self, void *ptr, size_t size);
size_t quebuf_read(quebuf_t *self, void *ptr, size_t size);

#ifdef __cplusplus
}
#endif
#endif
