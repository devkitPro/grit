
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([grit],[0.8.15],[https://github.com/devkitPro/grit/issues])
AC_CONFIG_SRCDIR([srcgrit/cli.cpp])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([subdir-objects])

# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AC_CANONICAL_HOST
AC_CANONICAL_BUILD
AC_PROG_LIBTOOL

#
# check for libfreeimage
#
case "${host}" in
	*-mingw32*)
		WIN_FREEIMAGE_LIBS=-lws2_32
	;;
esac

AC_CHECK_HEADER([FreeImage.h])

AC_MSG_CHECKING([for libfreeimage])
save_LIBS="$LIBS"
LIBS="$LIBS -lfreeimage ${WIN_FREEIMAGE_LIBS} -lstdc++ -lm"
AC_LINK_IFELSE(
	[AC_LANG_PROGRAM([[#include <FreeImage.h>]],
	[[FreeImage_DeInitialise()]])],
	[freeimage_result=yes],
	[freeimage_result=no])
AC_MSG_RESULT([$freeimage_result])
LIBS="$save_LIBS"
if test "x$freeimage_result" = "xyes"; then
	FREEIMAGE_LIBS="-lfreeimage ${WIN_FREEIMAGE_LIBS} -lm"
else
	 AC_MSG_ERROR(['libfreeimage' not found])
fi

AC_SUBST([FREEIMAGE_LIBS], [${FREEIMAGE_LIBS}])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
