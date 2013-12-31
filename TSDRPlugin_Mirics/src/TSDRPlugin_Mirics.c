#include <stdio.h>
#include <string.h>

#include "TSDRPlugin.h"
#include "TSDRCodes.h"

#include <mir_sdr.h>

#include <stdint.h>
#include <stdlib.h>

#define SAMPLE_RATE (8000000)

volatile int working = 0;
double freq = 200;
double desiredfreq = 200;

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
	printf("Mirics setgain %.4f\n", gain);
	return TSDR_NOT_IMPLEMENTED;
}

int tsdrplugin_setParams(const char * params) {
	return TSDR_OK;
}

int tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx) {
	working = 1;

	int err, sps, grc, rfc, fsc, i;
	unsigned int fs;
	int newGr = 40;

	freq = desiredfreq;
	err = mir_sdr_Init(newGr, 8, freq / 1000000.0, mir_sdr_BW_8_000, mir_sdr_IF_Zero, &sps);
	if (err != 0) {
		mir_sdr_Uninit();
		return TSDR_CANNOT_OPEN_DEVICE;
	}

	short * xi = (short *)malloc(sps * sizeof(short));
	short * xq = (short *)malloc(sps * sizeof(short));

	int outbufsize =  2 * sps;
	float * outbuf = (float *) malloc(sizeof(float) * outbufsize);

	while (working) {
		err = mir_sdr_ReadPacket(xi, xq, &fs, &grc, &rfc, &fsc);
		if (err != 0) break;

		if (freq != desiredfreq) {
			freq = desiredfreq;
			err = mir_sdr_SetRf(freq, 1, 0);

			if (err == mir_sdr_RfUpdateError) {
				mir_sdr_Uninit();
				err = mir_sdr_Init(newGr, 8, freq / 1000000.0, mir_sdr_BW_8_000, mir_sdr_IF_Zero, &sps);
			}

			if (err != 0) {
				printf("FS error id %d trying to set freq to %.4f\n", err, freq);
				break;
			}
		}


		for (i = 0; i < outbufsize; i++) {
			const short val = (i & 1) ? (xq[i >> 1]) : (xi[i >> 1]);
			outbuf[i] = (val - 32767) / 32767.0;
		}

		cb(outbuf, outbufsize, ctx);
	}

	free(outbuf);
	free(xi);
	free(xq);

	if (mir_sdr_Uninit() != 0) return TSDR_ERR_PLUGIN;

	return (err == 0) ? TSDR_OK : TSDR_ERR_PLUGIN;
}
