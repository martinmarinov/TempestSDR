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

#include "TSDRPluginLoader.h"
#include "include/TSDRLibrary.h"
#include "include/TSDRCodes.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
	plugin->tsdrplugin_cleanup = NULL;

    #if WINHEAD // Microsoft compiler
        plugin->fd = (void*)LoadLibrary(dlname);
    #else
        plugin->fd = dlopen(dlname,RTLD_NOW);
        if (plugin->fd == NULL)
        	fprintf(stderr,"Library load exception: %s\n",dlerror());
    #endif

    if (plugin->fd == NULL)
    	return TSDR_INCOMPATIBLE_PLUGIN;

    if ((plugin->tsdrplugin_init = tsdrplug_getfunction(plugin, "tsdrplugin_init")) == 0) return TSDR_ERR_PLUGIN;
    if ((plugin->tsdrplugin_getsamplerate = tsdrplug_getfunction(plugin, "tsdrplugin_getsamplerate")) == 0) return TSDR_ERR_PLUGIN;
    if ((plugin->tsdrplugin_getName = tsdrplug_getfunction(plugin, "tsdrplugin_getName")) == 0) return TSDR_ERR_PLUGIN;
    if ((plugin->tsdrplugin_setsamplerate = tsdrplug_getfunction(plugin, "tsdrplugin_setsamplerate")) == 0) return TSDR_ERR_PLUGIN;
    if ((plugin->tsdrplugin_setbasefreq = tsdrplug_getfunction(plugin, "tsdrplugin_setbasefreq")) == 0) return TSDR_ERR_PLUGIN;
    if ((plugin->tsdrplugin_stop = tsdrplug_getfunction(plugin, "tsdrplugin_stop")) == 0) return TSDR_ERR_PLUGIN;
    if ((plugin->tsdrplugin_setgain = tsdrplug_getfunction(plugin, "tsdrplugin_setgain")) == 0) return TSDR_ERR_PLUGIN;
    if ((plugin->tsdrplugin_readasync = tsdrplug_getfunction(plugin, "tsdrplugin_readasync")) == 0) return TSDR_ERR_PLUGIN;
    if ((plugin->tsdrplugin_getlasterrortext = tsdrplug_getfunction(plugin, "tsdrplugin_getlasterrortext")) == 0) return TSDR_ERR_PLUGIN;

    // this needs to be always last!
    if ((plugin->tsdrplugin_cleanup = tsdrplug_getfunction(plugin, "tsdrplugin_cleanup")) == 0) return TSDR_ERR_PLUGIN;

    plugin->initialized = 1;
    return TSDR_OK;
}


void tsdrplug_close(pluginsource_t * plugin)
{
	if (!plugin->initialized) return;
	if (plugin->fd == NULL) return;
	if (plugin->tsdrplugin_cleanup != NULL) plugin->tsdrplugin_cleanup(); // cleanup before closing
	plugin->initialized = 0;
#if WINHEAD
    FreeLibrary((HINSTANCE)plugin->fd);
#else
    dlclose(plugin->fd);
#endif

}
