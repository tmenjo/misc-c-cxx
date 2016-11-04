#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([nanosleep], [0.1], [takashi.menjo+github@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

PKG_CHECK_MODULES(CHECK, [check >= 0.9.4])

AC_CONFIG_SRCDIR([main/nanosleep.c])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([limits.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([strtol])

AC_CONFIG_FILES([Makefile
                 main/Makefile
                 test/Makefile])
AC_OUTPUT
