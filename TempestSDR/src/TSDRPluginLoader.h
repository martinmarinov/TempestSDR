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

	typedef void(*tsdrplugin_readasync_function)(float *buf, uint32_t len, void *ctx, int dropped_samples);

	struct pluginsource {
		void * fd;
		int (*tsdrplugin_init)(const char * params);
		void (*tsdrplugin_getName)(char *);
		uint32_t (*tsdrplugin_setsamplerate)(uint32_t);
		uint32_t (*tsdrplugin_getsamplerate)(void);
		int (*tsdrplugin_setbasefreq)(uint32_t);
		int (*tsdrplugin_stop)(void);
		int (*tsdrplugin_setgain)(float);
		int (*tsdrplugin_readasync)(tsdrplugin_readasync_function, void *);
		char * (*tsdrplugin_getlasterrortext) (void);
		void * (*tsdrplugin_cleanup) (void);

	} typedef pluginsource_t;

	int tsdrplug_load(pluginsource_t * plugin, const char * dlname);
	void tsdrplug_close(pluginsource_t * plugin);

#endif
