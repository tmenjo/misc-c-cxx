#include <fcntl.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <check.h>
#include "checkutil-inl.h"
#include "checkutil-wait-inl.h"

#define assert_event_read(fd, buf) \
	ck_assert_int_eq(8, read((fd), (buf), 8))

#define assert_event_write(fd, buf) \
	ck_assert_int_eq(8, write((fd), (buf), 8))

#define assert_cwd_root() do {			\
	char cwd[2];				\
	assert_not_nullptr(getcwd(cwd, 2));	\
	ck_assert_str_eq("/", cwd);		\
} while(0)

#define assert_fd_devnull(fd) do {					\
	char buf[10];							\
	ck_assert_int_eq(9, readlink("/proc/self/fd/" #fd, buf, 10));	\
	buf[9] = '\0';							\
	ck_assert_str_eq("/dev/null", buf);				\
} while(0)

#define assert_session_leader() do {		\
	const pid_t pid = getpid();		\
	const pid_t pgid = getpgid(pid);	\
	assert_not_failure(pgid);		\
	const pid_t sid = getsid(pid);		\
	assert_not_failure(sid);		\
	ck_assert_int_eq(pid, pgid);		\
	ck_assert_int_eq(pid, sid);		\
} while(0)

#define assert_not_session_leader() do {	\
	const pid_t pid = getpid();		\
	const pid_t pgid = getpgid(pid);	\
	assert_not_failure(pgid);		\
	const pid_t sid = getsid(pid);		\
	assert_not_failure(sid);		\
	ck_assert_int_ne(pid, pgid);		\
	ck_assert_int_ne(pid, sid);		\
	ck_assert_int_eq(pgid, sid);		\
} while(0)

static void assert_exit(int expected, int actual)
{
	const pid_t cpid = fork();
	switch (cpid) {
	case C_ERR:
		ck_abort();
	case C_CHILD:
		_exit(actual);
	}
	assert_child_exited(expected, cpid);
}

START_TEST(test_exit)
{
	assert_exit(   0,    0);
	assert_exit(   1,    1);
	assert_exit(   2,    2);
	assert_exit(0376, 0376);
	assert_exit(0377, 0377);
	assert_exit(   0, 0400);
	assert_exit(   1, 0401);
	assert_exit(   2, 0402);
	assert_exit(0376,   -2);
	assert_exit(0377,   -1);
}
END_TEST

START_TEST(test_pid)
{
	const pid_t pid = getpid();
	const pid_t pgid = getpgid(0);
	const pid_t sid = getsid(0);
	const pid_t cpid = fork();
	switch (cpid) {
	case C_ERR:
		ck_abort();
	case C_CHILD:
		ck_assert_int_eq(pid, getppid());
		ck_assert_int_eq(pgid, getpgid(0));
		ck_assert_int_eq(sid, getsid(0));
		_exit(0); /* should be here */
	}
	assert_child_exited(0, cpid);
}
END_TEST

START_TEST(test_daemonize_by_fork)
{
	const int efd1 = eventfd(0, 0);
	assert_not_failure(efd1);

	const pid_t cpid = fork();
	switch (cpid) {
	case C_ERR:
		ck_abort();
	case C_CHILD:
		; /* now variables can be defined */

		/* parent */

		const int efd2 = eventfd(0, 0);
		assert_not_failure(efd2);

		switch (fork()) {
		case C_ERR:
			ck_abort();
		case C_CHILD:
			break;
		default:
			; /* now variables can be defined */

			/* wait grandchild */
			uint64_t dpid;
			assert_event_read(efd2, &dpid);
			assert_event_write(efd1, &dpid);
			_exit(0);
		}

		/* child */

		const pid_t pgid = getpgid(0);
		const pid_t sid = getsid(0);

		assert_not_failure(setsid());

		ck_assert_int_ne(pgid, getpgid(0));
		ck_assert_int_ne(sid, getsid(0));

		assert_session_leader();

		switch (fork()) {
		case C_ERR:
			ck_abort();
		case C_CHILD:
			break;
		default:
			_exit(0);
		}

		/* grandchild */

		assert_success(chdir("/"));

		const int devnull = open("/dev/null", O_RDWR);
		assert_not_failure(devnull);
		assert_not_failure(dup2(devnull, 0)); /* stdin  */
		assert_not_failure(dup2(devnull, 1)); /* stdout */
		assert_not_failure(dup2(devnull, 2)); /* stderr */
		assert_success(close(devnull));

		/* assertion */
		assert_not_session_leader();
		assert_cwd_root();
		assert_fd_devnull(0);
		assert_fd_devnull(1);
		assert_fd_devnull(2);

		const uint64_t dpid = (uint64_t)getpid();
		assert_event_write(efd2, &dpid);
		assert_success(close(efd2));

		pause();
		_exit(1); /* should not be here */
	}

	uint64_t dpid = 0;
	assert_event_read(efd1, &dpid);
	assert_child_exited(0, cpid);
	assert_success(close(efd1));

	assert_success(kill((pid_t)dpid, SIGKILL));
}
END_TEST

START_TEST(test_daemonize_by_daemon)
{
	const int efd = eventfd(0, 0);
	assert_not_failure(efd);

	const pid_t cpid = fork();
	switch (cpid) {
	case C_ERR:
		ck_abort();
	case C_CHILD:
		; /* now variables can be defined */

		/* parent */

		const pid_t pid = getpid();
		const pid_t pgid = getpgid(0);
		const pid_t sid = getsid(0);

		assert_success(daemon(0, 0));

		/* child */

		ck_assert_int_ne(pid, getpid());
		ck_assert_int_ne(pgid, getpgid(0));
		ck_assert_int_ne(sid, getsid(0));

		/* assertion */
		assert_session_leader();
		assert_cwd_root();
		assert_fd_devnull(0);
		assert_fd_devnull(1);
		assert_fd_devnull(2);

		const uint64_t dpid = (uint64_t)getpid();
		assert_event_write(efd, &dpid);

		pause();
		_exit(1); /* should not be here */
	}

	/* cpid _exit(0) in daemon */
	assert_child_exited(0, cpid);

	uint64_t dpid = 0;
	assert_event_read(efd, &dpid);
	assert_success(close(efd));

	assert_success(kill((pid_t)dpid, SIGKILL));
}
END_TEST

int main()
{
	TCase *const tcase = tcase_create("all");
	tcase_add_test(tcase, test_exit);
	tcase_add_test(tcase, test_pid);
	tcase_add_test(tcase, test_daemonize_by_fork);
	tcase_add_test(tcase, test_daemonize_by_daemon);

	Suite *const suite = suite_create("fork");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
