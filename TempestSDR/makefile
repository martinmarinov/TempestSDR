ifeq ($(OS),Windows_NT)
    EXT = .dll
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        EXT = .so
    endif
endif

all : bin/TSDRLibrary$(EXT)

# $@ matches the target, $< matches the first dependancy
bin/TSDRLibrary$(EXT) : bin/TSDRLibrary.o
	gcc -Wl,--add-stdcall-alias -shared -o $@ $<

# $@ matches the target, $< matches the first dependancy
bin/TSDRLibrary.o : src/TSDRLibrary.c src/include/TSDRLibrary.h
	mkdir -p bin
	gcc $(foreach d, $(INC), -I$d) -c $< -o $@
	
clean :
	rm -f bin/*.o bin/TSDRLibrary$(EXT)
