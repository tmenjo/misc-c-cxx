#ifndef CHECKUTIL_SIGACTION_INL_H
#define CHECKUTIL_SIGACTION_INL_H

#include <signal.h>

#include <check.h>

#define assert_default_sighandler(sig) do {			\
	struct sigaction oldact = { 0 };			\
	assert_success(sigaction((sig), NULL, &oldact));	\
	ck_assert_ptr_eq(SIG_DFL, oldact.sa_handler);		\
} while(0)

#define assert_set_sighandler(sig, handler) do {			\
	struct sigaction oldact = { 0 };				\
	const struct sigaction newact = { .sa_handler = (handler) };	\
	assert_success(sigaction((sig), &newact, &oldact));		\
} while(0)

#define assert_set_default_sighandler(sig) \
	assert_set_sighandler((sig), SIG_DFL)

#define assert_set_ignoring_sighandler(sig) \
	assert_set_sighandler((sig), SIG_IGN)

#endif /* CHECKUTIL_SIGACTION_INL_H */
