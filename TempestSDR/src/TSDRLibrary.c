/*
 ============================================================================
 Name        : TSDRLibrary.c
 Author      : Martin Marinov
 Description : This is the TempestSDR library. More information will follow.
 ============================================================================
 */

#include "include/TSDRLibrary.h"
#include <stdio.h>
#include <stdlib.h>
#include "TSDRPluginLoader.h"

void test(void) {
	pluginsource_t plugin;

	char name[40] = "Not working :( ";

	if (tsdrplug_load(&plugin, "TSDRPlugin_RawFile.dll")) {
		plugin.tsdrplugin_getName(name);
		printf("%s\n",name);
	} else
		printf("Error!\n");

	tsdrplug_close(&plugin);
}
