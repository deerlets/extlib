#include "quebufx.h"
#include <string.h>
#include <stdlib.h>

struct __queue_buf {
    char *rawbuf;
    size_t size;
    size_t offset_in;
    size_t offset_out;
};

quebuf_t *quebuf_new(size_t size)
{
    if (size == 0)
        size = QUEBUF_DEFAULT_SIZE;

    quebuf_t *self = (quebuf_t*)calloc(sizeof(quebuf_t), 1);
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

void quebuf_delete(quebuf_t *self)
{
    if (self) {
        free(self->rawbuf);
        free(self);
    }
}

int quebuf_realloc(quebuf_t *self, size_t len)
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

char *quebuf_rawbuf_out_pos(quebuf_t *self)
{
    return self->rawbuf + self->offset_out;
}

char *quebuf_rawbuf_in_pos(quebuf_t *self)
{
    return self->rawbuf + self->offset_in;
}

size_t quebuf_size(quebuf_t *self)
{
    return self->size;
}

size_t quebuf_garbage(quebuf_t *self)
{
    return self->offset_out;
}

size_t quebuf_used(quebuf_t *self)
{
    return self->offset_in - self->offset_out;
}

size_t quebuf_spare(quebuf_t *self)
{
    return self->size - self->offset_in;
}

size_t quebuf_collect(quebuf_t *self, int policy)
{
    int policy_do = 1;

    if (policy & QUEBUF_COLLECT_POLICY_LESS_SPARE) {
        if (quebuf_spare(self) > self->size>>2)
            policy_do = 0;
    }

    if (policy & QUEBUF_COLLECT_POLICY_LARGE_GARBAGE) {
        if (quebuf_garbage(self) < self->size>>2)
            policy_do = 0;
    }

    // condition 0: unsed == 0
    // condition 1: policy_do == 1
    if (!quebuf_used(self)) {
        self->offset_in = self->offset_out = 0;
    } else if (policy_do) {
        /* method 1
        self->offset_in -= self->offset_out;
        memmove(self->rawbuf, quebuf_rawbuf_out_pos(self), self->offset_in);
        self->offset_out = 0;
        */
        memmove(self->rawbuf, quebuf_rawbuf_out_pos(self), quebuf_used(self));
        self->offset_in = quebuf_used(self);
        self->offset_out = 0;
    }

    return quebuf_spare(self);
}

size_t quebuf_offset_out_head(quebuf_t *self, size_t len)
{
    self->offset_out += len;

    if (self->offset_out > self->offset_in)
        self->offset_out = self->offset_in;

    return quebuf_used(self);
}

size_t quebuf_offset_in_head(quebuf_t *self, size_t len)
{
    self->offset_in += len;

    if (self->offset_in > self->size)
        self->offset_in = self->size;

    return quebuf_spare(self);
}

size_t quebuf_peek(quebuf_t *self, void *ptr, size_t len)
{
    size_t len_can_out = len <= quebuf_used(self) ? len : quebuf_used(self);
    memcpy(ptr, quebuf_rawbuf_out_pos(self), len_can_out);

    return len_can_out;
}

size_t quebuf_read(quebuf_t *self, void *ptr, size_t len)
{
    int nread = quebuf_peek(self, ptr, len);
    quebuf_offset_out_head(self, nread);

    return nread;
}

size_t quebuf_write(quebuf_t *self, const void *ptr, size_t len)
{
    if (len > quebuf_spare(self))
        quebuf_collect(self, QUEBUF_COLLECT_POLICY_NONE);

    if (len > quebuf_spare(self))
        quebuf_realloc(self, self->size > len ? self->size<<1 : len<<1);

    size_t len_can_in = len <= quebuf_spare(self) ? len : quebuf_spare(self);
    memcpy(quebuf_rawbuf_in_pos(self), ptr, len_can_in);
    quebuf_offset_in_head(self, len_can_in);

    return len_can_in;
}
