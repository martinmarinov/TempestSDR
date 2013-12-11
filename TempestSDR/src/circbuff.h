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
	float * buffer;
	int pos;
	int bufsize;
	volatile int size;

	volatile int isblocked;
	mutex_t mutex;
};

void cb_init(CircBuff_t * cb, int size);
int cb_add(CircBuff_t * cb, float * buff, const int size);
int cb_rem(CircBuff_t * cb, float * buff, const int size);
int cb_rem_blocking(CircBuff_t * cb, float * in, const int len);
void cb_free(CircBuff_t * cb);

#endif
