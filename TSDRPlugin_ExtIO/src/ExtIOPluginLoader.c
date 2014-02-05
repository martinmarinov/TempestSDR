#include "ExtIOPluginLoader.h"
#include "TSDRCodes.h"

#include <stdlib.h>

// A platform independent dynamic library loader

void *extio_getfunction(extiosource_t * plugin, char *functname)
{
#if WINHEAD
    return (void*)GetProcAddress((HINSTANCE)plugin->fd,functname);
#else
    return dlsym(plugin->fd,functname);
#endif
}

void extio_close(extiosource_t * plugin)
{
	if (plugin->fd == NULL) return;
#if WINHEAD
    FreeLibrary((HINSTANCE)plugin->fd);
#else
    dlclose(plugin->fd);
#endif
}

int extio_load(extiosource_t * plugin, const char *dlname)
{

    #if WINHEAD // Microsoft compiler
        plugin->fd = (void*)LoadLibrary(dlname);
    #else
        plugin->fd = dlopen(dlname,2);
    #endif

    if (plugin->fd == NULL)
    	return TSDR_ERR_PLUGIN;

    // mandatory
    if ((plugin->InitHW = extio_getfunction(plugin, "InitHW")) == 0) return TSDR_ERR_PLUGIN;
    if ((plugin->OpenHW = extio_getfunction(plugin, "OpenHW")) == 0) return TSDR_ERR_PLUGIN;
    if ((plugin->CloseHW = extio_getfunction(plugin, "CloseHW")) == 0) return TSDR_ERR_PLUGIN;
    if ((plugin->StartHW = extio_getfunction(plugin, "StartHW")) == 0) return TSDR_ERR_PLUGIN;
    if ((plugin->StopHW = extio_getfunction(plugin, "StopHW")) == 0) return TSDR_ERR_PLUGIN;
    if ((plugin->SetCallback = extio_getfunction(plugin, "SetCallback")) == 0) return TSDR_ERR_PLUGIN;
    if ((plugin->SetHWLO = extio_getfunction(plugin, "SetHWLO")) == 0) return TSDR_ERR_PLUGIN;
    if ((plugin->GetStatus = extio_getfunction(plugin, "GetStatus")) == 0) return TSDR_ERR_PLUGIN;

    // optional
    if ((plugin->RawDataReady = extio_getfunction(plugin, "RawDataReady")) == 0) plugin->RawDataReady = NULL;
    if ((plugin->ShowGUI = extio_getfunction(plugin, "ShowGUI")) == 0) plugin->ShowGUI = NULL;
    if ((plugin->HideGUI = extio_getfunction(plugin, "HideGUI")) == 0) plugin->HideGUI = NULL;

    return TSDR_OK;
}
