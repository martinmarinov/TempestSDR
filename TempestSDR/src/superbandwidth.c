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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "superbandwidth.h"
#include "internaldefinitions.h"
#include "fft.h"

#define SUPER_FRAMES_TO_REC (1.5)
#define SUPER_HOPS_TO_MAKE (4)

#define SUPER_STATE_STOPPED (0)
#define SUPER_STATE_STARTING (1)
#define SUPER_STATE_GATHERING (2)
#define SUPER_STATE_PAUSE (3)

#define SUPER_SECS_TO_PAUSE (1)

#define SUPERB_ACCURACY (2000)

void superb_init(superbandwidth_t * bw) {
	bw->state = SUPER_STATE_STOPPED;
	bw->buffscount = 0;
	bw->buffsbuffcount = 0;
	bw->samplerate = 0;
	bw->buffs = NULL;

	extbuffer_init(&bw->extb);
}

void super_freebuff(superbandwidth_t * bw) {
	if (bw->buffs) {
		int i;
		for (i = 0; i < bw->buffscount; i++) free(bw->buffs[i]);
		free (bw->buffs);
		bw->buffs = NULL;
	}
}

void superb_free(superbandwidth_t * bw) {
	super_freebuff(bw);

	extbuffer_free(&bw->extb);
}

inline static double superb_fitvalue(float * data1, float * data2, int offset_data2, uint32_t length) {
	double sum = 0.0;

	float * first = data1;
	float * second = data2 + offset_data2;
	float * firstend = data1 + length;
	float * secondend = data2 + length;

	const int toskip = (SUPERB_ACCURACY == 0) ? (0) : (length / SUPERB_ACCURACY);
	int counted = 0;

	float firstI = *(first++); float firstQ = *(first++);
	float first_prev = firstI * firstI + firstQ * firstQ;
	float secondI = *(second++); float secondQ = *(second++);
	float second_prev = secondI * secondI + secondQ * secondQ;

	while (first < firstend) {
		firstI = *(first++); firstQ = *(first++);
		const float curr_first = firstI * firstI + firstQ * firstQ;

		secondI = *(second++); secondQ = *(second++);
		float curr_second = secondI * secondI + secondQ * secondQ;

		const float fd = first_prev - curr_first;
		const float sd = second_prev - curr_second;

		first_prev = curr_first;
		second_prev = curr_second;

		first+=toskip;
		second+=toskip;

		if (second > secondend) second -= length;

		const float d1 = fd - sd;

		sum += d1 * d1;
		counted++;
	}

	return sum / (double) counted;
}

inline static int superb_bestfit(float * data1, float * data2, int length) {
	int l = 0;
	int bestlength = 0;
	double min_val = superb_fitvalue(data1, data2, 0, length);

	for (l = 1; l < length; l++) {
		const double val = superb_fitvalue(data1, data2, l, length);
		if (val < min_val) {
			min_val = val;
			bestlength = l;
		}
	}

	return bestlength;
}

void superb_ondataready(superbandwidth_t * bw, float ** outbuff, int * outbufsize, tsdr_lib_t * tsdr) {
	//printf("Data ready. Gathered %d frames\n", bw->buffsbuffcount / bw->samples_in_frame);fflush(stdout);

	const uint32_t totalsamples = bw->buffscount * bw->buffsbuffcount;
	const uint32_t fft_realsize_per_freq = fft_getrealsize(bw->buffsbuffcount);

	extbuffer_preparetohandle(&bw->extb, totalsamples * 2);

	// allign
	int i;
	for (i = 1; i < bw->buffscount; i++) {
		const int bufsize = bw->buffsbuffcount * 2;
		const int best_offset = superb_bestfit(bw->buffs[0], bw->buffs[i], bufsize);
		memcpy(bw->extb.buffer, &bw->buffs[i][best_offset], (bufsize - best_offset) * sizeof(float));
		memcpy(&bw->buffs[i][bufsize -best_offset], bw->buffs[i], best_offset * sizeof(float));
		memcpy(bw->buffs[i], bw->extb.buffer, (bufsize - best_offset) * sizeof(float));
		fft_perform(bw->buffs[i], bw->buffsbuffcount, 0);
	}
	fft_perform(bw->buffs[0], bw->buffsbuffcount, 0);

	// stick the ffts next to each other
	for (i = 0; i < bw->buffscount; i++)
		memcpy(&bw->extb.buffer[i*fft_realsize_per_freq*2], bw->buffs[i], fft_realsize_per_freq * 2 * sizeof(float));

	const uint32_t fft_outsize = fft_getrealsize(fft_realsize_per_freq * bw->buffscount);
	fft_perform(bw->extb.buffer, fft_outsize, 1);

	*outbuff = bw->extb.buffer;
	*outbufsize = fft_outsize;

	set_internal_samplerate(tsdr, bw->buffscount*bw->samplerate);
}

void superb_run(superbandwidth_t * bw, float * iq, int size, tsdr_lib_t * tsdr, int dropped, float ** outbuff, int * outbufsize) {
	*outbuff = NULL;

	if (bw->state == SUPER_STATE_STOPPED) bw->state = SUPER_STATE_STARTING;

	if (bw->state == SUPER_STATE_STARTING) {
		bw->buffid_current = 0;
		bw->samples_gathered = 0;
		bw->buffsbuffcount = 0;

		if (tsdr->samplerate_real != bw->samplerate) {
			bw->samplerate = tsdr->samplerate_real;

			bw->samples_in_frame =  tsdr->samplerate_real / tsdr->refreshrate;
			bw->samples_to_gather = SUPER_FRAMES_TO_REC * bw->samples_in_frame;
			bw->samples_to_pause = SUPER_SECS_TO_PAUSE * tsdr->samplerate_real;

			super_freebuff(bw);

			bw->buffscount = SUPER_HOPS_TO_MAKE;

			bw->buffs = malloc(sizeof(float *) * bw->buffscount);
			int i;
			for (i = 0; i < bw->buffscount; i++) bw->buffs[i] = malloc(sizeof(float) * bw->samples_to_gather * 2);
		}

		bw->state = SUPER_STATE_GATHERING;
	}

	if (bw->state == SUPER_STATE_PAUSE) {
		bw->samples_gathered+=size / 2;
		if (bw->samples_gathered > bw->samples_to_pause) {
			bw->samples_gathered = 0;
			bw->state = SUPER_STATE_GATHERING;
		}
	}

	if (bw->state == SUPER_STATE_GATHERING) {
		if (dropped) {bw->samples_gathered = 0; return;}

		const int samples_now = size / 2;
		if (bw->samples_gathered + samples_now < bw->samples_to_gather) {
			memcpy(&bw->buffs[bw->buffid_current][bw->samples_gathered*2], iq, size * sizeof(float));
			bw->samples_gathered+=samples_now;
		} else {

			bw->buffid_current++;
			if (bw->buffsbuffcount == 0) bw->buffsbuffcount = bw->samples_gathered;
			else if (bw->samples_gathered < bw->buffsbuffcount) bw->buffsbuffcount = bw->samples_gathered;
			bw->samples_gathered = 0;

			if (bw->buffid_current >= bw->buffscount) {
				superb_ondataready(bw, outbuff, outbufsize, tsdr);
				bw->state = SUPER_STATE_STARTING;
			} else {
				shiftfreq(tsdr, (bw->buffid_current-bw->buffscount/2)*bw->samplerate);
				bw->state = SUPER_STATE_PAUSE;
			}

		}

	}
}

void superb_stop(superbandwidth_t * bw, tsdr_lib_t * tsdr) {
	if (bw->state != SUPER_STATE_STOPPED) {
		bw->state = SUPER_STATE_STOPPED;
		shiftfreq(tsdr, 0);
		set_internal_samplerate(tsdr, tsdr->samplerate_real);
	}

}


