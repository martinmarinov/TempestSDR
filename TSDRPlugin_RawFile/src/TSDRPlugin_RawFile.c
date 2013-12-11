#include <stdio.h>
#include <string.h>

#include "TSDRPlugin.h"
#include "TSDRCodes.h"

#include <stdint.h>

#include "osdetect.h"
#include "timer.h"

#define MAX_ERRORS (5)

#define TYPE_FLOAT (0)
#define TYPE_BYTE (1)
#define TYPE_SHORT (2)

#define PERFORMANCE_BENCHMARK (0)
#define ENABLE_LOOP (0)

#define SAMPLES_TO_READ_AT_ONCE (512*1024)

TickTockTimer_t timer;
volatile int working = 0;

int type = TYPE_FLOAT;
int sizepersample = 4; // matlab single TODO! change via parameter

uint32_t samplerate = 100e6 / 4; // TODO! real sample rate

void thread_sleep(uint32_t milliseconds) {
#if WINHEAD
	Sleep(milliseconds);
#else
	usleep(1000 * milliseconds);
#endif
}

void tsdrplugin_getName(char * name) {
	strcpy(name, "TSDR Raw File Source Plugin");
}

uint32_t tsdrplugin_setsamplerate(uint32_t rate) {
	return samplerate;
}

uint32_t tsdrplugin_getsamplerate() {
	return samplerate;
}

int tsdrplugin_setbasefreq(uint32_t freq) {
	return TSDR_OK;
}

int tsdrplugin_stop(void) {
	working = 0;
	return TSDR_OK;
}

int tsdrplugin_setgain(float gain) {
	return TSDR_NOT_IMPLEMENTED;
}

int tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx, const char * params) {
	working = 1;
	size_t i;

	int counter;

	FILE * file=fopen(params,"rb");

	if (file == NULL)
		return TSDR_PLUGIN_PARAMETERS_WRONG;

	const size_t bytestoread = SAMPLES_TO_READ_AT_ONCE * sizepersample;
	char * buf = (char *) malloc(bytestoread);
	float * outbuf = (float *) malloc(sizeof(float) * SAMPLES_TO_READ_AT_ONCE);

#if !PERFORMANCE_BENCHMARK
	uint32_t delayms = (uint32_t) (1000.0f * (float) SAMPLES_TO_READ_AT_ONCE / (float) samplerate);
	if (delayms == 0) delayms = 1;
#endif

	while (working) {
		size_t to_read = bytestoread;
		size_t total_read = 0;
		int errors = 0;

		while (to_read) {
			size_t read = fread(&buf[total_read],1,to_read,file);
			to_read -= read;
			total_read += read;

			if (read == 0 && errors++ > MAX_ERRORS) {
#if ENABLE_LOOP
				rewind(file);
#else
				working = 0;
#endif
				break;
			}
		}

		if (working) {

			if (type == TYPE_FLOAT) {
				memcpy(outbuf, buf, bytestoread);
			}

			cb(outbuf, SAMPLES_TO_READ_AT_ONCE, ctx);

#if !PERFORMANCE_BENCHMARK
			const uint32_t timeelapsed = (uint32_t) (1000.0f*timer_ticktock(&timer));
			if (timeelapsed < delayms)
				thread_sleep(delayms-timeelapsed);
#endif
		}
	}

	free(buf);
	free(outbuf);
	fclose(file);

	return TSDR_OK;
}
