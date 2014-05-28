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
#ifndef _TSDRLibrary
#define _TSDRLibrary

	#include <stdint.h>

	// those PIXEL_* are only used for debugging purposes and
	// it is not required for the client to implement them
	#define PIXEL_SPECIAL_VALUE_R (256.0f)
	#define PIXEL_SPECIAL_VALUE_G (512.0f)
	#define PIXEL_SPECIAL_VALUE_B (1024.0f)
	#define PIXEL_SPECIAL_VALUE_TRANSPARENT (2048.0f)
	#define PIXEL_SPECIAL_COLOURS_ENABLED (1)

	#define DIRECTION_CUSTOM (0)
	#define DIRECTION_UP (1)
	#define DIRECTION_DOWN (2)
	#define DIRECTION_LEFT (3)
	#define DIRECTION_RIGHT (4)

	#define PARAM_INT_AUTOSHIFT (0)
	#define PARAM_INT_FRAMERATE_PLL (1)
	#define PARAM_AUTOCORR_PLOTS_RESET (2)
	#define PARAM_AUTOCORR_PLOTS_OFF (3)
	#define PARAM_AUTOCORR_SUPERRESOLUTION (4)
	#define PARAM_NEAREST_NEIGHBOUR_RESAMPLING (5)
	#define PARAM_LOW_PASS_BEFORE_SYNC (6)
	#define PARAM_AUTOGAIN_AFTER_PROCESSING (7)
	#define PARAM_AUTOCORR_DUMP (8)
	#define COUNT_PARAM_INT (9)

	#define COUNT_PARAM_DOUBLE (2)

	#define VALUE_ID_PLL_FRAMERATE (0)
	#define VALUE_ID_AUTOCORRECT_RESET (1)
	#define VALUE_ID_AUTOCORRECT_FRAMES_COUNT (2)
	#define VALUE_ID_AUTOGAIN_VALUES (3)
	#define VALUE_ID_SNR (4)
	#define VALUE_ID_AUTOCORRECT_DUMPED (5)

	#define PLOT_ID_FRAME (0)
	#define PLOT_ID_LINE (1)

	typedef struct tsdr_lib tsdr_lib_t;

	typedef void(*tsdr_readasync_function)(float *buf, int width, int height, void *ctx);
	typedef void(*tsdr_value_changed_callback)(int value_id, double arg0, double arg1, void * ctx);
	typedef void(*tsdr_on_plot_ready_callback)(int plot_id, int offset, double * values, int size, uint32_t samplerate, void * ctx);

	void tsdr_init(tsdr_lib_t ** tsdr, tsdr_value_changed_callback callback, tsdr_on_plot_ready_callback plotready_callback, void * ctx);
	void * tsdr_getctx(tsdr_lib_t * tsdr);
	int tsdr_setbasefreq(tsdr_lib_t * tsdr, uint32_t freq);
	int tsdr_stop(tsdr_lib_t * tsdr);
	int tsdr_setgain(tsdr_lib_t * tsdr, float gain);
	int tsdr_readasync(tsdr_lib_t * tsdr, tsdr_readasync_function cb, void * ctx);
	int tsdr_loadplugin(tsdr_lib_t * tsdr, const char * pluginfilepath, const char * params);
	int tsdr_unloadplugin(tsdr_lib_t * tsdr);
	int tsdr_setresolution(tsdr_lib_t * tsdr, int height, double refreshrate);
	int tsdr_isrunning(tsdr_lib_t * tsdr);
	int tsdr_sync(tsdr_lib_t * tsdr, int pixels, int direction);
	int tsdr_motionblur(tsdr_lib_t * tsdr, float coeff);
	void tsdr_free(tsdr_lib_t ** tsdr);
	char * tsdr_getlasterrortext(tsdr_lib_t * tsdr);
	int tsdr_setparameter_int(tsdr_lib_t * tsdr, int parameter, uint32_t value);
	int tsdr_setparameter_double(tsdr_lib_t * tsdr, int parameter, double value);

#endif
