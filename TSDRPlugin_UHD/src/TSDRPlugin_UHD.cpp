#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "TSDRPlugin.h"
#include "TSDRCodes.h"

#include <stdint.h>

#include "errors.h"

EXTERNC void __stdcall tsdrplugin_getName(char * name) {
	strcpy(name, "TSDR UHD USRP Compatible Plugin");
}

EXTERNC int __stdcall tsdrplugin_init(const char * params) {
	RETURN_EXCEPTION("tsdrplugin_init not implemented!", TSDR_NOT_IMPLEMENTED);
}

EXTERNC uint32_t __stdcall tsdrplugin_setsamplerate(uint32_t rate) {
	RETURN_EXCEPTION("tsdrplugin_setsamplerate not implemented!", TSDR_NOT_IMPLEMENTED);
}

EXTERNC uint32_t __stdcall tsdrplugin_getsamplerate() {
	RETURN_EXCEPTION("tsdrplugin_getsamplerate not implemented!", TSDR_NOT_IMPLEMENTED);
}

EXTERNC int __stdcall tsdrplugin_setbasefreq(uint32_t freq) {
	RETURN_EXCEPTION("tsdrplugin_setbasefreq not implemented!", TSDR_NOT_IMPLEMENTED);
}

EXTERNC int __stdcall tsdrplugin_stop(void) {
	RETURN_EXCEPTION("tsdrplugin_stop not implemented!", TSDR_NOT_IMPLEMENTED);
}

EXTERNC int __stdcall tsdrplugin_setgain(float gain) {
	RETURN_EXCEPTION("tsdrplugin_setgain not implemented!", TSDR_NOT_IMPLEMENTED);
}

EXTERNC int __stdcall tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx) {
	RETURN_EXCEPTION("tsdrplugin_readasync not implemented!", TSDR_NOT_IMPLEMENTED);
}

EXTERNC void __stdcall tsdrplugin_cleanup(void) {

}
