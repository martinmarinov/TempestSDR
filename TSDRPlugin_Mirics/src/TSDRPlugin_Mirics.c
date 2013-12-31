#include <stdio.h>
#include <string.h>

#include "TSDRPlugin.h"
#include "TSDRCodes.h"

#include <mir_sdr.h>

#include <stdint.h>

void tsdrplugin_getName(char * name) {
	strcpy(name, "TSDR Mirics SDR Plugin");
}

uint32_t tsdrplugin_setsamplerate(uint32_t rate) {
	printf("Mirics setsamplerate %d\n", rate);
	return TSDR_NOT_IMPLEMENTED;
}

uint32_t tsdrplugin_getsamplerate() {
	printf("Mirics getsamplerate\n");
	return TSDR_NOT_IMPLEMENTED;
}

int tsdrplugin_setbasefreq(uint32_t freq) {
	printf("Mirics setbasefreq %d\n", freq);
	return TSDR_NOT_IMPLEMENTED;
}

int tsdrplugin_stop(void) {
	printf("Mirics stop\n");
	return TSDR_NOT_IMPLEMENTED;
}

int tsdrplugin_setgain(float gain) {
	printf("Mirics setgain %.4f\n", gain);
	return TSDR_NOT_IMPLEMENTED;
}

int tsdrplugin_setParams(const char * params) {
	float ver;
	int err;

	err = mir_sdr_ApiVersion(&ver);
	printf("MIRICS VERSION %.4f with err %d\n", ver, err);
	printf("Mirics setParams %s\n", params);
	return TSDR_NOT_IMPLEMENTED;
}

int tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx) {
	printf("Mirics Readasync\n");
	return TSDR_NOT_IMPLEMENTED;
}
