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


# USER CONFIGURATION STARTS HERE

# Define the plugins that you like to compile with the library. They need to be in a folder
# in the parent directory with the exact same name as the plugin names. The library files
# produced need to have the name of the plugin folder and need to be in the bin directory.
# The default plugin file will make sure this is compatible.
PLUGINS=TSDRPlugin_RawFile 

ifeq ($(OSNAME),WINDOWS)
# Windows only plugins
PLUGINS += TSDRPlugin_SDRPlay TSDRPlugin_ExtIO
else ifeq ($(OSNAME), LINUX)
PLUGINS += TSDRPlugin_UHD
else ifeq ($(OSNAME), MAC)
PLUGINS += TSDRPlugin_UHD TSDRPlugin_SDRPlay
endif

# USER CONFIGURATION ENDS HERE

# Define a recursive wildcard function
rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

# Discover all the java sources
JAVAFILES:=$(call rwildcard,src/,*.java)

# Create the class dependencies
CLASSFILES:=$(patsubst %,bin/%,$(addsuffix .class,$(basename $(foreach jfile,$(JAVAFILES), $(subst src/,,$(jfile)) ))))

# The folder that will contain the libraries
OUTPUTFOLDER=lib/$(OSNAME)/$(ARCHNAME)

# Build the file names of the plugin files
PLUGINS_FILES=$(addprefix $(LIBPREFIX),$(addsuffix $(EXT),$(PLUGINS)))

# Build the directory names of the plugins
PLUGINS_DIRS=$(addprefix ../,$(addsuffix /,$(PLUGINS)))

debug : BUILD_TYPE = Debug

debugjnilib : BUILD_TYPE = Debug

debug : all

debugjnilib : jnilib

# When compiling from command line with all
# make sure we compile both jni and java files
all : jnilib jar

# Package compiled files into a runnable jar
jar : java
	jar cvfe JTempestSDR.jar martin.tempest.gui.Main lib -C bin .

# The compilation of the java classes from command line
# depends on the class files (we want to produce them)
java : $(CLASSFILES)

# For each class file, invoke javac with the source file which is constructed
# from the name of the class file
%.class : $(JAVAFILES)
	mkdir -p bin # Create the bin directory if it doesn't exist (if you downloaded it from github)
	javac $(patsubst %,src/%, $(subst bin/,,$(@:.class=.java))) -d bin/ -cp src/

# Invoke the makefile for the jni stuff
jnilib : plugins
	@$(MAKE) -C jni/ all JAVA_HOME=$(JAVA_HOME) BUILD_TYPE=$(BUILD_TYPE)

# Recompile each plugin and copy its shared library over to the java directory
plugins :
	mkdir -p $(OUTPUTFOLDER)
	$(foreach pdir, $(PLUGINS_DIRS), $(MAKE) -C $(pdir) all BUILD_TYPE=$(BUILD_TYPE); cp -f $(pdir)/bin/$(OSNAME)/$(ARCHNAME)/*$(EXT) $(OUTPUTFOLDER)/;)

# Clean the jni stuff
cleanjni: cleanplugins
	rm -rf *.log
	@$(MAKE) -C jni/ clean

# Clean the plugin libraries
cleanplugins:
	$(foreach pfile, $(PLUGINS_FILES), rm -rf $(pfile);)	

# Clean the TempestSDR and plugins
cleandependent :
	@$(MAKE) -C jni/ clean
	$(foreach pdir, $(PLUGINS_DIRS), $(MAKE) -C $(pdir) clean;)
	@$(MAKE) -C ../TempestSDR/ clean

# Clean the java classes as well
clean : cleanjni cleandependent
	rm -rf bin/ JTempestSDR.jar
