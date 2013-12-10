#ifndef _TSDRPluginLoader
#define _TSDRPluginLoader

// A platform independed dynamic library loader

#include <stdint.h>
#include "osdetect.h"

#if WINHEAD
	#include <windows.h>
#else
	#include <dlfcn.h>
#endif

	typedef void(*tsdrplugin_readasync_function)(float *buf, uint32_t len, void *ctx);

	struct pluginsource {
		void * fd;
		void (*tsdrplugin_getName)(char *);
		int (*tsdrplugin_setsamplerate)(uint32_t);
		int (*tsdrplugin_setbasefreq)(uint32_t);
		int (*tsdrplugin_stop)(void);
		int (*tsdrplugin_setgain)(float);
		int (*tsdrplugin_readasync)(tsdrplugin_readasync_function cb, void *ctx, const char * params);
	} typedef pluginsource_t;

	int tsdrplug_load(pluginsource_t * plugin, const char * dlname);
	void tsdrplug_close(pluginsource_t * plugin);

#endif
