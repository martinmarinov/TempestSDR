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

#define FRAMERATE_PLL_SPEED (0.000001)
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

static inline void findbestfit(float * data, const int size, const float totalsum, const int stripsize, double * bestfit, int * bestfitid) {
	const double bigstripsizef = size - stripsize;
	const double stripsizef = stripsize;

	int i;
	double currsum = 0.0;
	for (i = 0; i < stripsize; i++) currsum += data[i];

	// totalsum - currsum is the sum in the remainder of the strip
	// we want to maximize the difference squared
	const double bestfitsqzero = (totalsum - currsum)/bigstripsizef - currsum/stripsizef;
	*bestfit = bestfitsqzero * bestfitsqzero;
	*bestfitid = 0;

	const int size1 = size - 1;
	const int sizemstepsize = size-stripsize;
	for (i = 0; i < size1; i++) {
		// i is the id to remove from the sum
		// i + stripsize is the id to add to the sum
		const double datatoremove = data[i];
		const int toremoveid = (i < sizemstepsize) ? (i+stripsize) : (i-sizemstepsize); // (i+stripsize) % size
		const double datatoadd = data[toremoveid];

		currsum = currsum - datatoremove + datatoadd;
		const double bestfitsq = (totalsum - currsum)/bigstripsizef - currsum/stripsizef;
		const double bestfitcurr = bestfitsq * bestfitsq;

		if (bestfitcurr > *bestfit) {
				*bestfit = bestfitcurr;
				*bestfitid = i;
		}
	}
}

#define SWEETSPOT_INIT {.curr_stripsize=0}
typedef struct sweetspot_data {
	int curr_stripsize;
} sweetspot_data_t;

#define RUNWITH_SIZE(newsize) \
	stripsize = newsize; \
	if (stripsize >= minsize && stripsize < size2 && stripsize != db->curr_stripsize) { \
		findbestfit(data, size, totalsum, stripsize, &bestfit_temp, &beststripstart_temp); \
		if (bestfit_temp > bestfit) { \
			bestfit = bestfit_temp; \
			beststripstart = beststripstart_temp; \
			beststripsize = stripsize; \
		}; \
	}

int findthesweetspot(sweetspot_data_t * db, float * data, int size, int minsize) {
	int i;
	const int size2 = size >> 1;

	if (db->curr_stripsize < minsize) db->curr_stripsize = minsize;
	else if (db->curr_stripsize > size2) db->curr_stripsize = size2;

	gaussianblur(data, size);

	double totalsum = 0.0;
	for (i = 0; i < size; i++) totalsum += data[i];

	int beststripstart, beststripstart_temp, beststripsize = db->curr_stripsize, stripsize = db->curr_stripsize;
	double bestfit, bestfit_temp;

	// look for the best fit of strip size stripsize
	findbestfit(data, size, totalsum, stripsize, &bestfit, &beststripstart);

	RUNWITH_SIZE(db->curr_stripsize-4);
	RUNWITH_SIZE(db->curr_stripsize+4);
	RUNWITH_SIZE(db->curr_stripsize >> 1);
	RUNWITH_SIZE(db->curr_stripsize << 1);

	db->curr_stripsize = beststripsize;


	data[beststripstart] = PIXEL_SPECIAL_VALUE_B;
	data[(beststripstart+beststripsize) % size] = PIXEL_SPECIAL_VALUE_B;

	return (beststripstart + beststripsize/2) % size;
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

void setframerate(tsdr_lib_t * tsdr, double refreshrate) {
	tsdr->pixelrate = tsdr->width * tsdr->height * refreshrate;
	tsdr->pixeltime = 1.0/tsdr->pixelrate;
	tsdr->refreshrate = refreshrate;
	if (tsdr->sampletime != 0)
		tsdr->pixeltimeoversampletime = tsdr->pixeltime /  tsdr->sampletime;
	announce_callback_changed(tsdr, VALUE_ID_PLL_FRAMERATE, refreshrate, 0);
}

void frameratepll(tsdr_lib_t * tsdr, int dx) {
	static int lastx = 0;
	const int rawvx = dx - lastx;
	const int h2 = tsdr->height / 2;
	const int vx = (rawvx > h2) ? (rawvx - h2) : ((rawvx < -h2) ? (rawvx + h2) : (rawvx));
	const int absvx = (vx < 0) ? (-vx) : vx;
	const int vxsign = (vx < 0) ? (-1) : (1);
	lastx = dx;

	if (!tsdr->params_int[PARAM_INT_AUTORESOLUTION] && tsdr->params_int[PARAM_INT_FRAMERATE_PLL] && vx != 0 && absvx < tsdr->height / 5) {
		double frameratediff = (absvx < 2) ? (FRAMERATE_PLL_SPEED*vxsign) : (2*FRAMERATE_PLL_SPEED*vxsign);

		setframerate(tsdr, tsdr->refreshrate-frameratediff);
	}
}

float * syncdetector_run(tsdr_lib_t * tsdr, float * data, float * outputdata, int width, int height, float * widthbuffer, float * heightbuffer) {

	static sweetspot_data_t db_x = SWEETSPOT_INIT, db_y = SWEETSPOT_INIT;

	const int dx = findthesweetspot(&db_x, widthbuffer, width, width * 0.05f );
	const int dy = findthesweetspot(&db_y, heightbuffer, height, height * 0.01f );

	const int size = width * height;


	// do the framerate pll
	frameratepll(tsdr, dx);

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
