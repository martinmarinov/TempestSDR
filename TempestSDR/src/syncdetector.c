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

#define FRAMERATE_DX_LOWPASS_COEFF_HEIGHT (0.1)
#define FRAMERATE_DX_LOWPASS_COEFF_WIDTH (0.9)

#define FRAMERATE_PLL_SPEED_HI (0.00001)
#define FRAMERATE_PLL_SPEED_LO (0.000001)
#define FRAMERATE_PLL_LOCKED_VALUE (0.5)

#define SYNCDETECTOR_STATE_NOT_LOCKED (0)
#define SYNCDETECTOR_STATE_LOCKED (1)


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

void findthesweetspot(sweetspot_data_t * db, float * data, int size, int minsize, double lowpasscoeff) {
	int i;
	if (minsize < 1) minsize = 1;
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

	const int h2 = size / 2;

	int dxwithnolowpass = (beststripstart + beststripsize/2) % size;
	const int rawdiff = dxwithnolowpass - db->dx;
	if (rawdiff > h2)
		db->dx += size;
	else if (rawdiff < -h2)
		dxwithnolowpass += size;

	const int lastx = db->dx;
	db->dx = ((int64_t) round(dxwithnolowpass * lowpasscoeff + (1.0 - lowpasscoeff) * db->dx)) % ((int64_t) size);

	const int rawvx = db->dx - lastx;

	db->vx = (rawvx > h2) ? (size - rawvx) : ((rawvx < -h2) ? (-size - rawvx) : (rawvx));
	db->absvx = (db->vx >= 0) ? (db->vx) : (-db->vx);


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

void frameratepll(syncdetector_t * sy, tsdr_lib_t * tsdr, sweetspot_data_t * db_x, int width, int height) {
	sy->avg_speed = sy->avg_speed * 0.99 + 0.01 * db_x->vx;

	if (sy->avg_speed < FRAMERATE_PLL_LOCKED_VALUE && sy->avg_speed > -FRAMERATE_PLL_LOCKED_VALUE)
		sy->state = SYNCDETECTOR_STATE_LOCKED;
	else
		sy->state = SYNCDETECTOR_STATE_NOT_LOCKED;

	if ( tsdr->params_int[PARAM_INT_FRAMERATE_PLL] && db_x->vx != 0 ) {
		double frameratediff;

		if (sy->state == SYNCDETECTOR_STATE_NOT_LOCKED)
			frameratediff = db_x->vx * FRAMERATE_PLL_SPEED_HI;
		else
			frameratediff = sy->avg_speed * FRAMERATE_PLL_SPEED_LO;

		tsdr->refreshrate-=frameratediff;
		set_internal_samplerate(tsdr, tsdr->samplerate);
		announce_callback_changed(tsdr, VALUE_ID_PLL_FRAMERATE, tsdr->refreshrate, 0);
	}
}

void syncdetector_init(syncdetector_t * sy) {
	sy->db_x.curr_stripsize = 0;
	sy->db_x.dx = 0;
	sy->db_x.vx= 0;
	sy->db_x.absvx = 0;

	sy->db_y.curr_stripsize = 0;
	sy->db_y.dx = 0;
	sy->db_y.vx= 0;
	sy->db_y.absvx = 0;

	sy->last_frame_diff = 0.0;
	sy->state = SYNCDETECTOR_STATE_NOT_LOCKED;
	sy->avg_speed = 0;
}

float * syncdetector_run(syncdetector_t * sy, tsdr_lib_t * tsdr, float * data, float * outputdata, int width, int height, float * widthbuffer, float * heightbuffer, int greenlines, int modify_data_allowed) {

	findthesweetspot(&sy->db_x, widthbuffer, width, width * 0.05f, FRAMERATE_DX_LOWPASS_COEFF_WIDTH);
	findthesweetspot(&sy->db_y, heightbuffer, height, height * 0.01f, FRAMERATE_DX_LOWPASS_COEFF_HEIGHT );

	const int size = width * height;

	// do the framerate pll
	frameratepll(sy, tsdr, &sy->db_x, width, height);

	// do the shift itself
	if (tsdr->params_int[PARAM_INT_AUTOSHIFT]) {
		// fix the y offset
		const int ypixels = sy->db_y.dx*width;
		const int ypixelsrem = size-ypixels;
		const int xrem = width - sy->db_x.dx;
		const int xremfloatsize = xrem * sizeof(float);
		const int dxorigfloatsize = sy->db_x.dx * sizeof(float);

		int yshift = 0;
		for (yshift = 0; yshift < ypixels; yshift+=width) {
			const int offset = ypixelsrem+yshift;
			memcpy(&outputdata[offset+xrem], &data[yshift], dxorigfloatsize); // TL to BR
			memcpy(&outputdata[offset], &data[yshift+sy->db_x.dx], xremfloatsize); // TR to BL
		}
		for (yshift = ypixels; yshift < size; yshift+=width) {
			const int offset = yshift - ypixels;
			memcpy(&outputdata[offset+xrem], &data[yshift], dxorigfloatsize); // BL to TR
			memcpy(&outputdata[offset], &data[yshift+sy->db_x.dx], xremfloatsize); // BR to TL
		}

		return outputdata;
	} else {
#if PIXEL_SPECIAL_COLOURS_ENABLED
		if (greenlines && modify_data_allowed) {
			verticalline(sy->db_x.dx, data, width, height, PIXEL_SPECIAL_VALUE_G);
			horizontalline(sy->db_y.dx, data, width, height, PIXEL_SPECIAL_VALUE_G);
			return data;
		} else if (greenlines && !modify_data_allowed) {
			memcpy(outputdata, data, width * height * sizeof(float));
			verticalline(sy->db_x.dx, outputdata, width, height, PIXEL_SPECIAL_VALUE_G);
			horizontalline(sy->db_y.dx, outputdata, width, height, PIXEL_SPECIAL_VALUE_G);
			return outputdata;
		} else
			return data;
#else
		return data;
#endif
	}

}
