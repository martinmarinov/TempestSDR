/*
#-------------------------------------------------------------------------------
# Copyright (c) 2014 Martin Marinov.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the GNU Public License v3.0
# which accompanies this distribution, and is available at
# http://www.gnu.org/licenses/gpl.html
# 
# Contributors:
#     Martin Marinov - initial API and implementation
#-------------------------------------------------------------------------------
*/

#include "include/TSDRLibrary.h"
#include "include/TSDRCodes.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "TSDRPluginLoader.h"
#include "osdetect.h"
#include "threading.h"
#include <math.h>
#include "circbuff.h"
#include "syncdetector.h"
#include "internaldefinitions.h"
#include "fft.h"

#define MAX_ARR_SIZE (4000*4000)
#define MAX_SAMP_RATE (500e6)

#define INTEG_TYPE (0)
#define PI (3.141592653589793238462643383279502f)


#define NORMALISATION_LOWPASS_COEFF (0.1f)

#define DEFAULT_DECIMATOR_TO_POLL (64)

struct tsdr_context {
		tsdr_readasync_function cb;

		tsdr_lib_t * this;

		void *ctx;
		CircBuff_t circbuf_decimation_to_video;
		CircBuff_t circbuf_device_to_decimation;
		size_t decimator_items_to_poll;

		size_t device_items_dropped;
		size_t device_items_to_drop;

	} typedef tsdr_context_t;


#define RETURN_EXCEPTION(tsdr, message, status) {announceexception(tsdr, message, status); return status;}
#define RETURN_OK(tsdr) {tsdr->errormsg_code = TSDR_OK; return TSDR_OK;}
#define RETURN_PLUGIN_RESULT(tsdr,plugin,result) {if ((result) == TSDR_OK) RETURN_OK(tsdr) else RETURN_EXCEPTION(tsdr,plugin.tsdrplugin_getlasterrortext(),result);}

void dofft(tsdr_lib_t * tsdr, float * srcbuff, int srcbuffsize) {
	// do fft
	if (tsdr->fft_requested) {
		if (tsdr->fft_size < srcbuffsize) {
			tsdr->fft_exception = 0;
			memcpy(tsdr->fft_buffer,srcbuff,sizeof(float)*tsdr->fft_size);
		} else
			tsdr->fft_exception = 1;
		mutex_signal(&tsdr->fft_mutex);
	}
}

static inline void announceexception(tsdr_lib_t * tsdr, const char * message, int status) {
	tsdr->errormsg_code = status;
	if (status == TSDR_OK) return;

	if (message == NULL)
		message = "An exception with no detailed explanation cause has occurred. This could as well be a bug in the TSDRlibrary or in one of its plugins.";

	const int length = strlen(message);
	if (tsdr->errormsg_size == 0) {
		tsdr->errormsg_size = length;
		tsdr->errormsg = (char *) malloc(length+1);
	} else if (length > tsdr->errormsg_size) {
		tsdr->errormsg_size = length;
		tsdr->errormsg = (char *) realloc((void*) tsdr->errormsg, length+1);
	}
	strcpy(tsdr->errormsg, message);
}

 char * tsdr_getlasterrortext(tsdr_lib_t * tsdr) {
	if (tsdr->errormsg_code == TSDR_OK)
		return NULL;
	else
		return tsdr->errormsg;
}

 void announce_callback_changed(tsdr_lib_t * tsdr, int value_id, double arg0, int arg1) {
	 if (tsdr->callback != NULL)
		 tsdr->callback(value_id, arg0, arg1, tsdr->callbackctx);
 }

 void * tsdr_getctx(tsdr_lib_t * tsdr) {
	 return tsdr->callbackctx;
 }

 void tsdr_init(tsdr_lib_t ** tsdr, tsdr_value_changed_callback callback, void * ctx) {
	 int i;

	*tsdr = (tsdr_lib_t *) malloc(sizeof(tsdr_lib_t));

	(*tsdr)->nativerunning = 0;
	(*tsdr)->running = 0;
	(*tsdr)->plugin.initialized = 0;
	(*tsdr)->plugin.tsdrplugin_cleanup = NULL;
	(*tsdr)->centfreq = 0;
	(*tsdr)->syncoffset = 0;
	(*tsdr)->errormsg_size = 0;
	(*tsdr)->errormsg_code = TSDR_OK;
	(*tsdr)->fft_requested = 0;
	(*tsdr)->callback = callback;
	(*tsdr)->callbackctx = ctx;

	for (i = 0; i < COUNT_PARAM_INT; i++)
		(*tsdr)->params_int[i] = 0;
	for (i = 0; i < COUNT_PARAM_DOUBLE; i++)
		(*tsdr)->params_double[i] = 0.0;

	semaphore_init(&(*tsdr)->threadsync);
	mutex_init(&(*tsdr)->stopsync);
	mutex_init(&(*tsdr)->fft_mutex);

	frameratedetector_init(&(*tsdr)->frameratedetect, *tsdr);

}

 int tsdr_isrunning(tsdr_lib_t * tsdr) {
	return tsdr->nativerunning;
}

 int tsdr_getsamplerate(tsdr_lib_t * tsdr) {
	if (!tsdr->plugin.initialized) RETURN_EXCEPTION(tsdr, "Cannot change sample rate. Plugin not loaded yet.", TSDR_ERR_PLUGIN);

	tsdr->samplerate = tsdr->plugin.tsdrplugin_getsamplerate();
	if (tsdr->samplerate == 0 || tsdr->samplerate > 500e6) RETURN_EXCEPTION(tsdr, "Invalid/unsupported value for sample rate.", TSDR_SAMPLE_RATE_WRONG);

	tsdr->sampletime = 1.0 / (double) tsdr->samplerate;
	if (tsdr->sampletime != 0)
		tsdr->pixeltimeoversampletime = tsdr->pixeltime /  tsdr->sampletime;

	RETURN_OK(tsdr);
}

 int tsdr_setbasefreq(tsdr_lib_t * tsdr, uint32_t freq) {
	tsdr->centfreq = freq;

	if (tsdr->plugin.initialized)
		RETURN_PLUGIN_RESULT(tsdr, tsdr->plugin, tsdr->plugin.tsdrplugin_setbasefreq(tsdr->centfreq))
	else
		RETURN_OK(tsdr);
}


 int tsdr_stop(tsdr_lib_t * tsdr) {
	if (!tsdr->running) RETURN_OK(tsdr);
	int status = tsdr->plugin.tsdrplugin_stop();


	semaphore_wait(&tsdr->threadsync);
	mutex_signal(&tsdr->stopsync);

	RETURN_PLUGIN_RESULT(tsdr, tsdr->plugin, status);
}

 int tsdr_setgain(tsdr_lib_t * tsdr, float gain) {


	tsdr->gain = gain;

	if (tsdr->plugin.initialized)
		RETURN_PLUGIN_RESULT(tsdr, tsdr->plugin, tsdr->plugin.tsdrplugin_setgain(gain))
	else
		RETURN_OK(tsdr);
}

// bresenham algorithm
// polyface filterbank
// shielded loop antenna (magnetic)
void videodecodingthread(void * ctx) {

	int i;

	tsdr_context_t * context = (tsdr_context_t *) ctx;
	semaphore_enter(&context->this->threadsync);

	int height = context->this->height;
	int width = context->this->width;

	int bufsize = height * width;
	int sizetopoll = height * width;
	float * buffer = (float *) malloc(sizeof(float) * bufsize);
	float * screenbuffer = (float *) malloc(sizeof(float) * bufsize);
	float * sendbuffer = (float *) malloc(sizeof(float) * bufsize);
	float * corrected_sendbuffer = (float *) malloc(sizeof(float) * bufsize);

	float * widthcollapsebuffer =  (float *) malloc(sizeof(float) * width);
	float * heightcollapsebuffer =  (float *) malloc(sizeof(float) * height);

	for (i = 0; i < bufsize; i++) screenbuffer[i] = 0.0f;
	float lastmax = 0;
	float lastmin = 0;
	const float oneminusnorm = 1.0f - NORMALISATION_LOWPASS_COEFF;

	while (context->this->running) {

		const double lowpassvalue = context->this->motionblur;
		const double antilowpassvalue = 1.0 - lowpassvalue;

		if (context->this->height != height || context->this->width != width) {
			const int oldheight = height;
			const int oldwidth = width;

			height = context->this->height;
			width = context->this->width;
			sizetopoll = height * width;
			assert(sizetopoll > 0);

			if (sizetopoll > bufsize) {
				bufsize = sizetopoll;
				buffer = (float *) realloc(buffer, sizeof(float) * bufsize);
				screenbuffer = (float *) realloc(screenbuffer, sizeof(float) * bufsize);
				sendbuffer = (float *) realloc(sendbuffer, sizeof(float) * bufsize);
				corrected_sendbuffer = (float *) realloc(corrected_sendbuffer, sizeof(float) * bufsize);
				for (i = 0; i < bufsize; i++) screenbuffer[i] = 0.0f;
			}

			if (width != oldwidth) widthcollapsebuffer = (float *) realloc(widthcollapsebuffer, sizeof(float) * width);
			if (height != oldheight) heightcollapsebuffer = (float *) realloc(heightcollapsebuffer, sizeof(float) * height);

		}

		if (cb_rem_blocking(&context->circbuf_decimation_to_video, buffer, sizetopoll) == CB_OK) {

			for (i = 0; i < width; i++) widthcollapsebuffer[i] = 0.0f;
			for (i = 0; i < height; i++) heightcollapsebuffer[i] = 0.0f;

			float max = buffer[0];
			float min = max;
			for (i = 0; i < sizetopoll; i++) {
				const float val = screenbuffer[i] * lowpassvalue + buffer[i] * antilowpassvalue;
				if (val > max) max = val; else if (val < min) min = val;
				screenbuffer[i] = val;
			}

			lastmax = oneminusnorm*lastmax + NORMALISATION_LOWPASS_COEFF*max;
			lastmin = oneminusnorm*lastmin + NORMALISATION_LOWPASS_COEFF*min;
			const float span = (lastmax == lastmin) ? (1.0f) : (lastmax - lastmin);

			for (i = 0; i < sizetopoll; i++) {
				const float val = (screenbuffer[i] - lastmin) / span;
				sendbuffer[i] = val;
				widthcollapsebuffer[i % width] += val;
				heightcollapsebuffer[i / width] += val;
			}


			float * buf_to_send = syncdetector_run(context->this, sendbuffer, corrected_sendbuffer, width, height, widthcollapsebuffer, heightcollapsebuffer);

			assert(bufsize >= width * height);
			context->cb(buf_to_send, width, height, context->ctx);
		}
	}

	free (buffer);
	free (sendbuffer);
	free (screenbuffer);
	free (corrected_sendbuffer);
	free (widthcollapsebuffer);
	free (heightcollapsebuffer);

	semaphore_leave(&context->this->threadsync);
}

static inline void am_demod(float * buffer, int size) {
	int id;
	float * bref = buffer;
	float * brefout = buffer;
	for (id = 0; id < size; id++) {
		const float I = *(bref++);
		const float Q = *(bref++);

		*(brefout++) = sqrtf(I*I+Q*Q);
	}
}

void process(float *buf, uint32_t items_count, void *ctx, int samples_dropped) {
	tsdr_context_t * context = (tsdr_context_t *) ctx;

	context->decimator_items_to_poll = items_count;

	if (samples_dropped > 0)
		context->device_items_dropped += (samples_dropped << 1);

	if (context->device_items_dropped > 0) {

		const unsigned int size2 = round(((context->this->width * context->this->height) << 1) * context->this->pixeltimeoversampletime);
		const unsigned int moddropped = context->device_items_dropped % size2;
		context->device_items_to_drop += size2-moddropped; // how much to drop so that it ends up on one frame
		context->device_items_dropped = 0;
	}

	const size_t device_items_to_drop = context->device_items_to_drop;
	if (device_items_to_drop >= items_count)
		context->device_items_to_drop -= items_count;
	else if (cb_add(&context->circbuf_device_to_decimation, &buf[device_items_to_drop], items_count-device_items_to_drop) == CB_OK)
		context->device_items_to_drop = 0;
	else// we lost samples due to buffer overflow
		context->device_items_dropped += items_count;
}

void decimatingthread(void * ctx) {
	tsdr_context_t * context = (tsdr_context_t *) ctx;
	semaphore_enter(&context->this->threadsync);

	int bufsize = context->decimator_items_to_poll;
	float * buffer = (float *) malloc(sizeof(float) * bufsize);
	int dropped_samples = 0;
	unsigned int todrop = 0;

	int outbufsize = context->this->width * context->this->height;
	float * outbuf = (float *) malloc(sizeof(float) * outbufsize);

	float contrib = 0;
	double offset = 0;

	while (context->this->running) {

		const int len = context->decimator_items_to_poll;
		if (len > bufsize) {
			bufsize = len;
			buffer = (float *) realloc((void*) buffer, sizeof(float) * bufsize);
		}
		if (cb_rem_blocking(&context->circbuf_device_to_decimation, buffer, len) == CB_OK) {

			const int size = len/2;

			const double post = context->this->pixeltimeoversampletime;
			const int pids = (int) ((size - offset) / post);

			// resize buffer so it fits
			if (pids > outbufsize) {
				outbufsize = pids;
				outbuf = (float *) realloc(outbuf, sizeof(float) * outbufsize);
			}

			double t = offset;

			int pid = 0;
			int id;

			//fm_demod(buffer, size);
			am_demod(buffer, size);

			// we have the AM demodulated signal in buff
			//setframerate(context->this, frameratedetector_run(&context->this->frameratedetect, context->this, buffer, size, context->this->samplerate / context->this->pixeltimeoversampletime));
			frameratedetector_run(&context->this->frameratedetect, buffer, size, context->this->samplerate);

			float * bref = buffer;
			for (id = 0; id < size; id++) {

				const float val = *(bref++);

				// we are in case:
				//    pid
				//    t                                  (in terms of id)
				//  . ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !  pixels (pid)
				// ____|__val__|_______|_______|_______| samples (id)
				//    id     id+1    id+2

				if (t < id && (t + post) < (id+1)) {
					const float start = (id-t)/post;
#if INTEG_TYPE == -1
					outbuf[pid++] = val;
#else
					const float contrfract = 1.0 - start;
					outbuf[pid++] = contrib + val*contrfract;
#endif
					contrib = 0;
					t=offset+pid*post;
				}

				// we are in case:
				//      pid
				//      t t+post                        (in terms of id)
				//  . ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !  pixels (pid)
				// ____|__val__|_______|_______|_______| samples (id)
				//    id

				while ((t+post) < (id+1)) {
					// this only ever triggers if post < 1
					outbuf[pid++] = val;
					t=offset+pid*post;
				}

				// we are in case:
				//            pid
				//            t                          (in terms of id)
				//  . ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !  pixels (pid)
				// ____|__val__|_______|_______|_______| samples (id)
				//    id     id+1    id+2

				if (t < (id + 1) && t > id) {
					const float contrfract = (id+1-t)/post;
					contrib += contrfract * val;
				} else {
					const float idt = id - t;
					const float contrfract = (idt+1-idt)/post;
					contrib += contrfract * val;
				}
			}

			offset = t-size;

			//assert (pid == pids);
			//		printf("Pid %d; pids %d; t %.4f, size %d, offset %.4f\n", pid, pids, t, size, context->offset);

			// section for syncing lost samples
			if (dropped_samples > 0) {
				const unsigned int size = context->this->width * context->this->height;
				const unsigned int moddropped = dropped_samples % size;
				todrop += size - moddropped; // how much to drop so that it ends up on one frame
				dropped_samples = 0;
			}

			dofft(context->this,outbuf,pid);

			if (todrop >= pid)
				todrop -= pid;
			else if (cb_add(&context->circbuf_decimation_to_video, &outbuf[todrop], pid-todrop) == CB_OK)
				todrop = 0;
			else // we lost samples due to buffer overflow
				dropped_samples += pid;


			// section for manual syncing
			const int syncoffset = -context->this->syncoffset;
			if (syncoffset > 0)
				dropped_samples += syncoffset;
			else if (syncoffset < 0)
				dropped_samples += context->this->width * context->this->height + syncoffset;
			context->this->syncoffset = 0;
		}
	}

	if (buffer != NULL) free(buffer);
	buffer = NULL;
	free(outbuf);

	semaphore_leave(&context->this->threadsync);
}

void unloadplugin(tsdr_lib_t * tsdr) {
	if (tsdr->plugin.initialized)
		tsdrplug_close(&tsdr->plugin);
}

int tsdr_unloadplugin(tsdr_lib_t * tsdr) {
	if (!tsdr->plugin.initialized) RETURN_EXCEPTION(tsdr, "No plugin has been loaded so it can't be unloaded", TSDR_ERR_PLUGIN);

	if (tsdr->nativerunning || tsdr->running)
			RETURN_EXCEPTION(tsdr, "The library is already running in async mode. Stop it first!", TSDR_ALREADY_RUNNING);

	unloadplugin(tsdr);
	RETURN_OK(tsdr);
}

int tsdr_loadplugin(tsdr_lib_t * tsdr, const char * pluginfilepath, const char * params) {
	if (tsdr->nativerunning || tsdr->running)
			RETURN_EXCEPTION(tsdr, "The library is already running in async mode. Stop it first!", TSDR_ALREADY_RUNNING);

	unloadplugin(tsdr);

	int status = tsdrplug_load(&tsdr->plugin, pluginfilepath);
	if (status != TSDR_OK) {

		if (status == TSDR_INCOMPATIBLE_PLUGIN) {
			RETURN_EXCEPTION(tsdr, "The plugin cannot be loaded. It is incompatible or there are depending libraries missing. Please check the readme file that comes with the plugin.", status);
		} else {
			unloadplugin(tsdr);
			RETURN_EXCEPTION(tsdr, "The selected library is not a valid TSDR plugin!", status);
		}
	}

	char blah[200];
	tsdr->plugin.tsdrplugin_getName(blah);
	if ((status = tsdr->plugin.tsdrplugin_init(params)) != TSDR_OK) {
		announceexception(tsdr,tsdr->plugin.tsdrplugin_getlasterrortext(),status);
		unloadplugin(tsdr);
		return status;
	}

	RETURN_OK(tsdr);
}

 int tsdr_readasync(tsdr_lib_t * tsdr, tsdr_readasync_function cb, void * ctx) {
	 int pluginsfault = 0;

	if (tsdr->nativerunning || tsdr->running)
		RETURN_EXCEPTION(tsdr, "The library is already running in async mode. Stop it first!", TSDR_ALREADY_RUNNING);

	if (!tsdr->plugin.initialized)
		RETURN_EXCEPTION(tsdr, "Please load a working plugin first!", TSDR_ERR_PLUGIN);

	tsdr->nativerunning = 1;

	tsdr->running = 1;

	const int width = tsdr->width;
	const int height = tsdr->height;
	const int size = width * height;

	if (width <= 0 || height <= 0 || size > MAX_ARR_SIZE)
		RETURN_EXCEPTION(tsdr, "The supplied height and the width are invalid!", TSDR_WRONG_VIDEOPARAMS);

	int status;
	if ((status = tsdr_getsamplerate(tsdr)) != TSDR_OK) goto end;
	if ((status = tsdr_setbasefreq(tsdr, tsdr->centfreq)) != TSDR_OK) goto end;
	if ((status = tsdr_setgain(tsdr, tsdr->gain)) != TSDR_OK) goto end;

	if (tsdr->pixeltimeoversampletime <= 0) goto end;

	tsdr_context_t * context = (tsdr_context_t *) malloc(sizeof(tsdr_context_t));
	context->this = tsdr;
	context->cb = cb;
	context->ctx = ctx;
	context->decimator_items_to_poll = DEFAULT_DECIMATOR_TO_POLL;
	context->device_items_dropped = 0;
	context->device_items_to_drop = 0;
	cb_init(&context->circbuf_decimation_to_video, CB_SIZE_MAX_COEFF_LOW_LATENCY);
	cb_init(&context->circbuf_device_to_decimation, CB_SIZE_MAX_COEFF_LOW_LATENCY);

	frameratedetector_startthread(&tsdr->frameratedetect);
	thread_start(decimatingthread, (void *) context);
	thread_start(videodecodingthread, (void *) context);

	status = tsdr->plugin.tsdrplugin_readasync(process, (void *) context);

	frameratedetector_stopthread(&tsdr->frameratedetect);

	if (status != TSDR_OK) pluginsfault =1;

	tsdr->running = 0;
	mutex_waitforever(&tsdr->stopsync);
	free(context);

	cb_free(&context->circbuf_decimation_to_video);
	cb_free(&context->circbuf_device_to_decimation);

end:
	if (pluginsfault) announceexception(tsdr,tsdr->plugin.tsdrplugin_getlasterrortext(),status);

	tsdr->running = 0;
	tsdr->nativerunning = 0;

	return status;
}

 int tsdr_setresolution(tsdr_lib_t * tsdr, int width, int height, double refreshrate) {
	if (width <= 0 || height <= 0 || width*height > MAX_ARR_SIZE || refreshrate <= 0)
		RETURN_EXCEPTION(tsdr, "The supplied height and the width are invalid or refreshrate is negative!", TSDR_WRONG_VIDEOPARAMS);

	tsdr->width = width;
	tsdr->height = height;
	tsdr->pixelrate = width * height * refreshrate;
	tsdr->refreshrate = refreshrate;
	tsdr->pixeltime = 1.0/tsdr->pixelrate;
	if (tsdr->sampletime != 0)
		tsdr->pixeltimeoversampletime = tsdr->pixeltime /  tsdr->sampletime;

	frameratedetector_flushcachedestimation(&tsdr->frameratedetect);

	RETURN_OK(tsdr);
}

 int tsdr_motionblur(tsdr_lib_t * tsdr, float coeff) {
	if (coeff < 0.0f || coeff > 1.0f) return TSDR_WRONG_VIDEOPARAMS;
	tsdr->motionblur = coeff;
	RETURN_OK(tsdr);
}

 int tsdr_sync(tsdr_lib_t * tsdr, int pixels, int direction) {
	if (pixels == 0) return TSDR_OK;
	switch(direction) {
	case DIRECTION_CUSTOM:
		tsdr->syncoffset += pixels;
		break;
	case DIRECTION_UP:
		if (pixels > tsdr->height || pixels < 0) RETURN_EXCEPTION(tsdr, "Cannot shift up with more pixels than the height of the image or shift is negative!", TSDR_WRONG_VIDEOPARAMS);
		tsdr->syncoffset += pixels * tsdr->width;
		break;
	case DIRECTION_DOWN:
		if (pixels > tsdr->height || pixels < 0) RETURN_EXCEPTION(tsdr, "Cannot shift down with more pixels than the height of the image or shift is negative!", TSDR_WRONG_VIDEOPARAMS);
		tsdr->syncoffset -= pixels * tsdr->width;
		break;
	case DIRECTION_LEFT:
		if (pixels > tsdr->width || pixels < 0) RETURN_EXCEPTION(tsdr, "Cannot shift to the left with more pixels than the width of the image or shift is negative!", TSDR_WRONG_VIDEOPARAMS);
		tsdr->syncoffset+=pixels;
		break;
	case DIRECTION_RIGHT:
		if (pixels > tsdr->width || pixels < 0) RETURN_EXCEPTION(tsdr, "Cannot shift to the right with more pixels than the width of the image or shift is negative!", TSDR_WRONG_VIDEOPARAMS);
		tsdr->syncoffset-=pixels;
		break;
	}
	RETURN_OK(tsdr);
}

int tsdr_setparameter_int(tsdr_lib_t * tsdr, int parameter, uint32_t value) {
	if (parameter < 0 || parameter >= COUNT_PARAM_INT)
		RETURN_EXCEPTION(tsdr, "Invalid integer parameter id", TSDR_INVALID_PARAMETER);
	tsdr->params_int[parameter] = value;
	RETURN_OK(tsdr);
}

int tsdr_setparameter_double(tsdr_lib_t * tsdr, int parameter, double value) {
	if (parameter < 0 || parameter >= COUNT_PARAM_DOUBLE)
		RETURN_EXCEPTION(tsdr, "Invalid double floating point parameter id", TSDR_INVALID_PARAMETER);
	printf("Parameter %d to double value %f\n", parameter, value); fflush(stdout);
	RETURN_OK(tsdr);
}

 void tsdr_free(tsdr_lib_t ** tsdr) {
	 unloadplugin(*tsdr);

	 free((*tsdr)->errormsg);
	 (*tsdr)->errormsg_size = 0;

	 semaphore_free(&(*tsdr)->threadsync);
	 mutex_free(&(*tsdr)->stopsync);
	 mutex_free(&(*tsdr)->fft_mutex);

	frameratedetector_free(&(*tsdr)->frameratedetect);

	 free (*tsdr);
	 *tsdr = NULL;
 }

 int tsdr_getfft(tsdr_lib_t * tsdr, float * buffer, int fft_size, uint32_t * samplerate) {

	 if (!tsdr->running || !tsdr->nativerunning)
	 	RETURN_EXCEPTION(tsdr, "Cannot return FFT since the library is not running!", TSDR_NOT_RUNNING);
	 if (fft_size <= 5 || fft_size > 20)
		 RETURN_EXCEPTION(tsdr, "fft_size must be between 5 and 20!", TSDR_INVALID_PARAMETER_VALUE);

	 tsdr->fft_buffer = buffer;
	 tsdr->fft_size = 1 << fft_size;

	 tsdr->fft_requested = 1;

	 mutex_waitforever(&tsdr->fft_mutex);

	 tsdr->fft_requested = 0;
	 *samplerate = tsdr->samplerate / tsdr->pixeltimeoversampletime;

	 if (tsdr->fft_exception)
		 RETURN_EXCEPTION(tsdr, "fft_size too big!", TSDR_INVALID_PARAMETER_VALUE);

	 fft_real(buffer, fft_size);

	 RETURN_OK(tsdr);
 }
