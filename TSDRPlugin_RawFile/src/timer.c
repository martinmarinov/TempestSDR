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
#include "timer.h"
#include <stdlib.h>

#if WINHEAD
int gettimeofday(struct timeval *tv, void * ignoreme)
{
  FILETIME ft;
  unsigned __int64 tmpres = 0;

  if (NULL != tv)
  {
    GetSystemTimeAsFileTime(&ft);

    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;

    tmpres /= 10;  /*convert into microseconds*/
    /*converting file time to unix epoch*/
    tmpres -= DELTA_EPOCH_IN_MICROSECS;
    tv->tv_sec = (long)(tmpres / 1000000UL);
    tv->tv_usec = (long)(tmpres % 1000000UL);
  }

  return 0;
}

void timersub(const struct timeval *val1,
                                 const struct timeval *val2,
                                 struct timeval *res)
{
    res->tv_sec = val1->tv_sec - val2->tv_sec;
    if (val1->tv_usec - val2->tv_usec < 0) {
        res->tv_sec--;
        res->tv_usec = val1->tv_usec - val2->tv_usec + 1000 * 1000;
    } else {
        res->tv_usec = val1->tv_usec - val2->tv_usec;
    }
}
#endif

void timer_tick(TickTockTimer_t * timer) {
	timer->started = 1;
	gettimeofday(&timer->start, NULL);
}

float timer_tock(TickTockTimer_t * timer) {
	struct timeval now;
	struct timeval elapsed;

	if (!timer->started) return -1;

	gettimeofday(&now, NULL);
	timersub(&now, &timer->start, &elapsed);

	return (float) elapsed.tv_sec + (float) (elapsed.tv_usec) * (float) 1e-6;
}

float timer_ticktock(TickTockTimer_t * timer) {
	struct timeval now;
	struct timeval elapsed;

	gettimeofday(&now, NULL);

	if (!timer->started) {
		timer->start = now;
		timer->started = 1;
	} else {
		timersub(&now, &timer->start, &elapsed);
		timer->start = now;

		return (float) elapsed.tv_sec + (float) (elapsed.tv_usec) * (float) 1e-6;
	}
	return 0;
}
