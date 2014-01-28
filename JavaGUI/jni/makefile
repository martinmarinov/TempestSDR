# JAVA_HOME should be initialized to something like C:\PROGRA~2\Java\jdk1.7.0_45
ifndef JAVA_HOME
$(warning JAVA_HOME should be set so it points to your jdk installation dir)
endif

# Define all of the C files here
OBJS = TSDRLibraryNDK.o

# Define all of the dependencies here
DEPS = TSDRLibraryNDK.h include/TSDRLibrary.h include/TSDRCodes.h

# Flags
CFLAGS+=-O3

# Headers
INC = "$(JAVA_HOME)/include" "$(JAVA_HOME)/include/win32"

# Detect library extension depending on OS
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

# OS specific flags
ifeq ($(OSNAME),LINUX)
	CFLAGS+=-fPIC
else ifeq ($(OSNAME),WINDOWS)
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
	gcc -Wall -shared $(LDFLAGS) $(COMPILATION_TYPE) -o $(OUTPUTFOLDER)/$(LIBPREFIX)TSDRLibraryNDK$(EXT) $< ../../TempestSDR/bin/$(OSNAME)/$(ARCHNAME)/$(LIBPREFIX)TSDRLibrary.a

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
	gcc -Wall $(foreach d, $(INC), -I$d) $(CFLAGS) $(COMPILATION_TYPE) -c $< -o $@

# Generate the java header based on the class
TSDRLibraryNDK.h : ../bin/martin/tempest/core/TSDRLibrary.class 
	javah -classpath ../bin -o TSDRLibraryNDK.h -jni martin.tempest.core.TSDRLibrary

# If the class is not compiled, do it
../bin/martin/tempest/core/TSDRLibrary.class  : ../src/martin/tempest/core/TSDRLibrary.java
	mkdir -p include ../bin/martin/tempest/core/
	javac $< -d ../bin/ -cp ../src/

# Remove any compiled artifacts
clean :
	rm -f TSDRLibraryNDK.h ../lib/$(OSNAME)/$(ARCHNAME)/$(LIBPREFIX)TSDRLibraryNDK$(EXT)
	rm -rf $(OBJFOLDER)/
