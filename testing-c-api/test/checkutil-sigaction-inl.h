#ifndef CHECKUTIL_SIGACTION_INL_H
#define CHECKUTIL_SIGACTION_INL_H

#include <signal.h>
#include <check.h>

#define assert_default_sighandler(sig_) do {			\
	struct sigaction old_ = { 0 };				\
	ck_assert_int_eq(0, sigaction((sig_), NULL, &old_));	\
	ck_assert_ptr_eq(SIG_DFL, old_.sa_handler);		\
} while(0)

#define assert_set_sighandler(sig_, handler_) do {		\
	struct sigaction old_ = { 0 };				\
	struct sigaction new_ = { .sa_handler = (handler_) };	\
	ck_assert_int_eq(0, sigaction((sig_), &new_, &old_));	\
} while(0)

#define assert_set_default_sighandler(sig_) \
	assert_set_sighandler((sig_), SIG_DFL)

#define assert_set_ignoring_sighandler(sig_) \
	assert_set_sighandler((sig_), SIG_IGN)

#endif /* CHECKUTIL_SIGACTION_INL_H */
