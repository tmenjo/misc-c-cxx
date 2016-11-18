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
	ck_assert_int_lt(0, pagesize_);
	ck_assert_int_eq(pagesize_, (ssize_t)pagesize_);
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

static void subtest_direct_write(
	int err, ssize_t ret, void *(*alloc)(size_t), size_t size)
{
	ptr_ = alloc(size);
	assert_not_nullptr(ptr_);

	assert_error(err, ret, write(fd_, ptr_, size));
}

START_TEST(test_direct_write_success)
{
	subtest_direct_write(EOK, pagesize_, valloc, pagesize_);
}
END_TEST

START_TEST(test_direct_write_failure)
{
	subtest_direct_write(EINVAL, C_ERR, malloc, pagesize_);
}
END_TEST

int main()
{
	TCase *const tcase = tcase_create("direct_write");
	tcase_add_unchecked_fixture(tcase, setup_once, NULL);
	tcase_add_checked_fixture(tcase, setup, teardown);
	tcase_add_test(tcase, test_direct_write_success);
	tcase_add_test(tcase, test_direct_write_failure);

	Suite *const suite = suite_create("direct_io");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
