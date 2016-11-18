#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([testing-autotools], [0.1], [takashi.menjo+github@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

PKG_CHECK_MODULES(CHECK, [check >= 0.9.4])

AC_CONFIG_SRCDIR([main/main.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 main/Makefile
                 test/Makefile])
AC_OUTPUT
