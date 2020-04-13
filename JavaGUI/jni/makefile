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
# JAVA_HOME should be initialized to something like C:\PROGRA~2\Java\jdk1.7.0_45
ifndef JAVA_HOME
$(warning JAVA_HOME should be set so it points to your jdk installation dir)
endif

# Define all of the C files here
OBJS = TSDRLibraryNDK.o

# Define all of the dependencies here
DEPS = TSDRLibraryNDK.h include/TSDRLibrary.h include/TSDRCodes.h

# Flags
ifeq ($(BUILD_TYPE),Debug)
	DEBUGFLAGS+=-g -O0
else
	CFLAGS+=-O3
endif

# Headers
INC += "$(JAVA_HOME)/include" "$(JAVA_HOME)/include/win32" "$(JAVA_HOME)/include/linux" "$(JAVA_HOME)/include/darwin"

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
	INC += /System/Library/Frameworks/JavaVM.framework/Versions/Current/Headers
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


# OS specific flags
ifeq ($(OSNAME),LINUX)
	CFLAGS+=-fPIC
else ifeq ($(OSNAME),WINDOWS)
	CFLAGS+=-D__int64="long long" -D_JNI_IMPLEMENTATION_ -Wl,--kill-at
	LDFLAGS+=-Wl,--add-stdcall-alias
endif

# The folder that will contain the libraries
OUTPUTFOLDER=../lib/$(OSNAME)/$(ARCHNAME)

# The folder that will contain the object files
OBJFOLDER=obj

# The obj files reside in the bin/obj directory
_OBJS = $(patsubst %,$(OBJFOLDER)/%,$(OBJS))

# We want to build the library
all : rebuildtempestsdr library

# Linking
library : $(_OBJS)
	$(CC) -Wall -shared $(LDFLAGS) $(DEBUGFLAGS) $(COMPILATION_TYPE) -o $(OUTPUTFOLDER)/$(LIBPREFIX)TSDRLibraryNDK$(EXT) $< ../../TempestSDR/bin/$(OSNAME)/$(ARCHNAME)/$(LIBPREFIX)TSDRLibrary.a

# Rebuild the main library so that we have the most up-to-date version
# The main library is assumed to be two levels below this jni folder
rebuildtempestsdr :
	@$(MAKE) -C ../../TempestSDR all
	mkdir -p include
	@cp -f ../../TempestSDR/src/include/TSDRCodes.h include/
	@cp -f ../../TempestSDR/src/include/TSDRLibrary.h include/
	mkdir -p $(OUTPUTFOLDER)
	mkdir -p $(OBJFOLDER)

# Compile the sources to obj files
$(OBJFOLDER)/%.o : %.c $(DEPS)
	$(CC) -Wall $(foreach d, $(INC), -I$d) $(CFLAGS) $(DEBUGFLAGS) $(COMPILATION_TYPE) -c $< -o $@

# Generate the java header based on the class
TSDRLibraryNDK.h : ../src/martin/tempest/core/TSDRLibrary.java
	mkdir -p include ../bin/martin/tempest/core/
	javac $< -h ./ -d ../bin/ -cp ../src/
	# dirty fix to match the previous name
	mv martin_tempest_core_TSDRLibrary.h TSDRLibraryNDK.h


# Remove any compiled artifacts
clean :
	rm -f TSDRLibraryNDK.h
	rm -rf $(OBJFOLDER)/
