#define _GNU_SOURCE		/* syscall */
#include <errno.h>
#include <stdio.h>		/* fdopen, fileno */
#include <stdlib.h>
#include <sys/stat.h>		/* struct stat */
#include <sys/syscall.h>	/* SYS_* */
#include <unistd.h>		/* stat, syscall */

#include <check.h>
#include "checkutil-inl.h"

#define MEMFD_NAME "test_memfd_create"

/* glibc does not have memfd_create wrapper yet */
static int memfd_create(const char *name, unsigned int flags)
{
	return (int)syscall(SYS_memfd_create, name, flags);
}

START_TEST(test_create_and_close)
{
	const int fd = memfd_create(MEMFD_NAME, 0);
	assert_not_failure(fd);
	assert_success(close(fd));
}
END_TEST

START_TEST(test_create_two)
{
	/* we can open two fds that have the same name */
	const int fd1 = memfd_create(MEMFD_NAME, 0);
	assert_not_failure(fd1);
	const int fd2 = memfd_create(MEMFD_NAME, 0);
	assert_not_failure(fd2);
	ck_assert_int_ne(fd1, fd2);

	assert_success(close(fd2));
	assert_success(close(fd1));
}
END_TEST

static void assert_fdopen(const char *mode)
{
	const int fd = memfd_create(MEMFD_NAME, 0);
	assert_not_failure(fd);

	FILE *const stream = fdopen(fd, mode);
	assert_not_nullptr(stream);

	assert_success(close(fd));
	/* DO NOT close fd! it is already closed in fclose */
}

START_TEST(test_fdopen)
{
	assert_fdopen("r");  /* read-only */
	assert_fdopen("r+"); /* read and write */
}
END_TEST

START_TEST(test_fread)
{
	static const size_t len = (size_t)2*1024*1024; /* 2MiB */

	const int fd = memfd_create("test_memfd_create", 0);
	assert_not_failure(fd);

	char *const wbuf = malloc(len);
	assert_not_nullptr(wbuf);

	/* prepare some data */
	for (size_t i = 0; i < len; ++i) {
		wbuf[i] = (char)(uintptr_t)(&wbuf[i]);
	}

	/* write whole the data */
	const ssize_t ret = write(fd, wbuf, len);
	assert_not_failure(ret);
	ck_assert_uint_eq(len, (size_t)ret);
	ck_assert_int_eq(ret, lseek(fd, 0, SEEK_CUR)); /* tell */
	ck_assert_int_eq(0, lseek(fd, 0, SEEK_SET)); /* rewind */
	ck_assert_int_eq(0, lseek(fd, 0, SEEK_CUR)); /* tell */

	FILE *const stream = fdopen(fd, "r"); /* read-only */
	assert_not_nullptr(stream);

	struct stat st = { 0 };
	assert_success(fstat(fileno(stream), &st));
	ck_assert_int_eq(ret, st.st_size);

	void *const rbuf = malloc(len);
	assert_not_nullptr(rbuf);

	/* read whole the data */
	ck_assert_uint_eq(len, fread(rbuf, 1, len, stream));
	ck_assert_int_eq(C_FALSE, feof(stream));
	ck_assert_int_eq(C_EQUAL, memcmp(wbuf, rbuf, len));

	/* enf of file */
	ck_assert_uint_eq(0, fread(rbuf, 1, 1, stream));
	ck_assert_int_ne(C_FALSE, feof(stream));

	free(rbuf);
	free(wbuf);
	assert_success(fclose(stream));
	/* DO NOT close fd! it is already closed in fclose */
}
END_TEST

int main()
{
	TCase *const tcase = tcase_create("memfd_create");
	tcase_add_test(tcase, test_create_and_close);
	tcase_add_test(tcase, test_create_two);
	tcase_add_test(tcase, test_fdopen);
	tcase_add_test(tcase, test_fread);
	Suite *const suite = suite_create("memfd_create");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
