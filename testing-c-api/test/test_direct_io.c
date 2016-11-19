#define _GNU_SOURCE	/* mkostemp */
#include <errno.h>
#include <fcntl.h>	/* O_* flags */
#include <stdlib.h>	/* mkostemp, valloc */
#include <string.h>
#include <unistd.h>	/* close, unlink, sysconf */

#include <check.h>
#include "checkutil-inl.h"

static long pagesize_ = C_ERR;
static int fd_ = C_ERR;
static void *ptr_ = NULL;

static void setup_once(void)
{
	pagesize_ = sysconf(_SC_PAGESIZE);
}

static void setup(void)
{
	char template[16];
	strcpy(template, "/tmp/XXXXXX");

	fd_ = mkostemp(template, O_SYNC);
	assert_not_failure(fd_);
	assert_success(unlink(template));

	const int flags = fcntl(fd_, F_GETFL);
	assert_not_failure(flags);
	assert_success(fcntl(fd_, F_SETFL, flags | O_DIRECT));

	ptr_ = NULL;
	errno = EOK;
}

static void teardown(void)
{
	free(ptr_);
	assert_success(close(fd_));
}

/* pagesize */

START_TEST(test_pagesize)
{
	ck_assert_int_eq(4096L, pagesize_);
	ck_assert_int_eq(4096, getpagesize());
}
END_TEST

START_TEST(test_valloc_alignment)
{
	void *const ptr = valloc(1);
	assert_not_nullptr(ptr);
	ck_assert_uint_eq(0, (uintptr_t)ptr % pagesize_);
	free(ptr);
}
END_TEST

/* direct_write */

static void subtest_direct_write(
	int err, ssize_t ret, void *(*alloc)(size_t))
{
	ptr_ = alloc(pagesize_);
	assert_not_nullptr(ptr_);

	assert_error(err, ret, write(fd_, ptr_, pagesize_));
}

START_TEST(test_direct_write_success)
{
	subtest_direct_write(EOK, pagesize_, valloc);
}
END_TEST

START_TEST(test_direct_write_failure)
{
	subtest_direct_write(EINVAL, C_ERR, malloc);
}
END_TEST

int main()
{
	TCase *const tcase1 = tcase_create("pagesize");
	tcase_add_unchecked_fixture(tcase1, setup_once, NULL);
	tcase_add_test(tcase1, test_pagesize);
	tcase_add_test(tcase1, test_valloc_alignment);

	TCase *const tcase2 = tcase_create("direct_write");
	tcase_add_unchecked_fixture(tcase2, setup_once, NULL);
	tcase_add_checked_fixture(tcase2, setup, teardown);
	tcase_add_test(tcase2, test_direct_write_success);
	tcase_add_test(tcase2, test_direct_write_failure);

	Suite *const suite = suite_create("direct_io");
	suite_add_tcase(suite, tcase1);
	suite_add_tcase(suite, tcase2);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
