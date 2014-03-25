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

void frameratedetector_init(frameratedetector_t * frameratedetector) {
	frameratedetector->data = NULL;
	frameratedetector->size = 0;
	frameratedetector->data_size = 0;
}

inline static double frameratedetector_fitvalue(float * data, int offset, int length) {
	double sum = 0.0;
	int i;
	for (i = 0; i < length; i++) {
		const float val1 = data[i];
		const float val2 = data[i+offset];
		const double difff = val1 - val2;
		sum += difff * difff;
	}
	return sum;
}

inline static int frameratedetector_estimatedirectlength(float * data, int length, int endlength, int startlength) {
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

inline static double frameratedetector_estimatelength(float * data, int length, int endlength, int startlength) {
	// accuracy is
	// estmated framerate =  samplerate / (bestintestimate * tsdr->height)
	// there is +- 1 uncertainty in bestintestimate therefore difference could be as much as
	// 2 * samplerate / ((bestintestimate^2 - 1) * tsdr->height)

	const int bestintestimate = frameratedetector_estimatedirectlength(data, length, endlength, startlength);

	// we would expect the next image to be 2*bestintestimates away
	const double doublebest = frameratedetector_estimatedirectlength(data, bestintestimate, 2*bestintestimate + 2, 2*bestintestimate - 2) / 2.0;
	const double triplebest = frameratedetector_estimatedirectlength(data, doublebest, 3*doublebest + 2, 3*doublebest - 2) / 3.0;

	return triplebest;
}

/**
 * NOTE: For trying to estimate the size of the full frame, the error would be
 * err = 2 * samplerate / ((samplerate/framerate)^2 - 1)
 *  = 0.0009 for mirics/LCD at 60 fps
 *
 * For trying to estimate the size of a line
 * err = 2 * samplerate / (((samplerate/(framerate*tsdr->height))^2 - 1) * tsdr->height)
 *  = 0.8406 for mirics/LCD at 60 fps
 */

float frameratedetector_runontodata(tsdr_lib_t * tsdr, float * data, int size, uint32_t samplerate) {
	const int maxlength = samplerate / (double) (MIN_FRAMERATE * tsdr->height);
	const int minlength = samplerate / (double) (MAX_FRAMERATE * tsdr->height);

	const int searchsize = 3*maxlength + maxlength;
	const int lastindex = size - searchsize;

	assert (lastindex > 1);

	const int offsetstep = lastindex / FRAMERATE_RUNS;

	assert (offsetstep > 1);

	// estimate the length of a horizontal line in samples
	double crudelength = 0.0;
	int i;
	for (i = 0; i < FRAMERATE_RUNS; i++)
		crudelength += frameratedetector_estimatelength(&data[i*offsetstep], minlength, maxlength, minlength) / (double) FRAMERATE_RUNS;

	const double fps = samplerate / (double) (crudelength * tsdr->height);
	const double maxerror = samplerate / (double) (crudelength * (crudelength-1) * tsdr->height);

	printf("%f max error %f\n", fps, maxerror); fflush(stdout);

	return fps;
}

int frameratedetector_run(frameratedetector_t * frameratedetector, tsdr_lib_t * tsdr, float * data, int size, uint32_t samplerate) {
	int run = 0;

	// we need to have at least two frames of data present
	const int desireddatalength = 2.0 * samplerate / (double) (MIN_FRAMERATE);

	// resize the data to fit
	if (desireddatalength > frameratedetector->data_size) {
		frameratedetector->data_size = desireddatalength;
		if (frameratedetector->data == NULL)
			frameratedetector->data = malloc(sizeof(float)*frameratedetector->data_size);
		else
			frameratedetector->data = realloc((void *) frameratedetector->data, sizeof(float)*frameratedetector->data_size);
	}

	const int newsize = frameratedetector->size + size;
	if (newsize < desireddatalength) {
		// copy the data into the buffer
		memcpy(&frameratedetector->data[frameratedetector->size], data, size * sizeof(float));
		frameratedetector->size = newsize;
	} else {
		const int rem = desireddatalength - frameratedetector->size;

		memcpy(&frameratedetector->data[frameratedetector->size], data, rem * sizeof(float));

		// run algorithm
		frameratedetector_runontodata(tsdr, frameratedetector->data, desireddatalength, samplerate);

		run = 1;
		frameratedetector->size = 0;
	}

	return run;
}

void frameratedetector_free(frameratedetector_t * frameratedetector) {
	free (frameratedetector->data);
}
