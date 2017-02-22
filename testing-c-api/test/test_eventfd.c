#include <limits.h>
#include <sys/eventfd.h>
#include <sys/wait.h>
#include <unistd.h>

#include <check.h>
#include "checkutil-inl.h"

#define C_CHILD 0

START_TEST(test_eventfd_init_0)
{
	const int efd = eventfd(0U, EFD_NONBLOCK);
	assert_not_failure(efd);

	uint64_t dummy = 0;
	assert_failure(EAGAIN, read(efd, &dummy, 8));

	assert_success(close(efd));
}
END_TEST

static void assert_eventfd_init_nonzero(unsigned int initval)
{
	const int efd = eventfd(initval, 0);
	assert_not_failure(efd);

	uint64_t actual = 0;
	ck_assert_int_eq(8, read(efd, &actual, 8));
	ck_assert_uint_eq(initval, actual);

	assert_success(close(efd));
}

START_TEST(test_eventfd_init_1)
{
	assert_eventfd_init_nonzero(1U);
}
END_TEST

START_TEST(test_eventfd_init_2)
{
	assert_eventfd_init_nonzero(2U);
}
END_TEST

START_TEST(test_eventfd_init_uintmax)
{
	assert_eventfd_init_nonzero(UINT_MAX);
}
END_TEST

START_TEST(test_eventfd_write_0)
{
	static const uint64_t zero = 0;

	const int efd = eventfd(0, EFD_NONBLOCK);
	assert_not_failure(efd);

	ck_assert_int_eq(8, write(efd, &zero, 8));

	uint64_t dummy = 0;
	assert_failure(EAGAIN, read(efd, &dummy, 8));

	assert_success(close(efd));
}
END_TEST

START_TEST(test_eventfd_write_u64max)
{
	static const uint64_t u64max = UINT64_MAX;

	const int efd = eventfd(0, 0);
	assert_not_failure(efd);

	assert_failure(EINVAL, write(efd, &u64max, 8));

	assert_success(close(efd));
}
END_TEST

START_TEST(test_eventfd_write_overflow)
{
	static const uint64_t u64max_m1 = UINT64_MAX - 1;
	static const uint64_t one = 1;

	const int efd = eventfd(0, EFD_NONBLOCK);
	assert_not_failure(efd);

	ck_assert_int_eq(8, write(efd, &u64max_m1, 8));
	assert_failure(EAGAIN, write(efd, &one, 8));

	assert_success(close(efd));
}
END_TEST

static void assert_eventfd_oneshot_success(uint64_t expected)
{
	const int efd = eventfd(0, 0);
	assert_not_failure(efd);

	ck_assert_int_eq(8, write(efd, &expected, 8));

	uint64_t actual = 0;
	ck_assert_int_eq(8, read(efd, &actual, 8));
	ck_assert_uint_eq(expected, actual);

	assert_success(close(efd));
}

START_TEST(test_eventfd_oneshot_1)
{
	assert_eventfd_oneshot_success(1);
}
END_TEST

START_TEST(test_eventfd_oneshot_u64max_m1)
{
	assert_eventfd_oneshot_success(UINT64_MAX - 1);
}
END_TEST

int main()
{
	TCase *const tcase = tcase_create("all");
	tcase_add_test(tcase, test_eventfd_init_0);
	tcase_add_test(tcase, test_eventfd_init_1);
	tcase_add_test(tcase, test_eventfd_init_2);
	tcase_add_test(tcase, test_eventfd_init_uintmax);
	tcase_add_test(tcase, test_eventfd_write_0);
	tcase_add_test(tcase, test_eventfd_write_u64max);
	tcase_add_test(tcase, test_eventfd_write_overflow);
	tcase_add_test(tcase, test_eventfd_oneshot_1);
	tcase_add_test(tcase, test_eventfd_oneshot_u64max_m1);

	Suite *const suite = suite_create("eventfd");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
