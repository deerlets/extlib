#include "autobufx.h"
#include <stdlib.h>
#include <string.h>

struct autobuf {
    char *rawbuf;
    size_t size;
    size_t offset_in;
    size_t offset_out;
};

autobuf_t *autobuf_new(size_t size)
{
    if (size == 0)
        size = AUTOBUF_DEFAULT_SIZE;

    autobuf_t *self = (autobuf_t*)calloc(sizeof(autobuf_t), 1);
    if (!self) return NULL;

    self->rawbuf = (char*)calloc(size, 1);
    if (!self->rawbuf) {
        free(self);
        return NULL;
    }

    self->size = size;
    self->offset_in = 0;
    self->offset_out = 0;

    return self;
}

void autobuf_delete(autobuf_t *self)
{
    if (self) {
        free(self->rawbuf);
        free(self);
    }
}

int autobuf_realloc(autobuf_t *self, size_t len)
{
    void *newbuf = realloc(self->rawbuf, len);
    if (newbuf) {
        self->rawbuf = newbuf;
        self->size = len;
        return 0;
    } else {
        return -1;
    }
}

char *autobuf_read_pos(autobuf_t *self)
{
    return self->rawbuf + self->offset_out;
}

char *autobuf_write_pos(autobuf_t *self)
{
    return self->rawbuf + self->offset_in;
}

size_t autobuf_read_advance(autobuf_t *self, size_t len)
{
    self->offset_out += len;

    if (self->offset_out > self->offset_in)
        self->offset_out = self->offset_in;

    return autobuf_used(self);
}

size_t autobuf_write_advance(autobuf_t *self, size_t len)
{
    self->offset_in += len;

    if (self->offset_in > self->size)
        self->offset_in = self->size;

    return autobuf_spare(self);
}

size_t autobuf_size(autobuf_t *self)
{
    return self->size;
}

size_t autobuf_garbage(autobuf_t *self)
{
    return self->offset_out;
}

size_t autobuf_used(autobuf_t *self)
{
    return self->offset_in - self->offset_out;
}

size_t autobuf_spare(autobuf_t *self)
{
    return self->size - self->offset_in;
}

size_t autobuf_tidy(autobuf_t *self)
{
    if (autobuf_used(self) == 0) {
        self->offset_in = self->offset_out = 0;
    } else if (autobuf_spare(self) < self->size>>2 ||
               autobuf_garbage(self) > self->size>>2) {
        /* method 1
        self->offset_in -= self->offset_out;
        memmove(self->rawbuf, autobuf_read_pos(self), self->offset_in);
        self->offset_out = 0;
        */
        memmove(self->rawbuf, autobuf_read_pos(self), autobuf_used(self));
        self->offset_in = autobuf_used(self);
        self->offset_out = 0;
    }

    return autobuf_spare(self);
}

size_t autobuf_peek(autobuf_t *self, void *ptr, size_t len)
{
    size_t len_can_out = len <= autobuf_used(self) ? len : autobuf_used(self);
    memcpy(ptr, autobuf_read_pos(self), len_can_out);
    return len_can_out;
}

size_t autobuf_read(autobuf_t *self, void *ptr, size_t len)
{
    int nread = autobuf_peek(self, ptr, len);
    autobuf_read_advance(self, nread);
    return nread;
}

size_t autobuf_write(autobuf_t *self, const void *ptr, size_t len)
{
    if (len > autobuf_spare(self)) {
        autobuf_tidy(self);
        autobuf_realloc(self, self->size > len ? self->size<<1 : len<<1);
    }

    size_t len_can_in = len <= autobuf_spare(self) ? len : autobuf_spare(self);
    memcpy(autobuf_write_pos(self), ptr, len_can_in);
    autobuf_write_advance(self, len_can_in);

    return len_can_in;
}
