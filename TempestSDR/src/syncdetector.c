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
#include "gaussian.h"

#define FRAMERATE_PLL_FINE_SPEED (0.0000003)
#define FRAMERATE_PLL_SPEED (0.00001)
#define FRAMERATE_MAX_PLL_SPEED (0.0001)


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

void setframerate(tsdr_lib_t * tsdr, double refreshrate, int width, int height) {
	tsdr->pixelrate = width * height * refreshrate;
	tsdr->pixeltime = 1.0/tsdr->pixelrate;
	tsdr->refreshrate = refreshrate;
	if (tsdr->sampletime != 0)
		tsdr->pixeltimeoversampletime = tsdr->pixeltime /  tsdr->sampletime;
	announce_callback_changed(tsdr, VALUE_ID_PLL_FRAMERATE, refreshrate, 0);
}

void frameratepll(tsdr_lib_t * tsdr, int dx, int width, int height) {
	static int lastx = 0;
	const int rawvx = dx - lastx;
	const int h2 = height / 2;
	const int vx = (rawvx > h2) ? (rawvx - h2) : ((rawvx < -h2) ? (rawvx + h2) : (rawvx));
	const int absvx = (vx < 0) ? (-vx) : vx;
	const int vxsign = (vx < 0) ? (-1) : (1);
	lastx = dx;

	if (tsdr->params_int[PARAM_INT_FRAMERATE_PLL] && vx != 0 && absvx < height / 5) {
		double frameratediff = (absvx > (height / 30)) ? (FRAMERATE_PLL_SPEED*vx) : (vxsign*FRAMERATE_PLL_FINE_SPEED);
		if (frameratediff > FRAMERATE_MAX_PLL_SPEED) frameratediff = FRAMERATE_MAX_PLL_SPEED;
		else if (frameratediff < -FRAMERATE_MAX_PLL_SPEED) frameratediff = - FRAMERATE_MAX_PLL_SPEED;

		setframerate(tsdr, tsdr->refreshrate-frameratediff, width, height);
	}
}

float * syncdetector_run(tsdr_lib_t * tsdr, float * data, float * outputdata, int width, int height, float * widthbuffer, float * heightbuffer, int greenlines) {

	const int dx = findthesweetspot(&tsdr->db_x, widthbuffer, width, width * 0.05f );
	const int dy = findthesweetspot(&tsdr->db_y, heightbuffer, height, height * 0.01f );

	const int size = width * height;


	// do the framerate pll
	frameratepll(tsdr, dx, width, height);

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
#if PIXEL_SPECIAL_COLOURS_ENABLED
		if (greenlines) {
			verticalline(dx, data, width, height, PIXEL_SPECIAL_VALUE_G);
			horizontalline(dy, data, width, height, PIXEL_SPECIAL_VALUE_G);
		}
#endif
		return data;
	}

}
