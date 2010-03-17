GRIT_VERSION	:=	0.8.6
GRIT_BUILD	:=	20100317

CXXFLAGS	:=	-Icldib -Ilibgrit -Iextlib
CXXFLAGS	+=	-DGRIT_VERSION=\"$(GRIT_VERSION)\" -DGRIT_BUILD=\"$(GRIT_BUILD)\"
LDFLAGS		:=

# ---------------------------------------------------------------------
# Platform specific stuff
# ---------------------------------------------------------------------

UNAME	:=	$(shell uname -s)

ifneq (,$(findstring MINGW,$(UNAME)))
	PLATFORM	:= win32
	EXEEXT		:= .exe
	CFLAGS		+= -mno-cygwin
	LDFLAGS		+= -mno-cygwin -s
	OS	:=	win32
	EXTRAINSTALL	:=	extlib/FreeImage.dll
endif

ifneq (,$(findstring CYGWIN,$(UNAME)))
	CFLAGS		+= -mno-cygwin
	LDFLAGS		+= -mno-cygwin -s
	EXEEXT		:= .exe
	OS	:=	win32
	EXTRAINSTALL	:=	extlib/FreeImage.dll
endif

ifneq (,$(findstring Darwin,$(UNAME)))
	SDK	:=	/Developer/SDKs/MacOSX10.4u.sdk
	OSXCFLAGS	:= -mmacosx-version-min=10.4 -isysroot $(SDK) -arch i386 -arch ppc
	OSXCXXFLAGS	:=	$(OSXCFLAGS)
	CXXFLAGS	+=	-fvisibility=hidden
	LDFLAGS		+= -mmacosx-version-min=10.4 -Wl,-syslibroot,$(SDK) -arch i386 -arch ppc
endif

ifneq (,$(findstring Linux,$(UNAME)))
	LDFLAGS += -s -static
	OS := Linux
endif


TARGET	:=	grit$(EXEEXT)

# ---------------------------------------------------------------------
# Library directories
# ---------------------------------------------------------------------

# === cldib ===

CLDIB_DIR	:=	cldib
LIBCLDIB	:=	libcldib.a

LIBCLDIB_SRC	:=				\
			cldib_adjust.cpp	\
			cldib_conv.cpp		\
			cldib_core.cpp		\
			cldib_tmap.cpp		\
			cldib_tools.cpp		\
			cldib_wu.cpp

LIBCLDIB_OBJ	:=	$(addprefix build/, $(LIBCLDIB_SRC:.cpp=.o))

# === libgrit ===

LIBGRIT_DIR	:=	libgrit
LIBGRIT		:=	libgrit.a

LIBGRIT_SRC	:=				\
			cprs.cpp		\
			cprs_huff.cpp	\
			cprs_lz.cpp		\
			cprs_rle.cpp	\
			grit_core.cpp	\
			grit_misc.cpp	\
			grit_prep.cpp	\
			grit_shared.cpp	\
			grit_xp.cpp		\
			logger.cpp		\
			pathfun.cpp

LIBGRIT_OBJ	:=	$(addprefix build/, $(LIBGRIT_SRC:.cpp=.o))

# === External/shared library stuff ===

EXTLIB_DIR	:= extlib

# === Grit ===

GRIT_DIR	:=	srcgrit
GRIT_SRC	:=	grit_main.cpp cli.cpp fi.cpp
GRIT_OBJ	:=	$(addprefix build/, $(GRIT_SRC:.cpp=.o))

DEPENDS		:=	$(GRIT_OBJ:.o=.d) $(LIBCLDIB_OBJ:.o=.d) $(LIBGRIT_OBJ:.o=.d)

# ---------------------------------------------------------------------

SRCDIRS	:= $(CLDIB_DIR) $(LIBGRIT_DIR) $(GRIT_DIR) $(EXTLIB_DIR)
INCDIRS	:= $(CLDIB_DIR) $(LIBGRIT_DIR) $(EXTLIB_DIR)
LIBDIRS	:= .

INCLUDE		:= $(foreach dir, $(INCDIRS), -I$(dir))
LIBPATHS	:= $(foreach dir, $(LIBDIRS), -L$(dir))
VPATH		:= $(foreach dir, $(SRCDIRS), $(dir))

CPPFLAGS	+= $(INCLUDE)

# ---------------------------------------------------------------------
# Build rules
# ---------------------------------------------------------------------

.PHONY:
	all clean pre

all:	build $(TARGET)

# make grit_main.o dependent on makefile for version number
build/grit_main.o : Makefile

$(LIBCLDIB)	:	$(LIBCLDIB_OBJ)

$(LIBGRIT)	:	$(LIBGRIT_OBJ)

$(TARGET)	:	$(LIBCLDIB) $(LIBGRIT) $(GRIT_OBJ)
	$(CXX) $(LDFLAGS) -o $@ $(GRIT_OBJ) $(LIBPATHS) -lgrit -lcldib -lfreeimage


build:
	@[ -d $@ ] || mkdir -p $@

clean	:
	rm -fr $(TARGET) build $(LIBGRIT) $(LIBCLDIB) *.bz2

install:
	cp  $(TARGET) $(EXTRAINSTALL) $(PREFIX)

dist-src:
	@tar --exclude=*CVS* -cvjf grit-src-$(GRIT_VERSION).tar.bz2 $(SRCDIRS) *.txt Makefile 

dist-bin: all
	@tar -cjvf grit-$(VERSION)-$(OS).tar.bz2 $(TARGET) $(EXTRATAR)

dist: dist-src dist-bin

build/%.o	:	%.cpp
	$(CXX) -E -MMD -MF build/$(*).d $(CXXFLAGS) $< > /dev/null
	$(CXX) $(OSXCXXFLAGS) $(CXXFLAGS) -o $@ -c $<

%.a	:
	rm -f $@
	ar rcs $@ $^

-include $(DEPENDS)

