MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = -I../main
AM_CFLAGS = -Wall -Wextra

TESTS = test_nsutil
check_PROGRAMS = $(TESTS)

test_nsutil_SOURCES = test_nsutil.c
test_nsutil_CFLAGS = $(AM_CFLAGS) @CHECK_CFLAGS@
test_nsutil_LDADD = @CHECK_LIBS@
