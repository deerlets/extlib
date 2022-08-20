#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include "jsonx.h"

static void test_json(void **status)
{
    struct json_object *jo = json_object_new(
        "{header: '/hello/x', test: {len: 12}}");

    int value = 0;
    assert_true(json_get_int(jo, "/test/len", &value) == 0);
    assert_true(value == 12);

    json_object_delete(jo);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_json),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
