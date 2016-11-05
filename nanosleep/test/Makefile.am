MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = -I../main
AM_CFLAGS = -Wall -Wextra @CHECK_CFLAGS@
LDADD = @CHECK_LIBS@

TESTS = test_nsutil
check_PROGRAMS = $(TESTS)

test_nsutil_SOURCES = test_nsutil.c
