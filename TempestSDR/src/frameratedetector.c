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
#include "fft.h"

#define MIN_FRAMERATE (55)
#define MIN_HEIGHT (590)
#define MAX_FRAMERATE (87)
#define MAX_HEIGHT (1500)
#define FRAMES_TO_CAPTURE (2.1)

void autocorrelate(extbuffer_t * buff, float * data, int size) {
	extbuffer_preparetohandle(buff, 2*size);

	fft_autocorrelation(buff->buffer, data, size);
	if (!buff->valid) return;

}

void accummulate(extbuffer_t * out, extbuffer_t * in) {
	const uint32_t size = in->size_valid_elements;
	const int calls = in->calls;
	const int currcalls = calls - 1;

	extbuffer_preparetohandle(out, size);
	uint32_t i;

	if (currcalls < 0) {
		memcpy(out->buffer, in->buffer, size*sizeof(float));
	} else {
		for (i = 0; i < size; i++) {
			const float prev_avg = out->buffer[i];
			out->buffer[i] = (prev_avg*currcalls + in->buffer[i])/calls;
		}
	}
}

void frameratedetector_runontodata(frameratedetector_t * frameratedetector, float * data, int size, extbuffer_t * extbuff, extbuffer_t * extbuff_small) {

	if (frameratedetector->tsdr->params_int[PARAM_AUTOCORR_PLOTS_OFF]) return;

	const int maxlength = frameratedetector->samplerate / (double) (MIN_FRAMERATE);
	const int minlength = frameratedetector->samplerate / (double) (MAX_FRAMERATE);

	const int height_maxlength = frameratedetector->samplerate / (double) (MIN_HEIGHT * MIN_FRAMERATE);
	const int height_minlength = frameratedetector->samplerate / (double) (MAX_HEIGHT * MAX_FRAMERATE);

	if (frameratedetector->tsdr->params_int[PARAM_AUTOCORR_PLOTS_RESET]) {
		frameratedetector->tsdr->params_int[PARAM_AUTOCORR_PLOTS_RESET] = 0;
		extbuffer_cleartozero(extbuff);
		announce_callback_changed(frameratedetector->tsdr, VALUE_ID_AUTOCORRECT_RESET, 0, 0);
	}

	if (frameratedetector->tsdr->params_int[PARAM_AUTOCORR_PLOTS_OFF]) return;

	autocorrelate(extbuff, data, size);
	accummulate(extbuff_small, extbuff);

	announce_plotready(frameratedetector->tsdr, PLOT_ID_FRAME, extbuff_small, maxlength-minlength, minlength, frameratedetector->samplerate);
	announce_plotready(frameratedetector->tsdr, PLOT_ID_LINE, extbuff_small, height_maxlength-height_minlength, height_minlength, frameratedetector->samplerate);

	announce_callback_changed(frameratedetector->tsdr, VALUE_ID_AUTOCORRECT_FRAMES_COUNT, 0, extbuff->calls);

}

void frameratedetector_thread(void * ctx) {
	frameratedetector_t * frameratedetector = (frameratedetector_t *) ctx;

	extbuffer_t extbuff;
	extbuffer_t extbuff_small;

	extbuffer_init(&extbuff);
	extbuffer_init(&extbuff_small);

	float * buf = NULL;
	uint32_t bufsize = 0;

	while (frameratedetector->alive) {

		const uint32_t desiredsize = FRAMES_TO_CAPTURE * frameratedetector->samplerate / (double) (MIN_FRAMERATE);
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
