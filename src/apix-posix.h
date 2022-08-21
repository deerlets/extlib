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
#define APISINK_UDP "apisink_udp"
#define APISINK_SERIAL "apisink_serial"
#define APISINK_CAN "apisink_can"
#define APISINK_SPI "apisink_spi"
#define APISINK_I2C "apisink_i2c"
#define APISINK_PIPE "apisink_pipe"
#define APISINK_SHM "apisink_shm"
#define APISINK_SHM_MEMFD "apisink_shm_memfd"
#define APISINK_SHM_FTOK "apisink_shm_ftok"

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

#define apicore_open_unix(core, addr) apicore_open(core, APISINK_UNIX, addr)
#define apicore_open_tcp(core, addr) apicore_open(core, APISINK_TCP, addr)
#define apicore_open_serial(core, addr) apicore_open(core, APISINK_SERIAL, addr)

int apicore_enable_posix(struct apicore *core);
void apicore_disable_posix(struct apicore *core);

#endif

#ifdef __cplusplus
}
#endif
#endif
