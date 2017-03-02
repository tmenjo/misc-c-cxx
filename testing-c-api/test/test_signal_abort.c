#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <check.h>
#include "checkutil-inl.h"
#include "checkutil-sigaction-inl.h"
#include "checkutil-wait-inl.h"

static void do_nothing(int sig)
{
	(void)sig;
}

START_TEST(test_abort_default)
{
	const pid_t cpid = fork();
	switch (cpid) {
	case C_ERR:
		ck_abort();
	case C_CHILD:
		assert_default_sighandler(SIGABRT);
		abort();
		_exit(1); /* should not be here */
	}
	assert_child_signaled(SIGABRT, cpid);
}
END_TEST

START_TEST(test_abort_ignore)
{
	const pid_t cpid = fork();
	switch (cpid) {
	case C_ERR:
		ck_abort();
	case C_CHILD:
		assert_set_ignoring_sighandler(SIGABRT);
		abort();
		_exit(1); /* should not be here */
	}
	assert_child_signaled(SIGABRT, cpid);
}
END_TEST

START_TEST(test_abort_return)
{
	const pid_t cpid = fork();
	switch (cpid) {
	case C_ERR:
		ck_abort();
	case C_CHILD:
		assert_set_sighandler(SIGABRT, do_nothing);
		abort();
		_exit(1); /* should not be here */
	}
	assert_child_signaled(SIGABRT, cpid);
}
END_TEST

int main()
{
	TCase *const tcase = tcase_create("abort");
	tcase_add_test(tcase, test_abort_default);
	tcase_add_test(tcase, test_abort_ignore);
	tcase_add_test(tcase, test_abort_return);

	Suite *const suite = suite_create("signal");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
