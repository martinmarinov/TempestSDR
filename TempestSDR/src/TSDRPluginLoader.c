#include "TSDRPluginLoader.h"
#include "include/TSDRLibrary.h"
#include "include/TSDRCodes.h"

// A platform independent dynamic library loader

void *tsdrplug_getfunction(pluginsource_t * plugin, char *functname)
{
#if WINHEAD
    return (void*)GetProcAddress((HINSTANCE)plugin->fd,functname);
#else
    return dlsym(plugin->fd,functname);
#endif
}

int tsdrplug_load(pluginsource_t * plugin, const char *dlname)
{

    #if WINHEAD // Microsoft compiler
        plugin->fd = (void*)LoadLibrary(dlname);
    #else
        plugin->fd = dlopen(dlname,2);
    #endif

    if (plugin->fd == NULL)
    	return TSDR_ERR_PLUGIN;

    if ((plugin->tsdrplugin_setParams = tsdrplug_getfunction(plugin, "tsdrplugin_setParams")) == 0) return TSDR_ERR_PLUGIN;
    if ((plugin->tsdrplugin_getsamplerate = tsdrplug_getfunction(plugin, "tsdrplugin_getsamplerate")) == 0) return TSDR_ERR_PLUGIN;
    if ((plugin->tsdrplugin_getName = tsdrplug_getfunction(plugin, "tsdrplugin_getName")) == 0) return TSDR_ERR_PLUGIN;
    if ((plugin->tsdrplugin_setsamplerate = tsdrplug_getfunction(plugin, "tsdrplugin_setsamplerate")) == 0) return TSDR_ERR_PLUGIN;
    if ((plugin->tsdrplugin_setbasefreq = tsdrplug_getfunction(plugin, "tsdrplugin_setbasefreq")) == 0) return TSDR_ERR_PLUGIN;
    if ((plugin->tsdrplugin_stop = tsdrplug_getfunction(plugin, "tsdrplugin_stop")) == 0) return TSDR_ERR_PLUGIN;
    if ((plugin->tsdrplugin_setgain = tsdrplug_getfunction(plugin, "tsdrplugin_setgain")) == 0) return TSDR_ERR_PLUGIN;
    if ((plugin->tsdrplugin_readasync = tsdrplug_getfunction(plugin, "tsdrplugin_readasync")) == 0) return TSDR_ERR_PLUGIN;

    return TSDR_OK;
}


void tsdrplug_close(pluginsource_t * plugin)
{
	if (plugin->fd == NULL) return;
#if WINHEAD
    FreeLibrary((HINSTANCE)plugin->fd);
#else
    dlclose(plugin->fd);
#endif
}
