#include "stdafx.h"

#include "ExtIOPluginLoader.h"
#include "TSDRCodes.h"

#include <stdlib.h>
#include <stdio.h>

// A platform independent dynamic library loader

extern "C" void *extio_getfunction(extiosource_t * plugin, char *functname)
{
    return (void*)GetProcAddress(plugin->fd,functname);
}

extern "C" void extio_close(extiosource_t * plugin)
{
	if (plugin->fd == NULL) return;
    FreeLibrary(plugin->fd);
}

extern "C" int extio_load(extiosource_t * plugin, const char *dlname)
{
	plugin->fd = LoadLibraryA(dlname);

	printf("Errrr %d for %s\n", GetLastError(), dlname); fflush(stdout);

    if (plugin->fd == NULL)
    	return TSDR_INCOMPATIBLE_PLUGIN;

	printf("Kvo be 1\n"); fflush(stdout);

    // mandatory
	if ((plugin->InitHW = (bool(__stdcall *) (char *, char *, int *)) extio_getfunction(plugin, "InitHW")) == 0) return TSDR_ERR_PLUGIN;
	printf("Kvo be 2\n"); fflush(stdout);
	if ((plugin->OpenHW = (bool(__stdcall *) (void)) extio_getfunction(plugin, "OpenHW")) == 0) return TSDR_ERR_PLUGIN;
	if ((plugin->CloseHW = (void(__stdcall *) (void)) extio_getfunction(plugin, "CloseHW")) == 0) return TSDR_ERR_PLUGIN;
	if ((plugin->StartHW = (int(__stdcall *) (long)) extio_getfunction(plugin, "StartHW")) == 0) return TSDR_ERR_PLUGIN;
	if ((plugin->StopHW = (void(__stdcall *) (void)) extio_getfunction(plugin, "StopHW")) == 0) return TSDR_ERR_PLUGIN;
	if ((plugin->SetCallback = (void(__stdcall *) (pfnExtIOCallback)) extio_getfunction(plugin, "SetCallback")) == 0) return TSDR_ERR_PLUGIN;
	if ((plugin->SetHWLO = (int(__stdcall *) (long)) extio_getfunction(plugin, "SetHWLO")) == 0) return TSDR_ERR_PLUGIN;
	if ((plugin->GetStatus = (int(__stdcall *) (void)) extio_getfunction(plugin, "GetStatus")) == 0) return TSDR_ERR_PLUGIN;

    // mandatory functions that rtlsdr expects
	if ((plugin->GetHWSR = (long(__stdcall *) (void)) extio_getfunction(plugin, "GetHWSR")) == 0) return TSDR_ERR_PLUGIN;

    // completely optional functions
	if ((plugin->SetAttenuator = (int(__stdcall *) (int)) extio_getfunction(plugin, "SetAttenuator")) == 0) plugin->SetAttenuator = NULL;
	if ((plugin->GetAttenuators = (int(__stdcall *) (int, float *)) extio_getfunction(plugin, "GetAttenuators")) == 0) plugin->GetAttenuators = NULL;
	if ((plugin->ShowGUI = (int(__stdcall *) (void)) extio_getfunction(plugin, "ShowGUI")) == 0) plugin->ShowGUI = NULL;
	if ((plugin->HideGUI = (int(__stdcall *) (void)) extio_getfunction(plugin, "HideGUI")) == 0) plugin->HideGUI = NULL;
	printf("Kvo be 3\n"); fflush(stdout);

    return TSDR_OK;
}
