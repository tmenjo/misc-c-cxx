MAINTAINERCLEANFILES = Makefile.in

AM_CFLAGS = -Wall -Wextra

TESTS = test_sub

noinst_PROGRAMS = $(TESTS)

test_sub_SOURCES = test_sub.c
test_sub_CFLAGS = $(AM_CFLAGS) @CHECK_CFLAGS@
test_sub_LDADD = ../main/sub.o @CHECK_LIBS@
