#include <errno.h>
#include <stdlib.h>

#include <check.h>

#define EOK 0

static void *ptr_ = NULL;

static void setup(void)
{
	ptr_ = NULL;
	errno = EOK;
}

static void teardown(void)
{
	free(ptr_);
}

static void subtest_realloc(int err, int is_not_null, size_t size)
{
	ptr_ = realloc(ptr_, size);
	const int e = errno;
	ck_assert_int_eq(is_not_null, !!ptr_);
	ck_assert_int_eq(err, e);
}

START_TEST(test_ptr_is_null_and_size_is_zero)
{
	subtest_realloc(EOK, !NULL, 0);
}
END_TEST

START_TEST(test_ptr_is_null)
{
	subtest_realloc(EOK, !NULL, 1);
}
END_TEST

START_TEST(test_size_is_zero)
{
	ptr_ = malloc(1);
	ck_assert_ptr_ne(NULL, ptr_);
	subtest_realloc(EOK, !!NULL, 0);
}
END_TEST

START_TEST(test_enomem)
{
	subtest_realloc(ENOMEM, !!NULL, SIZE_MAX);
}
END_TEST

int main()
{
	TCase *const tcase = tcase_create("realloc");
	tcase_add_checked_fixture(tcase, setup, teardown);
	tcase_add_test(tcase, test_ptr_is_null_and_size_is_zero);
	tcase_add_test(tcase, test_ptr_is_null);
	tcase_add_test(tcase, test_size_is_zero);
	tcase_add_test(tcase, test_enomem);

	Suite *const suite = suite_create("realloc");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
