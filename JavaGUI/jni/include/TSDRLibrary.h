#ifndef _TSDRLibrary
#define _TSDRLibrary

	#include <stdint.h>

	struct tsdr_lib {
		void * plugin;
		void * mutex_sync_unload;
		uint32_t samplerate;
		int width;
		int height;
		float vf;
		float hf;
		volatile int running;
	} typedef tsdr_lib_t;

	typedef void(*tsdr_readasync_function)(float *buf, int width, int height, void *ctx);

	int tsdr_loadplugin(tsdr_lib_t * tsdr, const char * filepath);
	int tsdr_setsamplerate(tsdr_lib_t * tsdr, uint32_t rate);
	int tsdr_setbasefreq(tsdr_lib_t * tsdr, uint32_t freq);
	int tsdr_stop(tsdr_lib_t * tsdr);
	int tsdr_setgain(tsdr_lib_t * tsdr, float gain);
	int tsdr_readasync(tsdr_lib_t * tsdr, tsdr_readasync_function cb, void *, const char * params);
	int tsdr_unloadplugin(tsdr_lib_t * tsdr);
	int tsdr_setresolution(tsdr_lib_t * tsdr, int width, int height);
	int tsdr_setvfreq(tsdr_lib_t * tsdr, float freq);
	int tsdr_sethfreq(tsdr_lib_t * tsdr, float freq);

#endif
