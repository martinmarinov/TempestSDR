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

	if (tsdrplug_load(&plugin, "abc.dll"))
		printf(plugin.tsdrplugin_getName());
	else
		printf("Error!");

	tsdrplug_close(&plugin);
}
