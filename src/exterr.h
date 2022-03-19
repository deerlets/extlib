#ifndef __EXT_ERR_H
#define __EXT_ERR_H

#ifdef __cplusplus
extern "C" {
#endif

void exterr_unregister_all();
int exterr_register(int __errno, const char *errmsg);
const char *ext_strerr(int __errno);

/*
 * macro template with exterr_init()
 */

#define EXTERR_ERRNO_MAP(MACRO)                 \
    MACRO(EPERM, "no permission")               \
    MACRO(EINVAL, "invalid argument")           \
    MACRO(EFAIL, "operation failed")            \
    MACRO(ETIMEOUT, "operation timeout")        \
    MACRO(EEXIST, "object exist")               \
    MACRO(EBUSY, "object is busy")

enum {
    EXTERR_EOK = 0,
#define XX(code, _) EXTERR_##code,
    EXTERR_ERRNO_MAP(XX)
#undef XX
    EXTERR_ERRNO_USER = 10000,
};

#define EXTERR_INIT_GEN(name, errmsg) exterr_register(EXTERR_##name, errmsg);
static inline void exterr_init() { EXTERR_ERRNO_MAP(EXTERR_INIT_GEN) }
#undef EXTERR_INIT_GEN

#ifdef __cplusplus
}
#endif
#endif
