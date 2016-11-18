#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([testing-libpmem], [0.1], [takashi.menjo+github@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

PKG_CHECK_MODULES(CHECK, [check >= 0.9.4])
PKG_CHECK_MODULES(LIBPMEMLOG, [libpmemlog])

AC_CONFIG_SRCDIR([test/test_libpmemlog.c])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC

AC_CONFIG_FILES([Makefile
                 test/Makefile])
AC_OUTPUT
