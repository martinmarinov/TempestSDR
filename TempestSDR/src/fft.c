#include "fft.h"
#include <math.h>

uint32_t fft_getrealsize(uint32_t size) {
	uint32_t m =0;
	while ((size /= 2) != 0)
		m++;

	return 1 << m;
}

// out should have a size of 2*in_size
void real_to_complex(float * out, float * in, int in_size) {
	int i;

	// set the real ids in answer to the val, the imaginary ones to 0
	for (i = 0; i < in_size; i++) {
		*(out++) = *(in++);
		*(out++) = 0.0f;
	}
}

void complex_to_real(float * data, int data_size){
	float * src = data;
	const int size2 = data_size/2;
	int i;
	for (i = 0; i < size2; i++) {
		const float I = *(src++);
		const float Q = *(src++);
		*(data++) = sqrtf(I*I+Q*Q);
	}
}

// the array temp needs to be of size at least 2*size
// the answer will be written in answer at entries fro 0 to size
void fft_autocorrelation(float * answer, float * real, uint32_t size) {
	uint32_t i;

	// set the real ids in answer to the val, the imaginary ones to 0
	real_to_complex(answer, real, size);

	// do the fft
	uint32_t fft_size = fft_getrealsize(size);
	uint32_t fft_size2 = fft_size * 2;
	fft_perform(answer, fft_size, 0);

	// get the abs value
	for (i = 0; i < fft_size2; i+=2) {
		const int i1 = i+1;
		const float I = answer[i];
		const float Q = answer[i1];
		answer[i] = sqrtf(I*I+Q*Q);
		answer[i1] = 0;
	}

	// get the ifft
	fft_perform(answer, fft_size, 1);

	// get the abs value
	complex_to_real(answer, fft_size2);
}

// answer_out and answer_temp need to have size of size * 2
// a and b need to have size of size
// the final answer can be found in answer_out
void fft_crosscorrelation(float * answer_out, float * answer_temp, float * a, float * b, int size) {
	uint32_t i;

	// turn them to complex numbers
	real_to_complex(answer_out, a, size);
	real_to_complex(answer_temp, b, size);

	uint32_t fft_size = fft_getrealsize(size);
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

		answer_out[i] = aI*bI + aQ*bQ;
		answer_out[i1] = aI*bQ-aQ*bI;
	}

	// get the ifft
	fft_perform(answer_out, fft_size, 1);

	// get the abs value
	complex_to_real(answer_out, fft_size2);
}

// iq must have a size of size*2
void fft_perform(float * iq, uint32_t size, int inverse)
{
	int64_t i,i1,j,k,i2,l,l1,l2;
	float c1,c2,tx,ty,t1,t2,u1,u2,z;

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
		c2 = sqrtf((1.0f - c1) / 2.0f);
		if (!inverse)
			c2 = -c2;
		c1 = sqrtf((1.0f + c1) / 2.0f);
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
