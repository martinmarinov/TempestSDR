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
	} typedef mutex_t;

#define THREAD_INIT ={NULL, NULL}

	void thread_start(thread_function f, void * ctx);
	void thread_sleep(uint32_t milliseconds);

	// warning, there is not mutex_free so calling too many mutex_init could be a memory leak!
	void mutex_init(mutex_t * mutex);
	int mutex_wait(mutex_t * mutex);
	void mutex_signal(mutex_t * mutex);
	void mutex_free(mutex_t * mutex);

	void critical_enter(mutex_t * mutex);
	void critical_leave(mutex_t * mutex);

#endif
