AC_PREREQ([2.69])
AC_INIT([testing-autotools], [0.1], [takashi.menjo+github@gmail.com])
AM_INIT_AUTOMAKE([foreign subdir-objects])

PKG_CHECK_MODULES(CHECK, [check >= 0.9.4])

AC_CONFIG_SRCDIR([main/main.c])
#AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
AC_PROG_INSTALL

AC_CONFIG_FILES([Makefile
                 main/Makefile
                 test/Makefile])
AC_OUTPUT
