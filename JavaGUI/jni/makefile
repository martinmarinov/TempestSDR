# JAVA_HOME should be initialized to something like C:\PROGRA~2\Java\jdk1.7.0_45
ifndef JAVA_HOME
$(warning JAVA_HOME should be set so it points to your jdk installation dir)
endif

# Headers
INC = "$(JAVA_HOME)/include" "$(JAVA_HOME)/include/win32"

# Detect library extension depending on OS
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

# The folder that will contain the libraries
OUTPUTFOLDER=../lib/$(OSNAME)/$(ARCHNAME)

# We want to build the library
all : $(OUTPUTFOLDER)/TSDRLibraryNDK$(EXT)
	
# Linking
$(OUTPUTFOLDER)/TSDRLibraryNDK$(EXT) : TSDRLibraryNDK.o rebuildtempestsdr $(OUTPUTFOLDER)
	gcc -L../../TempestSDR/bin/$(OSNAME)/$(ARCHNAME)/ -lTSDRLibrary -Wl,--add-stdcall-alias -shared -o $@ $<

# Create output dir
$(OUTPUTFOLDER) :	
	mkdir -p $(OUTPUTFOLDER)

# Rebuild the main library so that we have the most up-to-date version
# The main library is assumed to be two levels below this jni folder
rebuildtempestsdr : $(OUTPUTFOLDER)
	@$(MAKE) -C ../../TempestSDR all
	@cp -f ../../TempestSDR/bin/$(OSNAME)/$(ARCHNAME)/TSDRLibrary$(EXT) $(OUTPUTFOLDER)/TSDRLibrary$(EXT)

# Compile the sources to obj files
TSDRLibraryNDK.o : TSDRLibraryNDK.c TSDRLibraryNDK.h include/TSDRLibrary.h
	gcc $(foreach d, $(INC), -I$d) -c $< -o $@

# Copy over a fresh set of library headers
include/TSDRLibrary.h : ../../TempestSDR/src/include/TSDRLibrary.h
	mkdir -p include
	@cp -f $< $@

# Generate the java header based on the class
TSDRLibraryNDK.h : ../bin/martin/tempest/core/TSDRLibrary.class 
	javah -classpath ../bin -o TSDRLibraryNDK.h -jni martin.tempest.core.TSDRLibrary

# If the class is not compiled, do it
../bin/martin/tempest/core/TSDRLibrary.class  : ../src/martin/tempest/core/TSDRLibrary.java
	mkdir -p include ../bin/martin/tempest/core/
	javac $< -d ../bin/ -cp ../src/

# Remove any compiled artifacts
clean :
	rm -f TSDRLibraryNDK.h *.o ../TSDRLibraryNDK$(EXT) ../TSDRLibrary$(EXT) include/*.h
	rm -rf include/
