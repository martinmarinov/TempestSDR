#-------------------------------------------------------------------------------
# Copyright (c) 2014 Martin Marinov.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the GNU Public License v3.0
# which accompanies this distribution, and is available at
# http://www.gnu.org/licenses/gpl.html
# 
# Contributors:
#     Martin Marinov - initial API and implementation
#-------------------------------------------------------------------------------
# Define all of the C files here
OBJS = TSDRLibrary.o TSDRPluginLoader.o threading.o circbuff.o syncdetector.o frameratedetector.o gaussian.o extbuffer.o superbandwidth.o fft.o dsp.o

# Define all of the dependencies here
DEPS = include/TSDRLibrary.h TSDRPluginLoader.h include/TSDRPlugin.h osdetect.h threading.h circbuff.h syncdetector.h internaldefinitions.h frameratedetector.h extbuffer.h gaussian.h

# Flags
ifeq ($(BUILD_TYPE),Debug)
	DEBUGFLAGS+=-g -O0
else
	CFLAGS+=-O3
endif

# USER CONFIGURATION ENDS HERE


# Discover the library extension for each OS
ifeq ($(OS),Windows_NT)

	OSNAME ?= WINDOWS

	ifndef $(ARCHNAME)

		ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
			ARCHNAME = X64
		endif
		ifeq ($(PROCESSOR_ARCHITECTURE),x86)
			ARCHNAME = X86
		endif

	endif
else ifeq ($(shell uname -s),Darwin)
	OSNAME ?= MAC
	INC ?= /System/Library/Frameworks/JavaVM.framework/Versions/Current/Headers
	EXT ?= .so
	ifeq ($(shell uname -m),x86)
		ARCHNAME = X86
	endif
	ifeq ($(shell uname -m),x86_64)
		ARCHNAME = X64
	endif
else

	ifndef $(OSNAME)

		UNAME_S := $(shell uname -s)
		ifeq ($(UNAME_S),Linux)
			OSNAME = LINUX
		endif

	endif

	ifndef $(ARCHNAME)

		UNAME_M := $(shell uname -m)
		ifeq ($(UNAME_M),x86_64)
			ARCHNAME = X64
		endif
		ifneq ($(filter %86,$(UNAME_M)),)
			ARCHNAME = X86
		endif
		ifneq ($(filter arm%,$(UNAME_M)),)
			ARCHNAME = ARM
		endif

	endif
endif

ifeq ($(OSNAME),WINDOWS)
	EXT=.dll
	LIBPREFIX=
endif
ifeq ($(OSNAME),LINUX)
	EXT=.so
	LIBPREFIX=lib
endif

ifeq ($(ARCHNAME),X86)
	COMPILATION_TYPE=-m32
else ifeq ($(ARCHNAME),X64)
	COMPILATION_TYPE=-m64
endif

AR?=ar

# If you need a different directory structure. Don't change that unless you really want to.
SOURCEFOLDER=src
OUTPUTFOLDER=bin/$(OSNAME)/$(ARCHNAME)
OBJFOLDER=obj

# OS specific flags
ifeq ($(OSNAME),LINUX)
	CFLAGS+=-fPIC
else ifeq ($(OSNAME),WINDOWS)
	LDFLAGS+=-Wl,--add-stdcall-alias
	CFLAGS+=-fPIC
endif

# The obj files reside in the bin/obj directory
_OBJS = $(patsubst %,$(OBJFOLDER)/%,$(OBJS))

# The header files are relative to the source directory
_DEPS = $(patsubst %,$(SOURCEFOLDER)/%,$(DEPS))

# Build library
all : $(OUTPUTFOLDER)/$(LIBPREFIX)TSDRLibrary$(EXT)

# Link
$(OUTPUTFOLDER)/$(LIBPREFIX)TSDRLibrary$(EXT) : $(_OBJS)
	$(CC) -Wall -shared $(LDFLAGS) $(DEBUGFLAGS) $(COMPILATION_TYPE) -o $@ $(_OBJS)
	$(AR) rcs $(OUTPUTFOLDER)/$(LIBPREFIX)TSDRLibrary.a $(_OBJS)

# Create folders if needed and compile
$(OBJFOLDER)/%.o : src/%.c $(_DEPS)
	mkdir -p $(OUTPUTFOLDER)
	mkdir -p $(OBJFOLDER)
	$(CC) -Wall $(foreach d, $(INC), -I$d) $(CFLAGS) $(DEBUGFLAGS) $(COMPILATION_TYPE) -c $< -o $@

.PHONY: clean

# Clean artifacts
clean :
	rm -f $(OBJFOLDER)/*.o $(OUTPUTFOLDER)/*.* $(OUTPUTFOLDER)/$(LIBPREFIX)TSDRLibrary$(EXT) $(OUTPUTFOLDER)/$(LIBPREFIX)TSDRLibrary.a
