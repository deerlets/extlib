#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include "autobufx.h"

static void test_autobuf(void **status)
{
    autobuf_t *buf;
    char msg[1024];
    size_t nread;
    char hello_msg[] = "hello buffer";
    int write_time = AUTOBUF_DEFAULT_SIZE / sizeof(hello_msg) / 3 * 2;
    int read_time = write_time / 2;

    buf = autobuf_new(AUTOBUF_DEFAULT_SIZE);
    assert_true(autobuf_size(buf) == AUTOBUF_DEFAULT_SIZE);
    assert_true(autobuf_garbage(buf) == 0);
    assert_true(autobuf_used(buf) == 0);
    assert_true(autobuf_spare(buf) == AUTOBUF_DEFAULT_SIZE);

    for (int i = 0; i < write_time; i++)
        autobuf_write(buf, (void*)hello_msg, sizeof(hello_msg));
    assert_true(autobuf_size(buf) == AUTOBUF_DEFAULT_SIZE);
    assert_true(autobuf_garbage(buf) == 0);
    assert_true(autobuf_used(buf) == sizeof(hello_msg)*write_time);
    assert_true(autobuf_spare(buf) == AUTOBUF_DEFAULT_SIZE - sizeof(hello_msg)*write_time);

    for (int i = 0; i < read_time; i++)
        nread = autobuf_read(buf, msg, sizeof(hello_msg));
    assert_true(autobuf_size(buf) == AUTOBUF_DEFAULT_SIZE);
    //assert_true(autobuf_garbage(buf) == sizeof(hello_msg)*read_time);
    assert_true(autobuf_used(buf) == sizeof(hello_msg)*(write_time-read_time));
    //assert_true(autobuf_spare(buf) == AUTOBUF_DEFAULT_SIZE - sizeof(hello_msg)*write_time);
    assert_true(memcmp(hello_msg, msg, strlen(hello_msg)) == 0);
    msg[nread] = 0;

    assert_true(autobuf_size(buf) == AUTOBUF_DEFAULT_SIZE);
    //assert_true(autobuf_garbage(buf) == 0);
    assert_true(autobuf_used(buf) == sizeof(hello_msg)*(write_time-read_time));
    //assert_true(autobuf_spare(buf) == AUTOBUF_DEFAULT_SIZE - sizeof(hello_msg)*(write_time-read_time));

    autobuf_delete(buf);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_autobuf),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
