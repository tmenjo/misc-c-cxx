#include <signal.h>
#include <stdbool.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <check.h>
#include "checkutil-inl.h"
#include "checkutil-sigaction-inl.h"
#include "checkutil-wait-inl.h"

#define SZ_U64 sizeof(uint64_t)
#define SSZ_U64 (ssize_t)SZ_U64

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

static int efd_ = -1;

static void setup()
{
	efd_ = eventfd(0, EFD_SEMAPHORE);
	assert_not_failure(efd_);
}

static void teardown()
{
	assert_success(close(efd_));
}

static void do_nothing(int sig)
{
	(void)sig;
}

#define assert_kill(sig) do {				\
	const pid_t cpid = fork();			\
	switch (cpid) {					\
	case C_ERR:					\
		ck_abort();				\
	case C_CHILD:					\
		assert_default_sighandler((sig));	\
		ck_assert(notify_event(efd_));		\
		pause(); /* never return */		\
		_exit(1); /* should not be here */	\
	}						\
	ck_assert(wait_event(efd_));			\
	assert_success(kill(cpid, (sig)));		\
	assert_child_signaled((sig), cpid);		\
} while(0)

#define assert_kill_default(sig) do {			\
	const pid_t cpid = fork();			\
	switch (cpid) {					\
	case C_ERR:					\
		ck_abort();				\
	case C_CHILD:					\
		assert_set_default_sighandler((sig));	\
		ck_assert(notify_event(efd_));		\
		pause(); /* never return */		\
		_exit(1); /* should not be here */	\
	}						\
	ck_assert(wait_event(efd_));			\
	assert_success(kill(cpid, (sig)));		\
	assert_child_signaled((sig), cpid);		\
} while(0)

#define assert_kill_ignore(sig) do {			\
	const pid_t cpid = fork();			\
	switch (cpid) {					\
	case C_ERR:					\
		ck_abort();				\
	case C_CHILD:					\
		assert_set_ignoring_sighandler((sig));	\
		ck_assert(notify_event(efd_));		\
		ck_assert(wait_event(efd_));		\
		_exit(0); /* should be here */		\
	}						\
	ck_assert(wait_event(efd_));			\
	assert_success(kill(cpid, (sig)));		\
	ck_assert(notify_event(efd_));			\
	assert_child_exited(0, cpid);			\
} while(0)

#define assert_kill_return(sig) do {				\
	const pid_t cpid = fork();				\
	switch (cpid) {						\
	case C_ERR:						\
		ck_abort();					\
	case C_CHILD:						\
		assert_set_sighandler((sig), do_nothing);	\
		ck_assert(notify_event(efd_));			\
		ck_assert(wait_event(efd_));			\
		_exit(0); /* should be here */			\
	}							\
	ck_assert(wait_event(efd_));				\
	assert_success(kill(cpid, (sig)));			\
	ck_assert(notify_event(efd_));				\
	assert_child_exited(0, cpid);				\
} while(0)

START_TEST(test_kill_default)
{
	assert_kill(SIGHUP);  /*  1 */
	assert_kill(SIGQUIT); /*  3 */
	assert_kill(SIGABRT); /*  6 */
	assert_kill(SIGKILL); /*  9 */
	assert_kill(SIGPIPE); /* 13 */
	assert_kill(SIGUSR1);
	assert_kill(SIGUSR2);

	/* libcheck uses these signals internally */
	assert_kill_default(SIGINT);  /*  2 */
	assert_kill_default(SIGALRM); /* 14 */
	assert_kill_default(SIGTERM); /* 15 */

	/* cannot kill with SIGILL(4), SIGFPE(8) and SIGSEGV(11) */
}
END_TEST

START_TEST(test_kill_ignore)
{
	assert_kill_ignore(SIGHUP);
	assert_kill_ignore(SIGINT);
	assert_kill_ignore(SIGQUIT);
	assert_kill_ignore(SIGABRT);
	assert_kill_ignore(SIGPIPE);
	assert_kill_ignore(SIGALRM);
	assert_kill_ignore(SIGTERM);
	assert_kill_ignore(SIGUSR1);
	assert_kill_ignore(SIGUSR2);

	/* cannot set sighandler for SIGKILL */
}
END_TEST

START_TEST(test_kill_return)
{
	assert_kill_return(SIGHUP);
	assert_kill_return(SIGINT);
	assert_kill_return(SIGQUIT);
	assert_kill_return(SIGABRT);
	assert_kill_return(SIGPIPE);
	assert_kill_return(SIGALRM);
	assert_kill_return(SIGTERM);
	assert_kill_return(SIGUSR1);
	assert_kill_return(SIGUSR2);

	/* cannot set sighandler for SIGKILL */
}
END_TEST

int main()
{
	TCase *const tcase = tcase_create("kill");
	tcase_add_checked_fixture(tcase, setup, teardown);
	tcase_add_test(tcase, test_kill_default);
	tcase_add_test(tcase, test_kill_ignore);
	tcase_add_test(tcase, test_kill_return);

	Suite *const suite = suite_create("signal");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
