#ifndef _TSDRLibrary
#define _TSDRLibrary

	#include <stdint.h>

	struct tsdr_lib {
		void * plugin;
	} typedef tsdr_lib_t;

	typedef void(*tsdr_readasync_function)(float *buf, uint32_t len, void *ctx);

	int tsdr_loadplugin(tsdr_lib_t * tsdr, char * filepath);
	int tsdr_pluginparams(tsdr_lib_t * tsdr, char * params);
	int tsdr_setsamplerate(tsdr_lib_t * tsdr, uint32_t rate);
	int tsdr_setbasefreq(tsdr_lib_t * tsdr, uint32_t freq);
	int tsdr_stop(tsdr_lib_t * tsdr);
	int tsdr_setgain(tsdr_lib_t * tsdr, float gain);
	int tsdr_readasync(tsdr_lib_t * tsdr, tsdr_readasync_function cb, void *ctx, uint32_t buf_num, uint32_t buf_len);
	int tsdr_unloadplugin(tsdr_lib_t * tsdr);

#endif
