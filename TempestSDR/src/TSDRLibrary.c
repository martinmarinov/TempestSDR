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
	tsdr->sampletime = 1.0f / (float) tsdr->samplerate;
	if (tsdr->sampletime != 0)
		tsdr->pixeltimeoversampletime = tsdr->pixeltime /  tsdr->sampletime;

	tsdr->contributionfromlast = 0;
	tsdr->offset = 0;
	return TSDR_OK;
}

int tsdr_getsamplerate(tsdr_lib_t * tsdr) {
	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);
	tsdr->samplerate = plugin->tsdrplugin_getsamplerate();
	tsdr->sampletime = 1.0f / (float) tsdr->samplerate;
	if (tsdr->sampletime != 0)
		tsdr->pixeltimeoversampletime = tsdr->pixeltime /  tsdr->sampletime;

	tsdr->contributionfromlast = 0;
	tsdr->offset = 0;
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

	int i = 0;
	int id;
	const int size = len/2;

	for (id = 0; id < size; id++) {
		const float I = buf[i++];
		const float Q = buf[i++];

		buf[id] = sqrtf(I*I+Q*Q);
	}

	const double post = context->this->pixeltimeoversampletime;

	float * outbuf = context->buffer;
	int outbufsize = context->bufsize;
	double t = context->this->offset;
	double contrib = context->this->contributionfromlast;

	int maxinited = 0 ;
	float max = 1;
	int mininited = 0;
	float min = 0;

	int pid;
	for (pid = 0; ;pid++) {

		const double startbuffid = t;
		const int startbuffidint = (int) startbuffid;
		const double endbuffid = t + post;
		const int endbuffidint = (int) endbuffid;

		if (pid > outbufsize) {
			outbufsize = pid + len;
			outbuf = (float *) realloc(outbuf, outbufsize);
		}

		if (startbuffid < 0) {

			if (endbuffid == 0)
				outbuf[pid] = post * (contrib + buf[endbuffidint] * (endbuffid-endbuffidint));
			else {
				float sum = 0;
				for (id = startbuffid+1; id < endbuffid; id++) sum += buf[id];
				outbuf[pid] = post * (contrib + sum + buf[endbuffidint] * (endbuffid-endbuffidint));
			}

		} else if (endbuffid > size) {

			if (startbuffid == size -1)
				contrib = buf[startbuffidint] * (size - startbuffid);
			else {
				float sum = 0;
				for (id = startbuffidint + 1; id < size; id++) sum += buf[id];
				contrib = sum + buf[startbuffidint] * (size - startbuffid);
			}

			break;
		} else if (startbuffid == endbuffid)
			outbuf[pid] = buf[startbuffidint];
		else if (endbuffid - startbuffid == 1)
			outbuf[pid] = post * (buf[startbuffidint] * (1.0f - startbuffid + startbuffidint) + buf[endbuffidint] * (endbuffid-endbuffidint));
		else {
			float sum = 0;
			for (id = startbuffid+1; id < endbuffid; id++) sum += buf[id];
			outbuf[pid] = post * (buf[startbuffidint] * (1.0f - startbuffid + startbuffidint) + sum + buf[endbuffidint] * (endbuffid-endbuffidint));
		}

		const float val = outbuf[pid];
		if (!maxinited || val > max) {
			max = val;
			maxinited = 1;
		} else if (!mininited || val < min) {
			min = val;
			mininited = 1;
		}

		t=endbuffid; // count time
	}

	if (max != min) {
		const float span = max - min;
		// auto balance
		for (id = 0; id < pid; id++)
			outbuf[id] = (outbuf[id]-min) / span;
	}

	context->bufsize = outbufsize;
	context->buffer = outbuf;
	context->this->offset = size - t - context->this->offset;
	context->this->contributionfromlast = contrib;

	//printf("Pids %d, t %.4f, size %d, post %.4f, offset %.4f\n", pid, t, size, post, context->this->offset);

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

	tsdr->contributionfromlast = 0;
	tsdr->offset = 0;

	return TSDR_OK;
}

