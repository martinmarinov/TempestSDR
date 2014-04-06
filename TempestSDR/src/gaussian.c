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

#include <math.h>

#define GAUSSIAN_ALPHA (1.0f)

// N is the number of points, i is between -(N-1)/2 and (N-1)/2 inclusive
#define CALC_GAUSSCOEFF(N,i) expf(-2.0f*GAUSSIAN_ALPHA*GAUSSIAN_ALPHA*i*i/(N*N))
void gaussianblur(float * data, int size) {
	static float norm = 0.0f, c_2 = 0.0f, c_1 = 0.0f, c0 = 0.0f, c1 = 0.0f, c2 = 0.0f;
	if (norm == 0.0f) {
		// calculate only first time we run
		norm = CALC_GAUSSCOEFF(5,-2)+CALC_GAUSSCOEFF(5, -1)+CALC_GAUSSCOEFF(5, 0)+CALC_GAUSSCOEFF(5, 1)+CALC_GAUSSCOEFF(5, 2);
		c_2 = CALC_GAUSSCOEFF(5, -2) / norm;
		c_1 = CALC_GAUSSCOEFF(5, -1) / norm;
		c0 = CALC_GAUSSCOEFF(5, 0) / norm;
		c1 = CALC_GAUSSCOEFF(5, 1) / norm;
		c2 = CALC_GAUSSCOEFF(5, 2) / norm;
	}

	float p_2, p_1, p0, p1, p2, data_2, data_3, data_4;
	if (size < 5) {
		p_2 = data[0];
		p_1 = data[1 % size];
		p0 = data[2 % size];
		p1 = data[3 % size];
		p2 = data[4 % size];
	} else {
		p_2 = data[0];
		p_1 = data[1];
		p0 = data[2];
		p1 = data[3];
		p2 = data[4];
	}

	data_2 = p0;
	data_3 = p1;
	data_4 = p2;

	int i;
	const int sizem2 = size - 2;
	const int sizem5 = size - 5;
	for (i = 0; i < size; i++) {

		const int idtoupdate = (i < sizem2) ? (i + 2) : (i - sizem2);
		const int nexti = (i < sizem5) ? (i + 5) : (i - sizem5);

		data[idtoupdate] = p_2 * c_2 + p_1 * c_1 + p0 * c0 + p1 * c1 + p2 * c2;
		p_2 = p_1;
		p_1 = p0;
		p0 = p1;
		p1 = p2;

		if (nexti < 2 || nexti >= 5)
			p2 = data[nexti];
		else {
			switch (nexti) {
			case 2:
				p2 = data_2;
				break;
			case 3:
				p2 = data_3;
				break;
			case 4:
				p2 = data_4;
				break;
			}
		}
	}
}

