# JAVA_HOME should be initialized to something like C:\PROGRA~2\Java\jdk1.7.0_45
ifndef JAVA_HOME
$(warning JAVA_HOME should be set so it points to your jdk installation dir)
endif

# Headers
INC = "$(JAVA_HOME)/include" "$(JAVA_HOME)/include/win32"

# Detect library extension depending on OS
ifeq ($(OS),Windows_NT)
    EXT = .dll
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        EXT = .so
    endif
endif

# We want to build the library
all : ../TSDRLibraryNDK$(EXT)
	
# Linking
../TSDRLibraryNDK$(EXT) : TSDRLibraryNDK.o rebuildtempestsdr
	gcc -L../../TempestSDR/bin/ -lTSDRLibrary -Wl,--add-stdcall-alias -shared -o $@ $<

# Rebuild the main library so that we have the most up-to-date version
# The main library is assumed to be two levels below this jni folder
rebuildtempestsdr :
	@$(MAKE) -C ../../TempestSDR bin/TSDRLibrary$(EXT)
	@cp -f ../../TempestSDR/bin/TSDRLibrary$(EXT) ../TSDRLibrary$(EXT)

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
