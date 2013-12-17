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
#define MAX_SAMP_RATE (500e6)

struct tsdr_context {
		tsdr_readasync_function cb;
		float * buffer;
		tsdr_lib_t * this;
		int bufsize;
		void *ctx;
		CircBuff_t circbuf;
		double offset;
		float contributionfromlast;
	} typedef tsdr_context_t;

int tsdr_setsamplerate(tsdr_lib_t * tsdr, uint32_t rate) {
	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);
	tsdr->samplerate = plugin->tsdrplugin_setsamplerate(rate);
	if (tsdr->samplerate == 0 || tsdr->samplerate > 500e6) return TSDR_SAMPLE_RATE_WRONG;

	tsdr->sampletime = 1.0f / (float) tsdr->samplerate;
	if (tsdr->sampletime != 0)
		tsdr->pixeltimeoversampletime = tsdr->pixeltime /  tsdr->sampletime;

	return TSDR_OK;
}

int tsdr_getsamplerate(tsdr_lib_t * tsdr) {
	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);
	tsdr->samplerate = plugin->tsdrplugin_getsamplerate();
	if (tsdr->samplerate == 0 || tsdr->samplerate > 500e6) return TSDR_SAMPLE_RATE_WRONG;

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



	//if (pix > max) max = pix; else if (pix < min) min = pix;
	//outbuf[pid++] = (pix - prev_min) * prev_span;

	float pmax, pmin;

	while (context->this->running) {
		float max = -9999999999999;
		float min = 9999999999999;
		const float span = (pmax == pmin) ? (0) : (pmax - pmin);

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
			int i;
			for (i = 0; i < sizetopoll; i++) {
				const float val = buffer[i];
				if (val > max) max = val; else if (val < min) min = val;
				buffer[i] = (val - pmin) / span;
			}

			context->cb(buffer, width, height, context->ctx);

			pmax = max;
			pmin = min;
		}
	}

	free (buffer);

	mutex_signal((mutex_t *) context->this->mutex_video_stopped);
}

// This is the pixel function. Start must be >=0 and end must be <=1
// Return the area of the pixel signal spanned by this if the pixel was from 0 to 1
float integrate(float start, float end) {
	if (start < 0 || end > 1 || end < start) printf("%.4f to %.4f\n", start, end);
	return end - start;
}

void process(float *buf, uint32_t len, void *ctx) {
	tsdr_context_t * context = (tsdr_context_t *) ctx;

	const int size = len/2;

	float * outbuf = context->buffer;
	int outbufsize = context->bufsize;

	const double post = context->this->pixeltimeoversampletime;
	const int pids = (int) ((size - context->offset) / post);
	const float normalize = integrate(0, 1);

	// resize buffer so it fits
	if (pids > outbufsize) {
				outbufsize = pids+len;
				outbuf = (float *) realloc(outbuf, outbufsize);
	}

	const double offset = context->offset;
	double t = context->offset;
	float contrib = context->contributionfromlast;

	int pid = 0;
	int i = 0;
	int id;
	int j;

	for (id = 0; id < size; id++) {
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
			const float start = (id-t)/post;
			outbuf[pid++] = contrib + val*integrate(start, 1)/normalize;
			contrib = 0;
			t=offset+pid*post;
		}

		// we are in case:
		//      pid
		//      t t+post                        (in terms of id)
		//  . ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !  pixels (pid)
		// ____|__val__|_______|_______|_______| samples (id)
		//    id

		// TODO the condition depends on pid, not on t!
		while (t+post < id+1) {
			outbuf[pid++] = val;
			t=offset+pid*post;
		}

		// we are in case:
		//            pid
		//            t                          (in terms of id)
		//  . ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !  pixels (pid)
		// ____|__val__|_______|_______|_______| samples (id)
		//    id     id+1    id+2

		// TODO! WHAT ABOUT ENLARGING
		const float contrfract = integrate(0,(id+1-t)/post)/normalize;
		contrib += contrfract * val;

	}

	context->bufsize = outbufsize;
	context->buffer = outbuf;
	context->offset = t-size;
	context->contributionfromlast = contrib;

//	if (pid != pids || context->offset > 0 || context->offset < -post)
//		printf("Pid %d; pids %d; t %.4f, size %d, offset %.4f\n", pid, pids, t, size, context->offset);

	cb_add(&context->circbuf, outbuf, pid);
}

int tsdr_readasync(tsdr_lib_t * tsdr, const char * pluginfilepath, tsdr_readasync_function cb, void *ctx, const char * params) {
	if (tsdr->running)
		return TSDR_ALREADY_RUNNING;

	tsdr->running = 1;

	const int width = tsdr->width;
	const int height = tsdr->height;
	const int size = width * height;

	if (width <= 0 || height <= 0 || size > MAX_ARR_SIZE)
		return TSDR_WRONG_VIDEOPARAMS;

	tsdr->plugin = malloc(sizeof(pluginsource_t));

	int status = tsdrplug_load((pluginsource_t *)(tsdr->plugin), pluginfilepath);
	if (status != TSDR_OK) goto end;

	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);

	if ((status = plugin->tsdrplugin_setParams(params)) != TSDR_OK) goto end;
	if ((status = tsdr_getsamplerate(tsdr)) != TSDR_OK) goto end;

	if (tsdr->pixeltimeoversampletime <= 0) goto end;

	tsdr->mutex_sync_unload = malloc(sizeof(mutex_t));
	mutex_init((mutex_t *) tsdr->mutex_sync_unload);

	tsdr->mutex_video_stopped = malloc(sizeof(mutex_t));
	mutex_init((mutex_t *) tsdr->mutex_video_stopped);

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
	status = plugin->tsdrplugin_readasync(process, (void *) context);
	tsdr->running = 0;

	mutex_wait((mutex_t *) tsdr->mutex_video_stopped);

	mutex_free((mutex_t *) tsdr->mutex_video_stopped);
	free(tsdr->mutex_video_stopped);

	free(context->buffer);
	free(context);

	cb_free(&context->circbuf);

	mutex_signal((mutex_t *) tsdr->mutex_sync_unload);

end:
	tsdrplug_close((pluginsource_t *)(tsdr->plugin));
	free(tsdr->plugin);
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

