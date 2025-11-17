/*
#-------------------------------------------------------------------------------
# Copyright (c) 2014 Martin Marinov.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the GNU Public License v3.0
# which accompanies this distribution, and is available at
# http://www.gnu.org/licenses/gpl.html
# 
# Contributors:
#     Martin Marinov - initial API and implementation
#-------------------------------------------------------------------------------
*/
#ifndef _TSDRPluginHeader
#define _TSDRPluginHeader

#ifdef __cplusplus
extern "C" {
#endif

#if _MSC_VER && !__INTEL_COMPILER
  #define TSDRPLUGIN_API __declspec(dllexport)
#elif defined _WIN32 || defined __CYGWIN__
  #ifdef __GNUC__
    #define TSDRPLUGIN_API __attribute__ ((dllexport))
  #else
    #define TSDRPLUGIN_API __declspec(dllexport)
  #endif
#else
  #if __GNUC__ >= 4
    #define TSDRPLUGIN_API __attribute__ ((visibility ("default")))
  #else
    #define TSDRPLUGIN_API
  #endif
#endif


	#ifdef __cplusplus
		#define EXTERNC extern "C"
	#else
		#define EXTERNC
	#endif

	#if !(defined(OS_WINDOWS) || defined(_WIN64)  || defined(_WIN32)  || defined(_MSC_VER) || defined(__stdcall))
		#define __stdcall
	#endif

	#include <stdint.h>

	typedef void(*tsdrplugin_readasync_function)(float *buf, uint64_t items_count, void *ctx, int64_t samples_dropped);

	EXTERNC TSDRPLUGIN_API void __stdcall tsdrplugin_getName(char *);
	EXTERNC TSDRPLUGIN_API int __stdcall tsdrplugin_init(const char * params);
	EXTERNC TSDRPLUGIN_API uint32_t __stdcall tsdrplugin_setsamplerate(uint32_t rate);
	EXTERNC TSDRPLUGIN_API uint32_t __stdcall tsdrplugin_getsamplerate();
	EXTERNC TSDRPLUGIN_API int __stdcall tsdrplugin_setbasefreq(uint32_t freq);
	EXTERNC TSDRPLUGIN_API int __stdcall tsdrplugin_stop(void);
	EXTERNC TSDRPLUGIN_API int __stdcall tsdrplugin_setgain(float gain);
	EXTERNC TSDRPLUGIN_API char* __stdcall tsdrplugin_getlasterrortext(void);
	EXTERNC TSDRPLUGIN_API int __stdcall tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx);
	EXTERNC TSDRPLUGIN_API void __stdcall tsdrplugin_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif
