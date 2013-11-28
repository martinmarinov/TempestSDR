# TODO! Make this platform independent!
INC = "C:\PROGRA~2\Java\jdk1.7.0_45\include" "C:\PROGRA~2\Java\jdk1.7.0_45\include\win32"

ifeq ($(OS),Windows_NT)
    EXT = .dll
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        EXT = .so
    endif
endif

all : ../TSDRLibraryNDK$(EXT)
	
# $@ matches the target, $< matches the firs8t dependancy
../TSDRLibraryNDK$(EXT) : TSDRLibraryNDK.o rebuildtempestsdr
	gcc -L../../TempestSDR/bin/ -lTSDRLibrary -Wl,--add-stdcall-alias -shared -o $@ $<

rebuildtempestsdr :
	@$(MAKE) -C ../../TempestSDR bin/TSDRLibrary$(EXT)
	@cp -f ../../TempestSDR/bin/TSDRLibrary$(EXT) ../TSDRLibrary$(EXT)

# $@ matches the target, $< matches the first dependancy
TSDRLibraryNDK.o : TSDRLibraryNDK.c TSDRLibraryNDK.h include/TSDRLibrary.h
	gcc $(foreach d, $(INC), -I$d) -c $< -o $@
	
include/TSDRLibrary.h : ../../TempestSDR/src/include/TSDRLibrary.h
	mkdir -p include
	@cp -f $< $@

TSDRLibraryNDK.h : ../bin/martin/tempest/core/TSDRLibrary.class 
	javah -classpath ../bin -o TSDRLibraryNDK.h -jni martin.tempest.core.TSDRLibrary
	
../bin/martin/tempest/core/TSDRLibrary.class  : ../src/martin/tempest/core/TSDRLibrary.java
	mkdir -p include ../bin/martin/tempest/core/
	javac $< -d ../bin/

clean :
	rm -f TSDRLibraryNDK.h *.o ../TSDRLibraryNDK$(EXT) ../TSDRLibrary$(EXT) include/*.h
	rm -rf include/
