#ifndef __JSONX_H
#define __JSONX_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct json_object;

struct json_object *json_object_new(const char *str);
void json_object_delete(struct json_object *jo);

int json_get_string(struct json_object *jo, const char *path, char *value, size_t size);
int json_get_int(struct json_object *jo, const char *path, int *value);

#ifdef __cplusplus
}
#endif
#endif
