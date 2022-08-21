#ifndef __ERR_H
#define __ERR_H

#ifdef __cplusplus
extern "C" {
#endif

#define ERRX_ERRNO_MAP(MACRO)                 \
    MACRO(EOK, "OK")                            \
    MACRO(EPERM, "no permission")               \
    MACRO(EINVAL, "invalid argument")           \
    MACRO(EFAIL, "operation failed")            \
    MACRO(ETIMEOUT, "operation timeout")        \
    MACRO(EEXIST, "object exist")               \
    MACRO(EBUSY, "object is busy")

enum {
#define XX(code, _) ERRX_##code,
    ERRX_ERRNO_MAP(XX)
#undef XX
    ERRX_ERRNO_USER = 10000,
};

#define ERRX_STRERR_GEN(name, msg) case ERRX_##name: return msg;
static __attribute__((unused)) const char *errx_strerr_base(int __errno) {
    switch (__errno) { ERRX_ERRNO_MAP(ERRX_STRERR_GEN) }
    return "Unknown errno";
}
#undef ERRX_STRERR_GEN

int errx_register(int __errno, const char *errmsg);
const char *errx_strerr(int __errno);

#ifdef __cplusplus
}
#endif
#endif
