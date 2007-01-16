AC_INIT([fntsample], [1], [eugen@debian.org])
AC_CONFIG_SRCDIR([fntsample.c])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([foreign])
AM_MAINTAINER_MODE

AC_PROG_CC_C99
AC_PROG_AWK

PKG_CHECK_MODULES([cairo], [cairo])
PKG_CHECK_MODULES([freetype2], [freetype2])
PKG_CHECK_MODULES([glib], [glib-2.0])

AC_SUBST([UNICODE_BLOCKS])
UNICODE_BLOCKS=/usr/share/unicode/Blocks.txt

AC_ARG_WITH([unicode-blocks],
	    AS_HELP_STRING([--with-unicode-blocks],
			   [path for Unicode blocks file (/usr/share/unicode/Blocks.txt)]),
	    [UNICODE_BLOCKS=$withwal])

AC_MSG_CHECKING([for Unicode blocks file])
if test -r "$UNICODE_BLOCKS"; then
	AC_MSG_RESULT([$UNICODE_BLOCKS])
else
	AC_MSG_ERROR([Unicode blocks file not found])
fi

DATE=`date +%F`
AC_SUBST([DATE])

AC_CONFIG_FILES([Makefile fntsample.1])
AC_OUTPUT
