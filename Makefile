GRIT_VERSION	:=	0.8.4
GRIT_BUILD	:=	20090207

CPPFLAGS	:=	-g -O3 -Icldib -Ilibgit
CPPFLAGS	+=	-DGRIT_VERSION=\"$(GRIT_VERSION)\" -DGRIT_BUILD=\"$(GRIT_BUILD)\"
LDFLAGS		:=	-g

# ---------------------------------------------------------------------
# Platform specific stuff
# ---------------------------------------------------------------------

UNAME	:=	$(shell uname -s)

ifneq (,$(findstring MINGW,$(UNAME)))
	OS	:=	win32
	EXEEXT		:=	.exe
	EXTRAINSTALL := extlib/FreeImage.dll
	EXTRATAR 	:=	-C extlib FreeImage.dll
endif

ifneq (,$(findstring CYGWIN,$(UNAME)))
	OS	:= win32
	CPPFLAGS	+= -mno-cygwin
	LDFLAGS		+= -mno-cygwin
	EXEEXT		:= .exe
	EXTRAINSTALL := extlib/FreeImage.dll
	EXTRATAR	:= -C extlib FreeImage.dll
endif

ifneq (,$(findstring Darwin,$(UNAME)))
	OS	:=	OSX
	CFLAGS	+=	-mmacosx-version-min=10.4 -isysroot /Developer/SDKs/MacOSX10.4u.sdk
	ARCH	:=	-arch i386 -arch ppc
	LDFLAGS	+=	$(ARCH)
else
	LDFLAGS += -s
endif

ifneq (,$(findstring Linux,$(UNAME)))
	OS	:=	Linux
	LDFLAGS += -static
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
	$(CXX) -E -MMD -MP -MF build/$*.d $(CPPFLAGS) $< > /dev/null 
	$(CXX)  $(CPPFLAGS) $(ARCH) -c $< -o $@

%.a	:
	rm -f $@
	ar rcs $@ $^

-include $(DEPENDS)

