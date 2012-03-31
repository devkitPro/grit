# Makefile.am -- Process this file with automake to produce Makefile.in
bin_PROGRAMS = grit

noinst_LTLIBRARIES      = libcldib.la libgrit.la

libcldib_la_SOURCES	= cldib/cldib_adjust.cpp cldib/cldib_conv.cpp cldib/cldib_core.cpp \
			cldib/cldib_tmap.cpp cldib/cldib_tools.cpp cldib/cldib_wu.cpp \
			cldib/cldib.h cldib/cldib_core.h cldib/cldib_files.h cldib/cldib_quant.h \
			cldib/cldib_tmap.h cldib/cldib_tools.h cldib/winglue.h

libgrit_la_SOURCES	= libgrit/cprs.cpp libgrit/cprs_huff.cpp libgrit/cprs_lz.cpp \
			libgrit/cprs_rle.cpp libgrit/grit_core.cpp libgrit/grit_misc.cpp \
			libgrit/grit_prep.cpp libgrit/grit_shared.cpp libgrit/grit_xp.cpp \
			libgrit/logger.cpp libgrit/pathfun.cpp \
			libgrit/cprs.h libgrit/grit.h libgrit/grit_core.h \
			libgrit/logger.h libgrit/pathfun.h

libgrit_la_CPPFLAGS	=	-I$(top_srcdir)/cldib

grit_SOURCES	=	srcgrit/cli.cpp srcgrit/grit_main.cpp srcgrit/cli.h extlib/fi.cpp extlib/fi.h
grit_LDADD	=	libgrit.la libcldib.la $(FREEIMAGE_LIBS)
grit_CPPFLAGS	=	-I$(top_srcdir)/cldib -I$(top_srcdir)/libgrit -I$(top_srcdir)/extlib

EXTRA_DIST = autogen.sh
