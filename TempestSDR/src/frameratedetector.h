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

typedef struct frameratedetector {
	double crudefpserr;
	double fpserr;
} frameratedetector_t;

void frameratedetector_init(frameratedetector_t * frameratedetector);
void frameratedetector_free(frameratedetector_t * frameratedetector);
void frameratedetector_run(frameratedetector_t * frameratedetector, tsdr_lib_t * tsdr, float * data, int size, uint32_t samplerate);

#endif
