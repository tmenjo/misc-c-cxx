#define _GNU_SOURCE	/* mkostemp */
#include <errno.h>
#include <fcntl.h>	/* O_* flags */
#include <stdlib.h>	/* mkostemp, valloc */
#include <string.h>
#include <unistd.h>	/* close, unlink */

#include <check.h>

static int fd_ = -1;
static void *ptr_ = NULL;

static void setup(void)
{
	char template[16];
	strcpy(template, "/tmp/XXXXXX");

	fd_ = mkostemp(template, O_SYNC);
	ck_assert_int_ne(-1, fd_);
	ck_assert_int_eq(0, unlink(template));

	const int flags = fcntl(fd_, F_GETFL);
	ck_assert_int_ne(-1, flags);
	ck_assert_int_eq(0, fcntl(fd_, F_SETFL, flags | O_DIRECT));

	ptr_ = NULL;
	errno = 0;
}

static void teardown(void)
{
	free(ptr_);
	ck_assert_int_eq(0, close(fd_));
}

static void subtest_direct_write(int err, ssize_t ret,
                                 void *(*alloc)(size_t), size_t size)
{
	ptr_ = alloc(size);
	ck_assert_ptr_ne(NULL, ptr_);

	const ssize_t r = write(fd_, ptr_, size);
	const int e = errno;
	ck_assert_int_eq(ret, r);
	ck_assert_int_eq(err, e);
}

START_TEST(test_direct_write_success)
{
	subtest_direct_write(0, 4096, valloc, 4096);
}
END_TEST

START_TEST(test_direct_write_fail)
{
	subtest_direct_write(EINVAL, -1, malloc, 4096);
}
END_TEST

int main()
{
	TCase *const tcase = tcase_create("direct_write");
	tcase_add_checked_fixture(tcase, setup, teardown);
	tcase_add_test(tcase, test_direct_write_success);
	tcase_add_test(tcase, test_direct_write_fail);

	Suite *const suite = suite_create("direct_io");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
