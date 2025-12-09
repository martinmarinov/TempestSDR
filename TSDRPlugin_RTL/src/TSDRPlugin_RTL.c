/*
# SPDX-License-Identifier: GPL-3.0-or-later
# TempestSDR Plugin Architecture:
#  Copyright (c) 2014 Martin Marinov.
# librtlsdr library:
#  Copyright (C) 2012-2013 by Steve Markgraf <steve@steve-m.de>
#  Copyright (C) 2012 by Dimitri Stolnikov <horiz0n@gmx.net>
# Plugin contribution:
#  Copyright (C) 2025 by Tymoteusz Vogt
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include <rtl-sdr.h>

#include "TSDRPlugin.h"
#include "TSDRCodes.h"

// Global device state
static rtlsdr_dev_t *dev = NULL;
static volatile int is_running = 0;
static uint32_t current_samplerate = 2400000; // 2.4 MHz default
static uint32_t current_frequency = 100000000; // 100 MHz default
static float current_gain = 0.5f;
static int ppm_error = 0;

// Callback data
static tsdrplugin_readasync_function user_callback = NULL;
static void *user_ctx = NULL;
static float *conversion_buffer = NULL;
static size_t conversion_buffer_size = 0;

// Error handling
static int errormsg_code = TSDR_OK;
static char *errormsg = NULL;
static int errormsg_size = 0;

#define RETURN_EXCEPTION(message, status) {announceexception(message, status); return status;}
#define RETURN_OK() {errormsg_code = TSDR_OK; return TSDR_OK;}

static inline void announceexception(const char *message, int status) {
    errormsg_code = status;
    if (status == TSDR_OK) return;

    const int length = strlen(message);
    if (errormsg_size == 0) {
        errormsg_size = length;
        errormsg = (char *)malloc(length + 1);
    } else if (length > errormsg_size) {
        errormsg_size = length;
        errormsg = (char *)realloc((void *)errormsg, length + 1);
    }
    strcpy(errormsg, message);
}

static void rtlsdr_callback(unsigned char *buf, uint32_t len, void *ctx) {
    (void)ctx; // unused
    
    if (!is_running || user_callback == NULL) return;
    
    uint32_t samples = len; 
    
    if (conversion_buffer_size < samples) {
        conversion_buffer = (float *)realloc(conversion_buffer, samples * sizeof(float));
        conversion_buffer_size = samples;
    }
    
    for (uint32_t i = 0; i < samples; i++) {
        conversion_buffer[i] = (buf[i] - 127.5f) / 127.5f;
    }
    
    user_callback(conversion_buffer, samples, user_ctx, 0);
}

// Plugin API architecture implementation

void TSDRPLUGIN_API __stdcall tsdrplugin_getName(char *name) {
    strcpy(name, "TSDR RTL-SDR Plugin");
}

int TSDRPLUGIN_API __stdcall tsdrplugin_init(const char *params) {
    uint32_t device_count = rtlsdr_get_device_count();
    if (device_count == 0) {
        RETURN_EXCEPTION("No rtl sdr device found, make sure other application isn't using it, drivers are installed and device is plugged in.", TSDR_CANNOT_OPEN_DEVICE);
    }
    
    // Parse parameters: [device_index] [sample_rate] [ppm_error]
    // All parameters are optional
    // Defaults: device 0 (first available), 2.4 MHz sample rate (max stable), 0 ppm correction
    uint32_t device_index = 0;
    uint32_t requested_samplerate = current_samplerate;
    ppm_error = 0;
    
    if (params && strlen(params) > 0) {
        int parsed = sscanf(params, "%u %u %d", &device_index, &requested_samplerate, &ppm_error);
        
        // Shown if user input parsing fails
        if (parsed == 0) {
            RETURN_EXCEPTION(
                "Invalid parameters.\n\n"
                "Usage: \"device_index sample_rate ppm_error\"\n"
                "Parameters are positional and optional\n\n"
                "Examples:\n"
                "  (empty)           - First device, 2.4 MHz, 0 ppm\n"
                "  \"1\"             - Second device, default settings\n"
                "  \"0 2048000\"     - First device, 2.048 MHz sample rate\n"
                "  \"0 2400000 15\"  - First device, 2.4 MHz, +15 ppm correction\n\n"
                "Parameters:\n"
                "  device_index:  0 to count of devices - 1, default: 0\n"
                "  sample_rate:   225000 to 3200000 Hz, default: 2400000Hz (maximum stable rate)\n"
                "  ppm_error:     -100 to +100 (frequency correction), default: 0",
                TSDR_INVALID_PARAMETER
            );
        }
    }
    
    if (device_index >= device_count) {
        char errmsg[256];
        snprintf(errmsg, sizeof(errmsg), 
                 "Device index %u is out of range. Found %u devices. Therefore max index 0-%u.", 
                 device_index, device_count, device_count - 1);
        RETURN_EXCEPTION(errmsg, TSDR_INVALID_PARAMETER);
    }
    
    // Validate sample rate range
    // There should be a list of allowed values instead tho
    if (requested_samplerate < 225000 || requested_samplerate > 3200000) {
        char errmsg[256];
        snprintf(errmsg, sizeof(errmsg),
                 "Sample rate %u Hz out of device supported range. Valid range 225000 to 32000000 (usually max 2400000 stable)",
                 requested_samplerate);
        RETURN_EXCEPTION(errmsg, TSDR_INVALID_PARAMETER);
    }
    
    // Initialize
    int r = rtlsdr_open(&dev, device_index);
    if (r < 0) {
        char errmsg[256];
        const char *device_name = rtlsdr_get_device_name(device_index);
        snprintf(errmsg, sizeof(errmsg),
                 "Failed to open rtl dongle #%u (%s). Error code: %d",
                 device_index, device_name ? device_name : "unknown", r);
        RETURN_EXCEPTION(errmsg, TSDR_CANNOT_OPEN_DEVICE);
    }
    
    // Configure 
    rtlsdr_set_sample_rate(dev, requested_samplerate);
    current_samplerate = rtlsdr_get_sample_rate(dev); // Get actual rate
    rtlsdr_set_center_freq(dev, current_frequency);
    rtlsdr_set_freq_correction(dev, ppm_error);
    
    // Set gain control via java gui
    rtlsdr_set_tuner_gain_mode(dev, 1);
    
    // Set initial gain
    int gain_count = rtlsdr_get_tuner_gains(dev, NULL);
    if (gain_count > 0) {
        int *gains = (int *)malloc(sizeof(int) * gain_count);
        rtlsdr_get_tuner_gains(dev, gains);
        int gain_index = (int)(current_gain * (gain_count - 1));
        if (gain_index < 0) gain_index = 0;
        if (gain_index >= gain_count) gain_index = gain_count - 1;
        rtlsdr_set_tuner_gain(dev, gains[gain_index]);
        free(gains);
    }
    
    RETURN_OK();
}

uint32_t TSDRPLUGIN_API __stdcall tsdrplugin_setsamplerate(uint32_t rate) {
    if (is_running) {
        return current_samplerate;
    }
    
    if (dev == NULL) {
        current_samplerate = rate;
        return current_samplerate;
    }
    
    // Clamp to valid range (225 kHz - 3.2 MHz)
    if (rate < 225000) rate = 225000;
    if (rate > 3200000) rate = 3200000;
    
    int r = rtlsdr_set_sample_rate(dev, rate);
    if (r == 0) {
        current_samplerate = rtlsdr_get_sample_rate(dev);
    }
    
    return current_samplerate;
}

uint32_t TSDRPLUGIN_API __stdcall tsdrplugin_getsamplerate(void) {
    if (dev != NULL) {
        current_samplerate = rtlsdr_get_sample_rate(dev);
    }
    return current_samplerate;
}

int TSDRPLUGIN_API __stdcall tsdrplugin_setbasefreq(uint32_t freq) {
    current_frequency = freq;
    
    if (dev != NULL) {
        int r = rtlsdr_set_center_freq(dev, freq);
        if (r < 0) {
            RETURN_EXCEPTION("Failed to set frequency. Frequency may be out of range.", TSDR_INVALID_PARAMETER_VALUE);
        }
    }
    
    RETURN_OK();
}

int TSDRPLUGIN_API __stdcall tsdrplugin_setgain(float gain) {
    if (gain < 0.0f) gain = 0.0f;
    if (gain > 1.0f) gain = 1.0f;
    current_gain = gain;
    
    if (dev != NULL) {
        int gain_count = rtlsdr_get_tuner_gains(dev, NULL);
        if (gain_count > 0) {
            int *gains = (int *)malloc(sizeof(int) * gain_count);
            rtlsdr_get_tuner_gains(dev, gains);
            
            int gain_index = (int)(gain * (gain_count - 1));
            if (gain_index < 0) gain_index = 0;
            if (gain_index >= gain_count) gain_index = gain_count - 1;
            
            rtlsdr_set_tuner_gain(dev, gains[gain_index]);
            
            free(gains);
        }
    }
    
    RETURN_OK();
}

int TSDRPLUGIN_API __stdcall tsdrplugin_stop(void) {
    is_running = 0;
    
    if (dev != NULL) {
        rtlsdr_cancel_async(dev);
        usleep(100000); //100ms delay 
    }
    
    RETURN_OK();
}

char * TSDRPLUGIN_API __stdcall tsdrplugin_getlasterrortext(void) {
    if (errormsg_code == TSDR_OK)
        return NULL;
    return errormsg;
}

int TSDRPLUGIN_API __stdcall tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx) {
    if (dev == NULL) {
        RETURN_EXCEPTION("Device not initialized. Call tsdrplugin_init first.", TSDR_ERR_PLUGIN);
    }
    
    user_callback = cb;
    user_ctx = ctx;
    is_running = 1;
    
    rtlsdr_reset_buffer(dev);
    
    int r = rtlsdr_read_async(dev, rtlsdr_callback, NULL, 0, 0);
    
    is_running = 0;
    
    if (r < 0 && r != -6) { // -6 is RTLSDR_ERR_CANCELED (normal stop)
        RETURN_EXCEPTION("Async read failed. Device may have been disconnected.", TSDR_ERR_PLUGIN);
    }
    
    RETURN_OK();
}

void TSDRPLUGIN_API __stdcall tsdrplugin_cleanup(void) {
    is_running = 0;
    
    if (dev != NULL) {
        rtlsdr_cancel_async(dev);
        usleep(200000); //200ms delay
    }
    
    if (conversion_buffer != NULL) {
        free(conversion_buffer);
        conversion_buffer = NULL;
        conversion_buffer_size = 0;
    }
    
    if (dev != NULL) {
        rtlsdr_close(dev);
        dev = NULL;
    }
    
    if (errormsg != NULL) {
        free(errormsg);
        errormsg = NULL;
        errormsg_size = 0;
    }
}
