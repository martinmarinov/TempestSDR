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
		double offset;
		float contributionfromlast;
		float max;
		float min;
	} typedef tsdr_context_t;

int tsdr_setsamplerate(tsdr_lib_t * tsdr, uint32_t rate) {
	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);
	tsdr->samplerate = plugin->tsdrplugin_setsamplerate(rate);
	tsdr->sampletime = 1.0f / (float) tsdr->samplerate;
	if (tsdr->sampletime != 0)
		tsdr->pixeltimeoversampletime = tsdr->pixeltime /  tsdr->sampletime;

	return TSDR_OK;
}

int tsdr_getsamplerate(tsdr_lib_t * tsdr) {
	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);
	tsdr->samplerate = plugin->tsdrplugin_getsamplerate();
	tsdr->sampletime = 1.0f / (float) tsdr->samplerate;
	if (tsdr->sampletime != 0)
		tsdr->pixeltimeoversampletime = tsdr->pixeltime /  tsdr->sampletime;

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

// bresenham algorithm
// polyface filterbank
// shielded loop antenna (magnetic)
void videodecodingthread(void * ctx) {
	tsdr_context_t * context = (tsdr_context_t *) ctx;

	const uint32_t samplerate = context->this->samplerate;
	int height = context->this->height;
	int width = context->this->width;

	int bufsize = height * width;
	int sizetopoll = bufsize;
	float * buffer = (float *) malloc(sizeof(float) * bufsize);

	while (context->this->running) {
		if (context->this->height != height || context->this->width != width) {
			height = context->this->height;
			width = context->this->width;
			sizetopoll = height * width;

			if (sizetopoll > bufsize) {
				bufsize = sizetopoll;
				float * buffer = (float *) realloc(buffer, sizeof(float) * bufsize);
			}
		}

		if (cb_rem_blocking(&context->circbuf, buffer, sizetopoll) == CB_OK) {
			context->cb(buffer, width, height, context->ctx);
		}
	}

	free (buffer);

	mutex_signal((mutex_t *) context->this->mutex_video_stopped);
}



void process(float *buf, uint32_t len, void *ctx) {
	tsdr_context_t * context = (tsdr_context_t *) ctx;

	const int size = len/2;

	float * outbuf = context->buffer;
	int outbufsize = context->bufsize;

	const double post = context->this->pixeltimeoversampletime;
	const double post1 = 1.0/post;
	const int pids = (int) ((size - context->offset) / post);

	// resize buffer so it fits
	if (pids > outbufsize) {
				outbufsize = pids+len;
				outbuf = (float *) realloc(outbuf, outbufsize);
	}

	const double offset = context->offset;
	double t = context->offset;
	double contrib = context->contributionfromlast;
	const float prev_max = context->max;
	const float prev_min = context->min;
	const float prev_span = (prev_min == prev_max) ? (1) : (1.0f / (prev_max - prev_min));
	float max = -1, min = 1;

	int pid = 0;
	int i = 0;
	int id;
	int j;

	for (id = 1; id < size; id++) {
		const float I = buf[i++];
		const float Q = buf[i++];

		const float val = sqrtf(I*I+Q*Q);

		// we are in case:
		//    pid
		//    t                                  (in terms of id)
		//  . ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !  pixels (pid)
		// ____|__val__|_______|_______|_______| samples (id)
		//    id     id+1    id+2

		if (t < id) {
			const float pix = (contrib + val*(t+post-id))*post1;
			if (pix > max) max = pix; else if (pix < min) min = pix;
			outbuf[pid++] = (pix - prev_min) * prev_span;
			contrib = 0;
			t=offset+pid*post;
		}

		// we are in case:
		//      pid
		//      t t+post                        (in terms of id)
		//  . ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !  pixels (pid)
		// ____|__val__|_______|_______|_______| samples (id)
		//    id

		while (t+post < id+1) {
			const float pix = val;
			if (pix > max) max = pix; else if (pix < min) min = pix;
			outbuf[pid++] = (pix - prev_min) * prev_span;
			t=offset+pid*post;
		}

		// we are in case:
		//            pid
		//            t                          (in terms of id)
		//  . ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !  pixels (pid)
		// ____|__val__|_______|_______|_______| samples (id)
		//    id     id+1    id+2

		const float contrfract = id+1-t;
		contrib += (contrfract < 0) ? (val) : (contrfract * val);


	// gaussian distrib is
	// exp(-x^2/(2*s^2))
	// s^2 = 0.2 is a nice number
	// if we put -1/(2*s^2) = a
	// the integral has expansion x + (a*x^3)/3 + (a^2*x^5)/10
	// i want this from -1 to 1 to be equal to 1
	// so multiply by = 1 / (2 + 2*a/3 + a^2/5)

	}

	context->bufsize = outbufsize;
	context->buffer = outbuf;
	context->offset = offset+pid*post-size;
	context->contributionfromlast = contrib;
	context->max = max;
	context->min = min;

	if (pid != pids)
		printf("Pid %d; pids %d; t %.4f, size %d, offset %.4f\n", pid, pids, t, size, context->offset);

	cb_add(&context->circbuf, outbuf, pid);
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

	tsdr_getsamplerate(tsdr);

	if (tsdr->pixeltimeoversampletime <= 0) {
		tsdrplug_close((pluginsource_t *)(tsdr->plugin));
		free(tsdr->plugin);
		return TSDR_WRONG_VIDEOPARAMS;
	}

	tsdr->mutex_sync_unload = malloc(sizeof(mutex_t));
	mutex_init((mutex_t *) tsdr->mutex_sync_unload);

	tsdr->mutex_video_stopped = malloc(sizeof(mutex_t));
	mutex_init((mutex_t *) tsdr->mutex_video_stopped);

	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);

	const int width = tsdr->width;
	const int height = tsdr->height;
	const int size = width * height;

	if (width <= 0 || height <= 0 || size > MAX_ARR_SIZE)
		return TSDR_WRONG_VIDEOPARAMS;

	tsdr_context_t * context = (tsdr_context_t *) malloc(sizeof(tsdr_context_t));
	context->this = tsdr;
	context->cb = cb;
	context->bufsize = width * height;
	context->buffer = (float *) malloc(sizeof(float) * context->bufsize);
	context->ctx = ctx;
	context->contributionfromlast = 0;
	context->offset = 0;
	cb_init(&context->circbuf);

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

int tsdr_setresolution(tsdr_lib_t * tsdr, int width, int height, double refreshrate) {
	if (width <= 0 || height <= 0 || width*height > MAX_ARR_SIZE || refreshrate <= 0)
				return TSDR_WRONG_VIDEOPARAMS;

	tsdr->width = width;
	tsdr->height = height;
	tsdr->pixelrate = width * height * refreshrate;
	tsdr->pixeltime = 1.0/tsdr->pixelrate;
	if (tsdr->sampletime != 0)
		tsdr->pixeltimeoversampletime = tsdr->pixeltime /  tsdr->sampletime;

	return TSDR_OK;
}

