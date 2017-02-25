#include <sys/wait.h>	/* waitpid */
#include <unistd.h>	/* fork */

#include <check.h>
#include "checkutil-inl.h"
#include "checkutil-wait-inl.h"

#define C_READ 0
#define C_WRITE 1

static const int MAGIC = 0xDEADBEEF;

static void assert_pipe(
	void (*assert_child)(int /*in*/, int /*out*/, int /*arg*/),
	int arg_child,
	void (*assert_parent)(int /*in*/, int /*out*/),
	void (*assert_status)(pid_t, int),
	int arg_status)
{
	int up[2] = {0}, down[2] = {0};
	assert_success(pipe(up));
	assert_success(pipe(down));

	const pid_t cpid = fork();
	switch (cpid) {
	case C_CHILD:
		assert_success(close(up[C_READ]));
		assert_success(close(down[C_WRITE]));
		assert_child(down[C_READ], up[C_WRITE], arg_child);
		/* fall through */
	case C_ERR:
		ck_abort();
	}
	assert_success(close(up[C_WRITE]));
	assert_success(close(down[C_READ]));
	assert_parent(up[C_READ], down[C_WRITE]);
	assert_status(cpid, arg_status);
}

static void assert_child_read_then_exit(
	void (*assert_read)(int /*in*/),
	int in, int out, int arg)
{
	assert_success(close(out));
	assert_read(in);
	assert_success(close(in));
	_exit(arg);
}

static void assert_parent_close_pipes(int in, int out)
{
	assert_success(close(in));
	assert_success(close(out));
}

static void assert_parent_write_magic(int in, int out)
{
	assert_success(close(in));
	const size_t c = sizeof(MAGIC);
	ck_assert_int_eq((ssize_t)c, write(out, &MAGIC, c));
	assert_success(close(out));
}

static void assert_status_exited(pid_t cpid, int code)
{
	assert_child_exited(code, cpid);
}

static void assert_status_signaled(pid_t cpid, int signal)
{
	assert_child_signaled(signal, cpid);
}

/*
 * test_downstream
 */
static void assert_read_magic(int in)
{
	int b = 0;
	const size_t c = sizeof(b);
	ck_assert_int_eq((ssize_t)c, read(in, &b, c));
	ck_assert_int_eq(MAGIC, b);
}

static void assert_downstream_child(int in, int out, int arg)
{
	assert_child_read_then_exit(assert_read_magic, in, out, arg);
}

START_TEST(test_downstream)
{
	assert_pipe(
		assert_downstream_child, 0,
		assert_parent_write_magic,
		assert_status_exited, 0);
}
END_TEST

/*
 * test_eof
 */
static void assert_read_eof(int in)
{
	int b = 0;
	assert_error(EOK, 0, read(in, &b, sizeof(b)));
}

static void assert_eof_child(int in, int out, int arg)
{
	assert_child_read_then_exit(assert_read_eof, in, out, arg);
}

START_TEST(test_eof)
{
	assert_pipe(
		assert_eof_child, 0,
		assert_parent_close_pipes,
		assert_status_exited, 0);
}
END_TEST

/*
 * test_sigpipe
 */
static void assert_sigpipe_child(int in, int out, int arg)
{
	(void)arg;

	assert_read_magic(in);
	assert_success(close(in));

	/* raise SIGPIPE */
	int b = 0;
	const ssize_t r = write(out, &b, sizeof(b));
	(void)r;
}

START_TEST(test_sigpipe)
{
	assert_pipe(
		assert_sigpipe_child, -1,
		assert_parent_write_magic,
		assert_status_signaled, SIGPIPE);
}
END_TEST

/*
 * main
 */
int main()
{
	TCase *const tcase = tcase_create("all");
	tcase_add_test(tcase, test_downstream);
	tcase_add_test(tcase, test_eof);
	tcase_add_test(tcase, test_sigpipe);

	Suite *const suite = suite_create("pipe");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
