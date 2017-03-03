#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
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

static void do_nothing(int sig)
{
	(void)sig;
}

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

static void exit_failure(int sig)
{
	(void)sig;
	_exit(1);
}

START_TEST(test_abort_catch_exit)
{
	const pid_t cpid = fork();
	switch (cpid) {
	case C_ERR:
		ck_abort();
	case C_CHILD:
		/*
		 * A signal handler can catch SIGABRT thrown by abort().
		 * It can also _exit() inside it.
		 */
		assert_set_sighandler(SIGABRT, exit_failure);
		abort();
		_exit(0); /* should not be here */
	}
	assert_child_exited(1, cpid);
}
END_TEST

static void notify_aborting(int sig)
{
	(void)sig;
	notify_event(efd_);
}

START_TEST(test_abort_catch_notify)
{
	const pid_t cpid = fork();
	switch (cpid) {
	case C_ERR:
		ck_abort();
	case C_CHILD:
		/*
		 * A signal handler can catch SIGABRT thrown by abort().
		 * Returning from it, this process will now abort.
		 */
		assert_set_sighandler(SIGABRT, notify_aborting);
		abort();
		_exit(1); /* should not be here */
	}
	ck_assert(wait_event(efd_));
	assert_child_signaled(SIGABRT, cpid);
}
END_TEST

int main()
{
	TCase *const tcase1 = tcase_create("abort");
	tcase_add_test(tcase1, test_abort_default);
	tcase_add_test(tcase1, test_abort_ignore);
	tcase_add_test(tcase1, test_abort_return);
	tcase_add_test(tcase1, test_abort_catch_exit);

	TCase *const tcase2 = tcase_create("abort_event");
	tcase_add_checked_fixture(tcase2, setup, teardown);
	tcase_add_test(tcase2, test_abort_catch_notify);

	Suite *const suite = suite_create("signal");
	suite_add_tcase(suite, tcase1);
	suite_add_tcase(suite, tcase2);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
