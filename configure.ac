#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([grit],[0.8.9],[davem@devkitpro.org])
AC_CONFIG_SRCDIR([srcgrit/cli.cpp])

AM_INIT_AUTOMAKE([1.10])


# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AC_CANONICAL_HOST
AC_CANONICAL_BUILD
AC_PROG_LIBTOOL

#
# check for libfreeimage
#
AC_CHECK_HEADER([FreeImage.h],
    [AC_CHECK_LIB([freeimage], [FreeImage_OpenMemory],
        [FREEIMAGE_LIBS="-lfreeimage"], [FREEIMAGE_LIBS=], [-lstdc++])],
    [FREEIMAGE_LIBS=])
AS_IF([test -z "$FREEIMAGE_LIBS"],
    [AC_MSG_ERROR(['libfreeimage' not found])])
AC_SUBST([FREEIMAGE_LIBS], [${FREEIMAGE_LIBS}])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
