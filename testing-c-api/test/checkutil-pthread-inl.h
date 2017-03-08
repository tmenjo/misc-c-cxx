#ifndef CHECKUTIL_PTHREAD_INL_H
#define CHECKUTIL_PTHREAD_INL_H

#include <pthread.h>
#include <stdlib.h>
#include <check.h>

#define assert_pthread_create(t_, run_, arg_) \
	ck_assert_int_eq(0, pthread_create((t_), NULL, (run_), (arg_)))

#define assert_pthread_join(expected_, t_) do {		\
	void *r_ = NULL;				\
	ck_assert_int_eq(0, pthread_join((t_), &r_));	\
	ck_assert_ptr_ne(NULL, r_);			\
	ck_assert_int_eq((expected_), *(int *)r_);	\
	free(r_);					\
} while(0)

#define assert_pthread_cancel(t_) do {			\
	void *r_ = NULL;				\
	ck_assert_int_eq(0, pthread_cancel((t_)));	\
	ck_assert_int_eq(0, pthread_join((t_), &r_));	\
	ck_assert_ptr_eq(PTHREAD_CANCELED, r_);		\
} while(0)

#define pthread_cleanup_push_free(p_) \
	pthread_cleanup_push(free, (p_))

#define pthread_cleanup_pop_noexec() \
	pthread_cleanup_pop(0)

#endif /* CHECKUTIL_PTHREAD_INL_H */
