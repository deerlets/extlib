#include "apix-inl.h"
#include "apix-posix.h"
#include "autobufx.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>
#include "stddefx.h"
#include "listx.h"
#include "logx.h"

#if defined __unix__ || defined __linux__ || defined __APPLE__

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static struct unix_sink {
    struct apisink sink;
    // for select
    fd_set fds;
    int nfds;
} __unix_sink;

static int unix_open(struct apisink *sink, const char *addr)
{
    int fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (fd == -1)
        return -1;

    int rc = 0;
    struct sockaddr_un sockaddr = {0};
    sockaddr.sun_family = PF_UNIX;
    snprintf(sockaddr.sun_path, sizeof(sockaddr.sun_path), "%s", addr);

    unlink(addr);
    rc = bind(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    if (rc == -1) {
        close(fd);
        return -1;
    }

    rc = listen(fd, 100);
    if (rc == -1) {
        close(fd);
        return -1;
    }

    struct sinkfd *sinkfd = sinkfd_new();
    sinkfd->fd = fd;
    sinkfd->listen = 1;
    snprintf(sinkfd->addr, sizeof(sinkfd->addr), "%s", addr);
    sinkfd->sink = sink;
    list_add(&sinkfd->node_sink, &sink->sinkfds);
    list_add(&sinkfd->node_core, &sink->core->sinkfds);

    struct unix_sink *unix_sink = container_of(sink, struct unix_sink, sink);
    FD_SET(fd, &unix_sink->fds);
    unix_sink->nfds = fd + 1;

    return fd;
}

static int unix_close(struct apisink *sink, int fd)
{
    struct sinkfd *sinkfd = find_sinkfd_in_apisink(sink, fd);
    if (sinkfd == NULL)
        return -1;
    close(sinkfd->fd);
    unlink(sinkfd->addr);
    sinkfd_destroy(sinkfd);
    return 0;
}

static int unix_send(struct apisink *sink, int fd, const void *buf, size_t len)
{
    UNUSED(sink);
    return send(fd, buf, len, 0);
}

static int unix_recv(struct apisink *sink, int fd, void *buf, size_t size)
{
    UNUSED(sink);
    return recv(fd, buf, size, 0);
}

static int unix_poll(struct apisink *sink, int timeout)
{
    struct unix_sink *unix_sink = container_of(sink, struct unix_sink, sink);

    struct timeval tv = { timeout / 1000, timeout % 1000 * 1000 };
    fd_set recvfds;
    memcpy(&recvfds, &unix_sink->fds, sizeof(recvfds));

    int nr_recv_fds = select(unix_sink->nfds, &recvfds, NULL, NULL, &tv);
    if (nr_recv_fds == -1) {
        if (errno == EINTR)
            return 0;
        LOG_ERROR("[select] (%d) %s", errno, strerror(errno));
        return -1;
    }

    struct sinkfd *pos, *n;
    list_for_each_entry_safe(pos, n, &sink->sinkfds, node_sink) {
        if (nr_recv_fds == 0) break;

        if (!FD_ISSET(pos->fd, &recvfds))
            continue;

        nr_recv_fds--;

        // accept
        if (pos->listen == 1) {
            int newfd = accept(pos->fd, NULL, NULL);
            if (newfd == -1) {
                LOG_ERROR("[accept] (%d) %s", errno, strerror(errno));
                continue;
            }

            struct sinkfd *sinkfd = sinkfd_new();
            sinkfd->fd = newfd;
            sinkfd->sink = sink;
            list_add(&sinkfd->node_sink, &sink->sinkfds);
            list_add(&sinkfd->node_core, &sink->core->sinkfds);

            if (unix_sink->nfds < newfd + 1)
                unix_sink->nfds = newfd + 1;
            FD_SET(newfd, &unix_sink->fds);
        } else /* recv */ {
            autobuf_tidy(pos->rxbuf);
            int nread = recv(pos->fd, autobuf_write_pos(pos->rxbuf),
                             autobuf_spare(pos->rxbuf), 0);
            if (nread == -1) {
                LOG_DEBUG("[recv] (%d) %s", errno, strerror(errno));
                sinkfd_destroy(pos);
            } else if (nread == 0) {
                LOG_DEBUG("[recv] (%d) finished");
                sinkfd_destroy(pos);
            } else {
                autobuf_write_advance(pos->rxbuf, nread);
            }
        }
    }

    return 0;
}

static apisink_ops_t unix_ops = {
    .open = unix_open,
    .close = unix_close,
    .send = unix_send,
    .recv = unix_recv,
    .poll = unix_poll,
};

static struct unix_sink __tcp_sink;

static int tcp_open(struct apisink *sink, const char *addr)
{
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1)
        return -1;

    uint32_t host;
    uint16_t port;
    char *tmp = strdup(addr);
    char *colon = strchr(tmp, ':');
    *colon = 0;
    host = inet_addr(tmp);
    port = htons(atoi(colon + 1));
    free(tmp);

    int rc = 0;
    struct sockaddr_in sockaddr = {0};
    sockaddr.sin_family = PF_INET;
    sockaddr.sin_addr.s_addr = host;
    sockaddr.sin_port = port;

    rc = bind(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    if (rc == -1) {
        close(fd);
        return -1;
    }

    rc = listen(fd, 100);
    if (rc == -1) {
        close(fd);
        return -1;
    }

    struct sinkfd *sinkfd = sinkfd_new();
    sinkfd->fd = fd;
    sinkfd->listen = 1;
    snprintf(sinkfd->addr, sizeof(sinkfd->addr), "%s", addr);
    sinkfd->sink = sink;
    list_add(&sinkfd->node_sink, &sink->sinkfds);
    list_add(&sinkfd->node_core, &sink->core->sinkfds);

    struct unix_sink *tcp_sink = container_of(sink, struct unix_sink, sink);
    FD_SET(fd, &tcp_sink->fds);
    tcp_sink->nfds = fd + 1;

    return fd;
}

static int tcp_close(struct apisink *sink, int fd)
{
    struct sinkfd *sinkfd = find_sinkfd_in_apisink(sink, fd);
    if (sinkfd == NULL)
        return -1;
    close(sinkfd->fd);
    sinkfd_destroy(sinkfd);
    return 0;
}

static apisink_ops_t tcp_ops = {
    .open = tcp_open,
    .close = tcp_close,
    .send = unix_send,
    .recv = unix_recv,
    .poll = unix_poll,
};

int apicore_enable_posix(struct apicore *core)
{
    apisink_init(&__unix_sink.sink, APISINK_UNIX, unix_ops);
    apicore_add_sink(core, &__unix_sink.sink);

    apisink_init(&__tcp_sink.sink, APISINK_TCP, tcp_ops);
    apicore_add_sink(core, &__tcp_sink.sink);

    return 0;
}

void apicore_disable_posix(struct apicore *core)
{
    apicore_del_sink(core, &__unix_sink.sink);
    apisink_fini(&__unix_sink.sink);

    apicore_del_sink(core, &__tcp_sink.sink);
    apisink_fini(&__tcp_sink.sink);
}

#endif
