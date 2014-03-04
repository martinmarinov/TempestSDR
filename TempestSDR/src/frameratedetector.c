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

}

inline static double frameratedetector_fitvalue(float * data, int offset, int length) {
	double sum = 0.0;
	int i;
	for (i = 0; i < length; i++) {
		const float val1 = data[i];
		const float val2 = data[i+offset];
		const double diff = val1 - val2;
		sum += diff * diff;
	}
	return sum;
}

inline static double frameratedetector_findone(tsdr_lib_t * tsdr, float * data, uint32_t samplerate, int maxlength, int minlength) {
	int bestlength = minlength;
	int l = minlength;
	float bestfitvalue = frameratedetector_fitvalue(data, l, maxlength);
	l++;
	while (l < maxlength) {
		const float fitvalue = frameratedetector_fitvalue(data, l, maxlength);
		if (fitvalue < bestfitvalue) {
			bestfitvalue = fitvalue;
			bestlength = l;
		}
		l++;
	}

	return samplerate / (double) (bestlength * tsdr->height);
}

void frameratedetector_run(frameratedetector_t * frameratedetector, tsdr_lib_t * tsdr, float * data, int size, uint32_t samplerate) {
	const int maxlength = samplerate / (double) (MIN_FRAMERATE * tsdr->height);
	const int minlength = samplerate / (double) (MAX_FRAMERATE * tsdr->height);

	const int searchsize = maxlength + maxlength;
	const int lastindex = size - searchsize;

	assert (lastindex > 1);

	const int offsetstep = lastindex / FRAMERATE_RUNS;

	assert (offsetstep > 1);

	int i;
	double averagefps = 0.0;
	for (i = 0; i < FRAMERATE_RUNS; i++) {
		const double currfps = frameratedetector_findone(tsdr, &data[i*offsetstep], samplerate, maxlength, minlength);
		//printf("%f\n", currfps);
		averagefps += currfps / (double) FRAMERATE_RUNS;
	}

	printf("Detected %.6f fps. Offsetstep %d. Length between %d and %d\n", averagefps, offsetstep, minlength, maxlength);  fflush(stdout);
}

void frameratedetector_free(frameratedetector_t * frameratedetector) {

}
