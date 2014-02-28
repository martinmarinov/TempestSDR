/*******************************************************************************
 * Copyright (c) 2014 Martin Marinov.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Public License v3.0
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/gpl.html
 *
 * Contributors:
 *     Martin Marinov - initial API and implementation
 ******************************************************************************/

#include "fft.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define ONEOVSQ2 (0.707106781186547524400844362104849f)

void fft_real(float * data, int m) {
	int i, j, k, i2, l, l1, l2;
	float c1, c2;

	int n = 1 << m;
	i2 = n >> 1;
	j = 0;

	for (i = 0; i < n - 1; i++) {

		if (i < j) {
			const float tx = data[i];

			data[i] = data[j];
			data[j] = tx;
		}
		k = i2;
		while (k <= j) {
			j -= k;
			k >>= 1;
		}
		j += k;
	}

	c1 = -1.0f;
	c2 = 0.0f;
	l2 = 1;
	for (l = 0; l < m; l++) {
		float u1 = 1.0f;
		float u2 = 0.0f;

		l1 = l2;
		l2 <<= 1;

		for (j = 0; j < l1; j++) {
			float z;

			for (i = j; i < n; i += l2) {
				const int i1 = i + l1;

				const float t1 = u1 * data[i1];

				data[i1] = data[i] - t1;
				data[i] += t1;
			}

			z = u1 * c1 - u2 * c2;
			u2 = u1 * c2 + u2 * c1;
			u1 = z;
		}
		c2 = ONEOVSQ2 * sqrtf(1.0f - c1);
		c2 = -c2;
		c1 = ONEOVSQ2 * sqrtf(1.0f + c1);
	}

	for (i = 0; i < n; i++)
		data[i] /= n;

}


