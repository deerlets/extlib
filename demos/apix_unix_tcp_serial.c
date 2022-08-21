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
#include "optx.h"
#include "logx.h"

#define SERIAL_ADDR "/dev/ttyUSB0"

static struct opt opttab[] = {
    INIT_OPT_BOOL("-h", "help", false, "print this usage"),
    INIT_OPT_BOOL("-D", "debug", false, "debug mode [defaut: false]"),
    INIT_OPT_STRING("-u:", "unix", "./apix-unix-domain", "unix domain"),
    INIT_OPT_STRING("-t:", "tcp", "0.0.0.0:12248", "tcp socket"),
    INIT_OPT_STRING("-s:", "serial", "/dev/ttyUSB0", "serial dev file"),
    INIT_OPT_NONE(),
};

static void run_apicore()
{
    struct opt *ud = find_opt("unix", opttab);
    struct opt *tcp = find_opt("tcp", opttab);
    struct opt *serial = find_opt("serial", opttab);

    struct apicore *core = apicore_new();
    apicore_enable_posix(core);

    int fd = 0;
    int rc = 0;

    fd = apicore_open_unix(core, opt_string(ud));
    assert(fd != -1);
    fd = apicore_open_tcp(core, opt_string(tcp));
    assert(fd != -1);

    fd = apicore_open_serial(core, opt_string(serial));
    assert(fd != -1);
    struct ioctl_serial_param sp = {
        .baud = SERIAL_ARG_BAUD_115200,
        .bits = SERIAL_ARG_BITS_8,
        .parity = SERIAL_ARG_PARITY_N,
        .stop = SERIAL_ARG_STOP_1,
    };
    rc = apicore_ioctl(core, fd, 0, (unsigned long)&sp);
    assert(rc != -1);

    while (1) {
        apicore_poll(core, 1000);
    }

    apicore_close(core, fd);
    apicore_disable_posix(core);
    apicore_destroy(core);
}

int main(int argc, char *argv[])
{
    log_set_level(LOG_LV_DEBUG);
    opt_init_from_arg(opttab, argc, argv);
    run_apicore();
    return 0;
}
