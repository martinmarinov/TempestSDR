#include <stdio.h>
#include <string.h>

#include "TSDRPlugin.h"
#include "TSDRCodes.h"

#include "timer.h"

TickTockTimer_t timer;

void tsdrplugin_getName(char * name) {
	strcpy(name, "TSDR Raw File Source Plugin");
}

int tsdrplugin_init(char * params) {
	return TSDR_NOT_IMPLEMENTED;
}

int tsdrplugin_setsamplerate(uint32_t rate) {
	return TSDR_NOT_IMPLEMENTED;
}

int tsdrplugin_setbasefreq(uint32_t freq) {
	return TSDR_NOT_IMPLEMENTED;
}

int tsdrplugin_stop(void) {
	return TSDR_OK;
}

int tsdrplugin_setgain(float gain) {
	return TSDR_NOT_IMPLEMENTED;
}

int tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx, uint32_t buf_num, uint32_t buf_len, char * params) {
	return TSDR_NOT_IMPLEMENTED;
}
