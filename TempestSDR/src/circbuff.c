#include "circbuff.h"
#include <errno.h>

#define SIZE_COEFF (10)

void cb_init(CircBuff_t * cb) {
	cb->size = 0;
	cb->pos = 0;
	cb->bufsize = 1;
	cb->bufsizereduced = cb->bufsize / SIZE_COEFF;
	cb->buffer = (float *) malloc(sizeof(float));

	cb->isblocked = 0;

	mutex_init(&cb->mutex);
	mutex_init(&cb->second);
	mutex_init(&cb->third);
}

int cb_add(CircBuff_t * cb, float * in, const int len) {

	if (len > cb->bufsizereduced) {
		critical_enter(&cb->second);
		cb->bufsize = len*SIZE_COEFF;
		cb->bufsizereduced = cb->bufsize / SIZE_COEFF;
		cb->buffer = (float *) realloc(cb->buffer, cb->bufsize*sizeof(float));
		critical_leave(&cb->second);
	}

	critical_enter(&cb->third);

	const int size = cb->size;
	const int bufsize = cb->bufsize;
	const int putpos = (cb->pos + size) % bufsize;

	if (size + len > bufsize)
		return CB_FULL;

	if (putpos + len <= bufsize)
		memcpy(&cb->buffer[putpos], in, len*sizeof(float));
	else {
		const int rem = bufsize - putpos;
		const int rem2 = len - rem;

		memcpy(&cb->buffer[putpos], in, rem*sizeof(float));
		memcpy(cb->buffer, &in[rem], rem2*sizeof(float));
	}
	cb->size += len;

	critical_leave(&cb->third);

	if (cb->isblocked)
		mutex_signal(&cb->mutex);

	return CB_OK;
}

int cb_rem_blocking(CircBuff_t * cb, float * in, const int len) {

	if (len > cb->bufsizereduced) {
		critical_enter(&cb->third);
		cb->bufsize = len*SIZE_COEFF;
		cb->bufsizereduced = cb->bufsize / SIZE_COEFF;
		cb->buffer = (float *) realloc(cb->buffer, cb->bufsize*sizeof(float));
		critical_leave(&cb->third);
	}

	if (cb->size < len) {
		cb->isblocked = 1;

		const int res = mutex_wait(&cb->mutex);

		cb->isblocked = 0;

		if (res == THREAD_TIMEOUT)
			return CB_EMPTY;

	}

	if (cb->size < len)
		return CB_EMPTY;

	critical_enter(&cb->second);

	const int size = cb->size;
	const int pos = cb->pos;
	const int nsize = size - len;

	if (pos + len <= cb->bufsize)
		memcpy(in, &cb->buffer[pos], len*sizeof(float));
	else {
		const int rem = cb->bufsize - pos;
		const int rem2 = len - rem;

		memcpy(in, &cb->buffer[pos], rem*sizeof(float));
		memcpy(&in[rem], cb->buffer, rem2*sizeof(float));
	}

	cb->pos = (pos + len) % cb->bufsize;
	cb->size = (nsize < 0) ? 0 : (int) nsize;

	critical_leave(&cb->second);
	return CB_OK;
}

void cb_free(CircBuff_t * cb) {
	free(cb->buffer);
	mutex_free(&cb->mutex);
	mutex_free(&cb->second);
	mutex_free(&cb->third);
}

