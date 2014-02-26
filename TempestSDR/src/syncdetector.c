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
#include "syncdetector.h"
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



// calculate the flattness of a strip in circular data
// sum is the sum of all of the elements in the strip
// offset is where the strip starts
// stripsize is the size of the strip
// return value is >=0. The bigger it is the less flat the curve is
float calculateroughness(float * data, int size, int offset, int stripsize, float sum) {
	// feedback loop, if we were growing and this is giving worse results, stop growing or the other way around
		int i;
		const int stripend = offset+stripsize;
		int c = 0;
		const int halfstripsize = stripsize >> 1;
		const double quorterstripsizef = stripsize / 4.0;
		float trianglecenteredsum = 0.0;
		for (i = offset; i < stripend; i++) {
			const int realid = (i < size) ? (i) : (i - size); // i % size
			const double coeff = ((c < halfstripsize) ? (c/quorterstripsizef) : (4.0-c/quorterstripsizef));
			trianglecenteredsum += coeff * data[realid];
			c++;
		}

		// difference between bestfitsum and triangled sum
		const double dbestfitcurrent = (sum - trianglecenteredsum) / (float) stripsize;

		// the bigger the value is, the less flat the data is
		return dbestfitcurrent * dbestfitcurrent;
}

#define SWEETSPOT_INIT {.growsize=0, .stripsize_val=0, .bestfit = 0.0f}
typedef struct sweetspot_data {
	int growsize;
	int stripsize_val;

	double bestfit;
} sweetspot_data_t;

int findthesweetspot(sweetspot_data_t * db, float * data, int size, int minsize) {
	int i;
	const int size2 = size >> 1;

	db->stripsize_val+=db->growsize;

	if (db->stripsize_val > size2) db->stripsize_val = size2;
	if (db->stripsize_val < minsize) db->stripsize_val = minsize;

	const int stripsize = db->stripsize_val;
	const double stripsizef = stripsize;
	const double bigstripsizef = size - stripsize;

	gaussianblur(data, size);

	double currsum = 0.0;
	for (i = 0; i < stripsize; i++) currsum += data[i];

	double totalsum = currsum;
	for (i = stripsize; i < size; i++) totalsum += data[i];

	// totalsum - currsum is the sum in the remainder of the strip
	// we want to maximize the difference squared
	const double bestfitsqzero = (totalsum - currsum)/bigstripsizef - currsum/stripsizef;
	double bestfit = bestfitsqzero * bestfitsqzero;
	int bestfitid = 0;

	const int size1 = size - 1;
	const int sizemstepsize = size-stripsize;
	float roughness = INFINITY;
	for (i = 0; i < size1; i++) {
		// i is the id to remove from the sum
		// i + stripsize is the id to add to the sum
		const double datatoremove = data[i];
		const int toremoveid = (i < sizemstepsize) ? (i+stripsize) : (i-sizemstepsize); // (i+stripsize) % size
		const double datatoadd = data[toremoveid];

		currsum = currsum - datatoremove + datatoadd;
		const double bestfitsq = (totalsum - currsum)/bigstripsizef - currsum/stripsizef;
		const double bestfitcurr = bestfitsq * bestfitsq;

		if (bestfitcurr > bestfit) {
			const float roughnesscurr = calculateroughness(data, size, i, stripsize, currsum);
			if (roughnesscurr < roughness) {
				roughness = roughnesscurr;
				bestfit = bestfitcurr;
				bestfitid = i;
			}
		}
	}

	int dirtochange = 0;
	if (bestfit < db->bestfit) dirtochange++;
	db->growsize = (db->growsize > 0) ? (-dirtochange) : (dirtochange);
	db->bestfit = bestfit;

	const double sqrtbestfit = sqrtf(bestfit);
	const int stripend = bestfitid+stripsize;
	for (i = bestfitid; i < stripend; i++) data[i % size] = sqrtbestfit;
	data[bestfitid] = PIXEL_SPECIAL_VALUE_B;
	data[stripend % size] = PIXEL_SPECIAL_VALUE_B;

	return (bestfitid + stripsize/2) % size;
}

void verticalline(int x, float * data, int width, int height, float val) {
	int i;
	for (i = 0; i < height; i++)
		data[x + width*i] = val;
}

void horizontalline(int y, float * data, int width, int height, float val) {
	int i;
	for (i = 0; i < width; i++)
		data[i+width*y] = val;
}

float * syncdetector_run(tsdr_lib_t * tsdr, float * data, float * outputdata, int width, int height, float * widthbuffer, float * heightbuffer) {

	static sweetspot_data_t db_x = SWEETSPOT_INIT, db_y = SWEETSPOT_INIT;

	static float olddx = 0.0f, olddy = 0.0f;

	int origdx = findthesweetspot(&db_x, widthbuffer, width, width * 0.1f );
	int origdy = findthesweetspot(&db_y, heightbuffer, height, height * 0.01f );

	if (origdx - olddx > width / 2) olddx += width; else if (olddx - origdx > width / 2) origdx += width;
	if (origdy - olddy > height / 2) olddy += height; else if (olddy - origdy > height / 2) origdy += height;

	olddx = ((int) (0.5f*origdx + 0.5f*olddx)) % width;
	olddy = ((int) (0.5f*origdy + 0.5f*olddy)) % height;

	const int dx = olddx;
	const int dy = olddy;

	int i;
	const int size = width * height;

	if (tsdr->params_int[PARAM_INT_AUTOPIXELRATE]) {
		for (i = 0; i < size; i++) {
			const int x = i % width;
			const int y = i / width;

			if (y < 200 && y < x) {
				const float wbv = widthbuffer[x];
				if (wbv == PIXEL_SPECIAL_VALUE_B || wbv == PIXEL_SPECIAL_VALUE_G || wbv == PIXEL_SPECIAL_VALUE_R)
					data[i] = wbv;
				else if (wbv != PIXEL_SPECIAL_VALUE_TRANSPARENT)
					data[i] = wbv / (float) height;
			} else if (x < 200 && y > x) {
				const float hbv = heightbuffer[y];
				if (hbv == PIXEL_SPECIAL_VALUE_B || hbv == PIXEL_SPECIAL_VALUE_G || hbv == PIXEL_SPECIAL_VALUE_R)
					data[i] = hbv;
				else if (hbv != PIXEL_SPECIAL_VALUE_TRANSPARENT)
					data[i] = hbv / (float) width;
			}

		}
	}

	// do the shift itself
	if (tsdr->params_int[PARAM_INT_AUTOSHIFT]) {
		// fix the y offset
		const int ypixels = dy*width;
		const int ypixelsrem = size-ypixels;
		const int xrem = width - dx;
		const int xremfloatsize = xrem * sizeof(float);
		const int dxorigfloatsize = dx * sizeof(float);

		int yshift = 0;
		for (yshift = 0; yshift < ypixels; yshift+=width) {
			const int offset = ypixelsrem+yshift;
			memcpy(&outputdata[offset+xrem], &data[yshift], dxorigfloatsize); // TL to BR
			memcpy(&outputdata[offset], &data[yshift+dx], xremfloatsize); // TR to BL
		}
		for (yshift = ypixels; yshift < size; yshift+=width) {
			const int offset = yshift - ypixels;
			memcpy(&outputdata[offset+xrem], &data[yshift], dxorigfloatsize); // BL to TR
			memcpy(&outputdata[offset], &data[yshift+dx], xremfloatsize); // BR to TL
		}

		return outputdata;
	} else {
		verticalline(dx, data, width, height, PIXEL_SPECIAL_VALUE_G);
		horizontalline(dy, data, width, height, PIXEL_SPECIAL_VALUE_G);
		return data;
	}

}
