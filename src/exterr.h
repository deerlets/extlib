#ifndef __EXT_ERR_H
#define __EXT_ERR_H

#ifdef __cplusplus
extern "C" {
#endif

#define EXTERR_ERRNO_MAP(MACRO)                 \
    MACRO(EOK, "OK")                            \
    MACRO(EPERM, "no permission")               \
    MACRO(EINVAL, "invalid argument")           \
    MACRO(EFAIL, "operation failed")            \
    MACRO(ETIMEOUT, "operation timeout")        \
    MACRO(EEXIST, "object exist")               \
    MACRO(EBUSY, "object is busy")

enum {
#define XX(code, _) EXTERR_##code,
    EXTERR_ERRNO_MAP(XX)
#undef XX
    EXTERR_ERRNO_USER = 10000,
};

#define EXT_STRERR_GEN(name, msg) case EXTERR_##name: return msg;
static const char *ext_strerr(int __errno) {
    switch (__errno) { EXTERR_ERRNO_MAP(EXT_STRERR_GEN) }
    return "Unknown errno";
}
#undef EXT_STRERR_GEN

/*
 * dynamic ext_strerr2
 */

void exterr_unregister_all();
int exterr_register(int __errno, const char *errmsg);
const char *ext_strerr2(int __errno);

#ifdef __cplusplus
}
#endif
#endif
