#ifndef __APIX_POSIX_H
#define __APIX_POSIX_H

#include "apix.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined __unix__ || defined __linux__ || defined __APPLE__

#define APISINK_UNIX "apisink_unix"
#define APISINK_TCP "apisink_tcp"
#define APISINK_SERIAL "apisink_serial"

#define SERIAL_ARG_BAUD_9600 9600
#define SERIAL_ARG_BAUD_115200 115200
#define SERIAL_ARG_BITS_7 7
#define SERIAL_ARG_BITS_8 8
#define SERIAL_ARG_PARITY_O 'O'
#define SERIAL_ARG_PARITY_E 'E'
#define SERIAL_ARG_PARITY_N 'N'
#define SERIAL_ARG_STOP_1 1
#define SERIAL_ARG_STOP_2 2

struct ioctl_serial_param {
    uint32_t baud;
    char bits;
    char parity;
    char stop;
};

typedef enum {
    IPC_MODE_PIPE = 0,
    IPC_MODE_SHM,
    IPC_MODE_SHM_MEMFD,
    IPC_MODE_SHM_FTOK,
    IPC_MODE_SERIAL,
    IPC_MODE_UNIX,
    IPC_MODE_TCP,
    IPC_MODE_UDP,
    IPC_MODE_HTTP,
} ipc_mode_t;

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
