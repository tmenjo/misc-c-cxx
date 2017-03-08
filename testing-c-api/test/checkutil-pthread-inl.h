#ifndef CHECKUTIL_PTHREAD_INL_H
#define CHECKUTIL_PTHREAD_INL_H

#include <pthread.h>

#include <check.h>
#include "checkutil-inl.h"

/* macros */

#define assert_pthread_create(t_, r_, a_) \
	assert_success(pthread_create((t_), NULL, (r_), (a_)))

#define assert_pthread_join(expected_, t_) do {		\
	void *ret_ = NULL;				\
	assert_success(pthread_join((t_), &ret_));	\
	assert_not_nullptr(ret_);			\
	ck_assert_int_eq((expected_), *(int *)ret_);	\
	free(ret_);					\
} while(0)

#define assert_pthread_cancel(t_) do {			\
	void *ret_ = NULL;				\
	assert_success(pthread_cancel((t_)));		\
	assert_success(pthread_join((t_), &ret_));	\
	ck_assert_ptr_eq(PTHREAD_CANCELED, ret_);	\
} while(0)

#define pthread_cleanup_push_free(p_) \
	pthread_cleanup_push(free, (p_))

#define pthread_cleanup_pop_noexec() \
	pthread_cleanup_pop(0)

#endif /* CHECKUTIL_PTHREAD_INL_H */
