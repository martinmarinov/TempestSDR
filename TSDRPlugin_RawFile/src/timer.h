#ifndef TIMR_M_H
#define TIMR_M_H
#include "osdetect.h"

#if WINHEAD
	#include <windows.h>
	#include <time.h>

	#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
		#define DELTA_EPOCH_IN_MICROSECS  116444736000000000Ui64 // CORRECT
	#else
		#define DELTA_EPOCH_IN_MICROSECS  116444736000000000ULL // CORRECT
	#endif

#else
	#include <sys/time.h>
#endif

typedef struct TickTockTimer TickTockTimer_t;

struct TickTockTimer
{
	struct timeval start;
	int started;
};

void timer_tick(TickTockTimer_t * timer);
float timer_tock(TickTockTimer_t * timer);
float timer_ticktock(TickTockTimer_t * timer);

#endif
