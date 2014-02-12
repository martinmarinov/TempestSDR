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

struct CircBuff
{
    volatile float * buffer; // the circular buffer itself
	volatile int buffer_size; // the size of the circular buffer
	volatile int desired_buf_size; // the size of the buffer that we want it to become

    volatile int remaining_capacity; // the available capacity. I.e. how many free elements are there in the buffer
    int pos; // the position where the next element will be inserted
    volatile int rempos; // the position where the next element will be taken from

    volatile int is_waiting;
    int buffering;

    mutex_t mutex; // for thread safety
    mutex_t locker; // for waiting
};

void cb_init(CircBuff_t * cb);
int cb_add(CircBuff_t * cb, float * buff, const int size);
int cb_rem_blocking(CircBuff_t * cb, float * in, const int len);
void cb_free(CircBuff_t * cb);

#endif
