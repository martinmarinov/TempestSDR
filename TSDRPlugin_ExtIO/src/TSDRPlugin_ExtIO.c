#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "TSDRPlugin.h"
#include "TSDRCodes.h"
#include "ExtIOPluginLoader.h"

#include "errors.h"

extiosource_t * source = NULL;
uint32_t init_freq = 100000000;

void closeextio(void) {
	if (source == NULL) return;
	source->CloseHW();
	extio_close(source);
	free (source);
	source = NULL;
}

void tsdrplugin_getName(char * name) {
	strcpy(name, "TSDR ExtIO Plugin");
}

uint32_t tsdrplugin_setsamplerate(uint32_t rate) {
	return 1024000;
}

uint32_t tsdrplugin_getsamplerate() {
	return 1024000;
}

int tsdrplugin_setbasefreq(uint32_t freq) {

	if (source == NULL) {
		init_freq = freq;
	}

	RETURN_OK();
}

int tsdrplugin_stop(void) {
	RETURN_OK();
}

int tsdrplugin_setgain(float gain) {
	// TODO! can we really set the gain?
	RETURN_OK();
}

int callback(int cnt, int status, float IQoffs, void *IQdata) {
	return 0;
}

int tsdrplugin_init(const char * params) {

	extiosource_t * result = source;

	// if an extio was already initialized before, now change
	if (result != NULL) {
		closeextio();
	}

	// inititalize source
	result = (extiosource_t *) malloc(sizeof(extiosource_t));
	if (extio_load(result, params) == TSDR_OK) {
		char name[200];
		char model[200];
		int hwtype;

		if (result->InitHW(name, model, &hwtype)) {
			result->SetCallback(&callback);

			if (result->OpenHW()) {
				RETURN_OK();
			} else {
				closeextio();
				RETURN_EXCEPTION("The ExtIO driver failed to open a device. Make sure your device is plugged in and its drivers are installed correctly.", TSDR_CANNOT_OPEN_DEVICE);
			}
		} else {
			closeextio();
			RETURN_EXCEPTION("The ExtIO driver failed to initialize a device. Make sure your device is plugged in and its drivers are installed correctly.", TSDR_CANNOT_OPEN_DEVICE);
		}
	} else {
		free (result);
		result = NULL;
		RETURN_EXCEPTION("Cannot load the specified ExtIO dll file. Please check the filename is correct and the file is a valid ExtIO dll file and try again.", TSDR_PLUGIN_PARAMETERS_WRONG);
	}

	source = result;
}

int tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx) {
	if (source == NULL)
		RETURN_EXCEPTION("Please, first set parameters. The parameters need to contain a full path to the ExtIO dll.", TSDR_PLUGIN_PARAMETERS_WRONG);

	RETURN_OK();
}

void tsdrplugin_cleanup(void) {
	if (source == NULL) return;

	//if (source->pfnHideGUI != NULL) source->pfnHideGUI();

	closeextio();
}
