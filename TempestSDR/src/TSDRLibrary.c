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
#include "osdetect.h"
#include "threading.h"

#define MAX_ARR_SIZE (4000*4000)

struct tsdr_context {
		tsdr_readasync_function cb;
		float * buffer;
		tsdr_lib_t * this;
		int bufsize;
		void *ctx;
	} typedef tsdr_context_t;

int tsdr_setsamplerate(tsdr_lib_t * tsdr, uint32_t rate) {
	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);
	tsdr->samplerate = plugin->tsdrplugin_setsamplerate(rate);
	return TSDR_OK;
}

int tsdr_setbasefreq(tsdr_lib_t * tsdr, uint32_t freq) {
	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);
	return plugin->tsdrplugin_setbasefreq(freq);
}

int tsdr_stop(tsdr_lib_t * tsdr) {
	if (!tsdr->running) return TSDR_OK;

	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);

	int status = plugin->tsdrplugin_stop();

	mutex_wait((mutex_t *) tsdr->mutex_sync_unload);

	free(tsdr->mutex_sync_unload);
	return status;
}

int tsdr_setgain(tsdr_lib_t * tsdr, float gain) {
	if (!tsdr->running) return TSDR_ERR_PLUGIN;

	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);
	return plugin->tsdrplugin_setgain(gain);
}

void process(float *buf, uint32_t len, void *ctx) {
	tsdr_context_t * context = (tsdr_context_t *) ctx;

	const int length = (len < context->bufsize) ? (len) : (context->bufsize);

	int i;
	for (i = 0; i < length; i++) context->buffer[i] = buf[i];

	context->cb(context->buffer, context->this->width, context->this->height, context->ctx);
}

int tsdr_readasync(tsdr_lib_t * tsdr, const char * pluginfilepath, tsdr_readasync_function cb, void *ctx, const char * params) {
	if (tsdr->running)
		return TSDR_ALREADY_RUNNING;
	tsdr->running = 1;

	tsdr->plugin = malloc(sizeof(pluginsource_t));

	int status = tsdrplug_load((pluginsource_t *)(tsdr->plugin), pluginfilepath);
	if (status != TSDR_OK) {
		tsdrplug_close((pluginsource_t *)(tsdr->plugin));
		free(tsdr->plugin);
		return status;
	}

	tsdr->mutex_sync_unload = malloc(sizeof(mutex_t));
	mutex_init((mutex_t *) tsdr->mutex_sync_unload);
	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);

	const int width = tsdr->width;
	const int height = tsdr->height;
	const int size = width * height;

	if (width < 0 || height < 0 || size > MAX_ARR_SIZE)
		return TSDR_WRONG_WIDTHHEIGHT;

	tsdr_context_t * context = (tsdr_context_t *) malloc(sizeof(tsdr_context_t));
	context->this = tsdr;
	context->cb = cb;
	context->bufsize = width * height;
	context->buffer = (float *) malloc(sizeof(float) * context->bufsize);
	context->ctx = ctx;

	status = plugin->tsdrplugin_readasync(process, (void *) context, params);

	if (status != TSDR_OK) return status;

	free(context->buffer);
	free(context);

	tsdrplug_close((pluginsource_t *)(tsdr->plugin));
	free(tsdr->plugin);

	mutex_signal((mutex_t *) tsdr->mutex_sync_unload);

	tsdr->running = 0;
	return status;
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
