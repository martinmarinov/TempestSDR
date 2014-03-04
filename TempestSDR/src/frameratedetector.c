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
#include <stdio.h>
#include <assert.h>

#define MIN_FRAMERATE (40)
#define MAX_FRAMERATE (90)

#define FRAMERATE_RUNS (5)


void frameratedetector_init(frameratedetector_t * frameratedetector) {
	frameratedetector->fps = -1.0;
}

inline static double frameratedetector_fitvalue(float * data, int offset, int length) {
	double sum = 0.0;
	int i;
	for (i = 0; i < length; i++) {
		const float val1 = data[i];
		const float val2 = data[i+offset];
		const double diff = val1 - val2;
		sum += diff * diff;
	}
	return sum;
}

inline static int frameratedetector_estimateintlength(float * data, int length, int endlength, int startlength) {
	int bestlength = startlength;
	int l = startlength;
	float bestfitvalue = frameratedetector_fitvalue(data, l, length);
	l++;
	while (l < endlength) {
		const float fitvalue = frameratedetector_fitvalue(data, l, length);
		if (fitvalue < bestfitvalue) {
			bestfitvalue = fitvalue;
			bestlength = l;
		}
		l++;
	}

	return bestlength;
}

float frameratedetector_run(frameratedetector_t * frameratedetector, tsdr_lib_t * tsdr, float * data, int size, uint32_t samplerate) {
	const int maxlength = samplerate / (double) (MIN_FRAMERATE * tsdr->height);
	const int minlength = samplerate / (double) (MAX_FRAMERATE * tsdr->height);

	const int searchsize = maxlength + maxlength;
	const int lastindex = size - searchsize;

	assert (lastindex > 1);

	const int offsetstep = lastindex / FRAMERATE_RUNS;

	assert (offsetstep > 1);

	// estimate the length of a horizontal line in samples
	double crudelength = 0.0;
	int i;
	for (i = 0; i < FRAMERATE_RUNS; i++)
		crudelength += frameratedetector_estimateintlength(&data[i*offsetstep], maxlength, maxlength, minlength) / (double) FRAMERATE_RUNS;

	const double fps = samplerate / (double) (crudelength * tsdr->height);

	if (frameratedetector->fps == -1.0)
		frameratedetector->fps = fps;
	else
		frameratedetector->fps = frameratedetector->fps * 0.99 + fps * 0.01;

	printf("%f\n", frameratedetector->fps); fflush(stdout);

	return frameratedetector->fps;
}

void frameratedetector_free(frameratedetector_t * frameratedetector) {

}
