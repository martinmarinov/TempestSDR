#ifndef _TSDRLibrary
#define _TSDRLibrary

	#include <stdint.h>

	struct tsdr_lib {
		void * plugin;
		void * mutex_sync_unload;
		void * mutex_video_stopped;
		uint32_t samplerate;
		double sampletime;
		int width;
		int height;
		double pixelrate;
		double pixeltime;
		double pixeltimeoversampletime;
		volatile int running;
		double offset;
		double contributionfromlast;
	} typedef tsdr_lib_t;

	typedef void(*tsdr_readasync_function)(float *buf, int width, int height, void *ctx);

	int tsdr_setsamplerate(tsdr_lib_t * tsdr, uint32_t rate);
	int tsdr_setbasefreq(tsdr_lib_t * tsdr, uint32_t freq);
	int tsdr_stop(tsdr_lib_t * tsdr);
	int tsdr_setgain(tsdr_lib_t * tsdr, float gain);
	int tsdr_readasync(tsdr_lib_t * tsdr, const char * pluginfilepath, tsdr_readasync_function cb, void *, const char * params);
	int tsdr_unloadplugin(tsdr_lib_t * tsdr);
	int tsdr_setresolution(tsdr_lib_t * tsdr, int width, int height, double refreshrate);

#endif
