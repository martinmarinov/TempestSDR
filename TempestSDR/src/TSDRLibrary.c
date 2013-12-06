/*
 ============================================================================
 Name        : TSDRLibrary.c
 Author      : Martin Marinov
 Description : This is the TempestSDR library. More information will follow.
 ============================================================================
 */

#include "include/TSDRLibrary.h"
#include "include/TSDRCodes.h"
#include <stdio.h>
#include <stdlib.h>
#include "TSDRPluginLoader.h"

int tsdr_loadplugin(tsdr_lib_t * tsdr, char * filepath) {
	return tsdrplug_load((pluginsource_t *)(&tsdr->plugin), filepath);
}


int tsdr_pluginparams(tsdr_lib_t * tsdr, char * params) {
	return TSDR_NOT_IMPLEMENTED;
}

int tsdr_setsamplerate(tsdr_lib_t * tsdr, uint32_t rate) {
	return TSDR_NOT_IMPLEMENTED;
}

int tsdr_setbasefreq(tsdr_lib_t * tsdr, uint32_t freq) {
	return TSDR_NOT_IMPLEMENTED;
}

int tsdr_start(tsdr_lib_t * tsdr) {
	return TSDR_NOT_IMPLEMENTED;
}

int tsdr_unloadplugin(tsdr_lib_t * tsdr) {
	tsdrplug_close((pluginsource_t *)(&tsdr->plugin));
	return TSDR_OK;
}

int tsdr_stop(tsdr_lib_t * tsdr) {
	return TSDR_NOT_IMPLEMENTED;
}

int tsdr_setgain(tsdr_lib_t * tsdr, float gain) {
	return TSDR_NOT_IMPLEMENTED;
}

int tsdr_readasync(tsdr_lib_t * tsdr, tsdr_readasync_function cb, void *ctx, uint32_t buf_num, uint32_t buf_len) {
	return TSDR_NOT_IMPLEMENTED;
}
