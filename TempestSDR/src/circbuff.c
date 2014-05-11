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
#include "circbuff.h"
#include <errno.h>

#if ASSERT_ENABLED
#include <assert.h>
#endif

#define CB_SIZE_COEFF_DEFAULT (2)

void cb_purge(CircBuff_t * cb) {
	if (cb->invalid) return;
	critical_enter(&cb->mutex);

    cb->remaining_capacity = cb->buffer_size; // how many elements could be loaded
    cb->pos = 0; // where the next element will be added
    cb->rempos = 0; // where the next element will be taken from

	if (cb->is_waiting) mutex_signal(&cb->locker);

	critical_leave(&cb->mutex);
}

void cb_init(CircBuff_t * cb, int max_size_coeff) {
#if ASSERT_ENABLED
	assert(max_size_coeff >= CB_SIZE_COEFF_DEFAULT);
#endif

	cb->max_size_coeff = max_size_coeff;
	cb->size_coeff = CB_SIZE_COEFF_DEFAULT;
    cb->desired_buf_size = cb->size_coeff; // initial size of buffer
    cb->buffer_size = cb->desired_buf_size;
    cb->buffer = (float *) malloc(sizeof(float) * cb->buffer_size); // allocate buffer
    cb->remaining_capacity = cb->buffer_size; // how many elements could be loaded
    cb->pos = 0; // where the next element will be added
    cb->rempos = 0; // where the next element will be taken from
    cb->invalid = 0;

    cb->is_waiting = 0;
    cb->buffering = 0;

    mutex_init(&cb->mutex);
    mutex_init(&cb->locker);
}

int cb_size(CircBuff_t * cb) {
	return cb->buffer_size - cb->remaining_capacity;
}

int cb_add(CircBuff_t * cb, float * in, const size_t len) {
	if (cb->invalid) return CB_ERROR;
    if (len <= 0) return CB_OK; // handle edge case

    critical_enter(&cb->mutex);

#if ASSERT_ENABLED
    assert(((cb->pos + cb->remaining_capacity) % cb->buffer_size) == cb->rempos);
#endif

    // if the size of the buffer is not large enough, request the buffer to be resized
    if (len*cb->size_coeff > cb->buffer_size) cb->desired_buf_size = len*cb->size_coeff;

    if (cb->buffer_size < cb->desired_buf_size) {
    	const size_t items_inside = cb->buffer_size - cb->remaining_capacity;
    	const size_t inflation = cb->desired_buf_size - cb->buffer_size;

        // if we need to resize the buffer, reset it
        cb->buffer = (float *) realloc((void *) cb->buffer, sizeof(float) * cb->desired_buf_size); // reallocate

        if (cb->rempos >= cb->pos) {
        	memmove((void *) &cb->buffer[cb->rempos+inflation], (void *) &cb->buffer[cb->rempos], sizeof(float) * (cb->buffer_size-cb->rempos));
        	if (items_inside != 0) cb->rempos += inflation;
        }

        cb->remaining_capacity += inflation;

        cb->buffer_size = cb->desired_buf_size; // set the size

#if ASSERT_ENABLED
        assert(cb->buffer_size - cb->remaining_capacity == items_inside);
#endif
    }

    if (cb->buffering && cb->remaining_capacity < 2*len) {
    	cb->buffering = 0;
    	critical_leave(&cb->mutex);
    	return CB_FULL; // if there is not enough space to put buffer, return error
    } else if (cb->remaining_capacity < len) {
    	cb->buffering = 1;
    	if (cb->size_coeff < cb->max_size_coeff) cb->size_coeff++;
        critical_leave(&cb->mutex);
        return CB_FULL; // if there is not enough space to put buffer, return error
    }

    const size_t oldpos = cb->pos;
    cb->pos = (oldpos + len) % cb->buffer_size; // calculate new position
    cb->remaining_capacity -= len; // the remaining capacity is reduced

    if (cb->pos <= oldpos) {
        // the add will wrap around
        const size_t remaining = cb->buffer_size - oldpos;
        memcpy((void *) &cb->buffer[oldpos], in, remaining * sizeof(float));
        memcpy((void *) cb->buffer, &in[remaining], cb->pos * sizeof(float));
    } else {
        // the add will not wrap around
        memcpy((void *) &cb->buffer[oldpos], in, len*sizeof(float));
    }

    if (cb->is_waiting) mutex_signal(&cb->locker);

    critical_leave(&cb->mutex);

    return CB_OK;
}

int cb_rem_nonblocking(CircBuff_t * cb, float * in, const size_t len) {
	if (cb->invalid) return CB_ERROR;
	if (len <= 0) return CB_OK;

	size_t items_inside = cb->buffer_size - cb->remaining_capacity;
    while (items_inside < len) return CB_EMPTY; // if there are not enough items

    if (cb->invalid) return CB_ERROR;
    critical_enter(&cb->mutex);

#if ASSERT_ENABLED
    assert(((cb->pos + cb->remaining_capacity) % cb->buffer_size) == cb->rempos);
#endif

    if (cb->buffer_size - cb->remaining_capacity < len) {
        critical_leave(&cb->mutex);
        return CB_EMPTY;
    }

    const size_t oldrempos = cb->rempos;
    cb->rempos = (oldrempos + len) % cb->buffer_size; // calculate new position

    if (cb->rempos <= oldrempos) {
        // we have to wrap around
        const size_t remaining = cb->buffer_size - oldrempos;
        memcpy(in, (void *) &cb->buffer[oldrempos], remaining*sizeof(float));
        memcpy(&in[remaining], (void *) cb->buffer, cb->rempos*sizeof(float));
    } else {
        // we don't have to wrap around
        memcpy(in, (void *) &cb->buffer[oldrempos], len*sizeof(float));
    }

    cb->remaining_capacity += len; // we have removed len items

    critical_leave(&cb->mutex);

    return CB_OK;
}

int cb_rem_blocking(CircBuff_t * cb, float * in, const size_t len) {
	if (cb->invalid) return CB_ERROR;
	if (len <= 0) return CB_OK;

	size_t items_inside = cb->buffer_size - cb->remaining_capacity;
    while (items_inside < len) {
            // if the size of the buffer is not large enough, request a resize during the next add
            if (len*cb->size_coeff > cb->buffer_size) cb->desired_buf_size = len*cb->size_coeff;

            const size_t before_items_inside = items_inside;
            cb->is_waiting = 1;
            if (mutex_wait(&cb->locker) == THREAD_TIMEOUT) {
            	cb->is_waiting = 0;
            	return CB_EMPTY;
            }

            cb->is_waiting = 0;
            items_inside = cb->buffer_size - cb->remaining_capacity;
            if (before_items_inside == items_inside)
                return CB_EMPTY; // if there are not enough items
    }

    if (cb->invalid) return CB_ERROR;
    critical_enter(&cb->mutex);

#if ASSERT_ENABLED
    assert(((cb->pos + cb->remaining_capacity) % cb->buffer_size) == cb->rempos);
#endif

    if (cb->buffer_size - cb->remaining_capacity < len) {
        critical_leave(&cb->mutex);
        return CB_EMPTY;
    }

    const size_t oldrempos = cb->rempos;
    cb->rempos = (oldrempos + len) % cb->buffer_size; // calculate new position

    if (cb->rempos <= oldrempos) {
        // we have to wrap around
        const size_t remaining = cb->buffer_size - oldrempos;
        memcpy(in, (void *) &cb->buffer[oldrempos], remaining*sizeof(float));
        memcpy(&in[remaining], (void *) cb->buffer, cb->rempos*sizeof(float));
    } else {
        // we don't have to wrap around
        memcpy(in, (void *) &cb->buffer[oldrempos], len*sizeof(float));
    }

    cb->remaining_capacity += len; // we have removed len items

    critical_leave(&cb->mutex);

    return CB_OK;
}

void cb_free(CircBuff_t * cb) {
	if (cb->invalid) return;

	critical_enter(&cb->mutex);
	free((void *) cb->buffer);
	cb->invalid = 1;

	if (cb->is_waiting) mutex_signal(&cb->locker);

	critical_leave(&cb->mutex);

    mutex_free(&cb->locker);
    mutex_free(&cb->mutex);

}

