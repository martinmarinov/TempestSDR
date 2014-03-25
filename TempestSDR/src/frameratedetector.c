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

inline static double frameratedetector_fitvalue(float * data, int offset, int length) {
	double sum = 0.0;
	int i;
	for (i = 0; i < length; i++) {
		const float val1 = data[i];
		const float val2 = data[i+offset];
		const double difff = val1 - val2;
		sum += difff * difff;
	}
	return sum / (double) length;
}

// estimate the next repetition of a size of datah length starting from data
// for distances from startlength to endlength in samples
inline static int frameratedetector_estimatedirectlength(float * data, int length, int endlength, int startlength, float * bestfitvalue) {
	int bestlength = startlength;
	int l = startlength;
	*bestfitvalue = frameratedetector_fitvalue(data, l, length);
	l++;
	while (l < endlength) {
		const float fitvalue = frameratedetector_fitvalue(data, l, length);
		if (fitvalue < *bestfitvalue) {
			*bestfitvalue = fitvalue;
			bestlength = l;
		}
		l++;
	}

	return bestlength;
}

double framedetector_estimatelinelength(tsdr_lib_t * tsdr, float * data, int size, uint32_t samplerate) {
	const int maxlength = samplerate / (double) (MIN_FRAMERATE * tsdr->height);
	const int minlength = samplerate / (double) (MAX_FRAMERATE * tsdr->height);

	float best1, best2, best3;
	const int bestintestimate = frameratedetector_estimatedirectlength(data, minlength, maxlength, minlength, &best1);

	// we would expect the next image to be 2*bestintestimates away
	const double doublebest = frameratedetector_estimatedirectlength(data, bestintestimate, 2*bestintestimate + 2, 2*bestintestimate - 2, &best2) / 2.0;
	const double triplebest = frameratedetector_estimatedirectlength(data, doublebest, 3*doublebest + 2, 3*doublebest - 2, &best3) / 3.0;

	return (best1 < best2 && best1 < best3) ? (bestintestimate) : ((best2 < best3) ? (doublebest) : (triplebest));
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

void frameratedetector_runontodata(frameratedetector_t * frameratedetector) {
	const double linelength = framedetector_estimatelinelength(frameratedetector->tsdr, frameratedetector->data, frameratedetector->desireddatalength, frameratedetector->samplerate);

	const int maxlength = (linelength+2)*frameratedetector->tsdr->height;
	const int minlength = (linelength-2)*frameratedetector->tsdr->height;

	// estimate the length of a horizontal line in samples
	float bestfit;
	int crudelength = frameratedetector_estimatedirectlength(frameratedetector->data, minlength, maxlength, minlength, &bestfit);

	const double fps = frameratedetector->samplerate / (double) (crudelength);
	//const double maxerror = frameratedetector->samplerate / (double) (crudelength * (crudelength-1));

	if (frameratedetector->bestfitvalue == 0 || bestfit < frameratedetector->bestfitvalue) {
		frameratedetector->setframerate(frameratedetector->tsdr, fps);
		printf("%f bestfit %f crudelength %d\n", fps, bestfit, crudelength); fflush(stdout);
		frameratedetector->bestfitvalue = bestfit;
	} else
		frameratedetector->bestfitvalue = frameratedetector->bestfitvalue*0.99f + 0.01f * bestfit;


}

void frameratedetector_thread(void * ctx) {
	frameratedetector_t * frameratedetector = (frameratedetector_t *) ctx;

	while (1) {
		mutex_waitforever(&frameratedetector->processing_mutex);
		if (!frameratedetector->alive) break;

		frameratedetector_runontodata(frameratedetector);
		frameratedetector->processing = 0;
	}

	mutex_free(&frameratedetector->processing_mutex);
}

void frameratedetector_init(frameratedetector_t * frameratedetector, frameratedetector_setframerate_function f) {
	frameratedetector->setframerate = f;

	frameratedetector->data = NULL;
	frameratedetector->size = 0;
	frameratedetector->data_size = 0;

	frameratedetector->processing = 0;
}

void frameratedetector_flushcachedestimation(frameratedetector_t * frameratedetector) {
	frameratedetector->bestfitvalue = 0;
}

void frameratedetector_startthread(frameratedetector_t * frameratedetector) {
	frameratedetector_flushcachedestimation(frameratedetector);

	frameratedetector->processing = 0;
	frameratedetector->alive = 1;
	mutex_init(&frameratedetector->processing_mutex);

	thread_start(frameratedetector_thread, frameratedetector);
}

void frameratedetector_stopthread(frameratedetector_t * frameratedetector) {
	frameratedetector->alive = 0;
	mutex_signal(&frameratedetector->processing_mutex);
}

void frameratedetector_run(frameratedetector_t * frameratedetector, tsdr_lib_t * tsdr, float * data, int size, uint32_t samplerate, int reset) {

	// if processing has not finished, wait
	if (frameratedetector->processing) return;

	if (reset) {
		frameratedetector->size = 0;
		return;
	}

	frameratedetector->tsdr = tsdr;
	frameratedetector->samplerate = samplerate;

	// we need to have at least two frames of data present
	frameratedetector->desireddatalength = 2.0 * samplerate / (double) (MIN_FRAMERATE);

	// resize the data to fit
	if (frameratedetector->desireddatalength > frameratedetector->data_size) {
		frameratedetector->data_size = frameratedetector->desireddatalength;
		if (frameratedetector->data == NULL)
			frameratedetector->data = malloc(sizeof(float)*frameratedetector->data_size);
		else
			frameratedetector->data = realloc((void *) frameratedetector->data, sizeof(float)*frameratedetector->data_size);
	}

	const int newsize = frameratedetector->size + size;
	if (newsize < frameratedetector->desireddatalength) {
		// copy the data into the buffer
		memcpy(&frameratedetector->data[frameratedetector->size], data, size * sizeof(float));
		frameratedetector->size = newsize;
	} else {
		const int rem = frameratedetector->desireddatalength - frameratedetector->size;

		memcpy(&frameratedetector->data[frameratedetector->size], data, rem * sizeof(float));

		// run algorithm
		frameratedetector->processing = 1;
		mutex_signal(&frameratedetector->processing_mutex);

		frameratedetector->size = 0;
	}

}

void frameratedetector_free(frameratedetector_t * frameratedetector) {
	free (frameratedetector->data);
}
