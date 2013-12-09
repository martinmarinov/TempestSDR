# Define all of the C files here
OBJS = TSDRLibrary.o TSDRPluginLoader.o

# Define all of the dependencies here
DEPS = include/TSDRLibrary.h TSDRPluginLoader.h include/TSDRPlugin.h

# USER CONFIGURATION ENDS HERE

# Detect library extension based on OS
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

# The obj files reside in the bin/obj directory
_OBJS = $(patsubst %,$(OBJFOLDER)/%,$(OBJS))

# The header files are relative to the source directory
_DEPS = $(patsubst %,$(SOURCEFOLDER)/%,$(DEPS))

# Build library
all : $(OUTPUTFOLDER)/TSDRLibrary$(EXT)

# Link
$(OUTPUTFOLDER)/TSDRLibrary$(EXT) : $(_OBJS)
	gcc -Wl,--add-stdcall-alias -shared -o $@ $(_OBJS)

# Create folders if needed and compile
$(OBJFOLDER)/%.o : src/%.c $(_DEPS)
	mkdir -p $(OUTPUTFOLDER)
	mkdir -p $(OBJFOLDER)
	gcc $(foreach d, $(INC), -I$d) $(CFLAGS) -c $< -o $@
	
.PHONY: clean

# Clean artifacts
clean :
	rm -f $(OBJFOLDER)/*.o $(OUTPUTFOLDER)/*.*
