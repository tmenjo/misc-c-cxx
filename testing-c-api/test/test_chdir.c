#include <errno.h>
#include <limits.h>	/* PATH_MAX */
#include <sys/stat.h>	/* mkdir */
#include <unistd.h>	/* chdir, getcwd */

#include <check.h>
#include "checkutil-inl.h"

#define assert_success_or_error(err, expr)	\
	do {					\
		if ((expr) == 0) break;		\
		const int e = errno;		\
		ck_assert_int_eq((err), e);	\
	} while (0)

static char cwd_base_[PATH_MAX] = { 0 };

static void setup_once(void)
{
	assert_not_nullptr(getcwd(cwd_base_, sizeof(cwd_base_)));
}

static void setup(void)
{
	assert_success(chdir(cwd_base_));
}

static void subtest_chdir_abs(const char *path)
{
	assert_success(chdir(path));

	char cwd[PATH_MAX] = { 0 };
	assert_not_nullptr(getcwd(cwd, sizeof(cwd)));
	ck_assert_str_eq(path, cwd);
}

START_TEST(test_chdir_root)
{
	subtest_chdir_abs("/");
}
END_TEST

START_TEST(test_chdir_tmp)
{
	assert_success_or_error(ENOENT, rmdir("/tmp/test_chdir"));

	subtest_chdir_abs("/tmp");

	assert_success(mkdir("test_chdir", 0700));
	assert_success(chdir("test_chdir"));

	char cwd[PATH_MAX] = { 0 };
	assert_not_nullptr(getcwd(cwd, sizeof(cwd)));
	ck_assert_str_eq("/tmp/test_chdir", cwd);

	assert_success(rmdir("/tmp/test_chdir"));
}
END_TEST

int main()
{
	TCase *const tcase = tcase_create("chdir");
	tcase_add_unchecked_fixture(tcase, setup_once, NULL);
	tcase_add_checked_fixture(tcase, setup, NULL);
	tcase_add_test(tcase, test_chdir_root);
	tcase_add_test(tcase, test_chdir_tmp);

	Suite *const suite = suite_create("chdir");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
