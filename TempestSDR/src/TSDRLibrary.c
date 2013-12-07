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

#define MAX_ARR_SIZE (4000*4000)

int tsdr_loadplugin(tsdr_lib_t * tsdr, char * filepath) {
	tsdr->plugin = malloc(sizeof(pluginsource_t));
	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);
	return tsdrplug_load(plugin, filepath);
}

int tsdr_pluginparams(tsdr_lib_t * tsdr, char * params) {
	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);
	return plugin->tsdrplugin_init(params);
}

int tsdr_setsamplerate(tsdr_lib_t * tsdr, uint32_t rate) {
	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);
	tsdr->samplerate = plugin->tsdrplugin_setsamplerate(rate);
	return TSDR_OK;
}

int tsdr_setbasefreq(tsdr_lib_t * tsdr, uint32_t freq) {
	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);
	return plugin->tsdrplugin_setbasefreq(freq);
}

int tsdr_unloadplugin(tsdr_lib_t * tsdr) {
	tsdrplug_close((pluginsource_t *)(tsdr->plugin));
	free(tsdr->plugin);
	return TSDR_OK;
}

int tsdr_stop(tsdr_lib_t * tsdr) {
	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);
	return plugin->tsdrplugin_stop();
}

int tsdr_setgain(tsdr_lib_t * tsdr, float gain) {
	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);
	return plugin->tsdrplugin_setgain(gain);
}

int tsdr_readasync(tsdr_lib_t * tsdr, tsdr_readasync_function cb, void *ctx) {
	const int width = tsdr->width;
	const int height = tsdr->height;
	const int size = width * height;

	if (width < 0 || height < 0 || size > MAX_ARR_SIZE)
		return TSDR_WRONG_WIDTHHEIGHT;

	float * buffer = (float *) malloc(sizeof(float) * size);

	int i;
	for (i = 0; i < size; i++) {
		const int x = i % width;
		const int y = i / width;

		const float rat = (x > width/2) ? (y / (float) height) : (1.0f - y / (float) height);
		buffer[i] = rat;
	}

	cb(buffer, width, height, ctx);

	free(buffer);
	return TSDR_OK;
}

int tsdr_setresolution(tsdr_lib_t * tsdr, int width, int height) {
	tsdr->width = width;
	tsdr->height = height;

	if (width < 0 || height < 0 || width*height > MAX_ARR_SIZE)
			return TSDR_WRONG_WIDTHHEIGHT;

	return TSDR_OK;
}

int tsdr_setvfreq(tsdr_lib_t * tsdr, float freq) {
	tsdr->vf = freq;
	return TSDR_OK;
}

int tsdr_sethfreq(tsdr_lib_t * tsdr, float freq) {
	tsdr->hf = freq;
	return TSDR_OK;

}
