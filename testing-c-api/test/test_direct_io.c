/*
 * mkostemp from stdlib.h
 * O_DIRECT from fcntl.h
 */
#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <check.h>
#include "checkutil-inl.h"
#include "checkutil-readwrite-inl.h"

#define SZ_PTR sizeof(void *)

#define SZ_PAGE 4096	/* 4-KiB */
#define SZ_HUGE 2097152	/* 2-MiB */

/* alignment */

START_TEST(test_pagesize)
{
	ck_assert_int_eq(SZ_PAGE, sysconf(_SC_PAGESIZE));
	ck_assert_int_eq(SZ_PAGE, getpagesize());
}
END_TEST

#define assert_valloc(align_, sz_) do {			\
	void *p_ = valloc((sz_));			\
	assert_not_nullptr(p_);				\
	ck_assert_uint_eq(0, (uintptr_t)p_ % (align_));	\
	free(p_);					\
} while(0)

START_TEST(test_valloc)
{
	assert_valloc(SZ_PAGE, 1);
	assert_valloc(SZ_PAGE, 3);
	assert_valloc(SZ_PAGE, SZ_PAGE);
	assert_valloc(SZ_PAGE, SZ_HUGE);
}
END_TEST

#define assert_posix_memalign(align_, sz_) do {			\
	void *p_ = NULL;					\
	assert_success(posix_memalign(&p_, (align_), (sz_)));	\
	assert_not_nullptr(p_);					\
	ck_assert_uint_eq(0, (uintptr_t)p_ % (align_));		\
	free(p_);						\
} while(0)

#define assert_posix_memalign_failure(err_, align_, sz_) do { 	\
	void *p_ = NULL;					\
	const int r_ = posix_memalign(&p_, (align_), (sz_));	\
	ck_assert_int_eq((err_), r_);				\
} while(0)

START_TEST(test_posix_memalign)
{
	assert_posix_memalign(SZ_PTR, 1);
	assert_posix_memalign(SZ_PTR, 3);
	assert_posix_memalign(SZ_PTR, SZ_PAGE);
	assert_posix_memalign(SZ_PTR, SZ_HUGE);

	assert_posix_memalign(SZ_PTR*2, 1);
	assert_posix_memalign(SZ_PTR*2, 3);
	assert_posix_memalign(SZ_PTR*2, SZ_PAGE);
	assert_posix_memalign(SZ_PTR*2, SZ_HUGE);

	assert_posix_memalign(SZ_PTR*4, 1);
	assert_posix_memalign(SZ_PTR*4, 3);
	assert_posix_memalign(SZ_PTR*4, SZ_PAGE);
	assert_posix_memalign(SZ_PTR*4, SZ_HUGE);

	assert_posix_memalign(SZ_PAGE, 1);
	assert_posix_memalign(SZ_PAGE, 3);
	assert_posix_memalign(SZ_PAGE, SZ_PAGE);
	assert_posix_memalign(SZ_PAGE, SZ_HUGE);

	assert_posix_memalign(SZ_HUGE, 1);
	assert_posix_memalign(SZ_HUGE, 3);
	assert_posix_memalign(SZ_HUGE, SZ_PAGE);
	assert_posix_memalign(SZ_HUGE, SZ_HUGE);

	assert_posix_memalign_failure(EINVAL,  0, 1);
	assert_posix_memalign_failure(EINVAL,  1, 1);
#if (__SIZEOF_POINTER__ != 2)
	assert_posix_memalign_failure(EINVAL,  2, 1);
#endif
	assert_posix_memalign_failure(EINVAL,  3, 1);
#if (__SIZEOF_POINTER__ != 2 && __SIZEOF_POINTER__ != 4)
	assert_posix_memalign_failure(EINVAL,  4, 1);
#endif
	assert_posix_memalign_failure(EINVAL,  5, 1);
	assert_posix_memalign_failure(EINVAL,  6, 1);
	assert_posix_memalign_failure(EINVAL,  7, 1);
	assert_posix_memalign_failure(EINVAL,  9, 1);
	assert_posix_memalign_failure(EINVAL, 12, 1);
	assert_posix_memalign_failure(EINVAL, 24, 1);

	assert_posix_memalign_failure(ENOMEM, SZ_PTR,  SIZE_MAX);
	assert_posix_memalign_failure(ENOMEM, SZ_PAGE, SIZE_MAX);
	assert_posix_memalign_failure(ENOMEM, SZ_HUGE, SIZE_MAX);
}
END_TEST

/* direct_write */

static blksize_t sz_block_ = 0;

static void setup_once(void)
{
	struct stat st;
	assert_success(stat("/tmp", &st));
	ck_assert(S_ISDIR(st.st_mode));

	sz_block_ = st.st_blksize;
	ck_assert_int_eq(0, SZ_PAGE % sz_block_);
}

static int fd_ = C_ERR;
static void *ptr_ = NULL;

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

static void *palloc(size_t size)
{
	void *ptr = NULL;
	const int ret = posix_memalign(&ptr, SZ_PAGE, size);
	return (ret == 0) ? ptr : NULL;
}

#define maketest_direct_write(name_, alloc_, sz_)	\
START_TEST(test_direct_write_ ## name_)			\
{							\
	ptr_ = (alloc_)(sz_);				\
	assert_not_nullptr(ptr_);			\
	assert_write_success(fd_, ptr_, (sz_));		\
}							\
END_TEST

maketest_direct_write(valloc_page1,  valloc, SZ_PAGE)
maketest_direct_write(valloc_page2,  valloc, SZ_PAGE*2)
maketest_direct_write(valloc_page3,  valloc, SZ_PAGE*3)
maketest_direct_write(valloc_page4,  valloc, SZ_PAGE*4)
maketest_direct_write(valloc_huge,   valloc, SZ_HUGE)
maketest_direct_write(valloc_block1, valloc, sz_block_)
maketest_direct_write(valloc_block2, valloc, sz_block_*2)
maketest_direct_write(valloc_block3, valloc, sz_block_*3)
maketest_direct_write(valloc_block4, valloc, sz_block_*4)

maketest_direct_write(palloc_page1,  palloc, SZ_PAGE)
maketest_direct_write(palloc_page2,  palloc, SZ_PAGE*2)
maketest_direct_write(palloc_page3,  palloc, SZ_PAGE*3)
maketest_direct_write(palloc_page4,  palloc, SZ_PAGE*4)
maketest_direct_write(palloc_huge,   palloc, SZ_HUGE)
maketest_direct_write(palloc_block1, palloc, sz_block_)
maketest_direct_write(palloc_block2, palloc, sz_block_*2)
maketest_direct_write(palloc_block3, palloc, sz_block_*3)
maketest_direct_write(palloc_block4, palloc, sz_block_*4)

#define maketest_direct_write_einval(name_, alloc_, sz_)	\
START_TEST(test_direct_write_einval_ ## name_)			\
{								\
	ptr_ = (alloc_)(sz_);					\
	assert_not_nullptr(ptr_);				\
	assert_write_failure(EINVAL, fd_, ptr_, (sz_));		\
}								\
END_TEST

maketest_direct_write_einval(valloc_1, valloc, 1)
maketest_direct_write_einval(valloc_3, valloc, 3)
maketest_direct_write_einval(palloc_1, palloc, 1)
maketest_direct_write_einval(palloc_3, palloc, 3)

int main()
{
	TCase *const tcase1 = tcase_create("alignment");
	tcase_add_test(tcase1, test_pagesize);
	tcase_add_test(tcase1, test_valloc);
	tcase_add_test(tcase1, test_posix_memalign);

	TCase *const tcase2 = tcase_create("direct_write");
	tcase_add_unchecked_fixture(tcase2, setup_once, NULL);
	tcase_add_checked_fixture(tcase2, setup, teardown);
	tcase_add_test(tcase2, test_direct_write_valloc_page1);
	tcase_add_test(tcase2, test_direct_write_valloc_page2);
	tcase_add_test(tcase2, test_direct_write_valloc_page3);
	tcase_add_test(tcase2, test_direct_write_valloc_page4);
	tcase_add_test(tcase2, test_direct_write_valloc_huge);
	tcase_add_test(tcase2, test_direct_write_valloc_block1);
	tcase_add_test(tcase2, test_direct_write_valloc_block2);
	tcase_add_test(tcase2, test_direct_write_valloc_block3);
	tcase_add_test(tcase2, test_direct_write_valloc_block4);
	tcase_add_test(tcase2, test_direct_write_palloc_page1);
	tcase_add_test(tcase2, test_direct_write_palloc_page2);
	tcase_add_test(tcase2, test_direct_write_palloc_page3);
	tcase_add_test(tcase2, test_direct_write_palloc_page4);
	tcase_add_test(tcase2, test_direct_write_palloc_huge);
	tcase_add_test(tcase2, test_direct_write_palloc_block1);
	tcase_add_test(tcase2, test_direct_write_palloc_block2);
	tcase_add_test(tcase2, test_direct_write_palloc_block3);
	tcase_add_test(tcase2, test_direct_write_palloc_block4);
	tcase_add_test(tcase2, test_direct_write_einval_valloc_1);
	tcase_add_test(tcase2, test_direct_write_einval_valloc_3);
	tcase_add_test(tcase2, test_direct_write_einval_palloc_1);
	tcase_add_test(tcase2, test_direct_write_einval_palloc_3);

	Suite *const suite = suite_create("direct_io");
	suite_add_tcase(suite, tcase1);
	suite_add_tcase(suite, tcase2);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
