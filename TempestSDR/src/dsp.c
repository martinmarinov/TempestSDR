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

#include "dsp.h"
#include <stddef.h>
#include <assert.h>
#include "syncdetector.h"

#define AUTOGAIN_REPORT_EVERY_FRAMES (5)

void dsp_timelowpass_run(const float lowpassvalue, int sizetopoll, float * buffer, float * screenbuffer) {

	const double antilowpassvalue = 1.0 - lowpassvalue;
	int i;
	for (i = 0; i < sizetopoll; i++)
		screenbuffer[i] = screenbuffer[i] * lowpassvalue + buffer[i] * antilowpassvalue;
}

void dsp_autogain_init(dsp_autogain_t * autogain) {
	autogain->lastmax = 0;
	autogain->lastmin = 0;
}

void dsp_autogain_run(dsp_autogain_t * autogain, int sizetopoll, float * screenbuffer, float * sendbuffer, float norm) {
	int i;

	float min = screenbuffer[0];
	float max = min;

	for (i = 0; i < sizetopoll; i++) {
		const float val = screenbuffer[i];
		if (val > max) max = val; else if (val < min) min = val;
	}

	const float oneminusnorm = 1.0f-norm;
	autogain->lastmax = oneminusnorm*autogain->lastmax + norm*max;
	autogain->lastmin = oneminusnorm*autogain->lastmin + norm*min;
	const float span = (autogain->lastmax == autogain->lastmin) ? (1.0f) : (autogain->lastmax - autogain->lastmin);

	for (i = 0; i < sizetopoll; i++)
		sendbuffer[i] = (screenbuffer[i] - autogain->lastmin) / span;
}

void dsp_average_v_h(int width, int height, float * sendbuffer, float * widthcollapsebuffer, float * heightcollapsebuffer) {
	const int totalpixels = width*height;
	int i;
	for (i = 0; i < width; i++) widthcollapsebuffer[i] = 0.0f;
	for (i = 0; i < height; i++) heightcollapsebuffer[i] = 0.0f;
	for (i = 0; i < totalpixels; i++) {
		const float val = sendbuffer[i];
		widthcollapsebuffer[i % width] += val;
		heightcollapsebuffer[i / width] += val;
	}
}

void dsp_post_process_init(dsp_postprocess_t * pp) {
	dsp_autogain_init(&pp->dsp_autogain);

	pp->screenbuffer = NULL;
	pp->sendbuffer = NULL;
	pp->corrected_sendbuffer = NULL;

	pp->widthcollapsebuffer = NULL;
	pp->heightcollapsebuffer = NULL;

	pp->bufsize = 0;
	pp->sizetopoll = 0;
	pp->width = 0;
	pp->height = 0;

	pp->runs = 0;
}

float * dsp_post_process(tsdr_lib_t * tsdr, dsp_postprocess_t * pp, float * buffer, int nowwidth, int nowheight, float motionblur, float lowpasscoeff) {

	if (nowheight != pp->height || nowwidth != pp->width) {
		const int oldheight = pp->height;
		const int oldwidth = pp->width;

		pp->height = nowheight;
		pp->width = nowwidth;
		pp->sizetopoll = pp->height * pp->width;
		assert(pp->sizetopoll > 0);

		if (pp->sizetopoll > pp->bufsize) {
			pp->bufsize = pp->sizetopoll;
			pp->screenbuffer = MALLOC_OR_REALLOC(pp->screenbuffer, pp->bufsize, float);
			pp->sendbuffer = MALLOC_OR_REALLOC(pp->sendbuffer, pp->bufsize, float);
			pp->corrected_sendbuffer = MALLOC_OR_REALLOC(pp->corrected_sendbuffer, pp->bufsize, float);
			int i;
			for (i = 0; i < pp->bufsize; i++) pp->screenbuffer[i] = 0.0f;
		}

		if (pp->width != oldwidth) pp->widthcollapsebuffer = MALLOC_OR_REALLOC(pp->widthcollapsebuffer, pp->width, float);
		if (pp->height != oldheight) pp->heightcollapsebuffer = MALLOC_OR_REALLOC(pp->heightcollapsebuffer, pp->height, float);

	}


	dsp_autogain_run(&pp->dsp_autogain, pp->sizetopoll, buffer, pp->sendbuffer, lowpasscoeff);

	if (pp->runs++ > AUTOGAIN_REPORT_EVERY_FRAMES) {
		pp->runs = 0;
		announce_callback_changed(tsdr, VALUE_ID_AUTOGAIN_VALUES, pp->dsp_autogain.lastmin, pp->dsp_autogain.lastmax);
	}

	dsp_average_v_h(pp->width, pp->height, pp->sendbuffer, pp->widthcollapsebuffer, pp->heightcollapsebuffer);
	float * syncresult = syncdetector_run(tsdr, pp->sendbuffer, pp->corrected_sendbuffer, pp->width, pp->height, pp->widthcollapsebuffer, pp->heightcollapsebuffer, motionblur == 0.0f);
	dsp_timelowpass_run(motionblur, pp->sizetopoll, syncresult, pp->screenbuffer);

	return pp->screenbuffer;

}

void dsp_post_process_free(dsp_postprocess_t * pp) {
	if (pp->screenbuffer != NULL) free(pp->screenbuffer);
	if (pp->sendbuffer != NULL) free(pp->sendbuffer);
	if (pp->corrected_sendbuffer != NULL) free(pp->corrected_sendbuffer);

	if (pp->widthcollapsebuffer != NULL) free(pp->widthcollapsebuffer);
	if (pp->heightcollapsebuffer != NULL) free(pp->heightcollapsebuffer);
}

void dsp_resample_init(dsp_resample_t * res) {
	extbuffer_init(&res->out);

	res->offset = 0;
}

float * dsp_resample_process(dsp_resample_t * res, int size, float * buffer, const double pixeloversampletme, int * pids) {

	*pids = (int) ((size - res->offset) / pixeloversampletme);

	// resize buffer so it fits
	extbuffer_preparetohandle(&res->out, *pids);

	double t = res->offset;

	int pid = 0;
	int id;

	float * bref = buffer;
	for (id = 0; id < size; id++) {

		const float val = *(bref++);

		if (t < id && (t + pixeloversampletme) < (id+1)) {
			res->out.buffer[pid++] = val;
			t=res->offset+pid*pixeloversampletme;
		}

		while ((t+pixeloversampletme) < (id+1)) {
			// this only ever triggers if post < 1
			res->out.buffer[pid++] = val;
			t=res->offset+pid*pixeloversampletme;
		}
	}

	res->offset = t-size;

	return res->out.buffer;
}

void dsp_resample_free(dsp_resample_t * res) {
	extbuffer_free(&res->out);
}

void dsp_dropped_compensation_init(dsp_dropped_compensation_t * res) {
	res->dropped_samples = 0;
	res->todrop = 0;
}

void dsp_dropped_compensation_add(dsp_dropped_compensation_t * res, CircBuff_t * cb, float * buff, const size_t size, int block) {
	if (res->dropped_samples > 0) {
		const unsigned int moddropped = res->dropped_samples % block;
		res->todrop += block - moddropped; // how much to drop so that it ends up on one frame
		res->dropped_samples = 0;
	}

	if (res->todrop >= size)
		res->todrop -= size;
	else if (cb_add(cb, &buff[res->todrop], size-res->todrop) == CB_OK)
		res->todrop = 0;
	else // we lost samples due to buffer overflow
		res->dropped_samples += size;
}

int dsp_dropped_compensation_will_drop_all(dsp_dropped_compensation_t * res, int size) {
	return res->todrop >= size;
}

void dsp_dropped_compensation_shift_with(dsp_dropped_compensation_t * res, int block, int syncoffset) {
	if (syncoffset >= 0)
		res->dropped_samples += syncoffset;
	else if (syncoffset < 0)
		res->dropped_samples += block + syncoffset;
}
