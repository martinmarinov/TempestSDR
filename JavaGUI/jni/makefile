# TODO! Make this platform independent!
INC = "C:\PROGRA~2\Java\jdk1.7.0_45\include" "C:\PROGRA~2\Java\jdk1.7.0_45\include\win32"
EXT = .dll

all : TSDRLibraryNDK$(EXT)

# $@ matches the target, $< matches the first dependancy
TSDRLibraryNDK$(EXT) : TSDRLibraryNDK.o
	gcc -Wl,--add-stdcall-alias -shared -o $@ $<

# $@ matches the target, $< matches the first dependancy
TSDRLibraryNDK.o : TSDRLibraryNDK.c TSDRLibraryNDK.h
	gcc $(foreach d, $(INC), -I$d) -c $< -o $@

# $* matches the target filename without the extension
TSDRLibraryNDK.h : ../bin/martin/tempest/core/TSDRLibrary.class 
	javah -classpath ../bin -o TSDRLibraryNDK.h -jni martin.tempest.core.TSDRLibrary

clean :
	rm -f *.h *.o *.dll *.so *.exe
