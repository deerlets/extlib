#include <sched.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "apix.h"
#include "apix-posix.h"

#define UNIX_ADDR "test_apisink_unix"

static int client_finished = 0;

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
    rc = send(fd, "hello", 5, 0);

    client_finished = 1;
    close(fd);
    return NULL;
}

static void test_api(void **status)
{
    struct apicore *core = apicore_new();
    apicore_enable_posix(core);
    int fd = apicore_open(core, APISINK_UNIX, UNIX_ADDR);

    pthread_t client_pid;
    pthread_create(&client_pid, NULL, client_thread, NULL);

    while (client_finished == 0)
        apicore_poll(core, 1000);

    pthread_join(client_pid, NULL);

    apicore_close(core, fd);
    apicore_disable_posix(core);
    apicore_destroy(core);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_api),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
