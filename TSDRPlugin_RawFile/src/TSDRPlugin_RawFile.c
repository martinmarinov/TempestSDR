#include <stdio.h>
#include <string.h>

#include "TSDRPlugin.h"
#include "TSDRCodes.h"

#include "timer.h"

#define MAX_ERRORS (5)

#define TYPE_FLOAT (0)
#define TYPE_BYTE (1)
#define TYPE_SHORT (2)

#define SAMPLES_TO_READ_AT_ONCE (800*600)

TickTockTimer_t timer;
volatile int working = 0;

int type = TYPE_FLOAT;
int sizepersample = 4; // matlab single TODO! change via parameter

void tsdrplugin_getName(char * name) {
	strcpy(name, "TSDR Raw File Source Plugin");
}

int tsdrplugin_setsamplerate(uint32_t rate) {
	return TSDR_NOT_IMPLEMENTED;
}

int tsdrplugin_setbasefreq(uint32_t freq) {
	return TSDR_NOT_IMPLEMENTED;
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

	while (working) {
		size_t to_read = bytestoread;
		size_t total_read = 0;
		int errors = 0;

		while (to_read) {
			size_t read = fread(&buf[total_read],1,to_read,file);
			to_read -= read;
			total_read += read;

			if (read == 0 && errors++ > MAX_ERRORS) {
				working = 0;
				break;
			}
		}

		if (working) {

			if (type == TYPE_FLOAT) {
				memccpy(outbuf, buf, 0, bytestoread);
			}

			cb(outbuf, SAMPLES_TO_READ_AT_ONCE, ctx);
		}
	}

	free(buf);
	free(outbuf);
	fclose(file);

	return TSDR_OK;
}
