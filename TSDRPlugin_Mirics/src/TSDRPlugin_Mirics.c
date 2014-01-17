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

#define SAMPLES_TO_PROCESS_AT_ONCE (20)

void tsdrplugin_getName(char * name) {
	strcpy(name, "TSDR Mirics SDR Plugin");
}

uint32_t tsdrplugin_setsamplerate(uint32_t rate) {
	return SAMPLE_RATE;
}

uint32_t tsdrplugin_getsamplerate() {
	return SAMPLE_RATE;
}

int tsdrplugin_setbasefreq(uint32_t freq) {
	desiredfreq = freq;
	return TSDR_OK;
}

int tsdrplugin_stop(void) {
	working = 0;
	return TSDR_OK;
}

int tsdrplugin_setgain(float gain) {
	desiredgainred = 102 - (int) (gain * 102);
	if (desiredgainred < 0) desiredgainred = 0;
	else if (desiredgainred > 102) desiredgainred = 102;
	return TSDR_OK;
}

int tsdrplugin_setParams(const char * params) {
	return TSDR_OK;
}

int tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx) {
	working = 1;

	int err, sps, grc, rfc, fsc, i, id;
	unsigned int fs;
	unsigned int frame = 0;

	double freq = desiredfreq;
	double gainred = desiredgainred;
	err = mir_sdr_Init(gainred, 8, freq / 1000000.0, mir_sdr_BW_8_000, mir_sdr_IF_Zero, &sps);
	if (err != 0) {
		mir_sdr_Uninit();
		return TSDR_CANNOT_OPEN_DEVICE;
	}

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
		if (freq != desiredfreq && !(desiredfreq < 60e6 || (desiredfreq > 245e6 && desiredfreq < 420e6) || desiredfreq > 1000e6)) {

			err = mir_sdr_SetRf(desiredfreq, 1, 0);
			if (err == 0)
				freq = desiredfreq; // if OK, frequency has been changed
			else if (err == mir_sdr_RfUpdateError) {
				// if an error occurs, try to reset the device
				mir_sdr_Uninit();
				gainred = desiredgainred;
				err = mir_sdr_Init(gainred, 8, desiredfreq / 1000000.0, mir_sdr_BW_8_000, mir_sdr_IF_Zero, &sps);
				if (err == 3) {
					// if the desired frequency is outside operational range, go back to the working frequency
					mir_sdr_Uninit();
					mir_sdr_Init(gainred, 8, freq / 1000000.0, mir_sdr_BW_8_000, mir_sdr_IF_Zero, &sps);
					desiredfreq = freq;
					err = 0;
				}
				else
					freq = desiredfreq;
			}
		}

		if (gainred != desiredgainred) {
			gainred = desiredgainred;
			mir_sdr_SetGr(gainred, 1, 0);
		}

		for (i = 0; i < outbufsize; i++) {
			const short val = (i & 1) ? (xq[i >> 1]) : (xi[i >> 1]);
			outbuf[i] = val / 32767.0;
		}

		cb(outbuf, outbufsize, ctx, dropped);
	}

	free(outbuf);
	free(xi);
	free(xq);

	if (mir_sdr_Uninit() != 0) return TSDR_ERR_PLUGIN;

	return (err == 0) ? TSDR_OK : TSDR_ERR_PLUGIN;
}
