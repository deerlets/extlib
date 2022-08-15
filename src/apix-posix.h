#ifndef __APIX_POSIX_H
#define __APIX_POSIX_H

#include "apix.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __unix__

#define APISINK_UNIX "apisink_unix"

/*fd*/ int apicore_open_pipe(struct apicore *core, /*out*/ int *txfd, /*out*/ int *rxfd);
/*fd*/ int apicore_open_shm(struct apicore *core, const char *txname, const char *rxname);
/*fd*/ int apicore_open_shm_memfd(struct apicore *core, /*out*/ int *txfd, /*out*/ int *rxfd);
/*fd*/ int apicore_open_shm_ftok(struct apicore *core, const char *txfile, const char *rxfile);
/*fd*/ int apicore_open_serial(struct apicore *core, const char *addr);
/*fd*/ int apicore_open_unix(struct apicore *core, const char *name);
/*fd*/ int apicore_open_tcp(struct apicore *core, uint32_t ipaddr, uint16_t port);
/*fd*/ int apicore_open_udp(struct apicore *core, uint32_t ipaddr, uint16_t port);
/*fd*/ int apicore_open_http(struct apicore *core, uint32_t ipaddr, uint16_t port);

int apicore_enable_posix(struct apicore *core);
void apicore_disable_posix(struct apicore *core);

#endif

#ifdef __cplusplus
}
#endif
#endif
