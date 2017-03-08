#ifndef CHECKUTIL_READWRITE_INL_H
#define CHECKUTIL_READWRITE_INL_H

#include <errno.h>
#include <unistd.h>
#include <check.h>

#define assert_close(fd_) \
	ck_assert_int_eq(0, close((fd_)));

#define assert_rw_error(rw_, err_, ret_, fd_, ptr_, sz_) do {	\
	errno = 0;						\
	const int r_ = (rw_)((fd_), (ptr_), (sz_));		\
	const int e_ = errno;					\
	ck_assert_int_eq((ret_), r_);				\
	ck_assert_int_eq((err_), e_);				\
} while(0)

/* read */
#define assert_read_error(err_, ret_, fd_, ptr_, sz_) \
	assert_rw_error(read, (err_), (ret_), (fd_), (ptr_), (sz_))

#define assert_read_success(fd_, ptr_, sz_) \
	assert_read_error(0, (ssize_t)(sz_), (fd_), (ptr_), (sz_))

#define assert_read_failure(err_, fd_, ptr_, sz_) \
	assert_read_error((err_), -1, (fd_), (ptr_), (sz_))

/* write */
#define assert_write_error(err_, ret_, fd_, ptr_, sz_) \
	assert_rw_error(write, (err_), (ret_), (fd_), (ptr_), (sz_))

#define assert_write_success(fd_, ptr_, sz_) \
	assert_write_error(0, (ssize_t)(sz_), (fd_), (ptr_), (sz_))

#define assert_write_failure(err_, fd_, ptr_, sz_) \
	assert_write_error((err_), -1, (fd_), (ptr_), (sz_))

#endif /* CHECKUTIL_READWRITE_INL_H */
