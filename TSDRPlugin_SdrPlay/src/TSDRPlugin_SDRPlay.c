/*#-------------------------------------------------------------------------------
# Copyright (c) 2014 Martin Marinov.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the GNU Public License v3.0
# which accompanies this distribution, and is available at
# http://wworg/licenses/gpl.html
# 
# Contributors:
#     Martin Marinov - initial API and implementation
#------------------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>

#include "TSDRPlugin.h"
#include "TSDRCodes.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(__CYGWIN__)
	#include <windows.h>
#endif
#include "sdrplay_api.h"

#include <stdint.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif

#define SAMPLE_RATE (8000000)

volatile int working = 0;

volatile double desiredfreq = 200;
volatile int desiredgainred = 40;

#define SAMPLES_TO_PROCESS_AT_ONCE (200)

// Global variables for callback handling
static tsdrplugin_readasync_function global_cb = NULL;
static void *global_ctx = NULL;
static int current_freq = 200;
static int current_gain = 40;

void streamCallback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params, unsigned int numSamples, unsigned int reset, void *cbContext) {
    if (!global_cb || !working) return;

    // Convert I/Q samples to interleaved float format
    float *outbuf = (float *)malloc(numSamples * 2 * sizeof(float));
    if (!outbuf) return;

    for (unsigned int i = 0; i < numSamples; i++) {
        outbuf[i*2] = xi[i] / 32767.0f;
        outbuf[i*2+1] = xq[i] / 32767.0f;
    }

    // Call the plugin callback
    global_cb(outbuf, numSamples * 2, global_ctx, params->numSamples - numSamples); // dropped calculation

    free(outbuf);
}

void eventCallback(sdrplay_api_EventT eventId, sdrplay_api_TunerSelectT tuner, sdrplay_api_EventParamsT *params, void *cbContext) {
    // Handle events like gain changes, etc.
    // For now, we mainly care about the stream starting/stopping
}

int errormsg_code;
char * errormsg;
int errormsg_size = 0;
#define RETURN_EXCEPTION(message, status) {announceexception(message, status); return status;}
#define RETURN_OK() {errormsg_code = TSDR_OK; return TSDR_OK;}

static inline void announceexception(const char * message, int status) {
	errormsg_code = status;
	if (status == TSDR_OK) return;

	const int length = strlen(message);
	if (errormsg_size == 0) {
			errormsg_size = length;
			errormsg = (char *) malloc(length+1);
		} else if (length > errormsg_size) {
			errormsg_size = length;
			errormsg = (char *) realloc((void*) errormsg, length+1);
		}
	strcpy(errormsg, message);
}

char TSDRPLUGIN_API __stdcall * tsdrplugin_getlasterrortext(void) {
	if (errormsg_code == TSDR_OK)
		return NULL;
	else
		return errormsg;
}

void TSDRPLUGIN_API __stdcall tsdrplugin_getName(char * name) {
	strcpy(name, "TSDR SDRplay SDR Plugin");
}

uint32_t TSDRPLUGIN_API __stdcall tsdrplugin_setsamplerate(uint32_t rate) {
	return SAMPLE_RATE;
}

uint32_t TSDRPLUGIN_API __stdcall tsdrplugin_getsamplerate() {
	return SAMPLE_RATE;
}

int TSDRPLUGIN_API __stdcall tsdrplugin_setbasefreq(uint32_t freq) {
	desiredfreq = freq;
	RETURN_OK();
}

int TSDRPLUGIN_API __stdcall tsdrplugin_stop(void) {
	working = 0;
	RETURN_OK();
}

int TSDRPLUGIN_API __stdcall tsdrplugin_setgain(float gain) {
	desiredgainred = 20 + (int) ((1.0f - gain) * (59-20));
	if (desiredgainred < 20) desiredgainred = 20;
	else if (desiredgainred > 59) desiredgainred = 59;
	RETURN_OK();
}

int TSDRPLUGIN_API __stdcall tsdrplugin_init(const char * params) {
	RETURN_OK();
}

int TSDRPLUGIN_API __stdcall tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx) {
	working = 1;

	sdrplay_api_ErrT err;
	float ver = 0.0;
	int sps, i, id;
	sdrplay_api_StreamCbParamsT streamParams;
	short *samplesBuffer = NULL;
	int inited = 0;

	sdrplay_api_DeviceT devs[6];
	sdrplay_api_DeviceT* device = NULL;
	sdrplay_api_DeviceParamsT *deviceParams = NULL;
	sdrplay_api_CallbackFnsT cbFns;
	sdrplay_api_RxChannelParamsT *chParams = NULL;

	double freq = desiredfreq;
	double gainred = desiredgainred;

	unsigned int ndevs = 0;

	// Initialize the API and check version
	err = sdrplay_api_Open();
	if (err != sdrplay_api_Success) {
		RETURN_EXCEPTION("Failed to open SDRplay API", TSDR_CANNOT_OPEN_DEVICE);
	}

	err = sdrplay_api_ApiVersion(&ver);
	if (ver < 3.07) {
		sdrplay_api_Close();
		RETURN_EXCEPTION("SDRplay API version too old, please update SDRplay drivers", TSDR_CANNOT_OPEN_DEVICE);
	}

	// Lock API while device selection in progress
	err = sdrplay_api_LockDeviceApi();
	if (err != sdrplay_api_Success) {
		sdrplay_api_Close();
		RETURN_EXCEPTION("Failed to lock device API", TSDR_CANNOT_OPEN_DEVICE);
	}

	// Get devices
	err = sdrplay_api_GetDevices(devs, &ndevs, sizeof(sdrplay_api_DeviceT) * 6);
	if (err != sdrplay_api_Success || ndevs == 0) {
		sdrplay_api_UnlockDeviceApi();
		sdrplay_api_Close();
		RETURN_EXCEPTION("No devices found", TSDR_CANNOT_OPEN_DEVICE);
	}

	// Select the device
	err = sdrplay_api_SelectDevice(device);
	if (err != sdrplay_api_Success) {
		sdrplay_api_UnlockDeviceApi();
		sdrplay_api_Close();
		RETURN_EXCEPTION("Failed to select device", TSDR_CANNOT_OPEN_DEVICE);
	}

	sdrplay_api_UnlockDeviceApi();

	// Get device parameters
	err = sdrplay_api_GetDeviceParams(device->dev, &deviceParams);
	if (err != sdrplay_api_Success) {
		sdrplay_api_ReleaseDevice(device);
		sdrplay_api_Close();
		RETURN_EXCEPTION("Failed to get device parameters", TSDR_CANNOT_OPEN_DEVICE);
	}

	// Configure channel parameters
	chParams = deviceParams->rxChannelA;
	chParams->tunerParams.rfFreq.rfHz = (unsigned int)freq;
	chParams->tunerParams.gain.gRdB = (int)gainred;
	chParams->tunerParams.bwType = sdrplay_api_BW_8_000;
	chParams->tunerParams.ifType = sdrplay_api_IF_Zero;
	chParams->ctrlParams.decimation.enable = 0;
	chParams->ctrlParams.agc.enable = sdrplay_api_AGC_50HZ;

	// Set up callback functions - we need to implement our own callback to handle the data
	cbFns.StreamACbFn = streamCallback;  // We'll define this function
	cbFns.StreamBCbFn = NULL;
	cbFns.EventCbFn = eventCallback;     // For handling events

	// Initialize the device
	err = sdrplay_api_Init(device->dev, &cbFns, ctx);
	if (err != sdrplay_api_Success) {
		sdrplay_api_ReleaseDevice(device);
		sdrplay_api_Close();
		RETURN_EXCEPTION(
			"Can't access the SDRplay RSP. Make sure it is properly plugged in, drivers are installed and the sdrplay_api.dll is in the executable's folder and try again. Please, refer to the TSDRPlugin_SDRplay readme file for more information.",
			 TSDR_CANNOT_OPEN_DEVICE);
	} else
		inited = 1;

	// Store global callback references
	global_cb = cb;
	global_ctx = ctx;
	current_freq = (int)freq;
	current_gain = (int)gainred;

	// Main loop - just handle parameter updates while callbacks handle data
	while (working) {
		// Check for frequency changes
		if (current_freq != (int)desiredfreq && !(desiredfreq < 60e6 || (desiredfreq > 245e6 && desiredfreq < 420e6) || desiredfreq > 1000e6)) {
			chParams->tunerParams.rfFreq.rfHz = (unsigned int)desiredfreq;
			err = sdrplay_api_Update(device->dev, sdrplay_api_Tuner_A, sdrplay_api_Update_Tuner_Frf, sdrplay_api_Update_None);
			if (err == sdrplay_api_Success || err == sdrplay_api_OutOfRange) {
				current_freq = (int)desiredfreq;
			}
		}

		// Check for gain changes
		if (current_gain != desiredgainred) {
			chParams->tunerParams.gain.gRdB = desiredgainred;
			err = sdrplay_api_Update(device->dev, sdrplay_api_Tuner_A, sdrplay_api_Update_Tuner_Gr, sdrplay_api_Update_None);
			if (err == sdrplay_api_Success) {
				current_gain = desiredgainred;
			}
		}

		// Small delay to avoid busy waiting
		#ifdef WIN32
		Sleep(10);
		#else
		usleep(10000);
		#endif
	}

	// Clean up
	if (inited) {
		sdrplay_api_Uninit(device->dev);
		sdrplay_api_ReleaseDevice(device);
	}
	sdrplay_api_Close();

	RETURN_EXCEPTION("SDRplay RSP dongle stopped responding.", (err == 0) ? TSDR_OK : TSDR_ERR_PLUGIN);
}

void TSDRPLUGIN_API __stdcall tsdrplugin_cleanup(void) {

}
