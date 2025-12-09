#-------------------------------------------------------------------------------
# Copyright (c) 2014 Martin Marinov.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the GNU Public License v3.0
# which accompanies this distribution, and is available at
# http://www.gnu.org/licenses/gpl.html
# 
# Contributors:
#     Martin Marinov - initial API and implementation
#-------------------------------------------------------------------------------

# This makefile will just do make clean and make all to all of the projects

# Detect library extension depending on OS
ifeq ($(OS),Windows_NT)
	OSNAME ?= WINDOWS
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		OSNAME ?= LINUX
	endif
endif

# Set to 1 to enable building TSDRPlugin_RTL, 0 to disable
# Requires librtlsdr to be installed (e.g., via 'brew install librtlsdr' on macOS)
BUILD_RTL_SDR ?= 0 
# Disabled by default

# Make all
all :
	@$(MAKE) -C TSDRPlugin_RawFile/ all JAVA_HOME=$(JAVA_HOME)
ifeq ($(OSNAME),WINDOWS)
	@$(MAKE) -C TSDRPlugin_ExtIO/ all
endif
ifeq ($(BUILD_RTL_SDR),1)
	@$(MAKE) -C TSDRPlugin_RTL/ all JAVA_HOME=$(JAVA_HOME)
endif
	@$(MAKE) -C TempestSDR/ all
	@$(MAKE) -C JavaGUI/ all

# Clean artifacts
clean :
	@$(MAKE) -C TSDRPlugin_RawFile/ clean
	@$(MAKE) -C TSDRPlugin_Mirics/ clean
	@$(MAKE) -C TSDRPlugin_ExtIO/ clean
	@$(MAKE) -C TSDRPlugin_SDRPlay/ clean
	@$(MAKE) -C TSDRPlugin_RTL/ clean
	@$(MAKE) -C TempestSDR/ clean
	@$(MAKE) -C JavaGUI/ clean
