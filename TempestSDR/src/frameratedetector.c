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
#include <math.h>

#define MIN_FRAMERATE (55)
#define MIN_HEIGHT (590)
#define MAX_FRAMERATE (87)
#define MAX_HEIGHT (1500)

#define FRAMERATE_RUNS (200)

#define NUMBER_OF_REPEATS_FOR_ACCURACY (2)

// higher is better
#define FRAMERATEDETECTOR_ACCURACY (1000)

// a state machine
#define FRAMERATEDETECTOR_STATE_COLLECT_DATA (0)
#define FRAMERATEDETECTOR_STATE_PROCESSING (1)
#define FRAMERATEDETECTOR_STATE_PAUSE (2)

#define FRAMERATEDETECTOR_RUN_EVERY_N_SECONDS (5)

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
	return sum / (double) counted;
}

// estimate the next repetition of a size of datah length starting from data
// for distances from startlength to endlength in samples

#define ENABLE_DUMP_FRAMEDATA_TO_FILE (0)
inline static int frameratedetector_estimatedirectlength(float * data, int size, int length, int endlength, int startlength, int accuracy) {
	assert(endlength + length <= size);

	double bestfitval;

#if ENABLE_DUMP_FRAMEDATA_TO_FILE
	static int first = 1;
	FILE *f = NULL;
	if (first) f = fopen("framedetectdata.csv", "a");
#endif

	int bestlength = startlength;
	int l = startlength;
	bestfitval = frameratedetector_fitvalue(data, l, length, accuracy);
#if ENABLE_DUMP_FRAMEDATA_TO_FILE
	if (first) fprintf(f, "%f, ", *bestfitvalue);
#endif
	l++;
	while (l < endlength) {
		const float fitvalue = frameratedetector_fitvalue(data, l, length, accuracy);
#if ENABLE_DUMP_FRAMEDATA_TO_FILE
	if (first) fprintf(f, "%f, ", fitvalue);
#endif
		if (fitvalue < bestfitval) {
			bestfitval = fitvalue;
			bestlength = l;
		}
		l++;
	}
#if ENABLE_DUMP_FRAMEDATA_TO_FILE
	if (first) {
		fclose(f);
		first = 0;
	}
#endif

	return bestlength;
}

int framedetector_estimatelinelength(float * data, int size, uint32_t samplerate, double framerate) {
	const int maxlength = samplerate / (double) (MIN_HEIGHT * framerate);
	const int minlength = samplerate / (double) (MAX_HEIGHT * framerate);
	const int meanlength = (maxlength + minlength) / 2;

	const int maxrange = maxlength - minlength + 1;
	int occurances[maxrange];
	memset(occurances, 0, sizeof(int) * maxrange);

	const int offset_maxsize = size - maxlength - meanlength;
	const int offset_step = offset_maxsize / FRAMERATE_RUNS;

	assert (offset_step != 0);

	int max_occurances = 0;

	int offset;
	int result = minlength;
	for (offset = 0; offset < offset_maxsize; offset += offset_step) {
		const int temp_result = frameratedetector_estimatedirectlength(&data[offset], size-offset, meanlength, maxlength, minlength, 0)-minlength;
		assert(temp_result >= 0 && temp_result < maxrange);
		occurances[temp_result]++;

		if (occurances[temp_result] > max_occurances) {
			max_occurances = occurances[temp_result];
			result=temp_result+minlength;
		}
	}

	return result;
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

	if (!frameratedetector->tsdr->params_int[PARAM_INT_AUTORESOLUTION]) return;

	if (frameratedetector->state == FRAMERATEDETECTOR_STATE_PROCESSING) {
		// State 1 of the state machine
		// Obtain rough estimation, CPU intensive

		const int maxlength = frameratedetector->samplerate / (double) (MIN_FRAMERATE);
		const int minlength = frameratedetector->samplerate / (double) (MAX_FRAMERATE);
		const int meanlength = (maxlength + minlength) / 2;

		// estimate the length of a horizontal line in samples
		const int crudelength = frameratedetector_estimatedirectlength(frameratedetector->data, frameratedetector->desireddatalength, meanlength, maxlength, minlength, FRAMERATEDETECTOR_ACCURACY);
		assert(crudelength >= minlength && crudelength <= maxlength);

		//const double maxerror = frameratedetector->samplerate / (double) (crudelength * (crudelength-1));
		const int linelength = framedetector_estimatelinelength(frameratedetector->data, frameratedetector->desireddatalength, frameratedetector->samplerate, frameratedetector->samplerate / (double) crudelength);

		const int estheight = crudelength / (double) linelength;
		printf("crudelength %d; framerate %.4f, height %d!\n", crudelength, frameratedetector->samplerate / (float) crudelength, estheight);fflush(stdout);

		if (estheight < MIN_HEIGHT || estheight > MAX_HEIGHT) return;

		if (stack_contains(&frameratedetector->stack, crudelength, estheight) == NUMBER_OF_REPEATS_FOR_ACCURACY) {
			// if we see the same length being calculated twice, switch state

			const double fps = frameratedetector->samplerate / (double) crudelength;

			stack_purge(&frameratedetector->stack);
			announce_callback_changed(frameratedetector->tsdr, VALUE_ID_AUTO_RESOLUTION, fps, estheight);

			frameratedetector->state = FRAMERATEDETECTOR_STATE_PAUSE;
			frameratedetector->size = 0;
			frameratedetector->samp_counter = 0;
			//printf("Change of state!\n");fflush(stdout);
		} else
			stack_push(&frameratedetector->stack, crudelength, estheight);
	}

}

void frameratedetector_thread(void * ctx) {
	frameratedetector_t * frameratedetector = (frameratedetector_t *) ctx;

	while (frameratedetector->alive) {

		mutex_waitforever(&frameratedetector->processing_mutex);

		if (!frameratedetector->alive) break;

		if (frameratedetector->data == NULL)
			break;

		semaphore_enter(&frameratedetector->freesemaphore);
		frameratedetector_runontodata(frameratedetector);
		semaphore_leave(&frameratedetector->freesemaphore);

		frameratedetector->state = FRAMERATEDETECTOR_STATE_COLLECT_DATA;
	}

	//printf("Thread dying\n");fflush(stdout);
	mutex_free(&frameratedetector->processing_mutex);
}

void frameratedetector_init(frameratedetector_t * frameratedetector, tsdr_lib_t * tsdr) {

	frameratedetector->data = NULL;
	frameratedetector->size = 0;
	frameratedetector->data_size = 1;

	frameratedetector->state = FRAMERATEDETECTOR_STATE_COLLECT_DATA;
	frameratedetector->samp_counter = 0;

	frameratedetector->tsdr = tsdr;

	frameratedetector->data = malloc(sizeof(float)*1);

	stack_init(&frameratedetector->stack);
	semaphore_init(&frameratedetector->freesemaphore);
}

void frameratedetector_flushcachedestimation(frameratedetector_t * frameratedetector) {
	stack_purge(&frameratedetector->stack);
	frameratedetector->state = FRAMERATEDETECTOR_STATE_COLLECT_DATA;
}

void frameratedetector_startthread(frameratedetector_t * frameratedetector) {
	frameratedetector_flushcachedestimation(frameratedetector);

	frameratedetector->state = FRAMERATEDETECTOR_STATE_COLLECT_DATA;
	frameratedetector->alive = 1;
	mutex_init(&frameratedetector->processing_mutex);

	thread_start(frameratedetector_thread, frameratedetector);
}

void frameratedetector_stopthread(frameratedetector_t * frameratedetector) {
	frameratedetector->alive = 0;
	mutex_signal(&frameratedetector->processing_mutex);
}

void frameratedetector_run(frameratedetector_t * frameratedetector, float * data, int size, uint32_t samplerate, int reset) {


	// if we don't want to call this at all
	if (!frameratedetector->tsdr->params_int[PARAM_INT_AUTORESOLUTION])
		return;

	if (frameratedetector->state == FRAMERATEDETECTOR_STATE_PAUSE) {
		frameratedetector->samp_counter+=size;
		if (frameratedetector->samp_counter > FRAMERATEDETECTOR_RUN_EVERY_N_SECONDS * samplerate) {
			// rerun the algorithm every n seconds
			frameratedetector_flushcachedestimation(frameratedetector);
			frameratedetector->samp_counter = 0;
		}

		return;
	} else if (frameratedetector->state == FRAMERATEDETECTOR_STATE_COLLECT_DATA) {

		if (reset) {
			frameratedetector->size = 0;
			return;
		}
		if (frameratedetector->data == NULL)
			return;

		semaphore_enter(&frameratedetector->freesemaphore);

		frameratedetector->samplerate = samplerate;

		// we need to have at least two frames of data present
		frameratedetector->desireddatalength = 2.5 * samplerate / (double) (MIN_FRAMERATE);

		// resize the data to fit
		if (frameratedetector->desireddatalength > frameratedetector->data_size) {
			frameratedetector->data_size = frameratedetector->desireddatalength;
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
			frameratedetector->state = FRAMERATEDETECTOR_STATE_PROCESSING;
			mutex_signal(&frameratedetector->processing_mutex);

			frameratedetector->size = 0;
		}

		semaphore_leave(&frameratedetector->freesemaphore);
	}

}

void frameratedetector_free(frameratedetector_t * frameratedetector) {
	semaphore_wait(&frameratedetector->freesemaphore);
	if (frameratedetector->data != NULL) free (frameratedetector->data);
	frameratedetector->data = NULL;
	semaphore_free(&frameratedetector->freesemaphore);
	stack_free(&frameratedetector->stack);
}
