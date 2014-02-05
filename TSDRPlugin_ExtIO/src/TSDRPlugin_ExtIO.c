#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "TSDRPlugin.h"
#include "TSDRCodes.h"
#include "ExtIOPluginLoader.h"

#include "errors.h"

extiosource_t * source = NULL;
uint32_t init_freq = 100000000;

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

int tsdrplugin_setParams(const char * params) {

	// if an extio was already initialized before, now change
	if (source != NULL) {
		extio_close(source);
		free (source);
		source = NULL;
	}

	// inititalize source
	source = (extiosource_t *) malloc(sizeof(extiosource_t));
	if (extio_load(source, params) == TSDR_OK) {
		// TODO! get samplerate?
		if (source->ShowGUI != NULL)
			source->ShowGUI();

		RETURN_OK();
	} else {
		free (source);
		source = NULL;
		RETURN_EXCEPTION("Cannot load the specified ExtIO dll file. Please check the filename is correct and the file is a valid ExtIO dll file and try again.", TSDR_PLUGIN_PARAMETERS_WRONG);
	}

}

int tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx) {
	RETURN_OK();
}

void tsdrplugin_cleanup(void) {
	if (source == NULL) return;

	//if (source->pfnHideGUI != NULL) source->pfnHideGUI();

	extio_close(source);
	free (source);
	source = NULL;
}
