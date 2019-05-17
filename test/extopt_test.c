#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <extopt.h>
#include <stdio.h>

static int argc = 4;
static char *argv[] = {
	"extopt-test",
	"-D",
	"-p",
	"1024",
};

#define OPTFILE_NAME "extopt-test.conf"
#define OPTFILE_MOCK \
	"debug: true\n" \
	"pid: 1024\n"

static struct opt opttab[] = {
	INIT_OPT_BOOL("-h", "help", false, "print this usage"),
	INIT_OPT_BOOL("-D", "debug", false, "debug mode [defaut: false]"),
	INIT_OPT_INT("-p:", "pid", 12, "pid of the target process"),
	INIT_OPT_NONE(),
};

static void check_opts(void)
{
	struct opt *help = find_opt("help", opttab);
	assert_true(help);
	assert_false(opt_bool(help));

	struct opt *debug = find_opt("debug", opttab);
	assert_true(debug);
	assert_true(opt_bool(debug));

	struct opt *pid = find_opt("pid", opttab);
	assert_true(pid);
	assert_true(opt_int(pid) == 1024);
}

static void test_opt_init_from_arg(void **status)
{
	assert_true(opt_init_from_arg(opttab, argc, argv) == 0);
	check_opts();
	opt_fini(opttab);
}

static void test_opt_init_from_file(void **status)
{
	FILE *fp = fopen(OPTFILE_NAME, "w+");
	assert_true(fp);
	int nr = fwrite(OPTFILE_MOCK, 1, sizeof(OPTFILE_MOCK), fp);
	assert_true(nr == sizeof(OPTFILE_MOCK));
	fclose(fp);

	assert_true(opt_init_from_file(opttab, OPTFILE_NAME) == 0);
	check_opts();
	opt_fini(opttab);

	remove(OPTFILE_NAME);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_opt_init_from_arg),
		cmocka_unit_test(test_opt_init_from_file),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
