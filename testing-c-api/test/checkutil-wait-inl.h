#ifndef CHECKUTIL_WAIT_INL_H
#define CHECKUTIL_WAIT_INL_H

#include <sys/wait.h>

#include <check.h>

/* constants */
#define C_CHILD 0

/* macros */
#define assert_child_exited(code, cpid) do {			\
	int status = -1;					\
	ck_assert_int_eq(cpid, waitpid(cpid, &status, 0));	\
	ck_assert(WIFEXITED(status));				\
	ck_assert_int_eq(code, WEXITSTATUS(status));		\
} while(0)

#define assert_child_signaled(sig, cpid) do {			\
	int status = -1;					\
	ck_assert_int_eq(cpid, waitpid(cpid, &status, 0));	\
	ck_assert(WIFSIGNALED(status));				\
	ck_assert_int_eq(sig, WTERMSIG(status));		\
} while(0)

#endif /* CHECKUTIL_WAIT_INL_H */
