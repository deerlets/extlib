#include <sched.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
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
#include "srrp.h"
#include "crc16.h"
#include "log.h"

#define UNIX_ADDR "test_apisink_unix"
#define TCP_ADDR "127.0.0.1:1224"

static int client_finished = 0;
static int server_finished = 0;

static void *client_thread(void *args)
{
    int fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (fd == -1)
        return NULL;

    int rc = 0;
    struct sockaddr_un addr = {0};
    addr.sun_family = PF_UNIX;
    strcpy(addr.sun_path, UNIX_ADDR);

    rc = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (rc == -1) {
        close(fd);
        return NULL;
    }

    sleep(1);

    char req[256] = {0};
    srrp_write_request(
        req, sizeof(req), fd, "/hello/x",
        "{name:'yon',age:'18',equip:['hat','shoes']}");

    rc = send(fd, req, strlen(req) + 1, 0);

    char buf[256] = {0};
    rc = recv(fd, buf, sizeof(buf), 0);
    LOG_INFO("client recv response: %s", buf);

    close(fd);
    client_finished = 1;
    return NULL;
}

static void *server_thread(void *args)
{
    int fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (fd == -1)
        return NULL;

    int rc = 0;
    struct sockaddr_un addr = {0};
    addr.sun_family = PF_UNIX;
    strcpy(addr.sun_path, UNIX_ADDR);

    rc = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (rc == -1) {
        close(fd);
        return NULL;
    }

    char buf[256] = {0};
    char req_add[256] = {0};
    char req_del[256] = {0};
    char resp[256] = {0};

    srrp_write_request(
        req_add, sizeof(req_add), fd, "/apicore/service/add",
        "{header:'/hello/x'}");
    srrp_write_request(
        req_del, sizeof(req_del), fd, "/apicore/service/del",
        "{header:'/hello/x'}");

    rc = send(fd, req_add, strlen(req_add) + 1, 0);
    memset(buf, 0, sizeof(buf));
    rc = recv(fd, buf, sizeof(buf), 0);
    LOG_INFO("server recv response: %s", buf);

    rc = recv(fd, buf, sizeof(buf), 0);
    LOG_INFO("server recv request: %s", buf);
    struct srrp_packet *pac;
    pac = srrp_read_one_packet(buf);
    uint16_t crc = crc16(pac->header, pac->header_len);
    crc = crc16_crc(crc, pac->data, pac->data_len);
    srrp_write_response(
        resp, sizeof(resp), pac->reqid, crc, pac->header,
        "{err:0,errmsg:'succ',data:{msg:'world'}}");
    rc = send(fd, resp, strlen(resp) + 1, 0);
    srrp_free(pac);

    rc = send(fd, req_del, strlen(req_del) + 1, 0);
    memset(buf, 0, sizeof(buf));
    rc = recv(fd, buf, sizeof(buf), 0);
    LOG_INFO("server recv response: %s", buf);

    close(fd);
    server_finished = 1;
    return NULL;
}

static void test_api_request_response(void **status)
{
    struct apicore *core = apicore_new();
    apicore_enable_posix(core);
    int fd = apicore_open_unix(core, UNIX_ADDR);

    pthread_t server_pid;
    pthread_create(&server_pid, NULL, server_thread, NULL);
    pthread_t client_pid;
    pthread_create(&client_pid, NULL, client_thread, NULL);

    while (client_finished == 0 || server_finished == 0)
        apicore_poll(core);

    pthread_join(client_pid, NULL);
    pthread_join(server_pid, NULL);

    apicore_close(core, fd);
    apicore_disable_posix(core);
    apicore_destroy(core);
}

static int publish_finished = 0;
static int subscribe_finished = 0;

static void *publish_thread(void *args)
{
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1)
        return NULL;

    uint32_t host;
    uint16_t port;
    char *tmp = strdup(TCP_ADDR);
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

    rc = connect(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    if (rc == -1) {
        close(fd);
        return NULL;
    }

    sleep(1);

    char pub[256] = {0};
    srrp_write_publish(pub, sizeof(pub), "/test-topic", "{msg:'ahaa'}");
    rc = send(fd, pub, strlen(pub) + 1, 0);

    close(fd);
    publish_finished = 1;
    return NULL;
}

static void *subscribe_thread(void *args)
{
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1)
        return NULL;

    uint32_t host;
    uint16_t port;
    char *tmp = strdup(TCP_ADDR);
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

    rc = connect(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    if (rc == -1) {
        close(fd);
        return NULL;
    }

    char buf[256] = {0};
    char sub[256] = {0};
    char unsub[256] = {0};

    srrp_write_subscribe(sub, sizeof(sub), "/test-topic", "{}");
    srrp_write_unsubscribe(unsub, sizeof(unsub), "/test-topic");

    rc = send(fd, sub, strlen(sub) + 1, 0);
    memset(buf, 0, sizeof(buf));
    rc = recv(fd, buf, sizeof(buf), 0);
    LOG_INFO("server recv sub: %s", buf);

    rc = recv(fd, buf, sizeof(buf), 0);
    LOG_INFO("server recv pub: %s", buf);

    rc = send(fd, unsub, strlen(unsub) + 1, 0);
    memset(buf, 0, sizeof(buf));
    rc = recv(fd, buf, sizeof(buf), 0);
    LOG_INFO("server recv unsub: %s", buf);

    close(fd);
    subscribe_finished = 1;
    return NULL;
}

static void test_api_subscribe_publish(void **status)
{
    struct apicore *core = apicore_new();
    apicore_enable_posix(core);
    int fd = apicore_open_tcp(core, TCP_ADDR);

    pthread_t subscribe_pid;
    pthread_create(&subscribe_pid, NULL, subscribe_thread, NULL);
    pthread_t publish_pid;
    pthread_create(&publish_pid, NULL, publish_thread, NULL);

    while (publish_finished == 0 || subscribe_finished == 0)
        apicore_poll(core);

    pthread_join(publish_pid, NULL);
    pthread_join(subscribe_pid, NULL);

    apicore_close(core, fd);
    apicore_disable_posix(core);
    apicore_destroy(core);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_api_request_response),
        cmocka_unit_test(test_api_subscribe_publish),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
