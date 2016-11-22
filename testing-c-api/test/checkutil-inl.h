#ifndef CHECKUTIL_INL_H
#define CHECKUTIL_INL_H

#include <errno.h>
#include <inttypes.h>

#include <check.h>

/* constants */
#define EOK 0
#define C_OK 0
#define C_ERR -1
#define C_FALSE 0
#define C_EQUAL 0

/* macros */
#define assert_success(ret) ck_assert_int_eq(C_OK, (ret))
#define assert_not_failure(ret) ck_assert_int_ne(C_ERR, (ret))
#define assert_nullptr(ret) ck_assert_ptr_eq(NULL, (ret))
#define assert_not_nullptr(ret) ck_assert_ptr_ne(NULL, (ret))
/* note that expr is lazy-evaluated */
#define assert_error(err, ret, expr)			\
	do {						\
		errno = EOK;				\
		const intmax_t r = (intmax_t)(expr);	\
		const int e = errno;			\
		ck_assert_int_eq((ret), r);		\
		ck_assert_int_eq((err), e);		\
	} while(0)
#define assert_failure(err, expr) assert_error((err), C_ERR, (expr))

#endif /* CHECKUTIL_INL_H */
