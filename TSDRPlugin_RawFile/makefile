# This makefile should work for all plugins that reside in the root directory of the repository.
# The header file is copied over from the source
PLUGNAME=TSDRPlugin_RawFile
HEADLOCATION=../TempestSDR/src/include/

SOURCEFOLDER=src
OUTPUTFOLDER=bin
OBJFOLDER=bin/obj

OBJS=$(PLUGNAME).o
DEPS=TSDRPlugin.h

_OBJS = $(patsubst %,$(OBJFOLDER)/%,$(OBJS))
_DEPS = $(patsubst %,$(SOURCEFOLDER)/%,$(DEPS))

ifeq ($(OS),Windows_NT)
	EXT = .dll
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		EXT = .so
	endif
endif

all : $(OUTPUTFOLDER)/$(PLUGNAME)$(EXT)

copyoverheaderfile:
	@cp -f $(HEADLOCATION)/TSDRPlugin.h $(SOURCEFOLDER)/TSDRPlugin.h

$(OUTPUTFOLDER)/$(PLUGNAME)$(EXT): $(_OBJS)
	gcc -Wl,--add-stdcall-alias -shared -o $@ $(_OBJS)

$(OBJFOLDER)/%.o : $(SOURCEFOLDER)/%.c $(_DEPS)
	mkdir -p $(OUTPUTFOLDER)
	mkdir -p $(OBJFOLDER)
	gcc $(foreach d, $(INC), -I$d) -c $< -o $@

.PHONY: clean

clean :
	rm -f $(OBJFOLDER)/*.o $(OUTPUTFOLDER)/$(PLUGNAME)$(EXT) $(SOURCEFOLDER)/TSDRPlugin.h
