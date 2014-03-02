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


void frameratedetector_init(frameratedetector_t * frameratedetector) {
	frameratedetector->totallength = 200;

	frameratedetector->id = 0;

	frameratedetector->bestfit = 0.0;

	frameratedetector->centerofmass = 0.0;
	frameratedetector->lastfractionalcenterofmass = 0.0;

	frameratedetector->mode = 0;
}

void frameratedetector_run(frameratedetector_t * frameratedetector, tsdr_lib_t * tsdr, float * data, int size, uint32_t samplerate) {
	int i;
	for (i = 0; i < size; i++) {
		const float val = data[i];

		if (frameratedetector->id < frameratedetector->totallength)
			frameratedetector->centerofmass += val * i;
		else {

			const double fractionalcenterofmass = frameratedetector->centerofmass / (double) frameratedetector->totallength;
			const double centmassdiff = fractionalcenterofmass - frameratedetector->lastfractionalcenterofmass;
			const double bestfit = centmassdiff*centmassdiff;

			if (bestfit > frameratedetector->bestfit)
				frameratedetector->mode = !frameratedetector->mode;
			else
				printf("Detected fps is %.4f\n", samplerate / (double) ((tsdr->height) * (frameratedetector->totallength)));

			frameratedetector->totallength += (frameratedetector->mode) ? (1) : (-1);
			if (samplerate < 40 * tsdr->height * frameratedetector->totallength) frameratedetector->totallength = samplerate / (40.0 * tsdr->height);
			if (samplerate > 100 * tsdr->height * frameratedetector->totallength) frameratedetector->totallength = samplerate / (100.0 * tsdr->height);

			frameratedetector->bestfit = bestfit;
			frameratedetector->centerofmass = 0.0;
			frameratedetector->lastfractionalcenterofmass = fractionalcenterofmass;
			frameratedetector->id = 0;
		}

		frameratedetector->id++;
	}

}

void frameratedetector_free(frameratedetector_t * frameratedetector) {

}
