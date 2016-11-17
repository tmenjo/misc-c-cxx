MAINTAINERCLEANFILES = Makefile.in

AM_CFLAGS = -Wall -Wextra @CHECK_CFLAGS@
LDADD = @CHECK_LIBS@

TESTS = \
	test_direct_io \
	test_memfd_create \
	test_pagesize_alignment \
	test_realloc

check_PROGRAMS = $(TESTS)

test_direct_io_SOURCES = test_direct_io.c
test_memfd_create_SOURCES = test_memfd_create.c
test_pagesize_alignment_SOURCES = test_pagesize_alignment.c
test_realloc_SOURCES = test_realloc.c
