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
#ifndef ERRORS_H_
#define ERRORS_H_

#include "TSDRCodes.h"
#include <string.h>
#include <stdlib.h>

extern "C" {

int errormsg_code;
char * errormsg = NULL;
int errormsg_size = 0;
#define RETURN_EXCEPTION(message, status) {announceexception(message, status); return status;}
#define RETURN_OK() {errormsg_code = TSDR_OK; return TSDR_OK;}

static inline void announceexception(const char * message, int status) {

	errormsg_code = status;
	if (status == TSDR_OK) return;

	const int length = strlen(message);
	if (errormsg_size == 0) {
		errormsg_size = length;
		errormsg = (char *)malloc(errormsg_size + 1);
		errormsg[errormsg_size] = 0;
	}
	else if (length > errormsg_size) {
		errormsg_size = length;
		errormsg = (char *)realloc((void*)errormsg, errormsg_size + 1);
		errormsg[errormsg_size] = 0;
	}


	strcpy(errormsg, message);
}

}

EXTERNC TSDRPLUGIN_API char * __stdcall tsdrplugin_getlasterrortext(void) {
	if (errormsg_code == TSDR_OK)
		return NULL;
	else
		return errormsg;
}

#endif
