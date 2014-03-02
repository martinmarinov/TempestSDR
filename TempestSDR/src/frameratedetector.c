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

	frameratedetector->brighteststpot = 0;
	frameratedetector->darkeststspot = 0;
	frameratedetector->last_brighteststpot = 0;
	frameratedetector->last_darkeststspot = 0;

	frameratedetector->maxval = 0;
	frameratedetector->minval = 0;

	frameratedetector->mode = 0;
}

void frameratedetector_run(frameratedetector_t * frameratedetector, tsdr_lib_t * tsdr, float * data, int size, uint32_t samplerate) {
	int i;
	for (i = 0; i < size; i++) {
		const float val = data[i];

		if (frameratedetector->id < frameratedetector->totallength) {
			if (val < frameratedetector->minval) {
				frameratedetector->brighteststpot = frameratedetector->id;
				frameratedetector->minval = val;
			} else if (val > frameratedetector->maxval){
				frameratedetector->darkeststspot = frameratedetector->id;
				frameratedetector->maxval = val;
			}
		} else {

			const int diff_bright = frameratedetector->last_brighteststpot - frameratedetector->brighteststpot;
			const int bestfit_bright = diff_bright * diff_bright;
			const int diff_dark = frameratedetector->last_darkeststspot - frameratedetector->darkeststspot;
			const int bestfit_dark = diff_dark * diff_dark;
			const int bestfit = bestfit_bright + bestfit_dark;

			if (bestfit > frameratedetector->bestfit)
				frameratedetector->mode = !frameratedetector->mode;
			else
				printf("Detected fps is %.4f\n", samplerate / (double) ((tsdr->height) * (frameratedetector->totallength)));

			frameratedetector->totallength += (frameratedetector->mode) ? (1) : (-1);
			if (samplerate < 40 * tsdr->height * frameratedetector->totallength) frameratedetector->totallength = samplerate / (40.0 * tsdr->height);
			if (samplerate > 100 * tsdr->height * frameratedetector->totallength) frameratedetector->totallength = samplerate / (100.0 * tsdr->height);

			frameratedetector->bestfit = bestfit;
			frameratedetector->last_brighteststpot = frameratedetector->brighteststpot;
			frameratedetector->last_darkeststspot = frameratedetector->darkeststspot;
			frameratedetector->maxval = val;
			frameratedetector->minval = val;
			frameratedetector->brighteststpot = 0;
			frameratedetector->darkeststspot = 0;
			frameratedetector->id = 0;
		}

		frameratedetector->id++;
	}

}

void frameratedetector_free(frameratedetector_t * frameratedetector) {

}
