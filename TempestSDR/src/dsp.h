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

// time based lowpass

/** Does a IIR simple lowpass onto output given input (past values are in output). Size is the size of the buffers. Coefficient could be 0 to 1 */
void dsp_timelowpass_run(const float coeff, int size, float * input, float * output);

// autogain

typedef struct {
	float lastmax;
	float lastmin;
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

	dsp_autogain_t dsp_autogain;
} dsp_postprocess_t;

void dsp_post_process_init(dsp_postprocess_t * pp);

/** Post processing */
float * dsp_post_process(tsdr_lib_t * tsdr, dsp_postprocess_t * pp, float * buffer, int nowwidth, int nowheight, float motionblur, float lowpasscoeff);

void dsp_post_process_free(dsp_postprocess_t * pp);

#endif
