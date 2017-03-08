#ifndef CHECKUTIL_INL_H
#define CHECKUTIL_INL_H

#include <errno.h>
#include <inttypes.h>
#include <check.h>

#define EOK 0
#define C_OK 0
#define C_ERR -1
#define C_FALSE 0
#define C_EQUAL 0

#define assert_success(expr_) \
	ck_assert_int_eq(C_OK, (expr_))

#define assert_not_failure(expr_) \
	ck_assert_int_ne(C_ERR, (expr_))

#define assert_error(err_, ret_, expr_) do {	\
	errno = EOK;				\
	const intmax_t r_ = (intmax_t)(expr_);	\
	const int e_ = errno;			\
	ck_assert_int_eq((ret_), r_);		\
	ck_assert_int_eq((err_), e_);		\
} while (0)

#define assert_failure(err_, expr_) \
	assert_error((err_), C_ERR, (expr_))

#define assert_nullptr(expr_) \
	ck_assert_ptr_eq(NULL, (expr_))

#define assert_not_nullptr(expr_) \
	ck_assert_ptr_ne(NULL, (expr_))

#endif /* CHECKUTIL_INL_H */
