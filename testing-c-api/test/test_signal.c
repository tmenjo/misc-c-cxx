#include <signal.h>	/* sigaction */
#include <stdbool.h>
#include <stdlib.h>	/* abort */
#include <sys/eventfd.h>
#include <sys/wait.h>	/* waitpid */
#include <unistd.h>	/* fork, pause */

#include <check.h>
#include "checkutil-inl.h"

#define C_CHILD 0

#define SZ_U64 sizeof(uint64_t)
#define SSZ_U64 (ssize_t)SZ_U64

void do_nothing_handler(int sig)
{
	(void)sig;
}

#define assert_child_signaled(sig, cpid) do {			\
	int status = C_ERR;					\
	ck_assert_int_eq(cpid, waitpid(cpid, &status, 0));	\
	ck_assert(WIFSIGNALED(status));				\
	ck_assert_int_eq(sig, WTERMSIG(status));		\
} while(0)

#define assert_reset_sigaction(sig) do {			\
	struct sigaction oldact = { 0 };			\
	assert_success(sigaction(sig, NULL, &oldact));		\
	if (oldact.sa_handler == SIG_DFL)			\
		break;						\
	const struct sigaction newact =				\
		{ .sa_handler = SIG_DFL };			\
	assert_success(sigaction(sig, &newact, &oldact));	\
} while(0)

#define assert_set_ignore_sigaction(sig) do {			\
	struct sigaction oldact = { 0 };			\
	const struct sigaction newact =				\
		{ .sa_handler = SIG_IGN };			\
	assert_success(sigaction(sig, &newact, &oldact));	\
} while(0)

#define assert_set_return_sigaction(sig) do {			\
	struct sigaction oldact = { 0 };			\
	const struct sigaction newact =				\
		{ .sa_handler = do_nothing_handler };		\
	assert_success(sigaction(sig, &newact, &oldact));	\
} while(0)

static int efd_ = -1;

static bool notify_event(int efd)
{
	static const uint64_t one = 1;
	return (SSZ_U64 == write(efd, &one, SZ_U64));
}

static bool wait_event(int efd)
{
	uint64_t ret = 0;
	return (SSZ_U64 == read(efd, &ret, SZ_U64) && ret == 1);
}

static void assert_child_signaled2(
	int sig, void (*c_run)(int), void (*p_run)(int, pid_t))
{
	const pid_t cpid = fork();
	switch (cpid) {
	case C_ERR:
		ck_abort();
	case C_CHILD:
		c_run(sig);
		_exit(1); /* should not be here */
	}
	if (p_run)
		p_run(sig, cpid);
	assert_child_signaled(sig, cpid);
}

static void assert_child_signaled1(int sig, void (*c_run)(int))
{
	assert_child_signaled2(sig, c_run, NULL);
}

static void c_abort(int sig)
{
	(void)sig;
	abort();
}

static void c_ignore_abort(int sig)
{
	(void)sig;
	assert_set_ignore_sigaction(SIGABRT);
	abort();
}

static void c_return_from_abort(int sig)
{
	(void)sig;
	assert_set_return_sigaction(SIGABRT);
	abort();
}

START_TEST(test_abort)
{
	assert_child_signaled1(SIGABRT, c_abort);
	assert_child_signaled1(SIGABRT, c_ignore_abort);
	assert_child_signaled1(SIGABRT, c_return_from_abort);
}
END_TEST

static void c_alarm(int sig)
{
	(void)sig;

	/* libcheck uses SIGALRM internally */
	assert_reset_sigaction(SIGALRM);
	alarm(1U);
	pause();
}

START_TEST(test_alarm)
{
	assert_child_signaled1(SIGALRM, c_alarm);
}
END_TEST

/*
 * test_raise_default
 */
static void c_raise(int sig)
{
	assert_reset_sigaction(sig);
	assert_success(raise(sig));
}

static void assert_raise(int sig)
{
	assert_child_signaled1(sig, c_raise);
}

START_TEST(test_raise)
{
	assert_raise(SIGHUP);
	assert_raise(SIGQUIT);
	assert_raise(SIGABRT);
	assert_raise(SIGKILL);
	assert_raise(SIGPIPE);
	assert_raise(SIGUSR1);
	assert_raise(SIGUSR2);

	/* libcheck uses these signals internally */
	assert_raise(SIGINT);
	assert_raise(SIGALRM);
	assert_raise(SIGTERM);

	/* POSIX says that SIGILL, SIGFPE and SIGSEGV cannot raise */
}
END_TEST

/*
 * test_kill
 */
static void setup_kill()
{
	efd_ = eventfd(0, EFD_SEMAPHORE);
	assert_not_failure(efd_);
}

static void teardown_kill()
{
	assert_success(close(efd_));
}

static void p_kill(int sig, pid_t cpid)
{
	ck_assert(wait_event(efd_));
	assert_success(kill(cpid, sig));
}

static void c_pause(int sig)
{
	assert_reset_sigaction(sig);
	ck_assert(notify_event(efd_));
	pause();
}

static void assert_kill(int sig)
{
	assert_child_signaled2(sig, c_pause, p_kill);
}

START_TEST(test_kill)
{
	assert_kill(SIGHUP);
	assert_kill(SIGQUIT);
	assert_kill(SIGABRT);
	assert_kill(SIGKILL);
	assert_kill(SIGPIPE);
	assert_kill(SIGUSR1);
	assert_kill(SIGUSR2);

	/* libcheck uses these signals internally */
	assert_kill(SIGINT);
	assert_kill(SIGALRM);
	assert_kill(SIGTERM);
}
END_TEST

int main()
{
	TCase *const tcase1 = tcase_create("all");
	tcase_add_test(tcase1, test_abort);
	tcase_add_test(tcase1, test_alarm);
	tcase_add_test(tcase1, test_raise);

	TCase *const tcase2 = tcase_create("kill");
	tcase_add_checked_fixture(tcase2, setup_kill, teardown_kill);
	tcase_add_test(tcase2, test_kill);

	Suite *const suite = suite_create("signal");
	suite_add_tcase(suite, tcase1);
	suite_add_tcase(suite, tcase2);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
