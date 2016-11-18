#include <stdint.h>	/* uintptr_t */
#include <stdlib.h>	/* valloc */
#include <unistd.h>	/* getpagesize, sysconf */

#include <check.h>
#include "checkutil-inl.h"

static long pagesize_ = C_ERR;
static void *ptr_ = NULL;

void setup_once(void)
{
	pagesize_ = sysconf(_SC_PAGESIZE);
	ck_assert_int_lt(0, pagesize_);
	ck_assert_int_eq(pagesize_, (ssize_t)pagesize_);
}

void setup(void)
{
	ptr_ = NULL;
}

void teardown(void)
{
	free(ptr_);
}

START_TEST(test_getpagesize)
{
	ck_assert_int_eq(4096, getpagesize());
}
END_TEST

START_TEST(test_sysconf_SC_PAGESIZE)
{
	ck_assert_int_eq(4096L, sysconf(_SC_PAGESIZE));
}
END_TEST

START_TEST(test_valloc_alignment)
{
	ptr_ = valloc(1);
	assert_not_nullptr(ptr_);
	ck_assert_uint_eq(0, (uintptr_t)ptr_ % pagesize_);
}
END_TEST

int main()
{
	TCase *const tcase1 = tcase_create("pagesize");
	tcase_add_test(tcase1, test_getpagesize);
	tcase_add_test(tcase1, test_sysconf_SC_PAGESIZE);
	TCase *const tcase2 = tcase_create("alignment");
	tcase_add_unchecked_fixture(tcase2, setup_once, NULL);
	tcase_add_checked_fixture(tcase2, setup, teardown);
	tcase_add_test(tcase2, test_valloc_alignment);

	Suite *const suite = suite_create("pagesize_alignment");
	suite_add_tcase(suite, tcase1);
	suite_add_tcase(suite, tcase2);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
