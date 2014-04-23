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
#ifndef _TSDRThreading
#define _TSDRThreading

// A platform independed threading library with mutex support

	#include "osdetect.h"
	#include <stdint.h>

	#define THREAD_OK (0)
	#define THREAD_TIMEOUT (1)
	#define THREAD_NOT_INITED (2)

	typedef void(*thread_function)(void *ctx);

	struct mutex {
		void * thing1;
		void * thing2;
		volatile int valid;
	} typedef mutex_t;

	struct semaphore {
		int count;
		mutex_t locker;
		mutex_t signaller;
	} typedef semaphore_t;

	struct locking_variable {
		volatile int value;
		mutex_t signaller;
		mutex_t locker;
		volatile int someonewaiting;
		volatile int set;
	} typedef locking_variable_t;

#define THREAD_INIT ={NULL, NULL}

	void thread_start(thread_function f, void * ctx);
	void thread_sleep(uint32_t milliseconds);

	// warning, there is not mutex_free so calling too many mutex_init could be a memory leak!
	void mutex_init(mutex_t * mutex);
	int mutex_wait(mutex_t * mutex);
	int mutex_waitforever(mutex_t * mutex);
	void mutex_signal(mutex_t * mutex);
	void mutex_free(mutex_t * mutex);

	void critical_enter(mutex_t * mutex);
	void critical_leave(mutex_t * mutex);

	void semaphore_init(semaphore_t * semaphore);
	void semaphore_enter(semaphore_t * semaphore);
	void semaphore_leave(semaphore_t * semaphore);
	void semaphore_wait(semaphore_t * semaphore);
	void semaphore_free(semaphore_t * semaphore);

	void lockvar_init(locking_variable_t * var);
	int lockvar_waitandgetval(locking_variable_t * var);
	void lockvar_free(locking_variable_t * var);
	void lockvar_setval(locking_variable_t * var, int value);

#endif
