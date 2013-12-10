#ifndef _TSDRPluginHeader
#define _TSDRPluginHeader

	#include <stdint.h>

	typedef void(*tsdrplugin_readasync_function)(float *buf, uint32_t len, void *ctx);

	void tsdrplugin_getName(char *);
	int tsdrplugin_init(char * params);
	int tsdrplugin_setsamplerate(uint32_t rate);
	int tsdrplugin_setbasefreq(uint32_t freq);
	int tsdrplugin_stop(void);
	int tsdrplugin_setgain(float gain);
	int tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx, uint32_t buf_num, uint32_t buf_len, char * params);

#endif
