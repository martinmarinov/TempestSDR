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
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "extbuffer.h"

void extbuffer_init(extbuffer_t * container) {
	container->buffer = NULL;
	container->buffer_max_size = 0;
	container->size_valid_elements = 0;

	container->offset = 0;
	container->valid = 0;
	container->cleartozero = 1;
	container->calls = 0;
}

void extbuffer_preparetohandle(extbuffer_t * container, int size) {
	assert (size > 0);

	if (container->buffer_max_size < size || container->buffer_max_size > (size << 1)) {
		if (container->buffer == NULL) {
			container->buffer = (float *) malloc(sizeof(float) * size);
			container->valid = 1;
		} else if (container->buffer_max_size != size)
			container->buffer = (float *) realloc((void *) container->buffer, sizeof(float) * size);
		container->buffer_max_size = size;
	}

	container->size_valid_elements = size;
	if (container->cleartozero) {
		int i;
		for (i = 0; i < container->size_valid_elements; i++)
			container->buffer[i] = 0.0f;
		container->cleartozero = 0;
		container->calls = 0;
	}

	container->calls++;
}

void extbuffer_cleartozero(extbuffer_t * container) {
	container->cleartozero = 1;
}

void extbuffer_free(extbuffer_t * container) {

	container->valid = 0;
	if (container->buffer != NULL) free(container->buffer);
}

void extbuffer_dumptofile(extbuffer_t * container, char * filename, char * xname, char * yname) {
	assert (container->valid);

	FILE *f = NULL;

	f = fopen(filename, "w");

	fprintf(f, "%s, %s\n", xname, yname);

	int i;
	for (i = 0; i < container->size_valid_elements; i++)
		fprintf(f, "%d, %f\n", container->offset + i, container->buffer[i]);

	fclose(f);

}
