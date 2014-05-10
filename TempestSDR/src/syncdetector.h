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
#include "internaldefinitions.h"

#ifndef SYNCDETECTOR_H_
#define SYNCDETECTOR_H_

typedef struct sweetspot_data {
	int curr_stripsize;
} sweetspot_data_t;

typedef struct syncdetector {
	int lastx;

	sweetspot_data_t db_x;
	sweetspot_data_t db_y;
} syncdetector_t;

void syncdetector_init(syncdetector_t * sy);
float * syncdetector_run(syncdetector_t * sy, tsdr_lib_t * tsdr, float * data, float * outputdata, int width, int height, float * widthbuffer, float * heightbuffer, int greenlines);

#endif
