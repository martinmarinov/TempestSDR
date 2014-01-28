# Define all of the C files here
OBJS = TSDRLibrary.o TSDRPluginLoader.o threading.o circbuff.o

# Define all of the dependencies here
DEPS = include/TSDRLibrary.h TSDRPluginLoader.h include/TSDRPlugin.h osdetect.h threading.h circbuff.h

# Flags
CFLAGS+=-O3

# USER CONFIGURATION ENDS HERE


# Detect library extension based on OS
ifeq ($(OS),Windows_NT)
	
	EXT=.dll
	OSNAME = WINDOWS
	
	ifndef $(ARCHNAME)
	
		ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
			ARCHNAME = X64
		endif
		ifeq ($(PROCESSOR_ARCHITECTURE),x86)
			ARCHNAME = X86
		endif
	
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
	CFLAGS+=-fPIC
else ifeq ($(OSNAME),WINDOWS)
	LDFLAGS+=-Wl,--add-stdcall-alias
endif

# The obj files reside in the bin/obj directory
_OBJS = $(patsubst %,$(OBJFOLDER)/%,$(OBJS))

# The header files are relative to the source directory
_DEPS = $(patsubst %,$(SOURCEFOLDER)/%,$(DEPS))

# Build library
all : $(OUTPUTFOLDER)/$(LIBPREFIX)TSDRLibrary$(EXT)

# Link
$(OUTPUTFOLDER)/$(LIBPREFIX)TSDRLibrary$(EXT) : $(_OBJS)
	gcc -Wall -shared $(LDFLAGS) $(COMPILATION_TYPE) -o $@ $(_OBJS)
	ar rcs -o $(OUTPUTFOLDER)/$(LIBPREFIX)TSDRLibrary.a $(_OBJS)

# Create folders if needed and compile
$(OBJFOLDER)/%.o : src/%.c $(_DEPS)
	mkdir -p $(OUTPUTFOLDER)
	mkdir -p $(OBJFOLDER)
	gcc -Wall $(foreach d, $(INC), -I$d) $(CFLAGS) $(COMPILATION_TYPE) -c $< -o $@
	
.PHONY: clean

# Clean artifacts
clean :
	rm -f $(OBJFOLDER)/*.o $(OUTPUTFOLDER)/*.* $(OUTPUTFOLDER)/$(LIBPREFIX)TSDRLibrary$(EXT) $(OUTPUTFOLDER)/$(LIBPREFIX)TSDRLibrary.a
