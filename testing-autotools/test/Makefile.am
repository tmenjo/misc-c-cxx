MAINTAINERCLEANFILES = Makefile.in

TESTS = test_sub

noinst_PROGRAMS = $(TESTS)

test_sub_SOURCES = test_sub.c
test_sub_CFLAGS = @CHECK_CFLAGS@
test_sub_LDADD = ../main/sub.o @CHECK_LIBS@
