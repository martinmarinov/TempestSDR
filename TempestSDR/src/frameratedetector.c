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

#define FRAMERATE_RUNS (50)
#define FRAMERATE_ESTIMATED_CRUDE_PIXEL_ERROR (2)
#define MIN_MULTIPLIER (20)
#define MAX_MULTIPLIER (50)


void frameratedetector_init(frameratedetector_t * frameratedetector) {
	frameratedetector->crudefpserr = 0.0;
	frameratedetector->fpserr = 0.0;
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

inline static double frameratedetector_estimatebetterlength(float * data, int lengthtocompare, double crudelength, int multiplier) {
	return frameratedetector_estimateintlength(data, lengthtocompare, (crudelength + FRAMERATE_ESTIMATED_CRUDE_PIXEL_ERROR)*multiplier, (crudelength - FRAMERATE_ESTIMATED_CRUDE_PIXEL_ERROR)*multiplier) / (double) multiplier;
}

void frameratedetector_run(frameratedetector_t * frameratedetector, tsdr_lib_t * tsdr, float * data, int size, uint32_t samplerate) {
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

	int number_of_better_estimators = ((size - crudelength) / (crudelength + FRAMERATE_ESTIMATED_CRUDE_PIXEL_ERROR));
	if (number_of_better_estimators > MAX_MULTIPLIER) number_of_better_estimators = MAX_MULTIPLIER;

	assert(number_of_better_estimators > MIN_MULTIPLIER);

	double bestlength = 0.0;
	for (i = MIN_MULTIPLIER; i < number_of_better_estimators; i++)
		bestlength += frameratedetector_estimatebetterlength(data, crudelength, crudelength, i) / (double) (number_of_better_estimators-MIN_MULTIPLIER);

	const double crudefps = samplerate / (double) (crudelength * tsdr->height);
	const double fps = samplerate / (double) (bestlength * tsdr->height);

	frameratedetector->crudefpserr = frameratedetector->crudefpserr * 0.5 + 0.5 * (crudefps - 59.980925) * (crudefps - 59.980925);
	frameratedetector->fpserr = frameratedetector->fpserr * 0.5 + 0.5 * (fps - 59.980925) * (fps - 59.980925);

	printf("Crudefpserr is %.2f%% bigger\n", -100.0 + 100.0 * frameratedetector->crudefpserr / frameratedetector->fpserr); fflush(stdout);
}

void frameratedetector_free(frameratedetector_t * frameratedetector) {

}
