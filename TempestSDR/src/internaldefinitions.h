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

#include "include/TSDRLibrary.h"

	struct tsdr_lib {
		pluginsource_t plugin;
		semaphore_t threadsync;
		mutex_t stopsync;
		uint32_t samplerate;
		double sampletime;
		int width;
		int height;
		double pixelrate;
		double refreshrate;
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

		uint32_t params_int[COUNT_PARAM_INT];
		double params_double[COUNT_PARAM_DOUBLE];

		float * fft_buffer;
		int fft_size;
		volatile int fft_exception;
		volatile int fft_requested;
		mutex_t fft_mutex;

		frameratedetector_t frameratedetect;

		tsdr_value_changed_callback callback;
		void * callbackctx;
	};

	void announce_callback_changed(tsdr_lib_t * tsdr, int value_id, double arg0, int arg1);

#endif
