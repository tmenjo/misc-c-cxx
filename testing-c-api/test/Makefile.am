MAINTAINERCLEANFILES = Makefile.in

AM_CFLAGS = -Wall -Wextra @CHECK_CFLAGS@
LDADD = @CHECK_LIBS@

TESTS = \
	test_chdir \
	test_direct_io \
	test_eventfd \
	test_fork \
	test_memfd_create \
	test_pipe \
	test_pthread \
	test_realloc \
	test_signal

check_PROGRAMS = $(TESTS)

test_pthread_CFLAGS = -pthread

test_chdir_SOURCES = test_chdir.c
test_direct_io_SOURCES = test_direct_io.c
test_eventfd_SOURCES = test_eventfd.c
test_fork_SOURCES = test_fork.c
test_memfd_create_SOURCES = test_memfd_create.c
test_pipe_SOURCES = test_pipe.c
test_pthread_SOURCES = test_pthread.c
test_realloc_SOURCES = test_realloc.c
test_signal_SOURCES = test_signal.c
