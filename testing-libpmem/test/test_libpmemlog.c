#include <errno.h>
#include <fcntl.h>	/* open, posix_fallocate */
#include <stdio.h>	/* rename */
#include <stdlib.h>	/* malloc, calloc, free, _exit */
#include <sys/stat.h>	/* stat */
#include <sys/wait.h>	/* wait */
#include <unistd.h>	/* close, unlink */

#include <libpmem.h>
#include <libpmemlog.h>

#include <check.h>

#define EOK 0
#define C_OK 0 
#define C_ERR -1
#define C_EQUAL 0
#define C_CHILD 0

#define assert_nullptr(expr)		ck_assert_ptr_eq(NULL, (expr))
#define assert_not_nullptr(expr)	ck_assert_ptr_ne(NULL, (expr))
#define assert_success(expr)		ck_assert_int_eq(C_OK, (expr))
#define assert_not_failure(expr)	ck_assert_int_ne(C_ERR, (expr))
#define assert_error(err, expr)			\
	do {					\
		errno = EOK;			\
		const intmax_t r = (expr);	\
		const int e = errno;		\
		ck_assert_int_eq(C_ERR, r);	\
		ck_assert_int_eq((err), e);	\
	} while(0)
#define assert_error_ptr(err, expr)		\
	do {					\
		errno = EOK;			\
		const void *const r = (expr);	\
		const int e = errno;		\
		assert_nullptr(r);		\
		ck_assert_int_eq((err), e);	\
	} while(0)

#define PMEMLOG_NAME	"/pmem.log"
#define PMEMLOG_PATH	PMEMDIR_PATH PMEMLOG_NAME
#define NONPMEMLOG_PATH	NONPMEMDIR_PATH PMEMLOG_NAME

static PMEMlogpool *plp_ = NULL;

static void safe_unlink(const char *path)
{
	errno = EOK;
	const int r = unlink(path);
	const int e = errno;
	ck_assert((r == C_OK && e == EOK) || (r == C_ERR && e == ENOENT));
}

static PMEMlogpool *PLO_default()
{
	return pmemlog_open(PMEMLOG_PATH);
}

static PMEMlogpool *PLC_default()
{
	return pmemlog_create(PMEMLOG_PATH, PMEMLOG_MIN_POOL, 0600);
}

static PMEMlogpool *PLC_with_size(size_t size)
{
	return pmemlog_create(PMEMLOG_PATH, size, 0600);
}

static PMEMlogpool *PLC_with_path(const char *path)
{
	return pmemlog_create(path, PMEMLOG_MIN_POOL, 0600);
}

/* fixture */

static void setup_new(void)
{
	safe_unlink(PMEMLOG_PATH);
}

static void teardown_new(void)
{
	safe_unlink(PMEMLOG_PATH);
}

static void setup_open(void)
{
	setup_new();
	plp_ = PLC_default();
	assert_not_nullptr(plp_);
}

static void teardown_open(void)
{
	pmemlog_close(plp_);
	teardown_new();
}

/* base */

static void assert_check_version(
	const char *(*check_version)(unsigned, unsigned),
	int major, int minor)
{
	assert_nullptr(check_version(major, minor));
}

START_TEST(test_check_version)
{
	assert_check_version(
		pmem_check_version,
		PMEM_MAJOR_REQUIRED,
		PMEM_MINOR_REQUIRED);
	assert_check_version(
		pmemlog_check_version,
		PMEMLOG_MAJOR_REQUIRED,
		PMEMLOG_MINOR_REQUIRED);
}
END_TEST

static void assert_pmem_is_pmem(int pmem, const char *dir)
{
	static const size_t mapping_len = 42;
	static const int flags =
		PMEM_FILE_CREATE|
		PMEM_FILE_EXCL|
		PMEM_FILE_TMPFILE;

	size_t mapped_len = 0;
	int is_pmem = 0;

	/* mapping */
	void *const addr = pmem_map_file(
		dir, mapping_len, flags, 0600, &mapped_len, &is_pmem);
	assert_not_nullptr(addr);
	ck_assert_uint_eq(mapping_len, mapped_len);
	ck_assert_int_eq(pmem, is_pmem);

	/* is mapped range really PMEM? */
	ck_assert_int_eq(pmem, pmem_is_pmem(addr, mapped_len));

	/* unmapping */
	assert_success(pmem_unmap(addr, mapped_len));
}

START_TEST(test_pmem_is_pmem)
{
	assert_pmem_is_pmem(1, PMEMDIR_PATH);
	assert_pmem_is_pmem(0, NONPMEMDIR_PATH);
}
END_TEST

static void assert_pmemlog_create(const char *path)
{
	safe_unlink(path);

	PMEMlogpool *const plp = PLC_with_path(path);
	assert_not_nullptr(plp);
	pmemlog_close(plp);

	safe_unlink(path);
}

START_TEST(test_pmemlog_create)
{
	assert_pmemlog_create(PMEMLOG_PATH);

	/* file can be created even if it is on non-PMEM device */
	assert_pmemlog_create(NONPMEMLOG_PATH);
}
END_TEST

START_TEST(test_pmemlog_rename)
{
	static const char *const orgpath = PMEMLOG_PATH;
	static const char *const tmppath = PMEMLOG_PATH ".tmp";

	safe_unlink(orgpath);
	safe_unlink(tmppath);

	PMEMlogpool *const orgplp = PLC_with_path(orgpath);
	assert_not_nullptr(orgplp);
	PMEMlogpool *const tmpplp = PLC_with_path(tmppath);
	assert_not_nullptr(tmpplp);

	/* move tmppath to orgpath */
	assert_success(rename(tmppath, orgpath));

	/* now only orgpath exists */
	struct stat dummy = { 0 };
	assert_success(stat(orgpath, &dummy));
	assert_error(ENOENT, stat(tmppath, &dummy));

	pmemlog_close(orgplp);
	pmemlog_close(tmpplp);
	safe_unlink(orgpath);
	safe_unlink(tmppath);
}
END_TEST

/* new */

START_TEST(test_pmemlog_open_notexist)
{
	assert_error_ptr(ENOENT, PLO_default());
}
END_TEST

START_TEST(test_pmemlog_open_nonpmemlog)
{
	static const int flags = O_RDWR|O_CREAT|O_EXCL;
	static const int mode = S_IRUSR|S_IWUSR;

	const int fd = open(PMEMLOG_PATH, flags, mode);
	assert_not_failure(fd);
	assert_success(posix_fallocate(fd, 0, PMEMLOG_MIN_POOL));
	assert_success(close(fd));

	assert_error_ptr(EINVAL, PLO_default());
}
END_TEST

START_TEST(test_pmemlog_create_again)
{
	PMEMlogpool *const plp = PLC_default();
	assert_not_nullptr(plp);
	pmemlog_close(plp);

	assert_error_ptr(EEXIST, PLC_default());
}
END_TEST

START_TEST(test_pmemlog_open_again)
{
	PMEMlogpool *const plp = PLC_default();
	assert_not_nullptr(plp);

	assert_error_ptr(EAGAIN, PLO_default());

	pmemlog_close(plp);
}
END_TEST

START_TEST(test_pmemlog_create_small)
{
	/* poolsize must be PMEMLOG_MIN_POOL or more */
	assert_error_ptr(EINVAL, PLC_with_size(PMEMLOG_MIN_POOL - 1));
}
END_TEST

START_TEST(test_pmemlog_create_module)
{
	/* file can be created even if its size is larger than module */
	PMEMlogpool *const plp = PLC_with_size(PMEMSIZE_PER_MODULE);
	assert_not_nullptr(plp);
	pmemlog_close(plp);
}
END_TEST

START_TEST(test_pmemlog_create_bank)
{
	/* file cannot be created if its size is larger than bank */
	assert_error_ptr(ENOSPC, PLC_with_size(PMEMSIZE_PER_BANK));
}
END_TEST

/* open */

START_TEST(test_pmemlog_append)
{
	static const char log[] = "abc\n";
	static const size_t logsize = sizeof(log) - 1; /* except NUL */
	static const long long logsizeLL = (long long)logsize;

	const size_t poolsize = pmemlog_nbyte(plp_);
	ck_assert_uint_gt(PMEMLOG_MIN_POOL, poolsize);
	ck_assert_int_eq(0LL, pmemlog_tell(plp_));

	assert_success(pmemlog_append(plp_, log, logsize));
	ck_assert_int_eq(logsizeLL, pmemlog_tell(plp_));
	ck_assert_uint_eq(poolsize, pmemlog_nbyte(plp_));

	pmemlog_close(plp_);
	plp_ = PLO_default();
	assert_not_nullptr(plp_);
	ck_assert_int_eq(logsizeLL, pmemlog_tell(plp_));
	ck_assert_uint_eq(poolsize, pmemlog_nbyte(plp_));

	pmemlog_rewind(plp_);
	ck_assert_int_eq(0LL, pmemlog_tell(plp_));
	ck_assert_uint_eq(poolsize, pmemlog_nbyte(plp_));
}
END_TEST

static size_t full(size_t poolsize)
{
	return poolsize;
}

static size_t half(size_t poolsize)
{
	return poolsize / 2;
}

static void assert_pmemlog_append_overflow(size_t (*func)(size_t))
{
	const size_t poolsize = pmemlog_nbyte(plp_);
	const size_t writesize = func(poolsize);
	const long long writesizeLL = (long long)writesize;

	void *const buf = calloc(poolsize, 1);
	assert_not_nullptr(buf);

	ck_assert_int_eq(0LL, pmemlog_tell(plp_));
	assert_success(pmemlog_append(plp_, buf, writesize));
	ck_assert_int_eq(writesizeLL, pmemlog_tell(plp_));

	/* overflow */
	assert_error(ENOSPC, pmemlog_append(plp_, buf, poolsize));
	ck_assert_int_eq(writesizeLL, pmemlog_tell(plp_));

	free(buf);
}

START_TEST(test_pmemlog_append_overflow_full)
{
	assert_pmemlog_append_overflow(full);
}
END_TEST

START_TEST(test_pmemlog_append_overflow_half)
{
	assert_pmemlog_append_overflow(half);
}
END_TEST

static int memcpy_once(const void *buf, size_t len, void *argv)
{
	struct iovec *const args = (struct iovec *)argv;

	/* 0 < len <= args->iov_len */
	ck_assert_uint_lt(0, len);
	ck_assert_uint_ge(args->iov_len, len);
	memcpy(args->iov_base, buf, len);

	return 0; /* terminate walk */
}

static int memcpy_loop(const void *buf, size_t len, void *argv)
{
	struct iovec *const args = (struct iovec *)argv;

	/* 0 < len <= args->iov_len */
	ck_assert_uint_lt(0, len);
	ck_assert_uint_ge(args->iov_len, len);
	memcpy(args->iov_base, buf, len);

	args->iov_base = (void *)((uintptr_t)args->iov_base + len);
	args->iov_len -= len;

	return 1; /* continue walk */
}

static void assert_pmemlog_walk(
	size_t chunksize,
	int (*func)(const void *, size_t, void *))
{
	const size_t size = pmemlog_nbyte(plp_) / 2;
	const long long sizeLL = (long long)size;

	char *const wbuf = malloc(size);
	assert_not_nullptr(wbuf);

	/* prepare non-zero data */
	for (size_t i = 0; i < size; ++i) {
		wbuf[i] = (char)(uintptr_t)(&wbuf[i]);
	}

	ck_assert_int_eq(0LL, pmemlog_tell(plp_));
	assert_success(pmemlog_append(plp_, wbuf, size));
	ck_assert_int_eq(sizeLL, pmemlog_tell(plp_));

	char *const rbuf = malloc(size);
	assert_not_nullptr(rbuf);

	/* walk */
	struct iovec arg = { .iov_base = rbuf, .iov_len = size };
	pmemlog_walk(plp_, chunksize, func, &arg);
	ck_assert_int_eq(C_EQUAL, memcmp(wbuf, rbuf, size));
	ck_assert_int_eq(sizeLL, pmemlog_tell(plp_));

	free(wbuf);
	free(rbuf);
}

START_TEST(test_pmemlog_walk_once)
{
	assert_pmemlog_walk(0, memcpy_once);
}
END_TEST

START_TEST(test_pmemlog_walk_loop)
{
	assert_pmemlog_walk(4096, memcpy_loop);
}
END_TEST

START_TEST(test_fork)
{
	const size_t half = pmemlog_nbyte(plp_) / 2;
	const long long halfLL = (long long)half;

	char *const wbuf = calloc(half, 1);
	assert_not_nullptr(wbuf);
	ck_assert_int_eq(0LL, pmemlog_tell(plp_));
	assert_success(pmemlog_append(plp_, wbuf, half));
	ck_assert_int_eq(halfLL, pmemlog_tell(plp_));

	const pid_t cpid = fork();
	switch (cpid) {
	case C_ERR:
		ck_abort();
	case C_CHILD:
		/* child rewinds */
		pmemlog_rewind(plp_);
		ck_assert_int_eq(0LL, pmemlog_tell(plp_));
		/* child exits */
		_exit(0);
	}

	/* check whether child process exit by 0 */
	int status = C_ERR;
	waitpid(cpid, &status, 0); /* wait until child is terminated */
	ck_assert(WIFEXITED(status));
	ck_assert_int_eq(0, WEXITSTATUS(status));

	/* pmemlog position seems shared between parent and child, */
	/* that is, parent rewinds if child rewinds */
	ck_assert_int_eq(0LL, pmemlog_tell(plp_));

	free(wbuf);
}
END_TEST

int main()
{
	TCase *const tcase_base = tcase_create("base");
	tcase_add_test(tcase_base, test_check_version);
	tcase_add_test(tcase_base, test_pmem_is_pmem);
	tcase_add_test(tcase_base, test_pmemlog_create);
	tcase_add_test(tcase_base, test_pmemlog_rename);
	TCase *const tcase_new = tcase_create("new");
	tcase_add_checked_fixture(tcase_new, setup_new, teardown_new);
	tcase_add_test(tcase_new, test_pmemlog_open_notexist);
	tcase_add_test(tcase_new, test_pmemlog_open_nonpmemlog);
	tcase_add_test(tcase_new, test_pmemlog_create_again);
	tcase_add_test(tcase_new, test_pmemlog_open_again);
	tcase_add_test(tcase_new, test_pmemlog_create_small);
	tcase_add_test(tcase_new, test_pmemlog_create_module);
	tcase_add_test(tcase_new, test_pmemlog_create_bank);
	TCase *const tcase_open = tcase_create("open");
	tcase_add_checked_fixture(tcase_open, setup_open, teardown_open);
	tcase_add_test(tcase_open, test_pmemlog_append);
	tcase_add_test(tcase_open, test_pmemlog_append_overflow_full);
	tcase_add_test(tcase_open, test_pmemlog_append_overflow_half);
	tcase_add_test(tcase_open, test_pmemlog_walk_once);
	tcase_add_test(tcase_open, test_pmemlog_walk_loop);
	tcase_add_test(tcase_open, test_fork);

	Suite *const suite = suite_create("libpmemlog");
	suite_add_tcase(suite, tcase_base);
	suite_add_tcase(suite, tcase_new);
	suite_add_tcase(suite, tcase_open);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
