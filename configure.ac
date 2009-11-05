AC_INIT([fntsample], [3.0], [eugen@debian.org])
AC_CONFIG_SRCDIR([fntsample.c])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([foreign 1.11])
AM_MAINTAINER_MODE
AM_SILENT_RULES([yes])

AM_GNU_GETTEXT([external])

AC_PROG_CC_C99
AC_PROG_AWK
AC_PROG_SED

PKG_CHECK_MODULES([cairo], [cairo])
PKG_CHECK_MODULES([fontconfig], [fontconfig])
PKG_CHECK_MODULES([freetype2], [freetype2])
PKG_CHECK_MODULES([glib], [glib-2.0])
PKG_CHECK_MODULES([pangocairo], [pangocairo >= 1.16])

AC_SUBST([UNICODE_BLOCKS])
UNICODE_BLOCKS=/usr/share/unicode/Blocks.txt

AC_ARG_WITH([unicode-blocks],
	    AS_HELP_STRING([--with-unicode-blocks=PATH],
			   [path for Unicode blocks file (/usr/share/unicode/Blocks.txt)]),
	    [UNICODE_BLOCKS=$withval])

AC_MSG_CHECKING([for Unicode blocks file])
if test -r "$UNICODE_BLOCKS"; then
	AC_MSG_RESULT([$UNICODE_BLOCKS])
else
	AC_MSG_ERROR([Unicode blocks file (Blocks.txt) not found at location $UNICODE_BLOCKS.
Use --with-unicode-blocks option to specify different location.
Blocks.txt file is available at Unicode site: http://unicode.org/Public/UNIDATA/Blocks.txt])
fi

DATE=`date +%F`
AC_SUBST([DATE])

AC_CONFIG_FILES([Makefile fntsample.1 pdfoutline.1 po/Makefile.in])
AC_OUTPUT
