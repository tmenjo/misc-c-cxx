#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <check.h>
#include "checkutil-inl.h"

#define C_CHILD 0

START_TEST(test_seqpacket)
{
	int sv[2];
	assert_success(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sv));
	ck_assert_int_ne(sv[0], sv[1]);

	char base[8];
	struct iovec buf = { .iov_base = base, .iov_len = 8 };
	struct msghdr msg = { .msg_iov = &buf, .msg_iovlen = 1 };

	/* sv[1] -> sv[0] */
	assert_failure(EAGAIN, recvmsg(sv[0], &msg, MSG_DONTWAIT));

	ck_assert_int_eq(4, write(sv[1], "hoge", 4));
	ck_assert_int_eq(3, write(sv[1], "foo",  3));

	ck_assert_int_eq(4, recvmsg(sv[0], &msg, 0));
	ck_assert_uint_eq(8, buf.iov_len);
	ck_assert_int_eq(0, memcmp("hoge", base, 4));
	ck_assert(MSG_TRUNC & ~msg.msg_flags);

	ck_assert_int_eq(3, recvmsg(sv[0], &msg, 0));
	ck_assert_uint_eq(8, buf.iov_len);
	ck_assert_int_eq(0, memcmp("foo", base, 3));
	ck_assert(MSG_TRUNC & ~msg.msg_flags);

	assert_failure(EAGAIN, recvmsg(sv[0], &msg, MSG_DONTWAIT));

	/* sv[0] -> sv[1] */
	assert_failure(EAGAIN, recvmsg(sv[1], &msg, MSG_DONTWAIT));

	ck_assert_int_eq(8, send(sv[0], "fugapiyo", 8, 0));
	ck_assert_int_eq(9, send(sv[0], "foobarbaz", 9, 0));

	ck_assert_int_eq(8, recvmsg(sv[1], &msg, 0));
	ck_assert_uint_eq(8, buf.iov_len);
	ck_assert_int_eq(0, memcmp("fugapiyo", base, 8));
	ck_assert(MSG_TRUNC & ~msg.msg_flags);

	ck_assert_int_eq(8, recvmsg(sv[1], &msg, 0));
	ck_assert_uint_eq(8, buf.iov_len);
	ck_assert_int_eq(0, memcmp("foobarba", base, 4));
	ck_assert(MSG_TRUNC & msg.msg_flags);

	assert_failure(EAGAIN, recvmsg(sv[1], &msg, MSG_DONTWAIT));

	assert_success(close(sv[0]));
	assert_success(close(sv[1]));
}
END_TEST

START_TEST(test_seqpacket_fork)
{
	int sv[2];
	assert_success(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sv));
	ck_assert_int_ne(sv[0], sv[1]);

	const pid_t cpid = fork();
	switch (cpid) {
	case C_ERR:
		ck_abort();
	case C_CHILD:
		assert_success(close(sv[0]));

		/* sv[1] -> sv[0] */
		ck_assert_int_eq(4, write(sv[1], "hoge", 4));

		assert_success(close(sv[1]));
		_exit(0);
	}
	assert_success(close(sv[1]));

	char base[8];
	struct iovec iov = { .iov_base = base, .iov_len = 8 };
	struct msghdr msg = { .msg_iov = &iov, .msg_iovlen = 1 };

	ck_assert_int_eq(4, recvmsg(sv[0], &msg, 0));
	ck_assert_uint_eq(8, iov.iov_len);
	ck_assert_int_eq(0, memcmp("hoge", iov.iov_base, 4));
	ck_assert(MSG_TRUNC & ~msg.msg_flags);

	assert_failure(EAGAIN, recvmsg(sv[0], &msg, MSG_DONTWAIT));

	assert_success(close(sv[0]));
}
END_TEST

int main()
{
	TCase *const tcase = tcase_create("all");
	tcase_add_test(tcase, test_seqpacket);
	tcase_add_test(tcase, test_seqpacket_fork);

	Suite *const suite = suite_create("seqpacket");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
