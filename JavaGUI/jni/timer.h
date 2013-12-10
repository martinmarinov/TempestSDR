#ifndef TIMR_M_H
#define TIMR_M_H
#include "osdetect.h"
#include <sys/time.h>

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
