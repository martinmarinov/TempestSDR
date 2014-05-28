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
#include <math.h>
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
	autogain->snr = 1.0;
}

void dsp_autogain_run(dsp_autogain_t * autogain, int sizetopoll, float * screenbuffer, float * sendbuffer, float norm) {
	int i;

	float min = screenbuffer[0];
	float max = min;
	double sum = 0.0;

	for (i = 0; i < sizetopoll; i++) {
		const float val = screenbuffer[i];
#if PIXEL_SPECIAL_COLOURS_ENABLED
		if (val > 250.0 || val < -250) continue;
#endif
		if (val > max) max = val; else if (val < min) min = val;
		sum+=val;
	}

	const float oneminusnorm = 1.0f-norm;
	autogain->lastmax = oneminusnorm*autogain->lastmax + norm*max;
	autogain->lastmin = oneminusnorm*autogain->lastmin + norm*min;
	const float span = (autogain->lastmax == autogain->lastmin) ? (1.0f) : (autogain->lastmax - autogain->lastmin);

	const double mean = sum / (double) sizetopoll;
	double sum2 = 0.0;
	double sum3 = 0.0;
#if PIXEL_SPECIAL_COLOURS_ENABLED
	for (i = 0; i < sizetopoll; i++) {
		const float val = screenbuffer[i];
		sendbuffer[i] = (val > 250.0 || val < -250) ? (val) : ((screenbuffer[i] - autogain->lastmin) / span);

		const double valmeandiff = val - mean;
		sum2 += valmeandiff*valmeandiff;
		sum3 += valmeandiff;
	}
#else
	for (i = 0; i < sizetopoll; i++) {
		const float val = screenbuffer[i];
		sendbuffer[i] = (val - autogain->lastmin) / span;

		const float valmeandiff = val - mean;
		sum2 += valmeandiff*valmeandiff;
		sum3 += valmeandiff;
	}
#endif

	const double stdev = sqrt((sum2 - sum3*sum3/(double) sizetopoll) / (double) (sizetopoll - 1));

	autogain->snr = mean / stdev; // as from http://en.wikipedia.org/wiki/Signal-to-noise_ratio_(imaging)
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

	pp->lowpass_before_sync = 0;

	syncdetector_init(&pp->sync);
}

float * dsp_post_process(tsdr_lib_t * tsdr, dsp_postprocess_t * pp, float * buffer, int nowwidth, int nowheight, float motionblur, float lowpasscoeff, const int lowpass_before_sync, const int autogain_after_proc) {

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

	if (pp->lowpass_before_sync != lowpass_before_sync) {
		pp->lowpass_before_sync = lowpass_before_sync;
		int i;
		for (i = 0; i < pp->sizetopoll; i++) {
			pp->screenbuffer[i] = 0.0;
			pp->sendbuffer[i] = 0.0;
			pp->corrected_sendbuffer[i] = 0.0;
		}
	}

	float * input = buffer;
	if (!autogain_after_proc) {
		dsp_autogain_run(&pp->dsp_autogain, pp->sizetopoll, input, pp->sendbuffer, lowpasscoeff);

		input = pp->sendbuffer;
	}

	float * result;

	if (lowpass_before_sync) {
		dsp_timelowpass_run(motionblur, pp->sizetopoll, input, pp->screenbuffer);
		dsp_average_v_h(pp->width, pp->height, pp->screenbuffer, pp->widthcollapsebuffer, pp->heightcollapsebuffer);

		float * syncresult = syncdetector_run(&pp->sync, tsdr, pp->screenbuffer, pp->corrected_sendbuffer, pp->width, pp->height, pp->widthcollapsebuffer, pp->heightcollapsebuffer, !tsdr->params_int[PARAM_AUTOCORR_SUPERRESOLUTION], 0);

		if (autogain_after_proc) {
			dsp_autogain_run(&pp->dsp_autogain, pp->sizetopoll, syncresult, pp->sendbuffer, lowpasscoeff);

			result = pp->sendbuffer;
		} else
			result = syncresult;

	} else {
		dsp_average_v_h(pp->width, pp->height, input, pp->widthcollapsebuffer, pp->heightcollapsebuffer);

		float * syncresult = syncdetector_run(&pp->sync, tsdr, input, pp->corrected_sendbuffer, pp->width, pp->height, pp->widthcollapsebuffer, pp->heightcollapsebuffer, (motionblur == 0.0f) && (!tsdr->params_int[PARAM_AUTOCORR_SUPERRESOLUTION]), 1);
		dsp_timelowpass_run(motionblur, pp->sizetopoll, syncresult, pp->screenbuffer);

		if (autogain_after_proc) {
			dsp_autogain_run(&pp->dsp_autogain, pp->sizetopoll, pp->screenbuffer, pp->sendbuffer, lowpasscoeff);

			result = pp->sendbuffer;
		} else
			result = pp->screenbuffer;
	}

	if (pp->runs++ > AUTOGAIN_REPORT_EVERY_FRAMES) {
		pp->runs = 0;
		announce_callback_changed(tsdr, VALUE_ID_AUTOGAIN_VALUES, pp->dsp_autogain.lastmin, pp->dsp_autogain.lastmax);
		// TO ENABLE SNR UNCOMMENT THIS announce_callback_changed(tsdr, VALUE_ID_SNR, pp->dsp_autogain.snr, 0);
	}

	return result;

}

void dsp_post_process_free(dsp_postprocess_t * pp) {
	if (pp->screenbuffer != NULL) free(pp->screenbuffer);
	if (pp->sendbuffer != NULL) free(pp->sendbuffer);
	if (pp->corrected_sendbuffer != NULL) free(pp->corrected_sendbuffer);

	if (pp->widthcollapsebuffer != NULL) free(pp->widthcollapsebuffer);
	if (pp->heightcollapsebuffer != NULL) free(pp->heightcollapsebuffer);
}

void dsp_resample_init(dsp_resample_t * res) {

	res->contrib = 0;
	res->offset = 0;
}

void dsp_resample_process(dsp_resample_t * res, extbuffer_t * in, extbuffer_t * out, const double upsample_by, const double downsample_by, int nearest_neighbour_sampling) {

	const double sampletimeoverpixel = upsample_by / downsample_by;
	const double pixeloversampletme = downsample_by / upsample_by;

	const uint32_t size = in->size_valid_elements;
	const uint32_t output_samples = (int) ((size - res->offset) * sampletimeoverpixel);

	// resize buffer so it fits
	extbuffer_preparetohandle(out, output_samples);

	uint32_t id;

	float * resbuff = out->buffer;
	float * buffer = in->buffer;

	const double offset_sample = -res->offset * sampletimeoverpixel;

	if (nearest_neighbour_sampling) {
		for (id = 0; id < output_samples; id++)
			*(resbuff++) = buffer[((uint64_t) size * id) / output_samples];
	} else {
		//const int idcheck_less_than_idcheck2 = 1.0 < sampletimeoverpixel;

		uint32_t pid = 0;
		for (id = 0; id < size; id++) {
			const double idcheck = id*sampletimeoverpixel + offset_sample;
			const double idcheck3 = idcheck + sampletimeoverpixel;
			const double idcheck2 = idcheck + sampletimeoverpixel - 1.0;

			const double val = *(buffer++);

			if (pid < idcheck && pid < idcheck2) {
				*(resbuff++) = res->contrib + val*(1.0 - idcheck+pid);
				res->contrib = 0;
				pid++;
			}

			while (pid < idcheck2) {
				*(resbuff++) = val;
				pid++;
			}

			if (pid < idcheck3 && pid > idcheck)
				res->contrib += (idcheck3-pid) * val;
			else
				res->contrib += sampletimeoverpixel * val;
		}
	}

	res->offset += output_samples*pixeloversampletme-size;
}

void dsp_resample_free(dsp_resample_t * res) {

}

void dsp_dropped_compensation_init(dsp_dropped_compensation_t * res) {
	// the difference is the one between the planned sample_id that needs to be dropped in order to have
	// the correct block and the actual dropped.
	// if it gets negative, it needs to be recalculated
	res->difference = 0;
}

// based on how many frames were dropped, it calculates how many new needs to be dropped
static inline uint64_t dsp_dropped_cal_compensation(const int block, const int dropped) {
	const uint64_t frames = dropped / block;
	return ((frames + 1) * block - dropped) % block;
}

void dsp_dropped_compensation_add(dsp_dropped_compensation_t * res, CircBuff_t * cb, float * buff, const uint32_t size, uint32_t block) {
	assert(res->difference >= 0);

	if (size <= res->difference)
		res->difference -= size;
	else if (cb_add(cb, &buff[res->difference], size-res->difference) == CB_OK)
		res->difference = 0;
	else {
		res->difference -= size % block;
		if (res->difference < 0) res->difference = dsp_dropped_cal_compensation(block, -res->difference);
	}
}

int dsp_dropped_compensation_will_drop_all(dsp_dropped_compensation_t * res, uint32_t size, uint32_t block) {
	assert(res->difference >= 0);

	return size <= res->difference;
}

void dsp_dropped_compensation_shift_with(dsp_dropped_compensation_t * res, uint32_t block, int64_t syncoffset) {


	if (syncoffset >= 0)
		res->difference -= syncoffset % block;
	else
		res->difference -= block + syncoffset % block;

	if (res->difference < 0) res->difference = dsp_dropped_cal_compensation(block, -res->difference);
}
