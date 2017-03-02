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

START_TEST(test_alarm_default)
{
	const pid_t cpid = fork();
	switch (cpid) {
	case C_ERR:
		ck_abort();
	case C_CHILD:
		/* libcheck uses SIGALRM internally */
		assert_set_default_sighandler(SIGALRM);

		alarm(1U);
		pause(); /* never return */
		_exit(1); /* should not be here */
	}
	assert_child_signaled(SIGALRM, cpid);
}
END_TEST

START_TEST(test_alarm_return)
{
	const pid_t cpid = fork();
	switch (cpid) {
	case C_ERR:
		ck_abort();
	case C_CHILD:
		assert_set_sighandler(SIGALRM, do_nothing);
		alarm(1U);
		assert_failure(EINTR, pause());
		_exit(0); /* should be here */
	}
	assert_child_exited(0, cpid);
}
END_TEST

int main()
{
	TCase *const tcase = tcase_create("alarm");
	tcase_add_test(tcase, test_alarm_default);
	tcase_add_test(tcase, test_alarm_return);

	Suite *const suite = suite_create("signal");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
