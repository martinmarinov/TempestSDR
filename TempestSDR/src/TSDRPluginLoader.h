#ifndef _TSDRPluginLoader
#define _TSDRPluginLoader

// A platform independed dynamic library loader

#include <stdint.h>

#ifdef OS_WINDOWS
   #include <windows.h>
	#define WINHEAD (1)
#elif defined(_WIN64)
   #include <windows.h>
	#define WINHEAD (1)
#elif defined(_WIN32)
   #include <windows.h>
	#define WINHEAD (1)
#elif defined(_MSC_VER) // Microsoft compiler
    #include <windows.h>
	#define WINHEAD (1)
#else
    #include <dlfcn.h>
	#define WINHEAD (0)
#endif

	typedef void(*tsdrplugin_readasync_function)(float *buf, uint32_t len, void *ctx);

	struct pluginsource {
		void * fd;
		void (*tsdrplugin_getName)(char *);
		int (*tsdrplugin_init)(char *);
		int (*tsdrplugin_setsamplerate)(uint32_t);
		int (*tsdrplugin_setbasefreq)(uint32_t);
		int (*tsdrplugin_stop)(void);
		int (*tsdrplugin_setgain)(float);
		int (*tsdrplugin_readasync)(tsdrplugin_readasync_function, void *ctx, uint32_t, uint32_t);
	} typedef pluginsource_t;

	int tsdrplug_load(pluginsource_t * plugin, char * dlname);
	void tsdrplug_close(pluginsource_t * plugin);

#endif
