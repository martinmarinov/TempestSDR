#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "TSDRPlugin.h"
#include "TSDRCodes.h"

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

char * tsdrplugin_getlasterrortext(void) {
	if (errormsg_code == TSDR_OK)
		return NULL;
	else
		return errormsg;
}

void tsdrplugin_getName(char * name) {
	strcpy(name, "TSDR ExtIO Plugin");
}

uint32_t tsdrplugin_setsamplerate(uint32_t rate) {
	return TSDR_NOT_IMPLEMENTED;
}

uint32_t tsdrplugin_getsamplerate() {
	return TSDR_NOT_IMPLEMENTED;
}

int tsdrplugin_setbasefreq(uint32_t freq) {
	RETURN_EXCEPTION("Not implemented", TSDR_NOT_IMPLEMENTED);
}

int tsdrplugin_stop(void) {
	RETURN_EXCEPTION("Not implemented", TSDR_NOT_IMPLEMENTED);
}

int tsdrplugin_setgain(float gain) {
	RETURN_EXCEPTION("Not implemented", TSDR_NOT_IMPLEMENTED);
}

int tsdrplugin_setParams(const char * params) {
	RETURN_EXCEPTION("Not implemented", TSDR_NOT_IMPLEMENTED);
}

int tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx) {
	RETURN_EXCEPTION("Not implemented", TSDR_NOT_IMPLEMENTED);
}
