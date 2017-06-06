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
# This makefile should work for all plugins that reside in the root directory of the repository.
# The header file is copied over from the source
PLUGNAME=TSDRPlugin_Mirics

# Dependencies
OBJS=$(PLUGNAME).o
DEPS=TSDRPlugin.h TSDRCodes.h

# Flags
ifeq ($(BUILD_TYPE),Debug)
	DEBUGFLAGS+=-g -O0
else
	CFLAGS+=-O3
endif

# Headers
INC += "$(MIRICS_HOME)/API/inc"

# END OF CONFIGURATION IF STANDARD DIR STRUCTURE IS USED

# Where the TSDRPlugin.h of the TempestSDR library resides (so it will be copied over)
HEADLOCATION=../TempestSDR/src/include/

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
	ARCHNAME_LOWER=x86
else ifeq ($(ARCHNAME),X64)
	COMPILATION_TYPE=-m64
	ARCHNAME_LOWER=x64
endif


# If you need a different directory structure. Don't change that unless you really want to.
SOURCEFOLDER=src
OUTPUTFOLDER=bin/$(OSNAME)/$(ARCHNAME)
OBJFOLDER=obj

# Calculate the path to dependencies
_OBJS = $(patsubst %,$(OBJFOLDER)/%,$(OBJS))
_DEPS = $(patsubst %,$(SOURCEFOLDER)/%,$(DEPS))

# OS specific flags
LDFLAGS+=-Wl,--add-stdcall-alias

# Generate the library
all : $(OUTPUTFOLDER)/$(PLUGNAME)$(EXT)

# Copy over a fresh version of the TSDRPlugin.h
copyoverheaderfile:
	@cp -f $(HEADLOCATION)/TSDRPlugin.h $(SOURCEFOLDER)/
	@cp -f $(HEADLOCATION)/TSDRCodes.h $(SOURCEFOLDER)/

# Link
$(OUTPUTFOLDER)/$(PLUGNAME)$(EXT): copyoverheaderfile $(_OBJS)
	$(CC) -Wall -L"$(MIRICS_HOME)/API/$(ARCHNAME_LOWER)/" -lmir_sdr_api $(LDFLAGS) $(DEBUGFLAGS) -shared $(COMPILATION_TYPE) -o $@ $(_OBJS)

# Make dirs and compile
$(OBJFOLDER)/%.o : $(SOURCEFOLDER)/%.c $(_DEPS)
	mkdir -p $(OUTPUTFOLDER)
	mkdir -p $(OBJFOLDER)
	$(CC) -Wall $(foreach d, $(INC), -I$d) $(CFLAGS) $(DEBUGFLAGS) $(COMPILATION_TYPE) -c $< -o $@

.PHONY: clean

# Clean artifacts
clean :
	rm -f $(OBJFOLDER)/*.o $(OUTPUTFOLDER)/*.* $(OUTPUTFOLDER)/$(PLUGNAME)$(EXT)
