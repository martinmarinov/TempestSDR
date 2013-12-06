#ifndef _TSDRPluginLoader
#define _TSDRPluginLoader

// A platform independed dynamic library loader

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

	struct pluginsource {
		void * fd;
		void (*tsdrplugin_getName)(char *);
	} typedef pluginsource_t;

	int tsdrplug_load(pluginsource_t * plugin, char * dlname);
	void tsdrplug_close(pluginsource_t * plugin);

#endif
