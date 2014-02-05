#ifndef _TSDRPluginHeader
#define _TSDRPluginHeader

	#ifdef __cplusplus
		#define EXTERNC extern "C"
	#else
		#define EXTERNC
	#endif

	#include <stdint.h>

	typedef void(*tsdrplugin_readasync_function)(float *buf, uint32_t len, void *ctx, int dropped_samples);

	EXTERNC void tsdrplugin_getName(char *);
	EXTERNC int tsdrplugin_init(const char * params);
	EXTERNC uint32_t tsdrplugin_setsamplerate(uint32_t rate);
	EXTERNC uint32_t tsdrplugin_getsamplerate();
	EXTERNC int tsdrplugin_setbasefreq(uint32_t freq);
	EXTERNC int tsdrplugin_stop(void);
	EXTERNC int tsdrplugin_setgain(float gain);
	EXTERNC char * tsdrplugin_getlasterrortext(void);
	EXTERNC int tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx);
	EXTERNC void tsdrplugin_cleanup(void);

#endif
