
#ifndef FFT_H_
#define FFT_H_

#include <stdint.h>

uint32_t fft_getrealsize(uint32_t size);
void fft_perform(float * iq, uint32_t size, int inverse);
void fft_autocorrelation(float * answer, float * real, uint32_t size);

#endif
