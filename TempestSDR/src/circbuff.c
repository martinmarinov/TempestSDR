#include "circbuff.h"
#include <errno.h>

void cb_init(CircBuff_t * cb, int size) {
	cb->size = 0;
	cb->pos = 0;
	cb->bufsize = size;
	cb->buffer = (float *) malloc(size*sizeof(float));

	cb->isblocked = 0;

	mutex_init(&(cb->mutex));
}

int cb_add(CircBuff_t * cb, float * in, const int len) {
	const int size = cb->size;
	const int bufsize = cb->bufsize;
	const int putpos = (cb->pos + size) % bufsize;
	float * buffer = cb->buffer;

	if (size + len > bufsize)
		return CB_FULL;

	if (putpos + len <= bufsize)
		memcpy(&buffer[putpos], in, len*sizeof(float));
	else {
		const int rem = bufsize - putpos;
		const int rem2 = len - rem;

		memcpy(&buffer[putpos], in, rem*sizeof(float));
		memcpy(buffer, &in[rem], rem2*sizeof(float));
	}
	cb->size += len;

	if (cb->isblocked)
		mutex_signal(&(cb->mutex));

	return CB_OK;
}

int cb_rem_blocking(CircBuff_t * cb, float * in, const int len) {
	int size = cb->size;

	if (size < len) {
		cb->isblocked = 1;

		const int res = mutex_wait(&(cb->mutex));

		cb->isblocked = 0;

		if (res == THREAD_TIMEOUT)
			return CB_EMPTY;

		size = cb->size;

	}

	const int pos = cb->pos;
	const int bufsize = cb->bufsize;
	float * buffer = cb->buffer;
	const int nsize = size - len;

	if (pos + len <= bufsize)
		memcpy(in, &buffer[pos], len*sizeof(float));
	else {
		const int rem = bufsize - pos;
		const int rem2 = len - rem;

		memcpy(in, &buffer[pos], rem*sizeof(float));
		memcpy(&in[rem], buffer, rem2*sizeof(float));
	}

	cb->pos = (pos + len) % bufsize;
	cb->size = (nsize < 0) ? 0 : (int) nsize;

	return CB_OK;
}

int cb_rem(CircBuff_t * cb, float * in, const int len) {
	const int size = cb->size;

	if (size < len)
		return CB_EMPTY;

	const int pos = cb->pos;
	const int bufsize = cb->bufsize;
	float * buffer = cb->buffer;
	const int nsize = size - len;

	if (pos + len <= bufsize)
		memcpy(in, &buffer[pos], len*sizeof(float));
	else {
		const int rem = bufsize - pos;
		const int rem2 = len - rem;

		memcpy(in, &buffer[pos], rem*sizeof(float));
		memcpy(&in[rem], buffer, rem2*sizeof(float));
	}

	cb->pos = (pos + len) % bufsize;
	cb->size = (nsize < 0) ? 0 : (int) nsize;

	return CB_OK;
}

void cb_free(CircBuff_t * cb) {
	free(cb->buffer);
	mutex_free(&cb->mutex);
}

