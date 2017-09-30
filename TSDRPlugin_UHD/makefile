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
PLUGNAME=TSDRPlugin_UHD

# Dependencies
OBJS=$(PLUGNAME).o

# Flags
ifeq ($(BUILD_TYPE),Debug)
	DEBUGFLAGS+=-g -O0
else
	CFLAGS+=-O3
endif

#for statically compiling
#INC+=-I"$(BOOST_ROOT)/include" -I"$(UHD_ROOT)/include"

# END OF CONFIGURATION IF STANDARD DIR STRUCTURE IS USED

# Compile with g++
GPP?=g++

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
else ifeq ($(shell uname -s),Darwin)
	OSNAME ?= MAC
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

# If you need a different directory structure. Don't change that unless you really want to.
SOURCEFOLDER=src
OUTPUTFOLDER=bin/$(OSNAME)/$(ARCHNAME)
OBJFOLDER=obj

# OS specific flags
ifeq ($(OSNAME),LINUX)
	CXXFLAGS+=-fPIC
	# static version of the libraries if compiled LIBRARIES+=-Ldependencies/$(OSNAME)/$(ARCHNAME) -lpthread -lboost_system -lboost_thread -lboost_program_options -lboost_unit_test_framework -luhd -lboost_serialization -lboost_filesystem  -lboost_date_time -lboost_regex -lrt
	LIBRARIES+=-lboost_thread -lboost_program_options -luhd
else ifeq ($(OSNAME),MAC)
	CXXFLAGS+=-fPIC
	LIBRARIES+=-lboost_thread-mt -lboost_program_options-mt -lboost_system-mt -luhd
else ifeq ($(OSNAME),WINDOWS)
	LDFLAGS+=-Wl,--add-stdcall-alias
	LIBRARIES+= -lboost_thread-mt -lboost_program_options-mt -lboost_system-mt -luhd 
endif

# Calculate the path to dependencies
ifneq ($(OSNAME),WINDOWS)
_OBJS = $(patsubst %,$(OBJFOLDER)/%,$(OBJS))
endif
_DEPS = $(patsubst %,$(SOURCEFOLDER)/%,$(DEPS))


# Generate the library
all : $(OUTPUTFOLDER)/$(LIBPREFIX)$(PLUGNAME)$(EXT)

# Copy over a fresh version of the TSDRPlugin.h
copyoverheaderfile:
	@cp -f $(HEADLOCATION)/TSDRPlugin.h $(SOURCEFOLDER)/
	@cp -f $(HEADLOCATION)/TSDRCodes.h $(SOURCEFOLDER)/

# Link
$(OUTPUTFOLDER)/$(LIBPREFIX)$(PLUGNAME)$(EXT): copyoverheaderfile $(_OBJS)
ifeq ($(OSNAME),WINDOWS)

ifeq ($(ARCHNAME),X86)
	msbuild.exe TSDRPlugin_UHD.vcxproj //p:Configuration=Release;Platform=Win32;msbuildemitsolution=1
else ifeq ($(ARCHNAME),X64)
	msbuild.exe TSDRPlugin_UHD.vcxproj //p:Configuration=Release;Platform=Win64;msbuildemitsolution=1
endif

else
	$(GPP) -shared $(LDFLAGS) $(DEBUGFLAGS) $(COMPILATION_TYPE) -o $@ $(_OBJS) $(LIBRARIES)
endif

# Make dirs and compile
$(OBJFOLDER)/%.o : $(SOURCEFOLDER)/%.cpp
	mkdir -p $(OUTPUTFOLDER)
	mkdir -p $(OBJFOLDER)
	$(GPP) $(INC) $(CXXFLAGS) $(DEBUGFLAGS) $(COMPILATION_TYPE) -c $< -o $@

.PHONY: clean

# Clean artifacts
clean :
	rm -rf $(OBJFOLDER)
	rm -f $(OUTPUTFOLDER)/*.* $(OUTPUTFOLDER)/$(LIBPREFIX)$(PLUGNAME)$(EXT)
