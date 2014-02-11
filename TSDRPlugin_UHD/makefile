# This makefile should work for all plugins that reside in the root directory of the repository.
# The header file is copied over from the source
PLUGNAME=TSDRPlugin_UHD

# Dependencies
OBJS=$(PLUGNAME).o
LIBRARIES=-luhd -lboost_thread -lboost_program_options

# Flags
ifeq ($(BUILD_TYPE),Debug)
	DEBUGFLAGS+=-g -O0
else
	CFLAGS+=-O3
endif

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
else

	ifndef $(OSNAME)
	
		UNAME_S := $(shell uname -s)
		ifeq ($(UNAME_S),Linux)
			OSNAME = LINUX
		endif
	
	endif
	
	ifndef $(ARCHNAME)
	
		UNAME_P := $(shell uname -p)
		ifeq ($(UNAME_P),x86_64)
			ARCHNAME = X64
		endif
		ifneq ($(filter %86,$(UNAME_P)),)
			ARCHNAME = X86
		endif
		ifneq ($(filter arm%,$(UNAME_P)),)
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
else ifeq ($(OSNAME),WINDOWS)
	LDFLAGS+=-Wl,--add-stdcall-alias
endif

# Calculate the path to dependencies
_OBJS = $(patsubst %,$(OBJFOLDER)/%,$(OBJS))
_DEPS = $(patsubst %,$(SOURCEFOLDER)/%,$(DEPS))

# Generate the library
all : $(OUTPUTFOLDER)/$(LIBPREFIX)$(PLUGNAME)$(EXT)

# Copy over a fresh version of the TSDRPlugin.h
copyoverheaderfile:
	@cp -f $(HEADLOCATION)/TSDRPlugin.h $(SOURCEFOLDER)/
	@cp -f $(HEADLOCATION)/TSDRCodes.h $(SOURCEFOLDER)/

# Link
$(OUTPUTFOLDER)/$(LIBPREFIX)$(PLUGNAME)$(EXT): copyoverheaderfile $(_OBJS)
	$(GPP) -Wall -shared $(LDFLAGS) $(DEBUGFLAGS) $(COMPILATION_TYPE) -o $@ $(_OBJS) $(LIBRARIES) 

# Make dirs and compile
$(OBJFOLDER)/%.o : $(SOURCEFOLDER)/%.cpp
	mkdir -p $(OUTPUTFOLDER)
	mkdir -p $(OBJFOLDER)
	$(GPP) -Wall $(foreach d, $(INC), -I$d) $(CXXFLAGS) $(DEBUGFLAGS) $(COMPILATION_TYPE) -c $< -o $@

.PHONY: clean

# Clean artifacts
clean :
	rm -f $(OBJFOLDER)/*.o $(OUTPUTFOLDER)/*.* $(OUTPUTFOLDER)/$(LIBPREFIX)$(PLUGNAME)$(EXT)
