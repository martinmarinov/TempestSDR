#ifndef _TSDRPluginHeader
#define _TSDRPluginHeader

	#ifdef __cplusplus
		#define EXTERNC extern "C"
	#else
		#define EXTERNC
	#endif

	#if !(defined(OS_WINDOWS) || defined(_WIN64)  || defined(_WIN32)  || defined(_MSC_VER) || defined(__stdcall))
		#define __stdcall
	#endif

	#include <stdint.h>

	typedef void(*tsdrplugin_readasync_function)(float *buf, uint32_t len, void *ctx, int dropped_samples);

	EXTERNC void __stdcall tsdrplugin_getName(char *);
	EXTERNC int __stdcall tsdrplugin_init(const char * params);
	EXTERNC uint32_t __stdcall tsdrplugin_setsamplerate(uint32_t rate);
	EXTERNC uint32_t __stdcall tsdrplugin_getsamplerate();
	EXTERNC int __stdcall tsdrplugin_setbasefreq(uint32_t freq);
	EXTERNC int __stdcall tsdrplugin_stop(void);
	EXTERNC int __stdcall tsdrplugin_setgain(float gain);
	EXTERNC char * __stdcall tsdrplugin_getlasterrortext(void);
	EXTERNC int __stdcall tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx);
	EXTERNC void __stdcall tsdrplugin_cleanup(void);

#endif
