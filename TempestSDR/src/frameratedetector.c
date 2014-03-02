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

#define MODE_SMALL_GROW_UP 0
#define MODE_SMALL_GROW_DOWN 2
#define MODE_TOTAL_GROW_UP 1
#define MODE_TOTAL_GROW_DOWN 3
#define MODE_OFFSET 4

#define MODES_COUNT 4

void frameratedetector_init(frameratedetector_t * frameratedetector) {
	frameratedetector->smalllength = 200;
	frameratedetector->totallength = 2 * frameratedetector->smalllength;

	frameratedetector->last_id = 0;

	frameratedetector->last_bestfit = 0.0;

	frameratedetector->last_small_sum = 0.0;
	frameratedetector->last_big_sum = 0.0;

	frameratedetector->mode = MODE_SMALL_GROW_UP;
}

void frameratedetector_run(frameratedetector_t * frameratedetector, tsdr_lib_t * tsdr, float * data, int size, uint32_t samplerate) {
	int i;
	for (i = 0; i < size; i++) {

		if (frameratedetector->last_id < frameratedetector->smalllength) {
			frameratedetector->last_small_sum+=data[i];
			data[i] = 0.0f;
		} else if (frameratedetector->last_id < frameratedetector->totallength)
			frameratedetector->last_big_sum+=data[i];
		else {
			const double diff = (100.0 * frameratedetector->last_small_sum / (double) frameratedetector->smalllength) - (100.0 * frameratedetector->last_big_sum / (double) (frameratedetector->totallength-frameratedetector->smalllength));
			const double currbestfit = (diff < 0) ? (-diff) : (diff);

			// do a PLL decision here
			if (currbestfit < frameratedetector->last_bestfit)
				frameratedetector->mode = (frameratedetector->mode + 1) % MODES_COUNT;// then our strategy is wrong, change it by taking next one
			else
				printf("Detected fps is %.4f (curr best %f, lastbest %f, smallength %d, totallength %d)\n", samplerate / (double) ((tsdr->height) * (frameratedetector->totallength)), currbestfit, frameratedetector->last_bestfit, frameratedetector->smalllength, frameratedetector->totallength);

			switch (frameratedetector->mode) {
			case MODE_SMALL_GROW_UP:
				if (frameratedetector->smalllength < frameratedetector->totallength/2) frameratedetector->smalllength++;
				break;
			case MODE_SMALL_GROW_DOWN:
				if (frameratedetector->smalllength > 2) frameratedetector->smalllength--;
				break;
			case MODE_TOTAL_GROW_UP:
				frameratedetector->totallength++;
				break;
			case MODE_TOTAL_GROW_DOWN:
				if (frameratedetector->totallength > 2*frameratedetector->smalllength) frameratedetector->totallength--;
				break;
			}

			frameratedetector->last_big_sum = 0.0f;
			frameratedetector->last_bestfit = currbestfit;

			if (frameratedetector->mode == MODE_OFFSET) {
				frameratedetector->last_small_sum = 0;
				frameratedetector->last_id = -1;
			} else {
				frameratedetector->last_small_sum = data[i];
				frameratedetector->last_id = 0;
			}
		}
		frameratedetector->last_id++;
	}

}

void frameratedetector_free(frameratedetector_t * frameratedetector) {

}
