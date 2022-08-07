#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include "quebufx.h"

static void test_quebuf(void **status)
{
    quebuf_t *qbuf = quebuf_new(4096);
    assert_true(quebuf_size(qbuf) == 4096);

    char *msg_hello = "hello world";
    size_t nr_hello = quebuf_write(qbuf, msg_hello, strlen(msg_hello));
    assert_true(nr_hello == strlen(msg_hello));
    assert_true(quebuf_used(qbuf) == strlen(msg_hello));
    assert_true(quebuf_spare(qbuf) == quebuf_size(qbuf) - strlen(msg_hello));

    char msg_large[1024 * 3] = {0};
    memset(msg_large, 0x7c, sizeof(msg_large));
    size_t nr_large = quebuf_write(qbuf, msg_large, sizeof(msg_large));
    assert_true(nr_large == sizeof(msg_large));
    assert_true(quebuf_used(qbuf) == nr_hello + sizeof(msg_large));
    assert_true(quebuf_spare(qbuf) == quebuf_size(qbuf) - nr_hello - nr_large);

    char buf_hello[256] = {0};
    size_t nr_read_hello = quebuf_read(qbuf, buf_hello, nr_hello);
    assert_true(nr_read_hello == nr_hello);
    assert_true(strcmp(msg_hello, buf_hello) == 0);
    assert_true(quebuf_used(qbuf) == sizeof(msg_large));
    assert_true(quebuf_spare(qbuf) == quebuf_size(qbuf) - nr_large);

    char buf_large[1024] = {0};
    size_t nr_read_large = quebuf_read(qbuf, buf_large, sizeof(buf_large));
    assert_true(nr_read_large == sizeof(buf_large));
    assert_true(memcmp(buf_large, msg_large, sizeof(buf_large)) == 0);
    assert_true(quebuf_used(qbuf) == sizeof(msg_large) - sizeof(buf_large));
    assert_true(quebuf_spare(qbuf) == quebuf_size(qbuf) - nr_large + sizeof(buf_large));

    char msg_last[1024] = {0};
    memset(msg_last, 0x3f, sizeof(msg_last));
    size_t nr_last = quebuf_write(qbuf, msg_last, sizeof(msg_last));
    assert_true(nr_last == sizeof(msg_last));
    assert_true(quebuf_used(qbuf) == sizeof(msg_large) -
                sizeof(buf_large) + sizeof(msg_last));
    assert_true(quebuf_spare(qbuf) == quebuf_size(qbuf) -
                nr_large + sizeof(buf_large) - sizeof(msg_last));

    char buf_last[4096] = {0};
    size_t nr_read_last = quebuf_read(qbuf, buf_last, sizeof(buf_last));
    assert_true(nr_read_last == sizeof(msg_large) -
                sizeof(buf_large) + sizeof(msg_last));
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_quebuf),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
