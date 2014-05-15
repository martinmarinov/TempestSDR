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

#ifndef EXTBUFFER_H_
#define EXTBUFFER_H_

#include "stdint.h"

#define EXTBUFFER_TYPE_FLOAT (0)
#define EXTBUFFER_TYPE_DOUBLE (1)

typedef struct extbuffer {

	float * buffer;
	double * dbuffer;

	uint32_t size_valid_elements;

	uint32_t buffer_max_size;

	volatile int valid;
	volatile int cleartozero;

	uint64_t calls;
	int type;

} extbuffer_t;

void extbuffer_init(extbuffer_t * container);
void extbuffer_init_double(extbuffer_t * container);
void extbuffer_preparetohandle(extbuffer_t * container, uint32_t size);
void extbuffer_cleartozero(extbuffer_t * container);
void extbuffer_dumptofile(extbuffer_t * container, int offset, char * filename, char * xname, char * yname);
void extbuffer_free(extbuffer_t * container);


#endif /* EXTBUFFER_H_ */
