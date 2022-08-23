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
#include "apix-posix.h"
#include "apix-station.h"
#include "srrp.h"
#include "log.h"

#define TCP_ADDR "127.0.0.1:12248"

static int test()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) return -1;

    struct sockaddr_in raddr;
    raddr.sin_family = AF_INET;
    raddr.sin_port = htons(12248);
    raddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(fd, (struct sockaddr*)&raddr, sizeof(raddr)) == -1) {
        close(fd);
        return -1;
    }
    LOG_DEBUG("Socket (C #%d) connected to %s:%d\n", fd,
              inet_ntoa(raddr.sin_addr), ntohs(raddr.sin_port));

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in laddr;
    socklen_t lsocklen = sizeof(laddr);
    getsockname(fd, (struct sockaddr*)&laddr, &lsocklen);

    while (1) {
        int nr = 0;
        char buf[256];
        struct srrp_packet *pac = srrp_write_request(fd, "/0012/echo", "{msg:'hello'}");
        nr = send(fd, pac->raw, pac->len, 0);
        LOG_INFO("%d, %s", nr, pac->raw);
        bzero(buf, sizeof(buf));
        usleep(50 * 1000);
        nr = recv(fd, buf, sizeof(buf), 0);
        LOG_INFO("%d, %s", nr, buf);
    }

    return 0;
}

int main(void)
{
    log_set_level(LOG_LV_DEBUG);
    test();
    return 0;
}
