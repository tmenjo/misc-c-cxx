#define _GNU_SOURCE	/* mkostemp */
#include <errno.h>
#include <fcntl.h>	/* O_* flags */
#include <stdlib.h>	/* mkostemp, valloc, posix_memalign */
#include <string.h>
#include <unistd.h>	/* close, unlink, sysconf */

#include <check.h>
#include "checkutil-inl.h"

static const size_t PTRSIZE = sizeof(void *);
static const size_t HUGEPAGESIZE = (size_t)2*1024*1024; /* 2MiB */

static size_t pagesize_ = 0;
static int fd_ = C_ERR;
static void *ptr_ = NULL;

static void setup_once(void)
{
	pagesize_ = (size_t)sysconf(_SC_PAGESIZE);
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

/* is 'ptr' a multiple of 'alignment'? */
static void assert_alignment(size_t alignment, void *ptr)
{
	assert_not_nullptr(ptr);
	ck_assert_uint_eq(0, (uintptr_t)ptr % alignment);
	free(ptr);
}

static void assert_posix_memalign_success(size_t alignment, size_t size)
{
	void *ptr = NULL;
	assert_success(posix_memalign(&ptr, alignment, size));
	assert_alignment(alignment, ptr);
}

static void assert_posix_memalign_einval(size_t alignment)
{
	void *ptr = NULL;
	ck_assert_int_eq(EINVAL, posix_memalign(&ptr, alignment, 1));
}

/* is pagesize 4KiB? */
START_TEST(test_pagesize)
{
	ck_assert_uint_eq(4096, pagesize_);
	ck_assert_int_eq(4096, getpagesize());
}
END_TEST

/* does valloc return an address which is a multiple of pagesize? */
START_TEST(test_valloc_alignment)
{
	assert_alignment(pagesize_, valloc(1));
	assert_alignment(pagesize_, valloc(3));
	assert_alignment(pagesize_, valloc(pagesize_));
	assert_alignment(pagesize_, valloc(HUGEPAGESIZE));
}
END_TEST

START_TEST(test_posix_memalign_success)
{
	assert_posix_memalign_success(PTRSIZE, 1);
	assert_posix_memalign_success(PTRSIZE, 3);
	assert_posix_memalign_success(PTRSIZE, pagesize_);
	assert_posix_memalign_success(PTRSIZE, HUGEPAGESIZE);

	assert_posix_memalign_success(pagesize_, 1);
	assert_posix_memalign_success(pagesize_, 3);
	assert_posix_memalign_success(pagesize_, pagesize_);
	assert_posix_memalign_success(pagesize_, HUGEPAGESIZE);
}
END_TEST

START_TEST(test_posix_memalign_einval)
{
	assert_posix_memalign_einval(0);
	assert_posix_memalign_einval(1);
	assert_posix_memalign_einval(2);
	assert_posix_memalign_einval(3);
#if (__SIZEOF_POINTER__ == 8)
	assert_posix_memalign_einval(4);
	assert_posix_memalign_einval(5);
	assert_posix_memalign_einval(6);
	assert_posix_memalign_einval(7);
#endif
}
END_TEST

/* direct_write */

static void *posix_memalign_pagesize(size_t size)
{
	void *ptr = NULL;
	const int ret = posix_memalign(&ptr, pagesize_, size);
	return (ret == 0) ? ptr : NULL;
}

static void assert_direct_write(
	int err, ssize_t ret, void *(*alloc)(size_t), size_t size)
{
	setup();
	ptr_ = alloc(size);
	assert_not_nullptr(ptr_);
	assert_error(err, ret, write(fd_, ptr_, size));
	teardown();
}

static void assert_direct_write_success(
	void *(*alloc)(size_t), size_t size)
{
	assert_direct_write(EOK, (ssize_t)size, alloc, size);
}

static void assert_direct_write_einval(
	void *(*alloc)(size_t), size_t size)
{
	assert_direct_write(EINVAL, C_ERR, alloc, size);
}

START_TEST(test_direct_write_success)
{
	assert_direct_write_success(valloc, pagesize_);
	assert_direct_write_success(valloc, HUGEPAGESIZE);

	assert_direct_write_success(posix_memalign_pagesize, pagesize_);
	assert_direct_write_success(posix_memalign_pagesize, HUGEPAGESIZE);
}
END_TEST

START_TEST(test_direct_write_einval)
{
	assert_direct_write_einval(valloc, 1);
	assert_direct_write_einval(valloc, 3);

	assert_direct_write_einval(posix_memalign_pagesize, 1);
	assert_direct_write_einval(posix_memalign_pagesize, 3);
}
END_TEST

int main()
{
	TCase *const tcase1 = tcase_create("pagesize");
	tcase_add_unchecked_fixture(tcase1, setup_once, NULL);
	tcase_add_test(tcase1, test_pagesize);
	tcase_add_test(tcase1, test_valloc_alignment);
	tcase_add_test(tcase1, test_posix_memalign_success);
	tcase_add_test(tcase1, test_posix_memalign_einval);

	TCase *const tcase2 = tcase_create("direct_write");
	tcase_add_unchecked_fixture(tcase2, setup_once, NULL);
	tcase_add_test(tcase2, test_direct_write_success);
	tcase_add_test(tcase2, test_direct_write_einval);

	Suite *const suite = suite_create("direct_io");
	suite_add_tcase(suite, tcase1);
	suite_add_tcase(suite, tcase2);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
