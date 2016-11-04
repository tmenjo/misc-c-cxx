MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = -I../main

TESTS = test_nsutil
noinst_PROGRAMS = $(TESTS)

test_nsutil_SOURCES = test_nsutil.c
test_nsutil_CFLAGS = @CHECK_CFLAGS@
test_nsutil_LDADD = @CHECK_LIBS@
