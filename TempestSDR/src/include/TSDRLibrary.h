#ifndef _TSDRLibrary
#define _TSDRLibrary

#if defined _WIN32 || defined __CYGWIN__
  #define TSDR_LOCAL
#else
  #if __GNUC__ >= 4
    #define TSDR_PUBLIC __attribute__ ((visibility ("default")))
    #define TSDR_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define TSDR_PUBLIC
    #define TSDR_LOCAL
  #endif
#endif


	#include <stdint.h>

	#define DIRECTION_CUSTOM (0)
	#define DIRECTION_UP (1)
	#define DIRECTION_DOWN (2)
	#define DIRECTION_LEFT (3)
	#define DIRECTION_RIGHT (4)

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
		volatile int nativerunning;
		uint32_t centfreq;
		float gain;
		float motionblur;
		volatile int syncoffset;
		char * errormsg;
		int errormsg_size;
		int errormsg_code;
	} typedef tsdr_lib_t;

	typedef void(*tsdr_readasync_function)(float *buf, int width, int height, void *ctx);

	TSDR_PUBLIC void tsdr_init(tsdr_lib_t * tsdr);
	TSDR_PUBLIC int tsdr_setsamplerate(tsdr_lib_t * tsdr, uint32_t rate);
	TSDR_PUBLIC int tsdr_setbasefreq(tsdr_lib_t * tsdr, uint32_t freq);
	TSDR_PUBLIC int tsdr_stop(tsdr_lib_t * tsdr);
	TSDR_PUBLIC int tsdr_setgain(tsdr_lib_t * tsdr, float gain);
	TSDR_PUBLIC int tsdr_readasync(tsdr_lib_t * tsdr, const char * pluginfilepath, tsdr_readasync_function cb, void *, const char * params);
	TSDR_PUBLIC int tsdr_unloadplugin(tsdr_lib_t * tsdr);
	TSDR_PUBLIC int tsdr_setresolution(tsdr_lib_t * tsdr, int width, int height, double refreshrate);
	TSDR_PUBLIC int tsdr_isrunning(tsdr_lib_t * tsdr);
	TSDR_PUBLIC int tsdr_sync(tsdr_lib_t * tsdr, int pixels, int direction);
	TSDR_PUBLIC int tsdr_motionblur(tsdr_lib_t * tsdr, float coeff);
	TSDR_PUBLIC char * tsdr_getlasterrortext(tsdr_lib_t * tsdr);

#endif
