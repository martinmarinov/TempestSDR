/*
#-------------------------------------------------------------------------------
# Copyright (c) 2014 Martin Marinov.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the GNU Public License v3.0
# which accompanies this distribution, and is available at
# http://www.gnu.org/licenses/gpl.html
#
# Contributors:
#     Martin Marinov - initial API and implementation
#-------------------------------------------------------------------------------
*/

#include "internaldefinitions.h"

#ifndef SUPERBANDWIDTH_H_
#define SUPERBANDWIDTH_H_

#include "extbuffer.h"
#include "threading.h"

typedef struct {
	int state;
	int samples_to_gather;
	int samples_gathered;
	int samples_in_frame;

	int samples_to_pause;

	uint32_t samplerate;

	float ** buffs;
	int buffscount;
	int buffsbuffcount;
	int buffid_current;

	float * outbuf;
	int outbufsize;
	tsdr_lib_t * tsdr;

	mutex_t thread_unlock;

	extbuffer_t extb;

	extbuffer_t extb_out;
	extbuffer_t extb_temp;

	volatile int alive;
} superbandwidth_t;

void superb_init(superbandwidth_t * bw);
void superb_free(superbandwidth_t * bw);
void superb_run(superbandwidth_t * bw, float * iq, int size, tsdr_lib_t * tsdr, int dropped, float ** outbuff, int * outbufsize);
void superb_stop(superbandwidth_t * bw, tsdr_lib_t * tsdr);
void super_startthread(superbandwidth_t * bw);
void super_stopthread(superbandwidth_t * bw);


#endif /* SUPERBANDWIDTH_H_ */
