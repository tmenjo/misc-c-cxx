#include <signal.h>	/* sigaction */
#include <sys/wait.h>	/* waitpid */
#include <unistd.h>	/* fork */

#include <check.h>
#include "checkutil-inl.h"

#define C_CHILD 0

static void assert_exit(int code)
{
	const pid_t cpid = fork();
	switch (cpid) {
	case C_ERR:
		ck_abort();
	case C_CHILD:
		_exit(code);
	}

	/* is child process 'cpid' exited by 'code'? */
	int status = C_ERR;
	ck_assert_int_eq(cpid, waitpid(cpid, &status, 0));
	ck_assert(WIFEXITED(status));
	ck_assert_int_eq(code, WEXITSTATUS(status));
}

static void assert_raise(void (*setup_sigaction)(int), int sig)
{
	const pid_t cpid = fork();
	switch (cpid) {
	case C_ERR:
		ck_abort();
	case C_CHILD:
		setup_sigaction(sig);
		assert_success(raise(sig));
		_exit(1); /* should not be here */
	}

	/* is child process 'cpid' signaled by 'sig'? */
	int status = C_ERR;
	ck_assert_int_eq(cpid, waitpid(cpid, &status, 0));
	ck_assert(WIFSIGNALED(status));
	ck_assert_int_eq(sig, WTERMSIG(status));
}

static void assert_default_sigaction(int sig)
{
	struct sigaction oldact = { 0 };
	assert_success(sigaction(sig, NULL, &oldact));
	ck_assert_ptr_eq(SIG_DFL, oldact.sa_handler);
}

static void reset_default_sigaction(int sig)
{
	struct sigaction oldact = { 0 };
	const struct sigaction newact = { .sa_handler = SIG_DFL };
	assert_success(sigaction(sig, &newact, &oldact));
	ck_assert_ptr_ne(SIG_DFL, oldact.sa_handler);
}

#define assert_raise_default(sig) \
	assert_raise(assert_default_sigaction, sig)
#define assert_raise_sigaction(sig) \
	assert_raise(reset_default_sigaction, sig)

START_TEST(test_exit)
{
	for (int i = 0; i < 0400; ++i)
		assert_exit(i);
}
END_TEST

START_TEST(test_raise_default)
{
	assert_raise_default(SIGHUP);
	assert_raise_default(SIGQUIT);
	assert_raise_default(SIGABRT);
	assert_raise_default(SIGKILL);
	assert_raise_default(SIGPIPE);
	assert_raise_default(SIGUSR1);
	assert_raise_default(SIGUSR2);

	/* POSIX says that SIGILL, SIGFPE and SIGSEGV cannot raise */
}
END_TEST

START_TEST(test_raise_sigaction)
{
	/* libcheck uses these signals internally */
	assert_raise_sigaction(SIGINT);
	assert_raise_sigaction(SIGALRM);
	assert_raise_sigaction(SIGTERM);
}
END_TEST

int main()
{
	TCase *const tcase = tcase_create("exit_raise");
	tcase_add_test(tcase, test_exit);
	tcase_add_test(tcase, test_raise_default);
	tcase_add_test(tcase, test_raise_sigaction);

	Suite *const suite = suite_create("signal");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
