#include "TSDRPluginLoader.h"

// A platform independent dynamic library loader

void *tsdrplug_getfunction(pluginsource_t * plugin, char *functname)
{
#if WINHEAD
    return (void*)GetProcAddress((HINSTANCE)plugin->fd,functname);
#else
    return dlsym(plugin->fd,functname);
#endif
}

int tsdrplug_load(pluginsource_t * plugin, char *dlname)
{
    #if WINHEAD // Microsoft compiler
        plugin->fd = (void*)LoadLibrary(dlname);
    #else
        plugin->fd = dlopen(dlname,2);
    #endif

    if (plugin->fd == 0)
    	return 0;

    plugin->tsdrplugin_getName = tsdrplug_getfunction(plugin, "tsdrplugin_getName");
    if (plugin->tsdrplugin_getName == 0)
    	return 0;

    return 1;
}


void tsdrplug_close(pluginsource_t * plugin)
{
	if (plugin->fd == 0) return;
#if WINHEAD
    FreeLibrary((HINSTANCE)plugin->fd);
#else
    dlclose(plugin->fd);
#endif
}
