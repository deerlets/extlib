#ifndef __AUTOBUF_H
#define __AUTOBUF_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AUTOBUF_DEFAULT_SIZE 4096

typedef struct autobuf autobuf_t;

autobuf_t *autobuf_new(size_t size);
void autobuf_delete(autobuf_t *self);
int autobuf_realloc(autobuf_t *self, size_t len);

char *autobuf_read_pos(autobuf_t *self);
char *autobuf_write_pos(autobuf_t *self);
size_t autobuf_read_advance(autobuf_t *self, size_t len);
size_t autobuf_write_advance(autobuf_t *self, size_t len);

size_t autobuf_size(autobuf_t *self);
size_t autobuf_garbage(autobuf_t *self);
size_t autobuf_used(autobuf_t *self);
size_t autobuf_spare(autobuf_t *self);
size_t autobuf_tidy(autobuf_t *self);

size_t autobuf_peek(autobuf_t *self, void *ptr, size_t len);
size_t autobuf_read(autobuf_t *self, void *ptr, size_t len);
size_t autobuf_write(autobuf_t *self, const void *ptr, size_t len);

#ifdef __cplusplus
}
#endif

#endif
