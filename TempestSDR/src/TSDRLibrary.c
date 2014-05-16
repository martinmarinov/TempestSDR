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
#include "superbandwidth.h"
#include "syncdetector.h"

#include "dsp.h"

#define MAX_ARR_SIZE (4000*4000)
#define MAX_SAMP_RATE (500e6)

#define INTEG_TYPE (0)
#define PI (3.141592653589793238462643383279502f)

#define NORMALISATION_LOWPASS_COEFF (0.1f)

#define DEFAULT_DECIMATOR_TO_POLL (128)

#define FRAMES_TO_POLL (0.1)

struct tsdr_context {
		tsdr_readasync_function cb;

		tsdr_lib_t * this;

		void *ctx;
		CircBuff_t circbuf_decimation_to_posproc;
		CircBuff_t circbuf_posproc_to_video;
		CircBuff_t circbuf_device_to_decimation;

		dsp_dropped_compensation_t dsp_device_dropped;

	} typedef tsdr_context_t;


#define RETURN_EXCEPTION(tsdr, message, status) {announceexception(tsdr, message, status); return status;}
#define RETURN_OK(tsdr) {tsdr->errormsg_code = TSDR_OK; return TSDR_OK;}
#define RETURN_PLUGIN_RESULT(tsdr,plugin,result) {if ((result) == TSDR_OK) RETURN_OK(tsdr) else RETURN_EXCEPTION(tsdr,plugin.tsdrplugin_getlasterrortext(),result);}

void tsdr_init(tsdr_lib_t ** tsdr, tsdr_value_changed_callback callback, tsdr_on_plot_ready_callback plotready_callback, void * ctx) {
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
	(*tsdr)->callback = callback;
	(*tsdr)->plotready_callback = plotready_callback;
	(*tsdr)->callbackctx = ctx;


	for (i = 0; i < COUNT_PARAM_INT; i++)
		(*tsdr)->params_int[i] = 0;
	for (i = 0; i < COUNT_PARAM_DOUBLE; i++)
		(*tsdr)->params_double[i] = 0.0;

	semaphore_init(&(*tsdr)->threadsync);
	mutex_init(&(*tsdr)->stopsync);

	dsp_post_process_init(&(*tsdr)->dsp_postprocess);
	dsp_resample_init(&(*tsdr)->dsp_resample);

	frameratedetector_init(&(*tsdr)->frameratedetect, *tsdr);
	superb_init(&(*tsdr)->super);

}

void tsdr_free(tsdr_lib_t ** tsdr) {
	(*tsdr)->callback = NULL;
	(*tsdr)->plotready_callback = NULL;

	unloadplugin(*tsdr);

	free((*tsdr)->errormsg);
	(*tsdr)->errormsg_size = 0;

	semaphore_free(&(*tsdr)->threadsync);
	mutex_free(&(*tsdr)->stopsync);

	dsp_post_process_free(&(*tsdr)->dsp_postprocess);
	dsp_resample_free(&(*tsdr)->dsp_resample);

	frameratedetector_free(&(*tsdr)->frameratedetect);
	superb_free(&(*tsdr)->super);

	free (*tsdr);
	*tsdr = NULL;
}

void tsdr_reset(tsdr_lib_t * tsdr) {

	tsdr->syncoffset = 0;

	dsp_post_process_free(&tsdr->dsp_postprocess);
	dsp_resample_free(&tsdr->dsp_resample);

	frameratedetector_free(&tsdr->frameratedetect);
	superb_free(&tsdr->super);

	dsp_post_process_init(&tsdr->dsp_postprocess);
	dsp_resample_init(&tsdr->dsp_resample);

	frameratedetector_init(&tsdr->frameratedetect, tsdr);
	superb_init(&tsdr->super);
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

 void announce_callback_changed(tsdr_lib_t * tsdr, int value_id, double arg0, double arg1) {
	 if (tsdr->callback != NULL)
		 tsdr->callback(value_id, arg0, arg1, tsdr->callbackctx);
 }

 void announce_plotready(tsdr_lib_t * tsdr, int plot_id, extbuffer_t * buffer, uint32_t data_size, uint32_t data_offset, uint32_t samplerate) {
		if (!buffer->valid || buffer->type != EXTBUFFER_TYPE_DOUBLE) return;

		if (tsdr->plotready_callback != NULL)
			tsdr->plotready_callback(plot_id, data_offset, buffer->dbuffer, data_size, samplerate, tsdr->callbackctx);
}

 void * tsdr_getctx(tsdr_lib_t * tsdr) {
	 return tsdr->callbackctx;
 }

 int tsdr_isrunning(tsdr_lib_t * tsdr) {
	return tsdr->nativerunning;
}

 int tsdr_getsamplerate(tsdr_lib_t * tsdr) {
	if (!tsdr->plugin.initialized) RETURN_EXCEPTION(tsdr, "Cannot change sample rate. Plugin not loaded yet.", TSDR_ERR_PLUGIN);

	tsdr->samplerate_real = tsdr->plugin.tsdrplugin_getsamplerate();

	if (tsdr->samplerate_real == 0 || tsdr->samplerate_real > 500e6) RETURN_EXCEPTION(tsdr, "Invalid/unsupported value for sample rate.", TSDR_SAMPLE_RATE_WRONG);

	set_internal_samplerate(tsdr, tsdr->samplerate_real);

	RETURN_OK(tsdr);

	return 0; // to avoid getting warning from stupid Eclpse
}

 int tsdr_setbasefreq(tsdr_lib_t * tsdr, uint32_t freq) {
	tsdr->centfreq = freq;

	if (tsdr->plugin.initialized) {
		frameratedetector_flushcachedestimation(&tsdr->frameratedetect);
		RETURN_PLUGIN_RESULT(tsdr, tsdr->plugin, tsdr->plugin.tsdrplugin_setbasefreq(tsdr->centfreq))
	} else
		RETURN_OK(tsdr);

	return 0; // to avoid getting warning from stupid Eclpse
}


void shiftfreq(tsdr_lib_t * tsdr, int32_t diff) {
	if (tsdr->plugin.initialized)
		tsdr->plugin.tsdrplugin_setbasefreq(tsdr->centfreq+diff);
}

 int tsdr_stop(tsdr_lib_t * tsdr) {
	if (!tsdr->running) RETURN_OK(tsdr);
	int status = tsdr->plugin.tsdrplugin_stop();


	semaphore_wait(&tsdr->threadsync);
	mutex_signal(&tsdr->stopsync);

	RETURN_PLUGIN_RESULT(tsdr, tsdr->plugin, status);

	return 0; // to avoid getting warning from stupid Eclpse
}

 int tsdr_setgain(tsdr_lib_t * tsdr, float gain) {


	tsdr->gain = gain;

	if (tsdr->plugin.initialized)
		RETURN_PLUGIN_RESULT(tsdr, tsdr->plugin, tsdr->plugin.tsdrplugin_setgain(gain))
	else
		RETURN_OK(tsdr);

	return 0; // to avoid getting warning from stupid Eclpse
}

// bresenham algorithm
// polyface filterbank
// shielded loop antenna (magnetic)


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

void process(float *buf, uint64_t items_count, void *ctx, int64_t samples_dropped) {
	assert((items_count & 1) == 0);

	tsdr_context_t * context = (tsdr_context_t *) ctx;

	const uint64_t size2 = items_count >> 1;

	if (context->this->params_int[PARAM_AUTOCORR_SUPERRESOLUTION]) {
		float * superbuf = NULL; int superbuff_samples;
		superb_run(&context->this->super, buf, items_count, context->this, samples_dropped, &superbuf, &superbuff_samples);

		if (superbuf) {
			am_demod(superbuf, superbuff_samples);
			cb_add(&context->circbuf_device_to_decimation, superbuf, superbuff_samples);
		}

	} else {
		superb_stop(&context->this->super, context->this);

		const int block = round(((context->this->width * context->this->height) << 1) * context->this->pixeltimeoversampletime);
		dsp_dropped_compensation_shift_with(&context->dsp_device_dropped, block, samples_dropped);

		if (!dsp_dropped_compensation_will_drop_all(&context->dsp_device_dropped, size2, block) || (	!context->this->params_int[PARAM_AUTOCORR_PLOTS_OFF] && samples_dropped != 0 ) ) {
			am_demod(buf, size2);

			frameratedetector_run(&context->this->frameratedetect, buf, size2, context->this->samplerate, samples_dropped != 0);

		} else
			frameratedetector_run(&context->this->frameratedetect, NULL, 0, context->this->samplerate, samples_dropped != 0);

		dsp_dropped_compensation_add(&context->dsp_device_dropped, &context->circbuf_device_to_decimation, buf, size2, block);
	}

}

void decimatingthread(void * ctx) {
	tsdr_context_t * context = (tsdr_context_t *) ctx;
	semaphore_enter(&context->this->threadsync);

	extbuffer_t buff;
	extbuffer_init(&buff);

	extbuffer_t outbuff;
	extbuffer_init(&outbuff);

	dsp_dropped_compensation_t dsp_dropped;

	dsp_dropped_compensation_init(&dsp_dropped);

	while (context->this->running) {

		const int width = context->this->width;
		const int height = context->this->height;

		const int totalpixels = width * height;
		const int size = FRAMES_TO_POLL * context->this->samplerate / context->this->refreshrate;
		extbuffer_preparetohandle(&buff, size);

		if (cb_rem_blocking(&context->circbuf_device_to_decimation, buff.buffer, size) == CB_OK) {

			dsp_resample_process(&context->this->dsp_resample, &buff, &outbuff, context->this->width * context->this->height * context->this->refreshrate, context->this->samplerate, context->this->params_int[PARAM_NEAREST_NEIGHBOUR_RESAMPLING]);

			dsp_dropped_compensation_add(&dsp_dropped, &context->circbuf_decimation_to_posproc, outbuff.buffer, outbuff.size_valid_elements, totalpixels);

			// section for manual syncing
			dsp_dropped_compensation_shift_with(&dsp_dropped,  totalpixels, -context->this->syncoffset);
			context->this->syncoffset = 0;

		}
	}

	extbuffer_free(&outbuff);
	extbuffer_free(&buff);

	semaphore_leave(&context->this->threadsync);
}

void postprocessingthread(void * ctx) {
	tsdr_context_t * context = (tsdr_context_t *) ctx;
	semaphore_enter(&context->this->threadsync);

	int oldwidth = context->this->width;
	int oldheight = context->this->height;

	extbuffer_t outbuff_final;
	extbuffer_init(&outbuff_final);

	while (context->this->running) {

		const int width = context->this->width;
		const int height = context->this->height;
		const int totalpixels = width * height;

		if (width != oldwidth || height != oldheight) {
			oldwidth = width;
			oldheight = height;
			cb_purge(&context->circbuf_posproc_to_video);
		}

		extbuffer_preparetohandle(&outbuff_final, totalpixels);
		if (cb_rem_blocking(&context->circbuf_decimation_to_posproc, outbuff_final.buffer, totalpixels) == CB_OK)
			cb_add(&context->circbuf_posproc_to_video, dsp_post_process(context->this , &context->this->dsp_postprocess, outbuff_final.buffer, width, height, context->this->motionblur, NORMALISATION_LOWPASS_COEFF, context->this->params_int[PARAM_LOW_PASS_BEFORE_SYNC], context->this->params_int[PARAM_AUTOGAIN_AFTER_PROCESSING]), totalpixels);
	}

	extbuffer_free(&outbuff_final);

	semaphore_leave(&context->this->threadsync);
}

void videodecodingthread(void * ctx) {

	tsdr_context_t * context = (tsdr_context_t *) ctx;
	semaphore_enter(&context->this->threadsync);

	extbuffer_t buff;
	extbuffer_init(&buff);

	while (context->this->running) {

		const int width = context->this->width;
		const int height = context->this->height;
		const int sizetopoll = height*width;

		extbuffer_preparetohandle(&buff, sizetopoll);

		if (cb_rem_blocking(&context->circbuf_posproc_to_video, buff.buffer, sizetopoll) == CB_OK)
			context->cb(buff.buffer, width, height, context->ctx);
	}

	extbuffer_free(&buff);

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

	return 0; // to avoid getting warning from stupid Eclpse
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

	return 0; // to avoid getting warning from stupid Eclpse
}

 int tsdr_readasync(tsdr_lib_t * tsdr, tsdr_readasync_function cb, void * ctx) {
	 int pluginsfault = 0;

	if (tsdr->nativerunning || tsdr->running)
		RETURN_EXCEPTION(tsdr, "The library is already running in async mode. Stop it first!", TSDR_ALREADY_RUNNING);

	if (!tsdr->plugin.initialized)
		RETURN_EXCEPTION(tsdr, "Please load a working plugin first!", TSDR_ERR_PLUGIN);

	tsdr_reset(tsdr);

	tsdr->nativerunning = 1;

	tsdr->running = 1;

	int status;
	if ((status = tsdr_getsamplerate(tsdr)) != TSDR_OK) goto end;

	const int width = tsdr->width;
	const int height = tsdr->height;
	const int size = width * height;

	if (width <= 0 || height <= 0 || size > MAX_ARR_SIZE) {
		status = TSDR_WRONG_VIDEOPARAMS;
		announceexception(tsdr, "The supplied height and the width are invalid!", TSDR_WRONG_VIDEOPARAMS);
		goto end;
	}

	if ((status = tsdr_setbasefreq(tsdr, tsdr->centfreq)) != TSDR_OK) goto end;
	if ((status = tsdr_setgain(tsdr, tsdr->gain)) != TSDR_OK) goto end;

	if (tsdr->pixeltimeoversampletime <= 0) goto end;

	tsdr_context_t * context = (tsdr_context_t *) malloc(sizeof(tsdr_context_t));
	context->this = tsdr;
	context->cb = cb;
	context->ctx = ctx;
	cb_init(&context->circbuf_decimation_to_posproc, CB_SIZE_MAX_COEFF_LOW_LATENCY);
	cb_init(&context->circbuf_posproc_to_video, CB_SIZE_MAX_COEFF_LOW_LATENCY);
	cb_init(&context->circbuf_device_to_decimation, CB_SIZE_MAX_COEFF_LOW_LATENCY);
	dsp_dropped_compensation_init(&context->dsp_device_dropped);

	frameratedetector_startthread(&tsdr->frameratedetect);

	thread_start(decimatingthread, (void *) context);
	thread_start(postprocessingthread, (void *) context);
	thread_start(videodecodingthread, (void *) context);

	status = tsdr->plugin.tsdrplugin_readasync(process, (void *) context);

	frameratedetector_stopthread(&tsdr->frameratedetect);

	if (status != TSDR_OK) pluginsfault =1;

	tsdr->running = 0;
	mutex_waitforever(&tsdr->stopsync);
	free(context);

	cb_free(&context->circbuf_posproc_to_video);
	cb_free(&context->circbuf_decimation_to_posproc);
	cb_free(&context->circbuf_device_to_decimation);

end:
	if (pluginsfault) announceexception(tsdr,tsdr->plugin.tsdrplugin_getlasterrortext(),status);

	tsdr->running = 0;
	tsdr->nativerunning = 0;

	return status;
}



 void set_internal_samplerate(tsdr_lib_t * tsdr, uint32_t samplerate) {
		tsdr->samplerate = samplerate;

		const double real_width = samplerate / (tsdr->refreshrate * tsdr->height);

		tsdr->width = (int) 2*real_width;
		tsdr->pixelrate = tsdr->width * tsdr->height * tsdr->refreshrate;

		if (tsdr->samplerate != 0 && tsdr->pixelrate != 0)
			tsdr->pixeltimeoversampletime = ((double) tsdr->samplerate) / tsdr->pixelrate;
 }

 int tsdr_setresolution(tsdr_lib_t * tsdr, int height, double refreshrate) {
	if (height <= 0 || refreshrate <= 0)
		RETURN_EXCEPTION(tsdr, "The supplied height is invalid or refreshrate is negative!", TSDR_WRONG_VIDEOPARAMS);

	tsdr->height = height;
	tsdr->refreshrate = refreshrate;

	if (tsdr->plugin.initialized)
		set_internal_samplerate(tsdr, tsdr->samplerate);

	RETURN_OK(tsdr);

	return 0; // to avoid getting warning from stupid Eclpse
}


 int tsdr_motionblur(tsdr_lib_t * tsdr, float coeff) {
	if (coeff < 0.0f || coeff > 1.0f) return TSDR_WRONG_VIDEOPARAMS;
	tsdr->motionblur = coeff;
	RETURN_OK(tsdr);

	return 0; // to avoid getting warning from stupid Eclpse
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

	return 0; // to avoid getting warning from stupid Eclpse
}

int tsdr_setparameter_int(tsdr_lib_t * tsdr, int parameter, uint32_t value) {
	if (parameter < 0 || parameter >= COUNT_PARAM_INT)
		RETURN_EXCEPTION(tsdr, "Invalid integer parameter id", TSDR_INVALID_PARAMETER);
	tsdr->params_int[parameter] = value;
	RETURN_OK(tsdr);

	return 0; // to avoid getting warning from stupid Eclpse
}

int tsdr_setparameter_double(tsdr_lib_t * tsdr, int parameter, double value) {
	if (parameter < 0 || parameter >= COUNT_PARAM_DOUBLE)
		RETURN_EXCEPTION(tsdr, "Invalid double floating point parameter id", TSDR_INVALID_PARAMETER);
	printf("Parameter %d to double value %f\n", parameter, value); fflush(stdout);
	RETURN_OK(tsdr);

	return 0; // to avoid getting warning from stupid Eclpse
}
