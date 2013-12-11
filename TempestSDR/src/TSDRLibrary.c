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
#include <math.h>
#include "circbuff.h"

#define MAX_ARR_SIZE (4000*4000)

struct tsdr_context {
		tsdr_readasync_function cb;
		float * buffer;
		tsdr_lib_t * this;
		int bufsize;
		void *ctx;
		CircBuff_t circbuf;
	} typedef tsdr_context_t;

int tsdr_setsamplerate(tsdr_lib_t * tsdr, uint32_t rate) {
	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);
	tsdr->samplerate = plugin->tsdrplugin_setsamplerate(rate);
	return TSDR_OK;
}

int tsdr_getsamplerate(tsdr_lib_t * tsdr) {
	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);
	tsdr->samplerate = plugin->tsdrplugin_getsamplerate();
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

	mutex_free((mutex_t *) tsdr->mutex_sync_unload);
	free(tsdr->mutex_sync_unload);
	return status;
}

int tsdr_setgain(tsdr_lib_t * tsdr, float gain) {
	if (!tsdr->running) return TSDR_ERR_PLUGIN;

	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);
	return plugin->tsdrplugin_setgain(gain);
}

void videodecodingthread(void * ctx) {
	tsdr_context_t * context = (tsdr_context_t *) ctx;

	const uint32_t samplerate = context->this->samplerate;
	const int height = context->this->height;
	const int width = context->this->width;
	const float fh = context->this->fh;
	const float fv = context->this->fv;

	while (context->this->running) {
		if (cb_rem_blocking(&context->circbuf, context->buffer, context->bufsize) == CB_OK) {
			context->cb(context->buffer, context->this->width, context->this->height, context->ctx);
		}
	}

	mutex_signal((mutex_t *) context->this->mutex_video_stopped);
}

void process(float *buf, uint32_t len, void *ctx) {
	tsdr_context_t * context = (tsdr_context_t *) ctx;

	int i = 0;
	int id;
	const int size = len/2;
	for (id = 0; id < size; id++) {
		const float I = buf[i++];
		const float Q = buf[i++];

		buf[id] = sqrtf(I*I+Q*Q);
	}

	cb_add(&context->circbuf, buf, size);
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

	tsdr->mutex_video_stopped = malloc(sizeof(mutex_t));
	mutex_init((mutex_t *) tsdr->mutex_video_stopped);

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
	cb_init(&context->circbuf);

	tsdr_getsamplerate(tsdr);

	thread_start(videodecodingthread, (void *) context);
	status = plugin->tsdrplugin_readasync(process, (void *) context, params);
	tsdr->running = 0;

	mutex_wait((mutex_t *) tsdr->mutex_video_stopped);

	mutex_free((mutex_t *) tsdr->mutex_video_stopped);
	free(tsdr->mutex_video_stopped);

	if (status != TSDR_OK) return status;

	free(context->buffer);
	free(context);

	cb_free(&context->circbuf);
	tsdrplug_close((pluginsource_t *)(tsdr->plugin));
	free(tsdr->plugin);

	mutex_signal((mutex_t *) tsdr->mutex_sync_unload);

	return status;
}

int tsdr_setresolution(tsdr_lib_t * tsdr, int width, int height, float refreshrate) {
	if (width < 0 || height < 0 || width*height > MAX_ARR_SIZE || refreshrate <= 0)
				return TSDR_WRONG_WIDTHHEIGHT;

	tsdr->width = width;
	tsdr->height = height;
	tsdr->refreshrate = refreshrate;
	tsdr->fh = refreshrate / (float) width;
	tsdr->fv = refreshrate / (float) height;

	return TSDR_OK;
}

