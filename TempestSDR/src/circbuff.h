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
#ifndef _Circubuffer
#define _Circubuffer

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "threading.h"

typedef struct CircBuff CircBuff_t;

#define CB_OK (1)
#define CB_FULL (0)
#define CB_EMPTY (0)
#define CB_ERROR (0)

#define CB_SIZE_MAX_COEFF_LOW_LATENCY (5)
#define CB_SIZE_MAX_COEFF_MED_LATENCY (10)
#define CB_SIZE_MAX_COEFF_HIGH_LATENCY (20)

struct CircBuff
{
    volatile float * buffer; // the circular buffer itself
	volatile size_t buffer_size; // the size of the circular buffer
	volatile size_t desired_buf_size; // the size of the buffer that we want it to become

    volatile size_t remaining_capacity; // the available capacity. I.e. how many free elements are there in the buffer
    volatile size_t pos; // the position where the next element will be inserted
    volatile size_t rempos; // the position where the next element will be taken from

    volatile int is_waiting;
    volatile int invalid;

    mutex_t mutex; // for thread safety
    mutex_t locker; // for waiting

    int buffering;

    int size_coeff;
    int max_size_coeff;
};

void cb_init(CircBuff_t * cb, int max_size_coeff);
int cb_add(CircBuff_t * cb, float * buff, const size_t size);
int cb_rem_blocking(CircBuff_t * cb, float * in, const size_t len);
int cb_rem_nonblocking(CircBuff_t * cb, float * in, const size_t len);
void cb_free(CircBuff_t * cb);
void cb_purge(CircBuff_t * cb);
int cb_size(CircBuff_t * cb);

#endif
