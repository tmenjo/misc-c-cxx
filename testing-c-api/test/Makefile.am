MAINTAINERCLEANFILES = Makefile.in

AM_CFLAGS = -Wall -Wextra @CHECK_CFLAGS@
LDADD = @CHECK_LIBS@

TESTS = \
	test_chdir \
	test_direct_io \
	test_memfd_create \
	test_realloc \
	test_signal

check_PROGRAMS = $(TESTS)

test_chdir_SOURCES = test_chdir.c
test_direct_io_SOURCES = test_direct_io.c
test_memfd_create_SOURCES = test_memfd_create.c
test_realloc_SOURCES = test_realloc.c
test_signal_SOURCES = test_signal.c
