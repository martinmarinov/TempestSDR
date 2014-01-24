# This makefile should work for all plugins that reside in the root directory of the repository.
# The header file is copied over from the source
PLUGNAME=TSDRPlugin_RawFile

# Dependencies
OBJS=$(PLUGNAME).o timer.o
DEPS=TSDRPlugin.h TSDRCodes.h timer.h osdetect.h

# END OF CONFIGURATION IF STANDARD DIR STRUCTURE IS USED

# Where the TSDRPlugin.h of the TempestSDR library resides (so it will be copied over)
HEADLOCATION=../TempestSDR/src/include/

# Discover the library extension for each OS
ifeq ($(OS),Windows_NT)
	EXT=.dll
	OSNAME = WINDOWS
	ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
		ARCHNAME = X64
	endif
	ifeq ($(PROCESSOR_ARCHITECTURE),x86)
		ARCHNAME = X86
	endif
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		EXT=.so
		OSNAME = LINUX
		LIBPREFIX=lib
	endif
	ifeq ($(UNAME_S),Darwin)
		EXT=.a
		OSNAME = OSX
	endif
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

# If you need a different directory structure. Don't change that unless you really want to.
SOURCEFOLDER=src
OUTPUTFOLDER=bin/$(OSNAME)/$(ARCHNAME)
OBJFOLDER=obj

# Flags
CPPFLAGS=-O3

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
	gcc -Wl -shared -o $@ $(_OBJS)  -fPIC

# Make dirs and compile
$(OBJFOLDER)/%.o : $(SOURCEFOLDER)/%.c $(_DEPS)
	mkdir -p $(OUTPUTFOLDER)
	mkdir -p $(OBJFOLDER)
	gcc $(foreach d, $(INC), -I$d) $(CFLAGS) -c $< -o $@  -fPIC

.PHONY: clean

# Clean artifacts
clean :
	rm -f $(OBJFOLDER)/*.o $(OUTPUTFOLDER)/*.*
