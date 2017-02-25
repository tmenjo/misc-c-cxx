#include <limits.h>
#include <stdint.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <check.h>
#include "checkutil-inl.h"

#define read_event(efd, ptr) \
	read(efd, ptr, 8)
#define assert_read_event(efd, ptr) \
	ck_assert_int_eq(8, read_event(efd, ptr))
#define write_event(efd, ptr) \
	write(efd, ptr, 8)
#define assert_write_event(efd, ptr) \
	ck_assert_int_eq(8, write_event(efd, ptr))

static const uint64_t ZERO = 0;
static const uint64_t ONE = 1;
static const uint64_t U64MAX_M1 = UINT64_MAX - 1;
static const uint64_t U64MAX = UINT64_MAX;

static int efd_ = -1;

static void setup_regular(void)
{
	efd_ = eventfd(0U, 0);
	assert_not_failure(efd_);
}

static void setup_nonblock(void)
{
	efd_ = eventfd(0U, EFD_NONBLOCK);
	assert_not_failure(efd_);
}

static void setup_nonblock_semaphore(void)
{
	efd_ = eventfd(0U, EFD_NONBLOCK|EFD_SEMAPHORE);
	assert_not_failure(efd_);
}

static void teardown(void)
{
	assert_success(close(efd_));
}

/*
 * all
 */
static void assert_init_nonzero(unsigned int initval)
{
	const int efd = eventfd(initval, EFD_NONBLOCK);
	assert_not_failure(efd);

	uint64_t actual = 0;
	assert_read_event(efd, &actual);
	ck_assert_uint_eq(initval, actual);
	assert_failure(EAGAIN, read_event(efd, &actual));

	assert_success(close(efd));
}

static void assert_init_nonzero_semaphore(unsigned int initval)
{
	const int efd = eventfd(initval, EFD_NONBLOCK|EFD_SEMAPHORE);
	assert_not_failure(efd);

	uint64_t actual = 0;
	for (unsigned int i = 0; i < initval; ++i) {
		assert_read_event(efd, &actual);
		ck_assert_uint_eq(1, actual);
	}
	assert_failure(EAGAIN, read_event(efd, &actual));

	assert_success(close(efd));
}

START_TEST(test_init_nonzero)
{
	assert_init_nonzero(1U);
	assert_init_nonzero(2U);
	assert_init_nonzero(UINT_MAX - 1U);
	assert_init_nonzero(UINT_MAX);
}
END_TEST

START_TEST(test_init_nonzero_semaphore)
{
	assert_init_nonzero_semaphore(1U);
	assert_init_nonzero_semaphore(2U);
	assert_init_nonzero_semaphore(16U);
}
END_TEST

/*
 * regular, semaphore
 */
START_TEST(test_write_0)
{
	assert_write_event(efd_, &ZERO);
}
END_TEST

START_TEST(test_write_u64max_m1)
{
	assert_write_event(efd_, &U64MAX_M1);
	assert_write_event(efd_, &ZERO);
}
END_TEST

START_TEST(test_write_u64max)
{
	assert_failure(EINVAL, write_event(efd_, &U64MAX));
}
END_TEST

/*
 * non-semaphore
 */
static void assert_oneshot(uint64_t expected)
{
	uint64_t actual = 0;
	assert_write_event(efd_, &expected);
	assert_read_event(efd_, &actual);
	ck_assert_uint_eq(expected, actual);
}

START_TEST(test_oneshot_1)
{
	assert_oneshot(1);
}
END_TEST

START_TEST(test_oneshot_2)
{
	assert_oneshot(2);
}
END_TEST

START_TEST(test_oneshot_u64max_m1)
{
	assert_oneshot(UINT64_MAX - 1);
}
END_TEST

static void assert_twoshot(uint64_t expected, uint64_t first, uint64_t second)
{
	uint64_t actual = 0;
	assert_write_event(efd_, &first);
	assert_write_event(efd_, &second);
	assert_read_event(efd_, &actual);
	ck_assert_uint_eq(expected, actual);
}

START_TEST(test_twoshot_0_1)
{
	assert_twoshot(1, 0, 1);
}
END_TEST

START_TEST(test_twoshot_0_2)
{
	assert_twoshot(2, 0, 2);
}
END_TEST

START_TEST(test_twoshot_1_0)
{
	assert_twoshot(1, 1, 0);
}
END_TEST

START_TEST(test_twoshot_2_0)
{
	assert_twoshot(2, 2, 0);
}
END_TEST

START_TEST(test_twoshot_1_1)
{
	assert_twoshot(2, 1, 1);
}
END_TEST

START_TEST(test_twoshot_1_2)
{
	assert_twoshot(3, 1, 2);
}
END_TEST

START_TEST(test_twoshot_2_1)
{
	assert_twoshot(3, 2, 1);
}
END_TEST

START_TEST(test_twoshot_2_2)
{
	assert_twoshot(4, 2, 2);
}
END_TEST

/*
 * nonblock
 */
START_TEST(test_init_0)
{
	uint64_t dummy = 0;
	assert_failure(EAGAIN, read_event(efd_, &dummy));
}
END_TEST

START_TEST(test_write_overflow)
{
	assert_write_event(efd_, &U64MAX_M1);
	assert_failure(EAGAIN, write_event(efd_, &ONE));
}
END_TEST

int main()
{
	TCase *const tcase1 = tcase_create("all");
	tcase_add_test(tcase1, test_init_nonzero);
	tcase_add_test(tcase1, test_init_nonzero_semaphore);

	TCase *const tcase2 = tcase_create("regular");
	tcase_add_checked_fixture(tcase2, setup_regular, teardown);
	tcase_add_test(tcase2, test_write_0);
	tcase_add_test(tcase2, test_write_u64max_m1);
	tcase_add_test(tcase2, test_write_u64max);
	tcase_add_test(tcase2, test_oneshot_1);
	tcase_add_test(tcase2, test_oneshot_2);
	tcase_add_test(tcase2, test_oneshot_u64max_m1);
	tcase_add_test(tcase2, test_twoshot_0_1);
	tcase_add_test(tcase2, test_twoshot_0_2);
	tcase_add_test(tcase2, test_twoshot_1_0);
	tcase_add_test(tcase2, test_twoshot_2_0);
	tcase_add_test(tcase2, test_twoshot_1_1);
	tcase_add_test(tcase2, test_twoshot_1_2);
	tcase_add_test(tcase2, test_twoshot_2_1);
	tcase_add_test(tcase2, test_twoshot_2_2);

	TCase *const tcase3 = tcase_create("nonblock");
	tcase_add_checked_fixture(tcase3, setup_nonblock, teardown);
	tcase_add_test(tcase3, test_write_0);
	tcase_add_test(tcase3, test_write_u64max_m1);
	tcase_add_test(tcase3, test_write_u64max);
	tcase_add_test(tcase3, test_oneshot_1);
	tcase_add_test(tcase3, test_oneshot_2);
	tcase_add_test(tcase3, test_oneshot_u64max_m1);
	tcase_add_test(tcase3, test_twoshot_0_1);
	tcase_add_test(tcase3, test_twoshot_0_2);
	tcase_add_test(tcase3, test_twoshot_1_0);
	tcase_add_test(tcase3, test_twoshot_2_0);
	tcase_add_test(tcase3, test_twoshot_1_1);
	tcase_add_test(tcase3, test_twoshot_1_2);
	tcase_add_test(tcase3, test_twoshot_2_1);
	tcase_add_test(tcase3, test_twoshot_2_2);
	tcase_add_test(tcase3, test_init_0);
	tcase_add_test(tcase3, test_write_overflow);

	TCase *const tcase5 = tcase_create("nonblock_semaphore");
	tcase_add_checked_fixture(tcase5, setup_nonblock_semaphore, teardown);
	tcase_add_test(tcase5, test_write_0);
	tcase_add_test(tcase5, test_write_u64max_m1);
	tcase_add_test(tcase5, test_write_u64max);
	tcase_add_test(tcase5, test_init_0);
	tcase_add_test(tcase5, test_write_overflow);

	Suite *const suite = suite_create("eventfd");
	suite_add_tcase(suite, tcase1);
	suite_add_tcase(suite, tcase2);
	suite_add_tcase(suite, tcase3);
	suite_add_tcase(suite, tcase5);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
