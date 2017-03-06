MAINTAINERCLEANFILES = Makefile.in

AM_CFLAGS = -Wall -Wextra @CHECK_CFLAGS@
LDADD = @CHECK_LIBS@

TESTS = \
	test_blocking_queue \
	test_chdir \
	test_direct_io \
	test_epoll \
	test_eventfd \
	test_fork \
	test_memfd_create \
	test_pipe \
	test_pthread \
	test_realloc \
	test_seqpacket \
	test_signal_abort \
	test_signal_alarm \
	test_signal_kill \
	test_signal_raise \
	test_stailq \
	test_tsearch

check_PROGRAMS = $(TESTS)

test_blocking_queue_CFLAGS = -pthread
test_pthread_CFLAGS = -pthread

test_blocking_queue_SOURCES = test_blocking_queue.c blocking_queue.c
test_chdir_SOURCES = test_chdir.c
test_direct_io_SOURCES = test_direct_io.c
test_epoll_SOURCES = test_epoll.c
test_eventfd_SOURCES = test_eventfd.c
test_fork_SOURCES = test_fork.c
test_memfd_create_SOURCES = test_memfd_create.c
test_pipe_SOURCES = test_pipe.c
test_pthread_SOURCES = test_pthread.c
test_realloc_SOURCES = test_realloc.c
test_seqpacket_SOURCES = test_seqpacket.c
test_signal_abort_SOURCES = test_signal_abort.c
test_signal_alarm_SOURCES = test_signal_alarm.c
test_signal_kill_SOURCES = test_signal_kill.c
test_signal_raise_SOURCES = test_signal_raise.c
test_stailq_SOURCES = test_stailq.c
test_tsearch_SOURCES = test_tsearch.c
