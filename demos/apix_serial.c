#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "apix.h"
#include "apix-posix.h"
#include "srrpx.h"
#include "crc16.h"
#include "logx.h"

#define SERIAL_ADDR "/dev/ttyUSB0"

static void test_api_serial(void **status)
{
    struct apicore *core = apicore_new();
    apicore_enable_posix(core);
    int fd = apicore_open_serial(core, SERIAL_ADDR);
    assert(fd != -1);

    struct ioctl_serial_param sp = {
        .baud = SERIAL_ARG_BAUD_115200,
        .bits = SERIAL_ARG_BITS_8,
        .parity = SERIAL_ARG_PARITY_N,
        .stop = SERIAL_ARG_STOP_1,
    };
    int rc = apicore_ioctl(core, fd, 0, (unsigned long)&sp);
    assert(rc != -1);

    //int nr = 0;
    //char buf[256];
    //for (int i = 0; i < 3; i++) {
    //    nr = apicore_send(core, fd, "hello world\n", 12);
    //    LOG_INFO("%d", nr);
    //    bzero(buf, sizeof(buf));
    //    sleep(1);
    //    nr = apicore_recv(core, fd, buf, sizeof(buf));
    //    LOG_INFO("%d, %s", nr, buf);
    //}

    while (1) {
        apicore_poll(core, 1000);
    }

    apicore_close(core, fd);
    apicore_disable_posix(core);
    apicore_destroy(core);
}

int main(void)
{
    log_set_level(LOG_LV_DEBUG);
    test_api_serial(NULL);
    return 0;
}
