#ifndef _TSDRPluginHeader
#define _TSDRPluginHeader

	#include <stdint.h>

	typedef void(*tsdrplugin_readasync_function)(float *buf, uint32_t len, void *ctx, int dropped_samples);

	void tsdrplugin_getName(char *);
	int tsdrplugin_setParams(const char * params);
	uint32_t tsdrplugin_setsamplerate(uint32_t rate);
	uint32_t tsdrplugin_getsamplerate();
	int tsdrplugin_setbasefreq(uint32_t freq);
	int tsdrplugin_stop(void);
	int tsdrplugin_setgain(float gain);
	char * tsdrplugin_getlasterrortext(void);
	int tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx);

#endif
