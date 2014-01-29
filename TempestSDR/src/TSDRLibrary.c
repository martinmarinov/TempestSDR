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

#define INTEG_TYPE (0)
#define PI (3.141592653589793238462643383279502f)

struct tsdr_context {
		tsdr_readasync_function cb;
		float * buffer;
		tsdr_lib_t * this;
		int bufsize;
		void *ctx;
		CircBuff_t circbuf;
		double offset;
		float contributionfromlast;
		int dropped;
		unsigned int todrop;
	} typedef tsdr_context_t;


#define RETURN_EXCEPTION(tsdr, message, status) {announceexception(tsdr, message, status); return status;}
#define RETURN_OK(tsdr) {tsdr->errormsg_code = TSDR_OK; return TSDR_OK;}
#define RETURN_PLUGIN_RESULT(tsdr,plugin,result) {if (result == TSDR_OK) RETURN_OK(tsdr) else RETURN_EXCEPTION(tsdr,plugin->tsdrplugin_getlasterrortext(),result);}

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

 void tsdr_init(tsdr_lib_t * tsdr) {
	tsdr->nativerunning = 0;
	tsdr->plugin = NULL;
	tsdr->centfreq = 0;
	tsdr->syncoffset = 0;
	tsdr->errormsg_size = 0;
	tsdr->errormsg_code = TSDR_OK;
}

 int tsdr_isrunning(tsdr_lib_t * tsdr) {
	return tsdr->nativerunning;
}

 int tsdr_setsamplerate(tsdr_lib_t * tsdr, uint32_t rate) {
	if (tsdr->plugin == NULL) RETURN_EXCEPTION(tsdr, "Cannot change sample rate. Plugin not loaded yet.", TSDR_ERR_PLUGIN);

	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);
	tsdr->samplerate = plugin->tsdrplugin_setsamplerate(rate);
	if (tsdr->samplerate == 0 || tsdr->samplerate > MAX_SAMP_RATE) RETURN_EXCEPTION(tsdr, "Invalid/unsupported value for sample rate.", TSDR_SAMPLE_RATE_WRONG);

	tsdr->sampletime = 1.0f / (float) tsdr->samplerate;
	if (tsdr->sampletime != 0)
		tsdr->pixeltimeoversampletime = tsdr->pixeltime /  tsdr->sampletime;

	RETURN_OK(tsdr);
}

 int tsdr_getsamplerate(tsdr_lib_t * tsdr) {
	if (tsdr->plugin == NULL) RETURN_EXCEPTION(tsdr, "Cannot change sample rate. Plugin not loaded yet.", TSDR_ERR_PLUGIN);

	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);
	tsdr->samplerate = plugin->tsdrplugin_getsamplerate();
	if (tsdr->samplerate == 0 || tsdr->samplerate > 500e6) RETURN_EXCEPTION(tsdr, "Invalid/unsupported value for sample rate.", TSDR_SAMPLE_RATE_WRONG);

	tsdr->sampletime = 1.0f / (float) tsdr->samplerate;
	if (tsdr->sampletime != 0)
		tsdr->pixeltimeoversampletime = tsdr->pixeltime /  tsdr->sampletime;

	RETURN_OK(tsdr);
}

 int tsdr_setbasefreq(tsdr_lib_t * tsdr, uint32_t freq) {
	tsdr->centfreq = freq;

	if (tsdr->plugin != NULL) {
		pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);
		RETURN_PLUGIN_RESULT(tsdr, plugin, plugin->tsdrplugin_setbasefreq(tsdr->centfreq));
	} else
		RETURN_OK(tsdr);
}

 int tsdr_stop(tsdr_lib_t * tsdr) {
	if (!tsdr->running) RETURN_OK(tsdr);

	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);

	int status = plugin->tsdrplugin_stop();

	mutex_wait((mutex_t *) tsdr->mutex_sync_unload);

	mutex_t * msu_ref = (mutex_t *) tsdr->mutex_sync_unload;
	tsdr->mutex_sync_unload = NULL;
	mutex_free(msu_ref);
	free(tsdr->mutex_sync_unload);
	RETURN_PLUGIN_RESULT(tsdr, plugin, status);
}

 int tsdr_setgain(tsdr_lib_t * tsdr, float gain) {

	tsdr->gain = gain;

	if (tsdr->plugin != NULL) {
		pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);
		RETURN_PLUGIN_RESULT(tsdr, plugin, plugin->tsdrplugin_setgain(gain));
	} else
		RETURN_OK(tsdr);
}

// bresenham algorithm
// polyface filterbank
// shielded loop antenna (magnetic)
void videodecodingthread(void * ctx) {
	int i;

	tsdr_context_t * context = (tsdr_context_t *) ctx;

	int height = context->this->height;
	int width = context->this->width;

	int bufsize = height * width;
	int sizetopoll = height * width;
	float * buffer = (float *) malloc(sizeof(float) * bufsize);
	float * screenbuffer = (float *) malloc(sizeof(float) * bufsize);
	float * sendbuffer = (float *) malloc(sizeof(float) * bufsize);
	for (i = 0; i < bufsize; i++) screenbuffer[i] = 0.0f;

	while (context->this->running) {

		const double lowpassvalue = context->this->motionblur;
		const double antilowpassvalue = 1.0 - lowpassvalue;

		if (context->this->height != height || context->this->width != width) {
			height = context->this->height;
			width = context->this->width;
			sizetopoll = height * width;

			if (sizetopoll > bufsize) {
				bufsize = sizetopoll;
				buffer = (float *) realloc(buffer, sizeof(float) * bufsize);
				screenbuffer = (float *) realloc(screenbuffer, sizeof(float) * bufsize);
				sendbuffer = (float *) realloc(sendbuffer, sizeof(float) * bufsize);
				for (i = 0; i < bufsize; i++) screenbuffer[i] = 0.0f;
			}
		}

		if (cb_rem_blocking(&context->circbuf, buffer, sizetopoll) == CB_OK) {

			float max = buffer[0];
			float min = max;
			for (i = 1; i < sizetopoll; i++) {
				const float val = screenbuffer[i] * lowpassvalue + buffer[i] * antilowpassvalue;
				if (val > max) max = val; else if (val < min) min = val;
				screenbuffer[i] = val;
			}

			const float span = max - min;

			for (i = 0; i < sizetopoll; i++)
				sendbuffer[i] = (screenbuffer[i] - min) / span;

			context->cb(sendbuffer, width, height, context->ctx);
		}
	}

	free (buffer);

	mutex_signal((mutex_t *) context->this->mutex_video_stopped);
}


static inline float definiteintegral(float x) {
//if (x > 1 || x < 0) printf("Requested %.4f/n", x);
#if INTEG_TYPE == 0
	// box
	return x;
#elif INTEG_TYPE == 1
	// triangle
	if (x < 0.5f)
		return 2*x*x;
	else {
		const int x1 = 1.0f-x;
		return 1+2*x1*x1;
	}
#elif INTEG_TYPE == 2
	// sin from 0 to 1
	return -cosf(x*PI);
#elif INTEG_TYPE == 3
	// normal distribution centred around 0.5
	return -0.177245f * erff(2.5f-5.0f*x);
#endif
}

// This is the pixel function. Start must be >=0 and end must be <=1
// Return the area of the pixel signal spanned by this if the pixel was from 0 to 1
#define integrate(start, end) (definiteintegral(end) - definiteintegral(start))

void process(float *buf, uint32_t len, void *ctx, int dropped) {
	tsdr_context_t * context = (tsdr_context_t *) ctx;

	const int size = len/2;

	float * outbuf = context->buffer;
	int outbufsize = context->bufsize;

	const double post = context->this->pixeltimeoversampletime;
	const int pids = (int) ((size - context->offset) / post);
	const float normalize = integrate(0, 1);

	if (dropped > 0)
		context->dropped += dropped / post;

	// resize buffer so it fits
	if (pids > outbufsize) {
				outbufsize = pids+len;
				outbuf = (float *) realloc(outbuf, sizeof(float) * outbufsize);
	}

	const double offset = context->offset;
	double t = context->offset;
	float contrib = context->contributionfromlast;

	int pid = 0;
	int i = 0;
	int id;

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

		if (t < id && (t + post) < (id+1)) {
			const float start = (id-t)/post;
#if INTEG_TYPE == -1
			outbuf[pid++] = val;
#else
			const float contrfract = integrate(start, 1)/normalize;
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
			const float contrfract = integrate(0,(id+1-t)/post)/normalize;
			contrib += contrfract * val;
		} else {
			const float idt = id - t;
			const float contrfract = integrate(idt/post,(idt+1)/post)/normalize;
			contrib += contrfract * val;
		}
	}

	context->bufsize = outbufsize;
	context->buffer = outbuf;
	context->offset = t-size;
	context->contributionfromlast = contrib;

//	if (pid != pids || context->offset > 0 || context->offset < -post)
//		printf("Pid %d; pids %d; t %.4f, size %d, offset %.4f\n", pid, pids, t, size, context->offset);

	// section for syncing lost samples
	if (context->dropped > 0) {
		const unsigned int size = context->this->width * context->this->height;
		const unsigned int moddropped = context->dropped % size;
		context->todrop += size - moddropped; // how much to drop so that it ends up on one frame
		context->dropped = 0;
	}

	if (context->todrop >= pid)
		context->todrop -= pid;
	else if (cb_add(&context->circbuf, &outbuf[context->todrop], pid-context->todrop) == CB_OK)
		context->todrop = 0;
	else // we lost samples due to buffer overflow
		context->dropped += pid;


	// section for manual syncing
	const int syncoffset = -context->this->syncoffset;
	if (syncoffset > 0)
		context->dropped += syncoffset;
	else if (syncoffset < 0)
		context->dropped += context->this->width * context->this->height + syncoffset;
	context->this->syncoffset = 0;

}

 int tsdr_readasync(tsdr_lib_t * tsdr, const char * pluginfilepath, tsdr_readasync_function cb, void *ctx, const char * params) {
	if (tsdr->nativerunning || tsdr->running)
		RETURN_EXCEPTION(tsdr, "The library is already running in async mode. Stop it first!", TSDR_ALREADY_RUNNING);

	tsdr->nativerunning = 1;

	tsdr->running = 1;

	const int width = tsdr->width;
	const int height = tsdr->height;
	const int size = width * height;

	if (width <= 0 || height <= 0 || size > MAX_ARR_SIZE)
		RETURN_EXCEPTION(tsdr, "The supplied height and the width are invalid!", TSDR_WRONG_VIDEOPARAMS);

	tsdr->plugin = malloc(sizeof(pluginsource_t));

	int pluginsfault = 0;
	int status = tsdrplug_load((pluginsource_t *)(tsdr->plugin), pluginfilepath);
	if (status != TSDR_OK) {announceexception(tsdr, "The plugin cannot be loaded. It is incompatible or there are depending libraries missing. Please check the readme file that comes with the plugin.", status); goto end;};

	pluginsource_t * plugin = (pluginsource_t *)(tsdr->plugin);

	if ((status = plugin->tsdrplugin_setParams(params)) != TSDR_OK) {pluginsfault =1; goto end;};
	if ((status = tsdr_getsamplerate(tsdr)) != TSDR_OK) goto end;
	if ((status = tsdr_setbasefreq(tsdr, tsdr->centfreq)) != TSDR_OK) goto end;
	if ((status = tsdr_setgain(tsdr, tsdr->gain)) != TSDR_OK) goto end;

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
	context->dropped = 0;
	context->todrop = 0;
	cb_init(&context->circbuf);

	thread_start(videodecodingthread, (void *) context);
	status = plugin->tsdrplugin_readasync(process, (void *) context);
	if (status != TSDR_OK) pluginsfault =1;

	tsdr->running = 0;


	mutex_wait((mutex_t *) tsdr->mutex_video_stopped);

	mutex_free((mutex_t *) tsdr->mutex_video_stopped);
	free(tsdr->mutex_video_stopped);

	free(context->buffer);
	free(context);

	cb_free(&context->circbuf);

	if (tsdr->mutex_sync_unload != NULL) mutex_signal((mutex_t *) tsdr->mutex_sync_unload);
end:
	if (pluginsfault) announceexception(tsdr,plugin->tsdrplugin_getlasterrortext(),status);

	tsdrplug_close((pluginsource_t *)(tsdr->plugin));
	free(tsdr->plugin);
	tsdr->plugin = NULL;

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
	tsdr->pixeltime = 1.0/tsdr->pixelrate;
	if (tsdr->sampletime != 0)
		tsdr->pixeltimeoversampletime = tsdr->pixeltime /  tsdr->sampletime;

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

