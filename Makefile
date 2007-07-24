GRIT_VERSION	:=	0.7.2
GRIT_BUILD	:=	20070331

CPPFLAGS	:=	-g -DGRIT_VERSION=\"$(GRIT_VERSION)\" -DGRIT_BUILD=\"$(GRIT_BUILD)\" -O3 -Icldib -Ilibgit
LDFLAGS		:=	-g

# ---------------------------------------------------------------------
# Platform specific stuff
# ---------------------------------------------------------------------

UNAME	:=	$(shell uname -s)

ifneq (,$(findstring MINGW,$(UNAME)))
	EXEEXT := .exe
	EXTRAINSTALL := extlib/FreeImage.dll
	EXTRATAR := -C extlib FreeImage.dll
	OS := win32
endif

ifneq (,$(findstring CYGWIN,$(UNAME)))
	CPPFLAGS	+= -mno-cygwin
	LDFLAGS		+= -mno-cygwin
	EXEEXT		:= .exe
	EXTRAINSTALL := extlib/FreeImage.dll
	EXTRATAR := -C extlib FreeImage.dll
	OS := win32
endif

ifneq (,$(findstring Darwin,$(UNAME)))
	OS := OSX
else
	LDFLAGS += -s
endif

ifneq (,$(findstring Linux,$(UNAME)))
	LDFLAGS += -static
	OS := Linux
endif

TARGET	:=	grit$(EXEEXT)

# ---------------------------------------------------------------------
# Library directories
# ---------------------------------------------------------------------

# === cldib ===

CLDIB_DIR	:=	cldib
LIBCLDIB	:=	libcldib.a

LIBCLDIB_SRC	:=	cldib_adjust.cpp	\
			cldib_conv.cpp	\
			cldib_tmap.cpp	\
			cldib_core.cpp	\
			cldib_tools.cpp	\
			cldib_wu.cpp

LIBCLDIB_OBJ	:=	$(addprefix build/, $(LIBCLDIB_SRC:.cpp=.o))

# === libgrit ===

LIBGRIT_DIR	:=	libgrit
LIBGRIT		:=	libgrit.a

LIBGRIT_SRC	:=	grit_core.cpp	\
			grit_huff.cpp	\
			grit_lz.cpp	\
			grit_misc.cpp	\
			grit_prep.cpp	\
			grit_rle.cpp	\
			grit_shared.cpp	\
			grit_xp.cpp	\
			logger.cpp	\
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

SRCDIRS	:= $(CLDIB_DIR) $(LIBGRIT_DIR) $(EXTLIB_DIR) $(GRIT_DIR)
INCDIRS	:= $(CLDIB_DIR) $(LIBGRIT_DIR) $(EXTLIB_DIR)
LIBDIRS	:= . $(EXTLIB_DIR)

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

$(LIBGRIT)	:	$(LIBCLDIB) $(LIBGRIT_OBJ)

$(TARGET)	:	$(LIBGRIT) $(GRIT_OBJ)
	$(CXX) $(LDFLAGS) -o $@ $(GRIT_OBJ) $(LIBPATHS) -lgrit -lcldib -lfreeimage


build:
	@[ -d $@ ] || mkdir -p $@

clean	:
	rm -fr $(TARGET) build $(LIBGRIT) $(LIBCLDIB) *.bz2

install:
	cp  $(TARGET) $(EXTRAINSTALL) $(PREFIX)

dist-src:
	@tar --exclude=*CVS* -cvjf git-src-$(GRIT_VERSION).tar.bz2 $(SRCDIRS) *.txt Makefile 

dist-bin: all
	@tar -cjvf git-$(VERSION)-$(OS).tar.bz2 $(TARGET) $(EXTRATAR)

dist: dist-src dist-bin

build/%.o	:	%.cpp
	$(CXX) -MMD -MP -MF build/$*.d $(CPPFLAGS) -c $< -o $@

%.a	:
	rm -f $@
	ar rcs $@ $^

-include $(DEPENDS)

