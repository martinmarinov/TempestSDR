OBJS = TSDRLibrary.o TSDRPluginLoader.o
DEPS = include/TSDRLibrary.h TSDRPluginLoader.h

_OBJS = $(patsubst %,bin/obj/%,$(OBJS))
_DEPS = $(patsubst %,src/%,$(DEPS))

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
bin/TSDRLibrary$(EXT) : $(_OBJS)
	gcc -Wl,--add-stdcall-alias -shared -o $@ $(_OBJS)

# $@ matches the target, $< matches the first dependancy
bin/obj/%.o : src/%.c $(_DEPS)
	mkdir -p bin
	mkdir -p bin/obj
	gcc $(foreach d, $(INC), -I$d) -c $< -o $@
	
.PHONY: clean

clean :
	rm -f bin/obj/*.o bin/TSDRLibrary$(EXT)
