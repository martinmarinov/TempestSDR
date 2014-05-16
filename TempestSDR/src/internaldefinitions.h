/*******************************************************************************
 * Copyright (c) 2014 Martin Marinov.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Public License v3.0
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/gpl.html
 *
 * Contributors:
 *     Martin Marinov - initial API and implementation
 ******************************************************************************/
#ifndef INTERNALDEFINITIONS_H_
#define INTERNALDEFINITIONS_H_

#include "threading.h"
#include "circbuff.h"
#include "TSDRPluginLoader.h"
#include "frameratedetector.h"
#include "superbandwidth.h"

#include "extbuffer.h"
#include "dsp.h"

#include "include/TSDRLibrary.h"

#include <stdlib.h>
#include <stddef.h>

#define MALLOC_OR_REALLOC(buffer, size, type) ((buffer == NULL) ? ((type *) malloc(size*sizeof(type))) : ((type *) realloc((void *) buffer, size*sizeof(type))))

	struct tsdr_lib {
		pluginsource_t plugin;
		semaphore_t threadsync;
		mutex_t stopsync;
		uint32_t samplerate;
		uint32_t samplerate_real;
		int width;
		int height;
		double pixelrate;
		double refreshrate;
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

		uint32_t params_int[COUNT_PARAM_INT];
		double params_double[COUNT_PARAM_DOUBLE];

		frameratedetector_t frameratedetect;

		tsdr_value_changed_callback callback;
		tsdr_on_plot_ready_callback plotready_callback;
		void * callbackctx;

		superbandwidth_t super;

		dsp_postprocess_t dsp_postprocess;
		dsp_resample_t dsp_resample;

	};

	void announce_callback_changed(tsdr_lib_t * tsdr, int value_id, double arg0, double arg1);
	void announce_plotready(tsdr_lib_t * tsdr, int plot_id, extbuffer_t * buffer, uint32_t data_size, uint32_t data_offset, uint32_t samplerate);

	void shiftfreq(tsdr_lib_t * tsdr, int32_t diff);
	void set_internal_samplerate(tsdr_lib_t * tsdr, uint32_t samplerate);

	void unloadplugin(tsdr_lib_t * tsdr);

#endif
