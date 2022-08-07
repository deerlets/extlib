#include "quebufx.h"
#include <assert.h>
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

size_t quebuf_size(quebuf_t *self)
{
    return self->size;
}

size_t quebuf_used(quebuf_t *self)
{
    if (self->offset_in >= self->offset_out)
        return self->offset_in - self->offset_out;
    else
        return self->offset_in + (self->size - self->offset_out);
}

size_t quebuf_spare(quebuf_t *self)
{
    return self->size - quebuf_used(self);
}

static char *quebuf_in_pos(quebuf_t *self)
{
    return self->rawbuf + self->offset_in;
}

static char *quebuf_out_pos(quebuf_t *self)
{
    return self->rawbuf + self->offset_out;
}

void quebuf_in_head(quebuf_t *self, size_t len)
{
    assert(quebuf_spare(self) >= len);
    self->offset_in = (self->offset_in + len) % self->size;
}

void quebuf_out_head(quebuf_t *self, size_t len)
{
    assert(quebuf_used(self) >= len);
    self->offset_out = (self->offset_out + len) % self->size;
}

size_t quebuf_write(quebuf_t *self, const void *ptr, size_t len)
{
    assert(len < self->size);
    size_t cpy_cnt = 0;

    if (self->offset_in >= self->offset_out) {
        size_t spare_right = self->size - self->offset_in;
        size_t spare_left = self->offset_out;
        if (len <= spare_right) {
            memcpy(quebuf_in_pos(self), ptr, len);
            cpy_cnt += len;
        } else {
            memcpy(quebuf_in_pos(self), ptr, spare_right);
            cpy_cnt += spare_right;
            if (len - spare_right <= spare_left) {
                memcpy(self->rawbuf, ptr + cpy_cnt, len - spare_right);
                cpy_cnt += len - spare_right;
            } else {
                memcpy(self->rawbuf, ptr + cpy_cnt, spare_left);
                cpy_cnt += spare_left;
            }
        }
    } else {
        size_t spare = self->offset_out - self->offset_in;
        if (len <= spare) {
            memcpy(quebuf_in_pos(self), ptr, len);
            cpy_cnt += len;
        } else {
            memcpy(quebuf_in_pos(self), ptr, spare);
            cpy_cnt += spare;
        }
    }

    quebuf_in_head(self, cpy_cnt);
    return cpy_cnt;
}

size_t quebuf_peek(quebuf_t *self, void *ptr, size_t size)
{
    size_t cpy_cnt = quebuf_used(self);
    if (size < cpy_cnt)
        cpy_cnt = size;

    if (self->offset_in >= self->offset_out) {
        memcpy(ptr, quebuf_out_pos(self), cpy_cnt);
    }
    else {
        size_t cpy_right = self->size - self->offset_out;
        size_t cpy_left = cpy_cnt - cpy_right;
        memcpy(ptr, quebuf_out_pos(self), cpy_right);
        memcpy(ptr + cpy_right, self->rawbuf, cpy_left);
    }

    return cpy_cnt;
}

size_t quebuf_read(quebuf_t *self, void *ptr, size_t size)
{
    size_t cpy_cnt = quebuf_peek(self, ptr, size);
    quebuf_out_head(self, cpy_cnt);
    return cpy_cnt;
}
