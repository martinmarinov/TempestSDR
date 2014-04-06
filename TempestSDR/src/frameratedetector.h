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

#ifndef FRAMERATEDETECTOR_H_
#define FRAMERATEDETECTOR_H_

#include "include/TSDRLibrary.h"
#include "threading.h"
#include "stack.h"

#include <stddef.h>

typedef struct frameratedetector {

	float * data;
	size_t data_size;

	tsdr_lib_t * tsdr;
	uint32_t samplerate;

	size_t size;

	volatile int alive;
	mutex_t processing_mutex;
	size_t desireddatalength;

	uint64_t samp_counter;

	stack_t stack;
	int state;

	semaphore_t freesemaphore;

} frameratedetector_t;

void frameratedetector_startthread(frameratedetector_t * frameratedetector);
void frameratedetector_stopthread(frameratedetector_t * frameratedetector);
void frameratedetector_flushcachedestimation(frameratedetector_t * frameratedetector);

void frameratedetector_init(frameratedetector_t * frameratedetector, tsdr_lib_t * tsdr);
void frameratedetector_free(frameratedetector_t * frameratedetector);
void frameratedetector_run(frameratedetector_t * frameratedetector, float * data, int size, uint32_t samplerate, int reset);

#endif
