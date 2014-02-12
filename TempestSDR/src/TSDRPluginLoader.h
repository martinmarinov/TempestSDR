/*
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
*/
#ifndef _TSDRPluginLoader
#define _TSDRPluginLoader

// A platform independed dynamic library loader

#include <stdint.h>
#include "osdetect.h"
#include "include/TSDRPlugin.h"

#if WINHEAD
	#include <windows.h>
#else
	#include <dlfcn.h>
#endif

	struct pluginsource {
		void * fd;
		int (__stdcall *tsdrplugin_init)(const char * params);
		void (__stdcall *tsdrplugin_getName)(char *);
		uint32_t (__stdcall *tsdrplugin_setsamplerate)(uint32_t);
		uint32_t (__stdcall *tsdrplugin_getsamplerate)(void);
		int (__stdcall *tsdrplugin_setbasefreq)(uint32_t);
		int (__stdcall *tsdrplugin_stop)(void);
		int (__stdcall *tsdrplugin_setgain)(float);
		int (__stdcall *tsdrplugin_readasync)(tsdrplugin_readasync_function, void *);
		char * (__stdcall *tsdrplugin_getlasterrortext) (void);
		void * (__stdcall *tsdrplugin_cleanup) (void);
		volatile int initialized;

	} typedef pluginsource_t;

	int tsdrplug_load(pluginsource_t * plugin, const char * dlname);
	void tsdrplug_close(pluginsource_t * plugin);

#endif
