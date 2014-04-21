/*******************************************************************************
 * Copyright (c) 2014 Martin Marinov.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Public License v3.0
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/gpl.html
 *
 * Contributors:
 *     Martin Marinov - initial API and implementation
 ******************************************************************************/

#include "frameratedetector.h"
#include "internaldefinitions.h"
#include <assert.h>
#include "threading.h"
#include <math.h>
#include "extbuffer.h"

#define MIN_FRAMERATE (55)
#define MIN_HEIGHT (590)
#define MAX_FRAMERATE (87)
#define MAX_HEIGHT (1500)

#define FRAMERATE_RUNS (50)

// higher is better
#define FRAMERATEDETECTOR_ACCURACY (2000)

inline static double frameratedetector_fitvalue(float * data, int offset, int length, int accuracy) {
	double sum = 0.0;

	float * first = data;
	float * second = data + offset;
	float * firstend = data + length;

	const int toskip = (accuracy == 0) ? (0) : (length / accuracy);
	int counted = 0;

	while (first < firstend) {
		const float d1 = *(first++) - *(second++);

		first+=toskip;
		second+=toskip;

		sum += d1 * d1;
		counted++;
	}

	assert(counted != 0);
	return sum / (double) counted;
}

inline static void frameratedetector_estimatedirectlength(extbuffer_t * buff, float * data, int size, int length, int endlength, const int startlength, int accuracy) {

	assert(endlength + length <= size);

	const int buffsize = endlength - startlength;
	extbuffer_preparetohandle(buff, buffsize);
	if (!buff->valid) return;

	buff->offset = startlength;

	int l = startlength;
	buff->buffer[0] += frameratedetector_fitvalue(data, l, length, accuracy);
	l++;
	while (l < endlength) {
		const float fitvalue = frameratedetector_fitvalue(data, l, length, accuracy);
		buff->buffer[l-startlength] += fitvalue;
		l++;
	}
}

void framedetector_estimatelinelength(extbuffer_t * buff, float * data, int size, uint32_t samplerate) {

	const int maxlength = samplerate / (double) (MIN_HEIGHT * MIN_FRAMERATE);
	const int minlength = samplerate / (double) (MAX_HEIGHT * MAX_FRAMERATE);

	const int offset_maxsize = size - 2*maxlength;
	const int offset_step = offset_maxsize / FRAMERATE_RUNS;

	assert (offset_step != 0);

	int offset;
	for (offset = 0; offset < offset_maxsize; offset += offset_step)
		frameratedetector_estimatedirectlength(buff, &data[offset], size-offset, maxlength, maxlength, minlength, 0);
}

float toheight(int linelength, void  * ctx) {
	int crudelength = *((int *) ctx);
	return crudelength / (double) linelength;
}

void frameratedetector_runontodata(frameratedetector_t * frameratedetector, float * data, int size, extbuffer_t * extbuff, extbuffer_t * extbuff_small) {

	if (frameratedetector->tsdr->params_int[PARAM_AUTOCORR_PLOTS_OFF]) return;

	const int maxlength = frameratedetector->samplerate / (double) (MIN_FRAMERATE);
	const int minlength = frameratedetector->samplerate / (double) (MAX_FRAMERATE);

	if (frameratedetector->tsdr->params_int[PARAM_AUTOCORR_PLOTS_RESET]) {
		frameratedetector->tsdr->params_int[PARAM_AUTOCORR_PLOTS_RESET] = 0;
		extbuffer_cleartozero(extbuff);
		extbuffer_cleartozero(extbuff_small);
		announce_callback_changed(frameratedetector->tsdr, VALUE_ID_AUTOCORRECT_RESET, 0, 0);
	}

	// estimate the length of a horizontal line in samples
	frameratedetector_estimatedirectlength(extbuff, data, size, minlength, maxlength, minlength, FRAMERATEDETECTOR_ACCURACY);
	framedetector_estimatelinelength(extbuff_small, data, size, frameratedetector->samplerate);

	if (frameratedetector->tsdr->params_int[PARAM_AUTOCORR_PLOTS_OFF]) return;

	announce_plotready(frameratedetector->tsdr, PLOT_ID_FRAME, extbuff, frameratedetector->samplerate);
	announce_plotready(frameratedetector->tsdr, PLOT_ID_LINE, extbuff_small, frameratedetector->samplerate);
	announce_callback_changed(frameratedetector->tsdr, VALUE_ID_AUTOCORRECT_FRAMES_COUNT, 0, extbuff->calls);

}

void frameratedetector_thread(void * ctx) {
	frameratedetector_t * frameratedetector = (frameratedetector_t *) ctx;

	extbuffer_t extbuff;
	extbuffer_t extbuff_small;

	extbuffer_init(&extbuff);
	extbuffer_init(&extbuff_small);

	float * buf = NULL;
	int bufsize = 0;

	while (frameratedetector->alive) {

		const int desiredsize = 2.5 * frameratedetector->samplerate / (double) (MIN_FRAMERATE);
		if (desiredsize == 0) {
			thread_sleep(10);
			continue;
		}

		if (desiredsize > bufsize) {
			bufsize = desiredsize;
			if (buf == NULL) buf = malloc(sizeof(float) * bufsize); else buf = realloc(buf, sizeof(float) * bufsize);
		}

		if (frameratedetector->purge_buffers) {
			extbuffer_cleartozero(&extbuff);
			extbuffer_cleartozero(&extbuff_small);
			frameratedetector->purge_buffers = 0;
		}

		if (cb_rem_blocking(&frameratedetector->circbuff, buf, desiredsize) == CB_OK)
			frameratedetector_runontodata(frameratedetector, buf, desiredsize, &extbuff, &extbuff_small);
	}

	free (buf);

	extbuffer_free(&extbuff);
	extbuffer_free(&extbuff_small);
}

void frameratedetector_init(frameratedetector_t * frameratedetector, tsdr_lib_t * tsdr) {
	frameratedetector->tsdr = tsdr;
	frameratedetector->samplerate = 0;
	frameratedetector->alive = 0;

	cb_init(&frameratedetector->circbuff, CB_SIZE_MAX_COEFF_HIGH_LATENCY);
}

void frameratedetector_flushcachedestimation(frameratedetector_t * frameratedetector) {
	frameratedetector->purge_buffers = 1;
	cb_purge(&frameratedetector->circbuff);
}

void frameratedetector_startthread(frameratedetector_t * frameratedetector) {
	frameratedetector_flushcachedestimation(frameratedetector);

	frameratedetector->alive = 1;

	thread_start(frameratedetector_thread, frameratedetector);
}

void frameratedetector_stopthread(frameratedetector_t * frameratedetector) {
	frameratedetector->alive = 0;
}

void frameratedetector_run(frameratedetector_t * frameratedetector, float * data, int size, uint32_t samplerate, int drop) {

	// if we don't want to call this at all
	if (frameratedetector->tsdr->params_int[PARAM_AUTOCORR_PLOTS_OFF])
		return;

	if (drop) {
		cb_purge(&frameratedetector->circbuff);
		return;
	}

	frameratedetector->samplerate = samplerate;
	if (cb_add(&frameratedetector->circbuff, data, size) != CB_OK)
		cb_purge(&frameratedetector->circbuff);
}

void frameratedetector_free(frameratedetector_t * frameratedetector) {
	frameratedetector_stopthread(frameratedetector);
	cb_free(&frameratedetector->circbuff);
}
