AC_PREREQ([2.61])
AC_INIT([Lightum], 1.0)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([lightum.c])

AC_PROG_CC
AC_PROG_CC_C99
if test "x$ac_cv_prog_cc_c99" = "xno"
then
  AC_MSG_ERROR("C99 compiler is required", 1)
fi

AC_PROG_INSTALL
AC_PROG_MKDIR_P

PKG_CHECK_MODULES([X11], [x11 xt xext xscrnsaver])
PKG_CHECK_MODULES([DBUS], [dbus-1 dbus-glib-1])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
