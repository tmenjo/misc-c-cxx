#ifndef CHECKUTIL_WAIT_INL_H
#define CHECKUTIL_WAIT_INL_H

#include <sys/wait.h>
#include <check.h>

#define C_CHILD 0

#define assert_child_exited(code_, pid_) do {			\
	int st_ = -1;						\
	ck_assert_int_eq((pid_), waitpid((pid_), &st_, 0));	\
	ck_assert(WIFEXITED(st_));				\
	ck_assert_int_eq((code_), WEXITSTATUS(st_));		\
} while(0)

#define assert_child_signaled(sig_, pid_) do {			\
	int st_ = -1;						\
	ck_assert_int_eq((pid_), waitpid((pid_), &st_, 0));	\
	ck_assert(WIFSIGNALED(st_));				\
	ck_assert_int_eq((sig_), WTERMSIG((st_)));		\
} while(0)

#endif /* CHECKUTIL_WAIT_INL_H */
