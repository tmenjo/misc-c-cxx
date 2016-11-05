MAINTAINERCLEANFILES = Makefile.in

AM_CFLAGS = -Wall -Wextra @CHECK_CFLAGS@
LDADD = @CHECK_LIBS@

TESTS = test_sub
check_PROGRAMS = $(TESTS)

test_sub_SOURCES = test_sub.c
test_sub_LDADD = ../main/sub.o $(LDADD)
