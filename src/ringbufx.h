#ifndef __RINGBUFX_H
#define __RINGBUFX_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RINGBUF_DEFAULT_SIZE 4096

typedef struct ringbuf ringbuf_t;

ringbuf_t *ringbuf_new(size_t size);
void ringbuf_delete(ringbuf_t *self);

size_t ringbuf_size(ringbuf_t *self);
size_t ringbuf_used(ringbuf_t *self);
size_t ringbuf_spare(ringbuf_t *self);

void ringbuf_in_head(ringbuf_t *self, size_t len);
void ringbuf_out_head(ringbuf_t *self, size_t len);

size_t ringbuf_write(ringbuf_t *self, const void *ptr, size_t len);
size_t ringbuf_peek(ringbuf_t *self, void *ptr, size_t size);
size_t ringbuf_read(ringbuf_t *self, void *ptr, size_t size);

#ifdef __cplusplus
}
#endif
#endif
