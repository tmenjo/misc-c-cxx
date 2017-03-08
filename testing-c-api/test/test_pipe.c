#include <stdlib.h>
#include <unistd.h>

#include <check.h>
#include "checkutil-inl.h"
#include "checkutil-readwrite-inl.h"
#include "checkutil-wait-inl.h"

#define C_READ 0
#define C_WRITE 1

START_TEST(test_downstream)
{
	const int magic = 0xDEADBEEF;

	int up[2] = {0}, down[2] = {0};
	assert_success(pipe(up));
	assert_success(pipe(down));

	const pid_t cpid = fork();
	switch (cpid) {
	case C_ERR:
		ck_abort();
	case C_CHILD:
		; /* now variables can be defined */
		assert_close(up[C_READ]);
		assert_close(down[C_WRITE]);

		const int in = down[C_READ], out = up[C_WRITE];
		assert_close(out);

		/* read magic */
		int deadbeef = 0;
		assert_read_success(in, &deadbeef, sizeof(int));
		ck_assert_int_eq(magic, deadbeef);

		/* read EOF */
		assert_read_error(EOK, 0, in, &deadbeef, sizeof(int));

		assert_close(in);
		_exit(EXIT_SUCCESS);
	}
	assert_close(up[C_WRITE]);
	assert_close(down[C_READ]);

	const int in = up[C_READ], out = down[C_WRITE];
	assert_close(in);

	/* write magic */
	assert_write_success(out, &magic, sizeof(int));

	assert_close(out);
	assert_child_exited(EXIT_SUCCESS, cpid);
}
END_TEST

START_TEST(test_sigpipe)
{
	const int magic = 0xDEADBEEF;

	int up[2] = {0}, down[2] = {0};
	assert_success(pipe(up));
	assert_success(pipe(down));

	const pid_t cpid = fork();
	switch (cpid) {
	case C_ERR:
		ck_abort();
	case C_CHILD:
		; /* now variables can be defined */
		assert_close(up[C_READ]);
		assert_close(down[C_WRITE]);

		const int in = down[C_READ], out = up[C_WRITE];

		/* read magic */
		int deadbeef = 0;
		assert_read_success(in, &deadbeef, sizeof(int));
		ck_assert_int_eq(magic, deadbeef);
		assert_close(in);

		/* raise SIGPIPE */
		const ssize_t dummy = write(out, &deadbeef, sizeof(int));
		(void)dummy;

		/* should not be here */
		assert_close(out);
		_exit(EXIT_FAILURE);
	}
	assert_close(up[C_WRITE]);
	assert_close(down[C_READ]);

	const int in = up[C_READ], out = down[C_WRITE];
	assert_close(in); /* child raise SIGPIPE if write */

	/* write magic */
	assert_write_success(out, &magic, sizeof(int));

	assert_close(out);
	assert_child_signaled(SIGPIPE, cpid);
}
END_TEST

int main()
{
	TCase *const tcase = tcase_create("all");
	tcase_add_test(tcase, test_downstream);
	tcase_add_test(tcase, test_sigpipe);

	Suite *const suite = suite_create("pipe");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
