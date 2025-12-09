# SPDX-License-Identifier: GPL-3.0-or-later
# TempestSDR Plugin Architecture:
#  Copyright (c) 2014 Martin Marinov.
# librtlsdr library:
#  Copyright (C) 2012-2013 by Steve Markgraf <steve@steve-m.de>
#  Copyright (C) 2012 by Dimitri Stolnikov <horiz0n@gmx.net>
# Plugin contribution:
#  Copyright (C) 2025 Tymoteusz Vogt

PLUGNAME=TSDRPlugin_RTL

OBJS=$(PLUGNAME).o
DEPS=TSDRPlugin.h TSDRCodes.h

LIBS+=-lrtlsdr

INC+=-I/usr/local/include

ifeq ($(BUILD_TYPE),Debug)
	DEBUGFLAGS+=-g -O0
else
	CFLAGS+=-O3
endif

# END OF CONFIGURATION

# TSDRPlugin.h location
HEADLOCATION=../TempestSDR/src/include/

# Discover the library extension for each OS
# NOTE: Windows is not supported – use ExtIO plugin instead.
ifeq ($(shell uname -s),Darwin)
	OSNAME ?= MAC
	EXT ?= .so
	HOMEBREW_PREFIX ?= $(shell brew --prefix 2>/dev/null || echo /opt/homebrew)
	INC += -I$(HOMEBREW_PREFIX)/include
	LIBS += -L$(HOMEBREW_PREFIX)/lib
	ifndef ARCHNAME
		ifeq ($(shell uname -m),x86)
			ARCHNAME = X86
		endif
		ifeq ($(shell uname -m),x86_64)
			ARCHNAME = X64
		endif
		ifeq ($(shell uname -m),arm64)
			ARCHNAME = ARM64
		endif
	endif
else

	ifndef OSNAME

		UNAME_S := $(shell uname -s)
		ifeq ($(UNAME_S),Linux)
			OSNAME = LINUX
		endif

	endif

	ifndef ARCHNAME

		UNAME_M := $(shell uname -m)
		ifeq ($(UNAME_M),x86_64)
			ARCHNAME = X64
		endif
		ifneq ($(filter %86,$(UNAME_M)),)
			ARCHNAME = X86
		endif
		ifeq ($(UNAME_M),aarch64)
			ARCHNAME = ARM64
		endif
		ifneq ($(filter arm%,$(UNAME_M)),)
			ARCHNAME = ARM
		endif

	endif
endif

ifeq ($(OSNAME),LINUX)
	EXT=.so
	LIBPREFIX=lib
	CFLAGS+=-fPIC
endif

ifeq ($(OSNAME),MAC)
	EXT=.so
	LIBPREFIX=
	CFLAGS+=-fPIC
endif

# Output dir
OUTDIR=bin/$(OSNAME)/$(ARCHNAME)/
OUTSOURCE=$(OUTDIR)$(LIBPREFIX)$(PLUGNAME)$(EXT)

CC?=gcc
CFLAGS+=-Wall -Wextra $(INC) $(DEBUGFLAGS) -I./src

all: before_build actual_build after_build

before_build:
	@echo "Building $(PLUGNAME) for $(OSNAME)/$(ARCHNAME)..."
	@mkdir -p $(OUTDIR)
	@mkdir -p obj
	@cp $(HEADLOCATION)TSDRPlugin.h ./src/
	@cp $(HEADLOCATION)TSDRCodes.h ./src/

actual_build: $(OBJS)
	$(CC) -shared -o $(OUTSOURCE) obj/*.o $(LIBS)
	@echo "Built $(OUTSOURCE)"

%.o: src/%.c
	$(CC) -c -o obj/$@ $< $(CFLAGS)

after_build:
	@echo "Build complete!"

clean:
	rm -f obj/*.o src/TSDRPlugin.h src/TSDRCodes.h $(OUTSOURCE)

install:
	@echo "Installing to ../Release/dlls/$(OSNAME)/$(ARCHNAME)/"
	@mkdir -p ../Release/dlls/$(OSNAME)/$(ARCHNAME)/
	@cp $(OUTSOURCE) ../Release/dlls/$(OSNAME)/$(ARCHNAME)/
	@echo "Install complete!".PHONY: all before_build actual_build after_build clean install
