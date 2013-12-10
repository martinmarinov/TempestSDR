#include "timer.h"

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
		return 0;
	} else {
		timersub(&now, &timer->start, &elapsed);
		timer->start = now;

		return (float) elapsed.tv_sec + (float) (elapsed.tv_usec) * (float) 1e-6;;
	}
}
