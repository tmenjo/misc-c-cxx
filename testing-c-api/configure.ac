#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([testing-c-api], [0.1])
AC_CONFIG_SRCDIR([test/test_direct_io.c])

AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

PKG_CHECK_MODULES(CHECK, [check >= 0.9.4])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h inttypes.h limits.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([getcwd getpagesize mkdir rmdir])

AC_CONFIG_FILES([Makefile
                 test/Makefile])
AC_OUTPUT
