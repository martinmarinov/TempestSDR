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

#define SUPER_FRAMES_TO_REC (3.5)
#define SUPER_HOPS_TO_MAKE (4)

#define SUPER_STATE_STOPPED (0)
#define SUPER_STATE_STARTING (1)
#define SUPER_STATE_GATHERING (2)
#define SUPER_STATE_PAUSE (3)
#define SUPER_SECS_TO_PAUSE (1)

void superb_init(superbandwidth_t * bw) {
	bw->state = SUPER_STATE_STOPPED;
	bw->buffscount = 0;
	bw->buffsbuffcount = 0;
	bw->samplerate = 0;
	bw->buffs = NULL;
}

void superb_free(superbandwidth_t * bw) {
	if (bw->buffs) {
		int i;
		for (i = 0; i < bw->buffscount; i++) free(bw->buffs[i]);
		free (bw->buffs);
		bw->buffs = NULL;
	}
}

void superb_ondataready(superbandwidth_t * bw) {
	printf("Data ready. Gathered %d frames\n", bw->buffsbuffcount / bw->samples_in_frame);fflush(stdout);
}

void superb_run(superbandwidth_t * bw, float * iq, int size, tsdr_lib_t * tsdr, int dropped) {

	if (bw->state == SUPER_STATE_STOPPED) bw->state = SUPER_STATE_STARTING;

	if (bw->state == SUPER_STATE_STARTING) {
		bw->buffid_current = 0;
		bw->samples_gathered = 0;
		bw->buffsbuffcount = 0;

		if (tsdr->samplerate != bw->samplerate) {
			bw->samples_in_frame =  tsdr->samplerate / tsdr->refreshrate;
			bw->samples_to_gather = SUPER_FRAMES_TO_REC * bw->samples_in_frame;
			bw->samples_to_pause = SUPER_SECS_TO_PAUSE * tsdr->samplerate;

			superb_free(bw);

			bw->samplerate = tsdr->samplerate;

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
			memcpy(&bw->buffs[bw->buffid_current][bw->samples_gathered*2], iq, size);
			bw->samples_gathered+=samples_now;
		} else {

			bw->buffid_current++;
			if (bw->buffsbuffcount == 0) bw->buffsbuffcount = bw->samples_gathered;
			else if (bw->samples_gathered < bw->buffsbuffcount) bw->buffsbuffcount = bw->samples_gathered;
			bw->samples_gathered = 0;

			if (bw->buffid_current >= bw->buffscount) {
				superb_ondataready(bw);
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
	}

}


