/*#-------------------------------------------------------------------------------
# Copyright (c) 2014 Martin Marinov.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the GNU Public License v3.0
# which accompanies this distribution, and is available at
# http://www.gnu.org/licenses/gpl.html
# 
# Contributors:
#     Martin Marinov - initial API and implementation
#------------------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>

#include "TSDRPlugin.h"
#include "TSDRCodes.h"

#include <mir_sdr.h>

#include <stdint.h>
#include <stdlib.h>

#define SAMPLE_RATE (8000000)

volatile int working = 0;

volatile double desiredfreq = 200;
volatile int desiredgainred = 40;

#define SAMPLES_TO_PROCESS_AT_ONCE (200)

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
	strcpy(name, "TSDR Mirics SDR Plugin");
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
	desiredgainred = 102 - (int) (gain * 102);
	if (desiredgainred < 0) desiredgainred = 0;
	else if (desiredgainred > 102) desiredgainred = 102;
	RETURN_OK();
}

int TSDRPLUGIN_API __stdcall tsdrplugin_init(const char * params) {
	RETURN_OK();
}

int TSDRPLUGIN_API __stdcall tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx) {
	working = 1;

	int err, sps, grc, rfc, fsc, i, id;
	unsigned int fs;
	unsigned int frame = 0;
	int inited = 0 ;

	double freq = desiredfreq;
	double gainred = desiredgainred;
	err = mir_sdr_Init(gainred, 8, freq / 1000000.0, mir_sdr_BW_8_000, mir_sdr_IF_Zero, &sps);
	if (err != 0) {
		mir_sdr_Uninit();
		RETURN_EXCEPTION("Can't access the Mirics SDR dongle. Make sure it is properly plugged in, drivers are installed and the mir_sdr_api.dll is in the executable's folder and try again. Please, refer to the TSDRPlugin_Miricis readme file for more information.", TSDR_CANNOT_OPEN_DEVICE);
	} else
		inited = 1;

	short * xi = (short *)malloc(sps * SAMPLES_TO_PROCESS_AT_ONCE * sizeof(short));
	short * xq = (short *)malloc(sps * SAMPLES_TO_PROCESS_AT_ONCE * sizeof(short));

	int outbufsize =  2 * sps * SAMPLES_TO_PROCESS_AT_ONCE;
	float * outbuf = (float *) malloc(sizeof(float) * outbufsize);

	while (working) {
		unsigned int dropped = 0;

		for (id = 0; id < SAMPLES_TO_PROCESS_AT_ONCE; id++) {
			err = mir_sdr_ReadPacket(&xi[id*sps], &xq[id*sps], &fs, &grc, &rfc, &fsc);
			if (fs > frame)
				dropped += fs - frame;
//			else if (fs < frame) {
//				// wrapped around
//				dropped += 2147483647 - frame;
//				dropped += fs;
//			}
			frame = fs + sps;
			if (err != 0) break;
		}

		// if desired frequency is different and valid
		if (err == 0 && freq != desiredfreq && !(desiredfreq < 60e6 || (desiredfreq > 245e6 && desiredfreq < 420e6) || desiredfreq > 1000e6)) {

			err = mir_sdr_SetRf(desiredfreq, 1, 0);
			if (err == 0)
				freq = desiredfreq; // if OK, frequency has been changed
			else if (err == mir_sdr_RfUpdateError) {
				// if an error occurs, try to reset the device
				if (mir_sdr_Uninit() == 0) inited = 0;
				gainred = desiredgainred;
				if ((err = mir_sdr_Init(gainred, 8, desiredfreq / 1000000.0, mir_sdr_BW_8_000, mir_sdr_IF_Zero, &sps))==0) inited = 1;
				if (err == mir_sdr_OutOfRange) {
					// if the desired frequency is outside operational range, go back to the working frequency
					if (mir_sdr_Uninit() == 0) inited = 0;
					if (mir_sdr_Init(gainred, 8, freq / 1000000.0, mir_sdr_BW_8_000, mir_sdr_IF_Zero, &sps) == 0) inited = 1;
					desiredfreq = freq;
					err = 0;
				} else if (err == 0)
					freq = desiredfreq;
			} else if (err == mir_sdr_OutOfRange) {
				mir_sdr_SetRf(freq, 1, 0);
				desiredfreq = freq;
				err = 0;
			}
		}

		if (err == 0 && gainred != desiredgainred) {
			gainred = desiredgainred;
			mir_sdr_SetGr(gainred, 1, 0);
		}

		if (err == 0)
		for (i = 0; i < outbufsize; i++) {
			const short val = (i & 1) ? (xq[i >> 1]) : (xi[i >> 1]);
			outbuf[i] = val / 32767.0;
		}

		if (err != 0)
			{ working = 0; break; }
		else
			cb(outbuf, outbufsize, ctx, dropped);
	}



	free(outbuf);
	free(xi);
	free(xq);

	if (inited && mir_sdr_Uninit() != 0) RETURN_EXCEPTION("Cannot properly close the Mirics USB dongle. If you experience any further issues, please disconnect it and reconnect it again.", TSDR_CANNOT_OPEN_DEVICE);

	RETURN_EXCEPTION("The Mirics SDR dongle stopped responding.", (err == 0) ? TSDR_OK : TSDR_ERR_PLUGIN);
}

void TSDRPLUGIN_API __stdcall tsdrplugin_cleanup(void) {

}
