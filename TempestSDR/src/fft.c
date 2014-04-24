#include "fft.h"
#include <math.h>

uint32_t fft_getrealsize(uint32_t size) {
	int m =0;
	while ((size /= 2) != 0)
		m++;

	return 1 << m;
}

void fft_perform(float * iq, uint32_t size, int inverse)
{
	long i,i1,j,k,i2,l,l1,l2;
	float c1,c2,tx,ty,t1,t2,u1,u2,z;

	int m =0;
	while ((size /= 2) != 0)
		m++;

	int nn = 1 << m;
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
