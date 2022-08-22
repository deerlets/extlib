#include <sched.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include "srrp.h"
#include "crc16.h"

#define UNIX_ADDR "test_apisink_unix"

static void test_srrp_request_reponse(void **status)
{
    struct srrp_packet *pac = NULL;

    char req[256] = {0};
    char resp[256] = {0};

    srrp_write_request(
        req, sizeof(req), 0x8888, "/hello/x",
        "{name:'yon',age:'18',equip:['hat','shoes']}");

    pac = srrp_read_one_packet(req);
    assert_true(pac);
    assert_true(pac->len == strlen(req) + 1);
    assert_true(pac->leader == '>');
    assert_true(pac->seat == '$');
    assert_true(pac->reqid == 0x8888);
    assert_true(memcmp(pac->header, "/hello/x", pac->header_len) == 0);

    uint16_t crc = crc16(pac->header, pac->header_len);
    crc = crc16_crc(crc, pac->data, pac->data_len);
    srrp_write_response(
        resp, sizeof(resp), 0x8888, crc, "/hello/x",
        "{err:0,errmsg:'succ',data:{msg:'world'}}");
    srrp_free(pac);

    pac = srrp_read_one_packet(resp);
    assert_true(pac);
    assert_true(pac->len == strlen(resp) + 1);
    assert_true(pac->leader == '<');
    assert_true(pac->seat == '$');
    assert_true(pac->reqid == 0x8888);
    assert_true(pac->reqcrc16 == crc);
    assert_true(memcmp(pac->header, "/hello/x", pac->header_len) == 0);
    srrp_free(pac);

    int buf_len = strlen(req) + strlen(resp) + 2;
    char *buf = malloc(buf_len);
    memset(buf, 0, buf_len);
    memcpy(buf, req, strlen(req));
    memcpy(buf + strlen(req) + 1, resp, strlen(resp));

    pac = srrp_read_one_packet(buf);
    assert_true(pac);
    assert_true(pac->len == strlen(req) + 1);
    assert_true(pac->leader == '>');
    assert_true(pac->seat == '$');
    assert_true(pac->reqid == 0x8888);
    assert_true(memcmp(pac->header, "/hello/x", pac->header_len) == 0);
    int len = pac->len;
    srrp_free(pac);

    pac = srrp_read_one_packet(buf + len);
    assert_true(pac);
    assert_true(pac->len == strlen(resp) + 1);
    assert_true(pac->leader == '<');
    assert_true(pac->seat == '$');
    assert_true(pac->reqid == 0x8888);
    assert_true(pac->reqcrc16 == crc);
    assert_true(memcmp(pac->header, "/hello/x", pac->header_len) == 0);
    srrp_free(pac);

    free(buf);
}

static void test_srrp_subscribe_publish(void **status)
{
    struct srrp_packet *pac = NULL;

    char sub[256] = {0};
    char unsub[256] = {0};
    char pub[256] = {0};

    srrp_write_subscribe(sub, sizeof(sub), "/motor/speed", "{ack:0,cache:100}");
    srrp_write_unsubscribe(unsub, sizeof(unsub), "/motor/speed");
    srrp_write_publish(pub, sizeof(pub), "/motor/speed", "{speed:12,voltage:24}");

    pac = srrp_read_one_packet(sub);
    assert_true(pac);
    assert_true(pac->len == strlen(sub) + 1);
    assert_true(pac->leader == '#');
    assert_true(pac->seat == '$');
    assert_true(memcmp(pac->header, "/motor/speed", pac->header_len) == 0);
    srrp_free(pac);

    pac = srrp_read_one_packet(unsub);
    assert_true(pac);
    assert_true(pac->len == strlen(unsub) + 1);
    assert_true(pac->leader == '%');
    assert_true(pac->seat == '$');
    assert_true(memcmp(pac->header, "/motor/speed", pac->header_len) == 0);
    srrp_free(pac);

    pac = srrp_read_one_packet(pub);
    assert_true(pac);
    assert_true(pac->len == strlen(pub) + 1);
    assert_true(pac->leader == '@');
    assert_true(pac->seat == '$');
    assert_true(memcmp(pac->header, "/motor/speed", pac->header_len) == 0);

    int buf_len = strlen(sub) + strlen(pub) + 2;
    char *buf = malloc(buf_len);
    memset(buf, 0, buf_len);
    memcpy(buf, sub, strlen(sub));
    memcpy(buf + strlen(sub) + 1, pub, strlen(pub));
    srrp_free(pac);

    pac = srrp_read_one_packet(buf);
    assert_true(pac);
    assert_true(pac->len == strlen(sub) + 1);
    assert_true(pac->leader == '#');
    assert_true(pac->seat == '$');
    assert_true(memcmp(pac->header, "/motor/speed", pac->header_len) == 0);
    int len = pac->len;
    srrp_free(pac);

    pac = srrp_read_one_packet(buf + len);
    assert_true(pac);
    assert_true(pac->len == strlen(pub) + 1);
    assert_true(pac->leader == '@');
    assert_true(pac->seat == '$');
    assert_true(memcmp(pac->header, "/motor/speed", pac->header_len) == 0);
    srrp_free(pac);

    free(buf);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_srrp_request_reponse),
        cmocka_unit_test(test_srrp_subscribe_publish),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
