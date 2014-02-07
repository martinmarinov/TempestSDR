// TSDRPlugin_ExtIO.cpp : Defines the exported functions for the DLL application.
//
#include "stdafx.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "TSDRCodes.h"
#include "ExtIOPluginLoader.h"

#ifdef TSDRPLUGIN_EXTIO_EXPORTS
#define TSDRPLUGIN_EXTIO_API __declspec(dllexport)
#else
#define TSDRPLUGIN_EXTIO_API __declspec(dllimport)
#endif

#include "errors.h"

#define EXTIO_HWTYPE_16B	3
#define EXTIO_HWTYPE_24B	5
#define EXTIO_HWTYPE_32B	6
#define EXTIO_HWTYPE_FLOAT	7

extern "C" typedef void(*tsdrplugin_readasync_function)(float *buf, uint32_t len, void *ctx, int dropped_samples);

extiosource_t * source = NULL;
int hwtype;
tsdrplugin_readasync_function tsdr_cb;
void * tsdr_ctx;
volatile int is_running = 0;
float * outbuf = NULL;
int outbuf_size = 0;
int max_att_id = -1;

uint32_t req_freq = 100000000;
uint32_t act_freq = -1;
float req_gain = 0.5;
float act_gain = -1;

extern "C" void closeextio(void) {
	if (source == NULL) return;
	if (source->HideGUI != NULL) source->HideGUI();
	source->CloseHW();
	extio_close(source);
	free(source);
	source = NULL;
}

extern "C" void TSDRPLUGIN_EXTIO_API __stdcall tsdrplugin_getName(char * name) {
	strcpy_s(name, 20, "TSDR ExtIO Plugin");
}

extern "C" uint32_t TSDRPLUGIN_EXTIO_API __stdcall tsdrplugin_setsamplerate(uint32_t rate) {
	if (source != NULL) return source->GetHWSR();
	return 0;
}

extern "C" uint32_t TSDRPLUGIN_EXTIO_API __stdcall tsdrplugin_getsamplerate() {
	if (source != NULL) return source->GetHWSR();
	return 0;
}

extern "C"  int TSDRPLUGIN_EXTIO_API __stdcall tsdrplugin_setbasefreq(uint32_t freq) {
	req_freq = freq;

	RETURN_OK();
}

extern "C" int TSDRPLUGIN_EXTIO_API __stdcall tsdrplugin_stop(void) {
	is_running = 0;

	RETURN_OK();
}

extern "C" int TSDRPLUGIN_EXTIO_API __stdcall tsdrplugin_setgain(float gain) {
	req_gain = gain;

	RETURN_OK();
}

extern "C" void callback(int cnt, int status, float IQoffs, void *IQdata) {
	if (!is_running) return;
	if (status < 0 || cnt < 0) return;

	int i;

	const int cntvalues = cnt;
	if (cntvalues > outbuf_size) {
		outbuf_size = cntvalues;
		outbuf = (float *)realloc((void *)outbuf, sizeof(float)* outbuf_size);
	}

	switch (hwtype) {
	case EXTIO_HWTYPE_FLOAT:
		memcpy(outbuf, IQdata, cntvalues * sizeof(float));
		break;
	case EXTIO_HWTYPE_16B:
	{
							 int16_t * buf = (int16_t *)IQdata;
							 for (i = 0; i < cntvalues; i++)
								 outbuf[i] = *(buf++) / 32767.0f;
	}
		break;
	case EXTIO_HWTYPE_24B:
	{
							 uint8_t * buf = (uint8_t *)IQdata;
							 for (i = 0; i < cntvalues; i++) {
								 const uint8_t first = *(buf++);
								 const uint8_t second = *(buf++);
								 const uint8_t third = *(buf++);

								 outbuf[i] = ((int16_t)(first | (second << 8) | (third << 16))) / 8388607.0f;
							 }
	}
		break;
	case EXTIO_HWTYPE_32B:
	{
							 int32_t * buf = (int32_t *)IQdata;
							 for (i = 0; i < cntvalues; i++)
								 outbuf[i] = *(buf++) / 2147483647.0f;
	}
		break;
	}

	tsdr_cb(outbuf, cntvalues, tsdr_ctx, 0);
}

extern "C" int TSDRPLUGIN_EXTIO_API __stdcall tsdrplugin_init(const char * params) {
	AFX_MANAGE_STATE(AfxGetAppModuleState());

	if (outbuf == NULL) {
		outbuf = (float *)malloc(sizeof(float));
		outbuf_size = 1;
	}

	// if an extio was already initialized before, now change
	if (source != NULL)
		closeextio();

	// inititalize source
	source = (extiosource_t *)malloc(sizeof(extiosource_t));
	printf("LOADING! PLUGCHOOO \n"); fflush(stdout);
	int status;
	if ((status = extio_load(source, params)) == TSDR_OK) {
		char name[200];
		char model[200];
		printf("LOADED! PLUGCHOOO \n"); fflush(stdout);
		if (source->InitHW(name, model, &hwtype)) {
			source->SetCallback(&callback);

			if (hwtype != EXTIO_HWTYPE_16B && hwtype != EXTIO_HWTYPE_24B && hwtype != EXTIO_HWTYPE_32B && hwtype != EXTIO_HWTYPE_FLOAT) {
				closeextio();
				RETURN_EXCEPTION("The sample format of the ExtIO plugin is not supported.", TSDR_CANNOT_OPEN_DEVICE);
			}

			if (source->OpenHW()) {
				//printf("Opened %s model %s!\n", name, model); fflush(stdout);

				printf("Showing GUI :) \n"); fflush(stdout);
				if (source->ShowGUI != NULL) source->ShowGUI();
				printf("Yey! \n"); fflush(stdout);

				// list attenuators
				if (source->GetAttenuators != NULL) {
					max_att_id = 0;
					float att;
					while (source->GetAttenuators(max_att_id++, &att) == 0) {};
				}

				RETURN_OK();
			}
			else {
				closeextio();
				RETURN_EXCEPTION("The ExtIO driver failed to open a device. Make sure your device is plugged in and its drivers are installed correctly.", TSDR_CANNOT_OPEN_DEVICE);
			}
		}
		else {
			closeextio();
			RETURN_EXCEPTION("The ExtIO driver failed to initialize a device. Make sure your device is plugged in and its drivers are installed correctly.", TSDR_CANNOT_OPEN_DEVICE);
		}
	}
	else {

		if (status != TSDR_INCOMPATIBLE_PLUGIN)
			extio_close(source);

		free(source);
		source = NULL;
		
		if (status == TSDR_INCOMPATIBLE_PLUGIN)
			RETURN_EXCEPTION("The ExtIO dll is not compatible with the current machine or does not exist. Please check the filename is correct and the file is a valid ExtIO dll file and try again.", TSDR_PLUGIN_PARAMETERS_WRONG)
		else 
			RETURN_EXCEPTION("The provided library is not a valid/compatible ExtIO dll. Please check the filename is correct and the file is a valid ExtIO dll file and try again.", TSDR_PLUGIN_PARAMETERS_WRONG);
	}
}

extern "C" void attenuate(float gain) {
	if (max_att_id > 0 && source->SetAttenuator != NULL) {
		int att_id = (int)(gain * max_att_id);
		if (att_id >= max_att_id) att_id = max_att_id - 1;
		if (att_id < 0) att_id = 0;
		source->SetAttenuator(att_id);
	}
}

extern "C" int TSDRPLUGIN_EXTIO_API __stdcall tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx) {
	AFX_MANAGE_STATE(AfxGetAppModuleState());

	if (source == NULL)
		RETURN_EXCEPTION("Please provide a full path to a valid ExtIO dll.", TSDR_PLUGIN_PARAMETERS_WRONG);

	// start callbacks
	tsdr_cb = cb;
	tsdr_ctx = ctx;
	is_running = 1;

	act_freq = req_freq;

	if (source->HideGUI != NULL) source->HideGUI();

	if (source->StartHW(act_freq) < 0)
		RETURN_EXCEPTION("The device has stopped responding.", TSDR_CANNOT_OPEN_DEVICE);

	act_gain = req_gain;
	attenuate(act_gain);

	while (is_running) {
		Sleep(50);

		if (req_freq != act_freq) {
			act_freq = req_freq;
			source->SetHWLO(act_freq);
		}

		if (req_gain != act_gain) {
			act_gain = req_gain;
			attenuate(act_gain);
		}
	}

	source->StopHW();

	RETURN_OK();
}

extern "C" void TSDRPLUGIN_EXTIO_API __stdcall tsdrplugin_cleanup(void) {

	if (outbuf != NULL) {
		free(outbuf);
		outbuf = NULL;
	}

	if (source == NULL) return;

	//if (source->pfnHideGUI != NULL) source->pfnHideGUI();

	closeextio();
}
