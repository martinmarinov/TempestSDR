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

#define MIN_FRAMERATE (55)
#define MIN_HEIGHT (590)
#define MAX_FRAMERATE (87)
#define MAX_HEIGHT (1500)

#define FRAMERATE_RUNS (50)
#define ITERATIONS_TO_CONSIDER_DONE (2)

// higher is better
#define FRAMERATEDETECTOR_ACCURACY (2000)

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

	assert(counted != 0);
	return sum / (double) counted;
}

// estimate the next repetition of a size of datah length starting from data
// for distances from startlength to endlength in samples

int getBestMinLengthFromExtBufferRange(extbuffer_t * buff, int start, int end) {
	assert (end > start);

	int corrected_start = start - buff->offset;
	int corrected_end = end - buff->offset;
	if (corrected_end > buff->size_valid_elements) corrected_end = buff->size_valid_elements;
	if (corrected_start < 0) corrected_start = 0;

	int max_id = buff->offset + corrected_start;

	if (!buff->valid) return max_id;

	float min = buff->buffer[corrected_start];

	int i;
	for (i =corrected_start+1; i < corrected_end; i++) {
		const float val = buff->buffer[i];

		if (val < min) {
			min = val;
			max_id = buff->offset + i;
		}
	}
	return max_id;
}

int getBestMinLengthFromExtBuffer(extbuffer_t * buff) {
	return getBestMinLengthFromExtBufferRange(buff, buff->offset, buff->offset+buff->size_valid_elements);
}

inline static void frameratedetector_estimatedirectlength(extbuffer_t * buff, float * data, int size, int length, int endlength, const int startlength, int accuracy) {
	assert(endlength + length <= size);

	const int buffsize = endlength - startlength;
	extbuffer_preparetohandle(buff, buffsize);
	buff->offset = startlength;

	int l = startlength;
	buff->buffer[0] += frameratedetector_fitvalue(data, l, length, accuracy);
	l++;
	while (l < endlength) {
		const float fitvalue = frameratedetector_fitvalue(data, l, length, accuracy);
		buff->buffer[l-startlength] += fitvalue;
		l++;
	}
}

void framedetector_estimatelinelength(extbuffer_t * buff, float * data, int size, uint32_t samplerate) {
	const int maxlength = samplerate / (double) (MIN_HEIGHT * MIN_FRAMERATE);
	const int minlength = samplerate / (double) (MAX_HEIGHT * MAX_FRAMERATE);

	const int offset_maxsize = size - maxlength - minlength;
	const int offset_step = offset_maxsize / FRAMERATE_RUNS;

	assert (offset_step != 0);

	int offset;
	for (offset = 0; offset < offset_maxsize; offset += offset_step)
		frameratedetector_estimatedirectlength(buff, &data[offset], size-offset, minlength, maxlength, minlength, 0);
}

float toheight(int linelength, void  * ctx) {
	int crudelength = *((int *) ctx);
	return crudelength / (double) linelength;
}

int calc_fps_and_height(frameratedetector_t * frameratedetector, float * fps, int * height) {

	int length_of_frame_in_samples  = getBestMinLengthFromExtBuffer(&frameratedetector->extbuff);
	*fps = frameratedetector->samplerate / (double) length_of_frame_in_samples;
	const int linelength  = getBestMinLengthFromExtBufferRange(&frameratedetector->extbuff_small,
			frameratedetector->samplerate / (double) (MAX_HEIGHT * (*fps)),
			frameratedetector->samplerate / (double) (MIN_HEIGHT * (*fps)));
	*height = length_of_frame_in_samples / (double) linelength;

	return length_of_frame_in_samples;
}

void frameratedetector_runontodata(frameratedetector_t * frameratedetector, float * data, int size) {

	if (!frameratedetector->tsdr->params_int[PARAM_INT_AUTORESOLUTION]) return;

	// State 1 of the state machine
	// Obtain rough estimation, CPU intensive

	const int maxlength = frameratedetector->samplerate / (double) (MIN_FRAMERATE);
	const int minlength = frameratedetector->samplerate / (double) (MAX_FRAMERATE);

	// estimate the length of a horizontal line in samples
	frameratedetector_estimatedirectlength(&frameratedetector->extbuff, data, size, minlength, maxlength, minlength, FRAMERATEDETECTOR_ACCURACY);
	framedetector_estimatelinelength(&frameratedetector->extbuff_small, data, size, frameratedetector->samplerate);

	frameratedetector->frames++;

	int height; float fps;
	const int length_of_frame_in_samples = calc_fps_and_height(frameratedetector, &fps, &height);

	printf("length_of_frame_in_samples %d; framerate %.4f, height %d!\n", length_of_frame_in_samples, fps, height);fflush(stdout);

	if (height < MIN_HEIGHT || height > MAX_HEIGHT || fps > MAX_FRAMERATE || fps < MIN_FRAMERATE) return;

	if (height == frameratedetector->last_height && length_of_frame_in_samples == frameratedetector->last_framelength)
	{
		if ((frameratedetector->encounters_count++) >= ITERATIONS_TO_CONSIDER_DONE) {
			extbuffer_dumptofile(&frameratedetector->extbuff_small, "small_data.csv", "linesize", "bestvalue", toheight, (void *) &length_of_frame_in_samples);
			extbuffer_cleartozero(&frameratedetector->extbuff);
			extbuffer_cleartozero(&frameratedetector->extbuff_small);
			announce_callback_changed(frameratedetector->tsdr, VALUE_ID_AUTO_RESOLUTION, fps, height);
			frameratedetector->last_framelength = 0;
			frameratedetector->last_height = 0;
			frameratedetector->encounters_count = 0;
			frameratedetector->frames = 0;
		} else
			frameratedetector->encounters_count ++;
	} else {
		frameratedetector->last_framelength = length_of_frame_in_samples;
		frameratedetector->last_height = height;
		frameratedetector->encounters_count = 0;
	}

}

void frameratedetector_thread(void * ctx) {
	frameratedetector_t * frameratedetector = (frameratedetector_t *) ctx;

	float * buf = NULL;
	int bufsize = 0;

	while (frameratedetector->alive) {

		const int desiredsize = 2.5 * frameratedetector->samplerate / (double) (MIN_FRAMERATE);
		if (desiredsize == 0) {
			thread_sleep(10);
			continue;
		}

		if (desiredsize > bufsize) {
			bufsize = desiredsize;
			if (buf == NULL) buf = malloc(sizeof(float) * bufsize); else buf = realloc(buf, sizeof(float) * bufsize);
		}

		if (cb_rem_blocking(&frameratedetector->circbuff, buf, desiredsize) == CB_OK)
			frameratedetector_runontodata(frameratedetector, buf, desiredsize);
	}

	free (buf);
}

void frameratedetector_init(frameratedetector_t * frameratedetector, tsdr_lib_t * tsdr) {
	frameratedetector->tsdr = tsdr;
	frameratedetector->samplerate = 0;
	frameratedetector->alive = 0;
	frameratedetector->frames = 0;

	frameratedetector->last_framelength = 0;
	frameratedetector->last_height = 0;
	frameratedetector->encounters_count = 0;

	cb_init(&frameratedetector->circbuff, CB_SIZE_MAX_COEFF_HIGH_LATENCY);
	extbuffer_init(&frameratedetector->extbuff);
	extbuffer_init(&frameratedetector->extbuff_small);
}

void frameratedetector_flushcachedestimation(frameratedetector_t * frameratedetector) {
	extbuffer_cleartozero(&frameratedetector->extbuff);
	extbuffer_cleartozero(&frameratedetector->extbuff_small);
	frameratedetector->frames = 0;
	frameratedetector->last_framelength = 0;
	frameratedetector->last_height = 0;
	frameratedetector->encounters_count = 0;
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
	if (!frameratedetector->tsdr->params_int[PARAM_INT_AUTORESOLUTION])
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
	cb_free(&frameratedetector->circbuff);
	extbuffer_free(&frameratedetector->extbuff);
	extbuffer_free(&frameratedetector->extbuff_small);
}
