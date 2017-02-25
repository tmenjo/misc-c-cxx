#include <errno.h>
#include <stdint.h>
#include <stdlib.h>	/* EXIT_{SUCCESS,FAILURE} */
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <check.h>
#include "checkutil-inl.h"
#include "checkutil-wait-inl.h"

#define INF		(-1)
#define SZ_U64		sizeof(uint64_t)
#define SSZ_U64		(ssize_t)SZ_U64
#define BITS_U64	(SZ_U64*8)

#define NR_CHILDREN	16
#define NR_RUN_LOOPS	(64/NR_CHILDREN)

static void c_run(int sfd, unsigned int i)
{
	uint64_t flag = (1 << i);
	for (int j = 0; j < NR_RUN_LOOPS; ++j) {
		if(SSZ_U64 != send(sfd, &flag, SZ_U64, MSG_NOSIGNAL))
			_exit(EXIT_FAILURE);
		flag <<= NR_CHILDREN;
	}
}

START_TEST(test_epoll)
{
	// sfds[*][0] for parent, sfds[*][1] for child
	int sfds[NR_CHILDREN][2] = {0};

	for (size_t i = 0; i < NR_CHILDREN; ++i) {
		assert_success(socketpair(
			AF_UNIX, SOCK_SEQPACKET, 0, sfds[i]));
		switch (fork()) {
		case C_ERR:
			ck_abort();
		case C_CHILD:
			;
			assert_success(close(sfds[i][0]));
			c_run(sfds[i][1], (unsigned int)i);

			uint64_t dummy = 0;
			ck_assert_int_eq(SSZ_U64,
				recv(sfds[i][1], &dummy, SZ_U64, 0));
			_exit(EXIT_SUCCESS); /* should be here */
		}
		assert_success(close(sfds[i][1]));
	}

	const int epfd = epoll_create(1);
	assert_not_failure(epfd);

	struct epoll_event events[NR_CHILDREN] = {0};
	for (size_t i = 0; i < NR_CHILDREN; ++i) {
		const int fd = sfds[i][0];
		events[i].events = EPOLLIN;
		events[i].data.fd = fd;
		assert_success(epoll_ctl(
			epfd, EPOLL_CTL_ADD, fd, &events[i]));
	}

	unsigned int count = 0;
	uint64_t flags = ~0;
	while (flags) {
		struct epoll_event event = {0};
		ck_assert_int_eq(1, epoll_wait(epfd, &event, 1, INF));
		if (event.events & EPOLLIN) {
			uint64_t base = 0;
			struct iovec iov =
				{ .iov_base = &base, .iov_len = SZ_U64 };
			struct msghdr msg =
				{ .msg_iov = &iov, .msg_iovlen = 1 };
			ck_assert_int_eq(SSZ_U64,
				recvmsg(event.data.fd, &msg, 0));
			ck_assert(!(MSG_TRUNC & msg.msg_flags));
			ck_assert(flags & base);
			flags &= ~base;
			++count;
		}
	}
	ck_assert_uint_eq(BITS_U64, count);

	for (size_t i = 0; i < NR_CHILDREN; ++i) {
		uint64_t dummy = 0;
		ck_assert_int_eq(SSZ_U64,
			send(sfds[i][0], &dummy, SZ_U64, MSG_NOSIGNAL));
	}

	int status = 0;
	for (int i = 0; i < NR_CHILDREN; ++i) {
		assert_not_failure(wait(&status));
		ck_assert(WIFEXITED(status));
		ck_assert_int_eq(EXIT_SUCCESS, WEXITSTATUS(status));
	}
	assert_failure(ECHILD, wait(&status));

	for (size_t i = 0; i < NR_CHILDREN; ++i)
		assert_success(close(sfds[i][0]));
	assert_success(close(epfd));
}
END_TEST

int main()
{
	TCase *const tcase = tcase_create("all");
	tcase_add_test(tcase, test_epoll);

	Suite *const suite = suite_create("epoll");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
