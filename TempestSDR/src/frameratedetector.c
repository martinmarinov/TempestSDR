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

#define MIN_FRAMERATE (40)
#define MAX_FRAMERATE (90)


void frameratedetector_init(frameratedetector_t * frameratedetector) {
	frameratedetector->totallength = 200;

	frameratedetector->id = 0;

	frameratedetector->bestfit = 0.0;
	frameratedetector->sumdiff = 0.0;

	frameratedetector->buff_size = frameratedetector->totallength;
	frameratedetector->buff = malloc(sizeof(float) * frameratedetector->buff_size);
	int i;
	for (i = 0; i < frameratedetector->buff_size; i++)  {
		frameratedetector->buff[i] = 0.0f;

	}

	frameratedetector->mode = 0;
	frameratedetector->fps = 0.0;
}

void frameratedetector_run(frameratedetector_t * frameratedetector, tsdr_lib_t * tsdr, float * data, int size, uint32_t samplerate) {
	int i;
	for (i = 0; i < size; i++) {
		const float val = data[i];
		const int id = frameratedetector->id;

		if (id < frameratedetector->totallength) {
			const float oldval = frameratedetector->buff[id];
			const double diff = (val - oldval);
			frameratedetector->sumdiff += diff * diff;
			frameratedetector->buff[id] = val;

		} else {
			const double bestfit = frameratedetector->sumdiff / (double) frameratedetector->totallength;

			if (bestfit > frameratedetector->bestfit)
				frameratedetector->mode = !frameratedetector->mode;
			else {
				const double fps = samplerate / (double) (frameratedetector->totallength);
				frameratedetector->fps = fps;
				printf("Detected fps is %.4f (bestfit %f < %f) -> %s\n", frameratedetector->fps, bestfit, frameratedetector->bestfit, (frameratedetector->mode) ? "Increasing" : "Decreasing"); fflush(stdout);
			}

			frameratedetector->totallength += (frameratedetector->mode) ? (1) : (-1);
			if (samplerate < MIN_FRAMERATE * frameratedetector->totallength) frameratedetector->totallength = samplerate / (double) (MIN_FRAMERATE);
			else if (samplerate > MAX_FRAMERATE * frameratedetector->totallength) frameratedetector->totallength = samplerate / (double) (MAX_FRAMERATE);

			if (frameratedetector->totallength > frameratedetector->buff_size) {
				frameratedetector->buff_size = frameratedetector->totallength;
				frameratedetector->buff = realloc(frameratedetector->buff, sizeof(float) * frameratedetector->buff_size);
			}

			frameratedetector->bestfit = bestfit;
			frameratedetector->sumdiff = 0.0;
			frameratedetector->id = 0;
		}

		frameratedetector->id++;
	}

}

void frameratedetector_free(frameratedetector_t * frameratedetector) {
	free (frameratedetector->buff);
}
