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

#include "internaldefinitions.h"

#ifndef DSP_H_
#define DSP_H_

#include "extbuffer.h"
#include "circbuff.h"
#include "syncdetector.h"

// time based lowpass

/** Does a IIR simple lowpass onto output given input (past values are in output). Size is the size of the buffers. Coefficient could be 0 to 1 */
void dsp_timelowpass_run(const float coeff, int size, float * input, float * output);

// autogain

typedef struct {
	float lastmax;
	float lastmin;
	float snr;
} dsp_autogain_t;

/** Initialize autogain storate */
void dsp_autogain_init(dsp_autogain_t * autogain);
/** Based on input, the output will be scaled to fit between 0 and 1. coeff regulates the lowpass with which the bounds change */
void dsp_autogain_run(dsp_autogain_t * autogain, int size, float * input, float * output, float coeff);

// averaging on vertical and horizontal

/** Averages all of the vertical and horizontal pixels of the input with height and width */
void dsp_average_v_h(int width, int height, float * input, float * widthcollapsebuffer, float * heightcollapsebuffer);

// dsp collection for image enhancement before

typedef struct {
	float * screenbuffer;
	float * sendbuffer;
	float * corrected_sendbuffer;

	float * widthcollapsebuffer;
	float * heightcollapsebuffer;

	int sizetopoll;
	int width;
	int height;
	int bufsize;

	int runs;

	dsp_autogain_t dsp_autogain;

	syncdetector_t sync;

	int lowpass_before_sync;
} dsp_postprocess_t;

void dsp_post_process_init(dsp_postprocess_t * pp);

/** Post processing */
float * dsp_post_process(tsdr_lib_t * tsdr, dsp_postprocess_t * pp, float * buffer, int nowwidth, int nowheight, float motionblur, float lowpasscoeff, const int lowpass_before_sync, const int autogain_after_proc);

void dsp_post_process_free(dsp_postprocess_t * pp);

// resampling

typedef struct {
	double contrib;
	double offset;
} dsp_resample_t;

void dsp_resample_init(dsp_resample_t * res);

void dsp_resample_process(dsp_resample_t * res, extbuffer_t * in, extbuffer_t * out, const double upsample_by, const double downsample_by, int nearest_neighbour_sampling);
void dsp_resample_free(dsp_resample_t * res);

// compensating for dropped samples

typedef struct {
	int64_t difference;
} dsp_dropped_compensation_t;

void dsp_dropped_compensation_init(dsp_dropped_compensation_t * res);
void dsp_dropped_compensation_add(dsp_dropped_compensation_t * res, CircBuff_t * cb, float * buff, const uint32_t size, uint32_t block);
void dsp_dropped_compensation_shift_with(dsp_dropped_compensation_t * res, uint32_t block, int64_t syncoffset);
int dsp_dropped_compensation_will_drop_all(dsp_dropped_compensation_t * res, uint32_t size, uint32_t block);

#endif
