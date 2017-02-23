#include <sys/wait.h>	/* waitpid */
#include <unistd.h>	/* fork, _exit */

#include <check.h>
#include "checkutil-inl.h"

#define C_CHILD 0

#define assert_child_exited(code, cpid) \
	do {								\
		int status = C_ERR;					\
		ck_assert_int_eq(cpid, waitpid(cpid, &status, 0));	\
		ck_assert(WIFEXITED(status));				\
		ck_assert_int_eq(code, WEXITSTATUS(status));		\
	} while(0)

static void assert_exit(int code1, int code2)
{
	const pid_t cpid = fork();
	switch (cpid) {
	case C_ERR:
		ck_abort();
	case C_CHILD:
		_exit(code1);
	}

	assert_child_exited(code2, cpid);
}

START_TEST(test_exit)
{
	assert_exit(   0,    0);
	assert_exit(   1,    1);
	assert_exit(   2,    2);
	assert_exit(0377, 0377);
	assert_exit(0400,    0);
	assert_exit(0401,    1);
	assert_exit(  -1, 0377);
}
END_TEST

int main()
{
	TCase *const tcase = tcase_create("all");
	tcase_add_test(tcase, test_exit);

	Suite *const suite = suite_create("fork");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
