#define _GNU_SOURCE /* F_GETPIPE_SZ from fcntl.h */

#include <fcntl.h>
#include <limits.h> /* PIPE_BUF */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <check.h>
#include "checkutil-inl.h"
#include "checkutil-readwrite-inl.h"
#include "checkutil-sigaction-inl.h"
#include "checkutil-wait-inl.h"

#define C_READ 0
#define C_WRITE 1

#define assert_notify(fd_) do {	\
	char x_ = 0;		\
	assert_write_success(	\
		(fd_), &x_, 1);	\
} while(0)

#define assert_wait(fd_) do {	\
	char x_ = 0;		\
	assert_read_success(	\
		(fd_), &x_, 1);	\
} while(0)

/*------------------------------------------------------------------------*/

static int in_ = C_ERR, out_ = C_ERR;
static pid_t cpid_ = (pid_t)C_ERR;

static void teardown(void)
{
	if (in_ != C_ERR)
		assert_close(in_);
	if (out_ != C_ERR)
		assert_close(out_);
	if (cpid_ != (pid_t)C_ERR)
		assert_child_exited(EXIT_SUCCESS, cpid_);
}

#define mksetup(name_, subtest_)		\
static void subtest_(int, int);			\
static void setup_ ## name_ (void)		\
{						\
	int up[2] = {0}, down[2] = {0};		\
	assert_success(pipe(up));		\
	assert_success(pipe(down));		\
						\
	cpid_ = fork();				\
	switch (cpid_) {			\
	case C_ERR:				\
		ck_abort();			\
	case C_CHILD:				\
		;				\
		assert_close(up[C_READ]);	\
		assert_close(down[C_WRITE]);	\
						\
		const int in = down[C_READ];	\
		const int out = up[C_WRITE];	\
						\
		/* wait for parent */		\
		assert_wait(in);		\
						\
		(subtest_)(in, out);		\
						\
		assert_close(in);		\
		assert_close(out);		\
		_exit(EXIT_SUCCESS);		\
	}					\
	assert_close(up[C_WRITE]);		\
	assert_close(down[C_READ]);		\
						\
	in_ = up[C_READ];			\
	out_ = down[C_WRITE];			\
}						\
static void subtest_(int in, int out)

START_TEST(test_notify)
{
	assert_notify(out_);
}
END_TEST

mksetup(eof, subtest_eof)
{
	(void)out;

	char dummy = 0;
	assert_read_error(EOK, 0, in, &dummy, 1);
}

START_TEST(test_capacity)
{
	const int size = fcntl(out_, F_GETPIPE_SZ);
	void *const buf = malloc(size);
	assert_not_nullptr(buf);
	memset(buf, 0, PIPE_BUF);

	assert_notify(out_);

	/* fill pipe */
	assert_write_success(out_, buf, size);

	/* non-blocking */
	assert_success(fcntl(out_, F_SETFL, O_NONBLOCK));
	assert_write_failure(EAGAIN, out_, buf, 1);

	/* unblock child */
	assert_read_success(in_, buf, size);
	assert_read_success(in_, buf, 1);

	free(buf);
}
END_TEST

mksetup(capacity, subtest_capacity)
{
	(void)in;

	const int size = fcntl(out, F_GETPIPE_SZ);
	void *const buf = malloc(size);
	assert_not_nullptr(buf);
	memset(buf, 0, PIPE_BUF);

	/* fill pipe */
	assert_write_success(out, buf, size);

	/* blocking */
	assert_write_success(out, buf, 1);

	free(buf);
}

START_TEST(test_sigpipe_raise)
{
	assert_close(in_);
	in_ = C_ERR;

	assert_notify(out_);

	assert_child_signaled(SIGPIPE, cpid_);
	cpid_ = (pid_t)C_ERR;
}
END_TEST

mksetup(raise, subtest_raise)
{
	(void)in;

	assert_default_sighandler(SIGPIPE);

	/* raise SIGPIPE */
	char dummy = 0;
	dummy = write(out, &dummy, 1);
	(void)dummy;
}

START_TEST(test_close_then_notify)
{
	assert_close(in_);
	in_ = C_ERR;

	assert_notify(out_);
}
END_TEST

mksetup(ignore, subtest_ignore)
{
	(void)in;

	assert_set_ignoring_sighandler(SIGPIPE);

	char dummy = 0;
	assert_write_failure(EPIPE, out, &dummy, 1);
}

static void do_nothing(int);
mksetup(catch, subtest_catch)
{
	(void)in;

	assert_set_sighandler(SIGPIPE, do_nothing);

	char dummy = 0;
	assert_write_failure(EPIPE, out, &dummy, 1);
}

static void do_nothing(int sig)
{
	(void)sig;
}

/*------------------------------------------------------------------------*/

#define NR_LOOPS_ATOMICITY 10000
START_TEST(test_atomicity)
{
	int up[2] = {0}, down[2] = {0};
	assert_success(pipe(up));
	assert_success(pipe(down));

	const pid_t cpid0 = fork();
	switch (cpid0) {
	case C_ERR:
		ck_abort();
	case C_CHILD:
		; /* now variables can be defined */
		assert_close(up[C_READ]);
		assert_close(down[C_WRITE]);

		const int in = down[C_READ], out = up[C_WRITE];

		/* writing 000...0 */
		char buf[PIPE_BUF];
		memset(buf, 0, PIPE_BUF);

		for (int i = 0; i < NR_LOOPS_ATOMICITY; ++i)
			assert_write_success(out, buf, PIPE_BUF);

		/* wait for parent */
		assert_read_success(in, buf, 1);

		assert_close(in);
		assert_close(out);
		_exit(EXIT_SUCCESS);
	}

	const pid_t cpid1 = fork();
	switch (cpid1) {
	case C_ERR:
		ck_abort();
	case C_CHILD:
		; /* now variables can be defined */
		assert_close(up[C_READ]);
		assert_close(down[C_WRITE]);

		const int in = down[C_READ], out = up[C_WRITE];

		/* writing 111...1 */
		char buf[PIPE_BUF];
		memset(buf, ~0, PIPE_BUF);

		for (int i = 0; i < NR_LOOPS_ATOMICITY; ++i)
			assert_write_success(out, buf, PIPE_BUF);

		/* wait for parent */
		assert_read_success(in, buf, 1);

		assert_close(in);
		assert_close(out);
		_exit(EXIT_SUCCESS);
	}

	assert_close(up[C_WRITE]);
	assert_close(down[C_READ]);

	const int in = up[C_READ], out = down[C_WRITE];

	/* 000...0 */
	char ooo[PIPE_BUF];
	memset(ooo, 0, PIPE_BUF);

	/* 111...1 */
	char lll[PIPE_BUF];
	memset(lll, ~0, PIPE_BUF);

	char buf[PIPE_BUF];
	for (int i = 0; i < NR_LOOPS_ATOMICITY*2; ++i) {
		assert_read_success(in, buf, PIPE_BUF);
		if (!buf[0])
			/* read 000...0 */
			ck_assert_int_eq(C_EQUAL, memcmp(ooo, buf, PIPE_BUF));
		else
			/* read 111...1 */
			ck_assert_int_eq(C_EQUAL, memcmp(lll, buf, PIPE_BUF));
	}

	/* notify children */
	assert_write_success(out, buf, 2);

	assert_close(in);
	assert_close(out);
	assert_child_exited(EXIT_SUCCESS, cpid0);
	assert_child_exited(EXIT_SUCCESS, cpid1);
}
END_TEST

int main()
{
	TCase *const tcase_eof = tcase_create("eof");
	tcase_add_checked_fixture(tcase_eof, setup_eof, teardown);
	tcase_add_test(tcase_eof, test_notify);

	TCase *const tcase_capacity = tcase_create("capacity");
	tcase_add_checked_fixture(tcase_capacity, setup_capacity, teardown);
	tcase_add_test(tcase_capacity, test_capacity);

	TCase *const tcase_raise = tcase_create("raise");
	tcase_add_checked_fixture(tcase_raise, setup_raise, teardown);
	tcase_add_test(tcase_raise, test_sigpipe_raise);

	TCase *const tcase_ignore = tcase_create("ignore");
	tcase_add_checked_fixture(tcase_ignore, setup_ignore, teardown);
	tcase_add_test(tcase_ignore, test_close_then_notify);

	TCase *const tcase_catch = tcase_create("catch");
	tcase_add_checked_fixture(tcase_catch, setup_catch, teardown);
	tcase_add_test(tcase_catch, test_close_then_notify);

	TCase *const tcase_atomicity = tcase_create("atomicity");
	tcase_add_test(tcase_atomicity, test_atomicity);

	Suite *const suite = suite_create("pipe");
	suite_add_tcase(suite, tcase_eof);
	suite_add_tcase(suite, tcase_capacity);
	suite_add_tcase(suite, tcase_raise);
	suite_add_tcase(suite, tcase_ignore);
	suite_add_tcase(suite, tcase_catch);
	suite_add_tcase(suite, tcase_atomicity);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
