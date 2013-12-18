#include <stdio.h>
#include <string.h>

#include "TSDRPlugin.h"
#include "TSDRCodes.h"

#include <stdint.h>

#include "osdetect.h"
#include "timer.h"

#define str_eq(s1,s2)  (!strcmp ((s1),(s2)))

#define MAX_ERRORS (5)

#define TYPE_FLOAT (0)
#define TYPE_BYTE (1)
#define TYPE_SHORT (2)
#define TYPE_UBYTE (3)
#define TYPE_USHORT (4)

#define PERFORMANCE_BENCHMARK (0)
#define ENABLE_LOOP (1)

#define TIME_STRETCH (1)
#define SAMPLES_TO_READ_AT_ONCE (512*1024)
#define MAX_SAMP_RATE (1000e6)

TickTockTimer_t timer;
volatile int working = 0;

int type = -1;
int sizepersample = -1;

uint32_t samplerate = 0;
char filename[300];

void thread_sleep(uint32_t milliseconds) {
#if WINHEAD
	Sleep(milliseconds);
#else
	usleep(1000 * milliseconds);
#endif
}

void tsdrplugin_getName(char * name) {
	strcpy(name, "TSDR Raw File Source Plugin");
}

uint32_t tsdrplugin_setsamplerate(uint32_t rate) {
	return samplerate;
}

uint32_t tsdrplugin_getsamplerate() {
	return samplerate;
}

int tsdrplugin_setbasefreq(uint32_t freq) {
	return TSDR_OK;
}

int tsdrplugin_stop(void) {
	working = 0;
	return TSDR_OK;
}

int tsdrplugin_setgain(float gain) {
	return TSDR_NOT_IMPLEMENTED;
}

char * strtoken = NULL;
int pos = 0;
// this function splits a string into tokens separated by spaces. If tokens are surrounded by ' or ", the spaces inside are ignored
// it will destroy the string it is provided with!
char * nexttoken(char * input) {
    if (input != NULL) {
        strtoken = input;
        pos = 0;
    } else if (strtoken == NULL)
        return NULL;

    int quotes = 0;
    int i = pos;

    char ch;
    while((ch = strtoken[i++]) != 0) {
        switch (ch) {
        case '\'':
        case '"':
            quotes = !quotes;
            if (quotes) {
                pos++;
                break;
            }
        case ' ':
            if (!quotes) {
                strtoken[i-1] = 0;
                const int start = pos;
                pos = i;
                if (pos - start > 1)
                    return &strtoken[start];
            }
            break;
        }
    }

    if (quotes)
        return NULL;
    else {
        char * answer = strtoken;
        strtoken = NULL;
        return &answer[pos];
    }
}

int tsdrplugin_setParams(const char * params) {
	char * fname = nexttoken((char *) params);
	if (fname == NULL) return TSDR_PLUGIN_PARAMETERS_WRONG;
	char * samplerate_s = nexttoken(NULL);
	if (samplerate_s == NULL) return TSDR_SAMPLE_RATE_WRONG;
	long samplerate_l = atol(samplerate_s);
	if (samplerate_l > MAX_SAMP_RATE || samplerate_l <= 0) return TSDR_SAMPLE_RATE_WRONG;
	char * type_s = nexttoken(NULL);
	if (type_s == NULL) return TSDR_PLUGIN_PARAMETERS_WRONG;

	if (str_eq(type_s,"float")) {
		type = TYPE_FLOAT;
		sizepersample = 4;
	} else if (str_eq(type_s,"int8")) {
		type = TYPE_BYTE;
		sizepersample = 1;
	} else if (str_eq(type_s,"int16")) {
		type = TYPE_SHORT;
		sizepersample = 2;
	} else if (str_eq(type_s,"uint8")) {
		type = TYPE_UBYTE;
		sizepersample = 1;
	} else if (str_eq(type_s,"uint16")) {
		type = TYPE_USHORT;
		sizepersample = 2;
	} else
		return TSDR_PLUGIN_PARAMETERS_WRONG;

	strcpy(filename, fname);
	samplerate = (uint32_t) samplerate_l;

	return TSDR_OK;
}

int tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx) {
	working = 1;
	int i;

	int counter;

	if (sizepersample == -1)
		return TSDR_PLUGIN_PARAMETERS_WRONG;

	FILE * file=fopen(filename,"rb");
	if (file == NULL) return TSDR_PLUGIN_PARAMETERS_WRONG;
	if (samplerate > MAX_SAMP_RATE || samplerate <= 0) return TSDR_SAMPLE_RATE_WRONG;

	const size_t bytestoread = SAMPLES_TO_READ_AT_ONCE * sizepersample;
	char * buf = (char *) malloc(bytestoread);
	float * outbuf = (float *) malloc(sizeof(float) * SAMPLES_TO_READ_AT_ONCE);

#if !PERFORMANCE_BENCHMARK
	uint32_t delayms = (uint32_t) (TIME_STRETCH*1000.0f * (float) SAMPLES_TO_READ_AT_ONCE / (float) samplerate);
	if (delayms == 0) delayms = 1;
#endif

	while (working) {
		size_t to_read = bytestoread;
		size_t total_read = 0;
		int errors = 0;

		while (to_read) {
			size_t read = fread(&buf[total_read],1,to_read,file);
			to_read -= read;
			total_read += read;

			if (read == 0 && errors++ > MAX_ERRORS) {
#if ENABLE_LOOP
				rewind(file);
#else
				working = 0;
#endif
				break;
			}
		}

		if (working) {

			switch (type) {
			case TYPE_FLOAT:
				memcpy(outbuf, buf, bytestoread);
				break;
			case TYPE_BYTE:
				for (i = 0; i < SAMPLES_TO_READ_AT_ONCE; i++)
					outbuf[i] = ((int8_t) buf[i]) / 128.0;
				break;
			case TYPE_SHORT:
				for (i = 0; i < SAMPLES_TO_READ_AT_ONCE; i++)
					outbuf[i] = (*((int16_t *) (&buf[i*sizepersample]))) / 32767.0;
				break;
			case TYPE_UBYTE:
				for (i = 0; i < SAMPLES_TO_READ_AT_ONCE; i++)
					outbuf[i] = (((uint8_t) buf[i]) - 128) / 128.0;
				break;
			case TYPE_USHORT:
				for (i = 0; i < SAMPLES_TO_READ_AT_ONCE; i++)
					outbuf[i] = ((*((uint16_t *) (&buf[i*sizepersample])))-32767) / 32767.0;
				break;
			}

			cb(outbuf, SAMPLES_TO_READ_AT_ONCE, ctx);

#if !PERFORMANCE_BENCHMARK
			const uint32_t timeelapsed = (uint32_t) (1000.0f*timer_ticktock(&timer));
			if (timeelapsed < delayms)
				thread_sleep(delayms-timeelapsed);
#endif
		}
	}

	free(buf);
	free(outbuf);
	fclose(file);

	return TSDR_OK;
}
