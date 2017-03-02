#include <signal.h>
#include <unistd.h>

#include <check.h>
#include "checkutil-inl.h"
#include "checkutil-sigaction-inl.h"
#include "checkutil-wait-inl.h"

static void do_nothing(int dummy)
{
	(void)dummy;
}

#define assert_raise(sig) do {				\
	const pid_t cpid = fork();			\
	switch (cpid) {					\
	case C_ERR:					\
		ck_abort();				\
	case C_CHILD:					\
		assert_default_sighandler((sig));	\
		assert_success(raise((sig)));		\
		_exit(1); /* should not be here */	\
	}						\
	assert_child_signaled((sig), cpid);		\
} while(0)

#define assert_raise_default(sig) do {			\
	const pid_t cpid = fork();			\
	switch (cpid) {					\
	case C_ERR:					\
		ck_abort();				\
	case C_CHILD:					\
		assert_set_default_sighandler((sig));	\
		assert_success(raise((sig)));		\
		_exit(1); /* should not be here */	\
	}						\
	assert_child_signaled((sig), cpid);		\
} while(0)

#define assert_raise_ignore(sig) do {			\
	const pid_t cpid = fork();			\
	switch (cpid) {					\
	case C_ERR:					\
		ck_abort();				\
	case C_CHILD:					\
		assert_set_ignoring_sighandler((sig));	\
		assert_success(raise((sig)));		\
		_exit(0); /* should be here */		\
	}						\
	assert_child_exited(0, cpid);			\
} while(0)

#define assert_raise_return(sig) do {				\
	const pid_t cpid = fork();				\
	switch (cpid) {						\
	case C_ERR:						\
		ck_abort();					\
	case C_CHILD:						\
		assert_set_sighandler((sig), do_nothing);	\
		assert_success(raise((sig)));			\
		_exit(0); /* should be here */			\
	}							\
	assert_child_exited(0, cpid);				\
} while(0)

START_TEST(test_raise_default)
{
	assert_raise(SIGHUP);  /*  1 */
	assert_raise(SIGQUIT); /*  3 */
	assert_raise(SIGABRT); /*  6 */
	assert_raise(SIGKILL); /*  9 */
	assert_raise(SIGPIPE); /* 13 */
	assert_raise(SIGUSR1);
	assert_raise(SIGUSR2);

	/* libcheck uses these signals internally */
	assert_raise_default(SIGINT);  /*  2 */
	assert_raise_default(SIGALRM); /* 14 */
	assert_raise_default(SIGTERM); /* 15 */

	/* cannot raise SIGILL(4), SIGFPE(8) and SIGSEGV(11) */
}
END_TEST

START_TEST(test_raise_ignore)
{
	assert_raise_ignore(SIGHUP);
	assert_raise_ignore(SIGINT);
	assert_raise_ignore(SIGQUIT);
	assert_raise_ignore(SIGABRT);
	assert_raise_ignore(SIGPIPE);
	assert_raise_ignore(SIGALRM);
	assert_raise_ignore(SIGTERM);
	assert_raise_ignore(SIGUSR1);
	assert_raise_ignore(SIGUSR2);

	/* cannot set sighandler for SIGKILL */
}
END_TEST

START_TEST(test_raise_return)
{
	assert_raise_return(SIGHUP);
	assert_raise_return(SIGINT);
	assert_raise_return(SIGQUIT);
	assert_raise_return(SIGABRT);
	assert_raise_return(SIGPIPE);
	assert_raise_return(SIGALRM);
	assert_raise_return(SIGTERM);
	assert_raise_return(SIGUSR1);
	assert_raise_return(SIGUSR2);

	/* cannot set sighandler for SIGKILL */
}
END_TEST

int main()
{
	TCase *const tcase = tcase_create("raise");
	tcase_add_test(tcase, test_raise_default);
	tcase_add_test(tcase, test_raise_ignore);
	tcase_add_test(tcase, test_raise_return);

	Suite *const suite = suite_create("signal");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
