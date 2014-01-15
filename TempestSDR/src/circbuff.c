#include "circbuff.h"
#include <errno.h>

#define SIZE_COEFF (20)

#define DEBUG (0)

void cb_init(CircBuff_t * cb) {
    cb->desired_buf_size = SIZE_COEFF; // initial size of buffer
    cb->buffer_size = cb->desired_buf_size;
    cb->buffer = (float *) malloc(sizeof(float) * cb->buffer_size); // allocate buffer
    cb->remaining_capacity = SIZE_COEFF; // how many elements could be loaded
    cb->pos = 0; // where the next element will be added
    cb->rempos = 0; // where the next element will be taken from

    cb->is_waiting = 0;

    mutex_init(&cb->mutex);
    mutex_init(&cb->locker);
}

int cb_add(CircBuff_t * cb, float * in, const int len) {

    if (len <= 0) return CB_OK; // handle edge case

    // if the size of the buffer is not large enough, request the buffer to be resized
    if (len*SIZE_COEFF > cb->buffer_size) cb->desired_buf_size = len*SIZE_COEFF;

    if (cb->buffer_size < cb->desired_buf_size) {
        // if we need to resize the buffer
        const int diff = cb->desired_buf_size - cb->buffer_size;
        if (cb->remaining_capacity == 0) cb->pos = cb->buffer_size; // if the buffer is full, the next element is here
        cb->remaining_capacity += diff; // expand the remaining capacity to match
        cb->buffer = (float *) realloc((void *) cb->buffer, sizeof(float) * cb->desired_buf_size); // reallocate
        cb->buffer_size = cb->desired_buf_size; // set the size
    }

    if (cb->remaining_capacity < len) return CB_FULL; // if there is not enough space to put buffer, return error

    critical_enter(&cb->mutex);

#if DEBUG
    printf("Adding "); showbuf(in, len);
#endif

    const int oldpos = cb->pos;
    cb->pos = (oldpos + len) % cb->buffer_size; // calculate new position
    cb->remaining_capacity -= len; // the remaining capacity is reduced

    if (cb->pos <= oldpos) {
        // the add will wrap around
        const int remaining = cb->buffer_size - oldpos;
        memcpy(&cb->buffer[oldpos], in, remaining * sizeof(float));
        memcpy(cb->buffer, &in[remaining], cb->pos * sizeof(float));
    } else {
        // the add will not wrap around
        memcpy(&cb->buffer[oldpos], in, len*sizeof(float));
    }

#if DEBUG
    printf("After add result "); showcb();
#endif

    if (cb->is_waiting) mutex_signal(&cb->locker);

    critical_leave(&cb->mutex);

    return CB_OK;
}

int cb_rem_blocking(CircBuff_t * cb, float * in, const int len) {
    if (len*SIZE_COEFF > cb->buffer_size) {
        // if the size of the buffer is not large enough, request a resize during the next add
        cb->desired_buf_size = len*SIZE_COEFF;
        return CB_EMPTY;
    }

    int items_inside = cb->buffer_size - cb->remaining_capacity;
    while (items_inside < len) {
            const int before_items_inside = items_inside;
            mutex_wait(&cb->locker);
            items_inside = cb->buffer_size - cb->remaining_capacity;
            if (before_items_inside == items_inside)
                return CB_EMPTY; // if there are not enough items
    }

    critical_enter(&cb->mutex);

    const int oldrempos = cb->rempos;
    cb->rempos = (oldrempos + len) % cb->buffer_size; // calculate new position

    if (cb->rempos <= oldrempos) {
        // we have to wrap around
        const int remaining = cb->buffer_size - oldrempos;
        memcpy(in, &cb->buffer[oldrempos], remaining*sizeof(float));
        memcpy(&in[remaining], cb->buffer, cb->rempos*sizeof(float));
    } else {
        // we don't have to wrap around
        memcpy(in, &cb->buffer[oldrempos], len*sizeof(float));
    }

    cb->remaining_capacity += len; // we have removed len items

    critical_leave(&cb->mutex);

#if DEBUG
    printf("After rem result "); showbuf(in, len);
#endif

    return CB_OK;
}

void cb_free(CircBuff_t * cb) {
    mutex_free(&cb->locker);
    mutex_free(&cb->mutex);
    free(cb->buffer);
}

