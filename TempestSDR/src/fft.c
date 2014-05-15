#include "fft.h"
#include <math.h>
#include <string.h>

uint32_t fft_getrealsize(uint32_t size) {
	uint32_t m =0;
	while ((size /= 2) != 0)
		m++;

	return 1 << m;
}

// out should have a size of 2*in_size
void real_to_complex(float * out, float * in, int in_size) {
	uint32_t i;

	// set the real ids in answer to the val, the imaginary ones to 0
	for (i = 0; i < in_size; i++) {
		*(out++) = *(in++);
		*(out++) = 0.0f;
	}
}

void complex_to_real(float * data, int samples){
	float * src = data;
	uint32_t i;
	for (i = 0; i < samples; i++) {
		const float I = *(src++);
		const float Q = *(src++);
		*(data++) = sqrtf(I*I+Q*Q);
	}
}

void fft_complex_to_absolute_complex(float * data, int samples) {
	uint32_t fft_size2 = samples * 2;

	uint32_t i;
	for (i = 0; i < fft_size2; i+=2) {
		const int i1 = i+1;
		const float I = data[i];
		const float Q = data[i1];
		data[i] = sqrtf(I*I+Q*Q);
		data[i1] = 0;
	}
}

// the array temp needs to be of size at least 2*size
// the answer will be written in answer at entries fro 0 to 2*size
void fft_autocorrelation(float * answer, float * real, uint32_t size) {

	// set the real ids in answer to the val, the imaginary ones to 0
	real_to_complex(answer, real, size);

	// do the fft
	uint32_t fft_size = fft_getrealsize(size);

	fft_perform(answer, fft_size, 0);

	// get the abs value
	fft_complex_to_absolute_complex(answer, size);

	// get the ifft
	fft_perform(answer, fft_size, 1);
}

// a and b need to be complex with size samples * 2
// the final answer can be found in answer_out and it is complex
// you may need to take its absolute value to get the crosscorrelation
void fft_crosscorrelation(float * answer_out, float * answer_temp, uint32_t samples) {
	uint32_t i;

	uint32_t fft_size = fft_getrealsize(samples);
	uint32_t fft_size2 = fft_size * 2;

	// perform the ffts
	fft_perform(answer_out, fft_size, 0);
	fft_perform(answer_temp, fft_size, 0);

	// multiply the cojugate
	for (i = 0; i < fft_size2; i+=2) {
		const int i1 = i+1;
		const float aI = answer_out[i];
		const float aQ = answer_out[i1];
		const float bI = answer_temp[i];
		const float bQ = answer_temp[i1];

		answer_out[i]  = aI*bI + aQ*bQ;
		answer_out[i1] = aI*bQ - aQ*bI;
	}

	// get the ifft
	fft_perform(answer_out, fft_size, 1);
}

// iq must have a size of size*2
void fft_perform(float * iq, uint32_t size, int inverse)
{
	int64_t i,i1,j,k,i2,l,l1,l2;
	double c1,c2,tx,ty,t1,t2,u1,u2,z;

	int m =0;
	while ((size /= 2) != 0)
		m++;

	uint32_t nn = 1 << m;
	i2 = nn >> 1;
	j = 0;

	for (i=0;i<nn-1;i++) {
		if (i < j) {
			const uint32_t Ii = i << 1;
			const uint32_t Qi = Ii + 1;

			const uint32_t Ij = j << 1;
			const uint32_t Qj = Ij + 1;

			tx = iq[Ii];
			ty = iq[Qi];
			iq[Ii] = iq[Ij];
			iq[Qi] = iq[Qj];
			iq[Ij] = tx;
			iq[Qj] = ty;
		}
		k = i2;
		while (k <= j) {
			j -= k;
			k >>= 1;
		}
		j += k;
	}

	c1 = -1.0;
	c2 = 0.0;
	l2 = 1;
	for (l=0;l<m;l++) {
		l1 = l2;
		l2 <<= 1;
		u1 = 1.0f;
		u2 = 0.0f;
		for (j=0;j<l1;j++) {
			for (i=j;i<nn;i+=l2) {
				const uint32_t Ii = i << 1;
				const uint32_t Qi = Ii + 1;

				i1 = i + l1;

				const uint32_t Ii1 = i1 << 1;
				const uint32_t Qi1 = Ii1 + 1;

				t1 = u1 * iq[Ii1] - u2 * iq[Qi1];
				t2 = u1 * iq[Qi1] + u2 * iq[Ii1];
				iq[Ii1] = iq[Ii] - t1;
				iq[Qi1] = iq[Qi] - t2;
				iq[Ii] += t1;
				iq[Qi] += t2;
			}
			z =  u1 * c1 - u2 * c2;
			u2 = u1 * c2 + u2 * c1;
			u1 = z;
		}
		c2 = sqrt((1.0 - c1) / 2.0);
		if (!inverse)
			c2 = -c2;
		c1 = sqrt((1.0 + c1) / 2.0);
	}

	if (!inverse) {
		for (i=0;i<nn;i++) {
			const uint32_t Ii = i << 1;
			const uint32_t Qi = Ii + 1;

			iq[Ii] /= (float)nn;
			iq[Qi] /= (float)nn;
		}
	}
}
