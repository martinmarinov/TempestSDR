# Define all of the C files here
OBJS = TSDRLibrary.o TSDRPluginLoader.o

# Define all of the dependencies here
DEPS = include/TSDRLibrary.h TSDRPluginLoader.h

# USER CONFIGURATION ENDS HERE

# The obj files reside in the bin/obj directory
_OBJS = $(patsubst %,bin/obj/%,$(OBJS))

# The header files are relative to the source directory
_DEPS = $(patsubst %,src/%,$(DEPS))

# Detect library extension based on OS
ifeq ($(OS),Windows_NT)
    EXT = .dll
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        EXT = .so
    endif
endif

# Build library
all : bin/TSDRLibrary$(EXT)

# Link
bin/TSDRLibrary$(EXT) : $(_OBJS)
	gcc -Wl,--add-stdcall-alias -shared -o $@ $(_OBJS)

# Create folders if needed and compile
bin/obj/%.o : src/%.c $(_DEPS)
	mkdir -p bin
	mkdir -p bin/obj
	gcc $(foreach d, $(INC), -I$d) -c $< -o $@
	
.PHONY: clean

# Clean artifacts
clean :
	rm -f bin/obj/*.o bin/TSDRLibrary$(EXT)
