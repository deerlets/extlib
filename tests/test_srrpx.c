#include <sched.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include "srrpx.h"

#define UNIX_ADDR "test_apisink_unix"

static void test_srrp(void **status)
{
    struct srrp_packet pac = {0};
    int nr = 0;

    const char req[] = ">0,$,58:hello/x{name:'yon',age:'18',equip:['hat','shoes']}";
    const char resp[] = "<0,$,60:0x13/hello/x{err:0,errmsg:'succ',data:{msg:'world'}}";

    nr = srrp_read_one_packet(req, sizeof(req), &pac);
    assert_true(nr == sizeof(req));
    assert_true(pac.leader == '>');
    assert_true(pac.seat == '$');
    assert_true(pac.len == strlen(req));
    assert_true(strcmp(pac.header, "hello/x") == 0);
    free((void *)pac.header);
    free((void *)pac.data);

    nr = srrp_read_one_packet(resp, sizeof(resp), &pac);
    assert_true(nr == sizeof(resp));
    assert_true(pac.leader == '<');
    assert_true(pac.seat == '$');
    assert_true(pac.len == strlen(resp));
    assert_true(strcmp(pac.header, "0x13/hello/x") == 0);
    free((void *)pac.header);
    free((void *)pac.data);

    int buf_len = strlen(req) + strlen(resp) + 2;
    char *buf = malloc(buf_len);
    memset(buf, 0, buf_len);
    memcpy(buf, req, strlen(req));
    memcpy(buf + strlen(req) + 1, resp, strlen(resp));

    nr = srrp_read_one_packet(buf, buf_len, &pac);
    assert_true(nr == strlen(req) + 1);
    assert_true(pac.leader == '>');
    assert_true(pac.seat == '$');
    assert_true(pac.len == strlen(req));
    assert_true(strcmp(pac.header, "hello/x") == 0);
    free((void *)pac.header);
    free((void *)pac.data);

    nr = srrp_read_one_packet(buf + nr, buf_len - nr, &pac);
    assert_true(nr == strlen(resp) + 1);
    assert_true(pac.leader == '<');
    assert_true(pac.seat == '$');
    assert_true(pac.len == strlen(resp));
    assert_true(strcmp(pac.header, "0x13/hello/x") == 0);
    free((void *)pac.header);
    free((void *)pac.data);

    free(buf);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_srrp),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
