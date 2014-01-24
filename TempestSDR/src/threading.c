#include "threading.h"
#include <stdio.h>
#include <stdlib.h>

// A platform independent threading library with mutexes
// based on: http://www.cs.wustl.edu/~schmidt/win32-cv-1.html


#if WINHEAD
	#include <windows.h>

	#define ETIMEDOUT WAIT_TIMEOUT

	typedef CRITICAL_SECTION pthread_mutex_t;

	struct winthreadargs {
		thread_function f;
		void * ctx;
	}typedef winthreadargs_t;

	DWORD WINAPI threadFunc(LPVOID arg) {
		winthreadargs_t * args = (winthreadargs_t *) arg;
		thread_function f = args->f;
		void * ctx = args->ctx;
		free(args);
		f(ctx);
		return 0;
	}
#else
	#include <errno.h>
	#include <pthread.h>
	#include <unistd.h>
#endif

	void thread_sleep(uint32_t milliseconds) {
#if WINHEAD
		Sleep(milliseconds);
#else
		usleep(1000 * milliseconds);
#endif
	}

	void thread_start(thread_function f, void * ctx) {
#if WINHEAD
		winthreadargs_t * args = (winthreadargs_t *) malloc(sizeof(winthreadargs_t));
		args->f = f;
		args->ctx = ctx;
		CreateThread(NULL,0,threadFunc,(LPVOID) args,0,NULL);
#else
		pthread_t thread;
		pthread_create (&thread, NULL, (void *) f, ctx);
#endif
	}

	void mutex_init(mutex_t * mutex) {
#if WINHEAD
		mutex->thing1 = (void *) CreateEvent (0, FALSE, FALSE, 0);
		mutex->thing2 = (void *) malloc(sizeof(CRITICAL_SECTION));
		InitializeCriticalSection((CRITICAL_SECTION *) mutex->thing2);
#else
		mutex->thing1 = malloc(sizeof(pthread_mutex_t));
		mutex->thing2 = malloc(sizeof(pthread_cond_t));

		pthread_mutex_init((pthread_mutex_t *) mutex->thing1, NULL);
		pthread_cond_init((pthread_cond_t *) mutex->thing2, NULL);
#endif
	}

	void critical_enter(mutex_t * mutex) {
#if WINHEAD
		EnterCriticalSection((CRITICAL_SECTION *) mutex->thing2);
#else
		pthread_mutex_lock((pthread_mutex_t *) mutex->thing1);
#endif
	}

	void critical_leave(mutex_t * mutex) {
#if WINHEAD
		LeaveCriticalSection((CRITICAL_SECTION *) mutex->thing2);
#else
		pthread_mutex_unlock((pthread_mutex_t *) mutex->thing1);
#endif
	}

	int mutex_wait(mutex_t * imutex) {
#if WINHEAD
		return (WaitForSingleObject ((HANDLE) imutex->thing1, 100) == WAIT_TIMEOUT) ? (THREAD_TIMEOUT) : (THREAD_OK);
#else
		if (imutex->thing1 == NULL || imutex->thing2 == NULL) return THREAD_NOT_INITED;

		pthread_mutex_t * mutex = (pthread_mutex_t *) imutex->thing1;
		pthread_cond_t * cond = (pthread_cond_t *) imutex->thing2;

		struct timespec timeToWait;
		struct timeval now;

		gettimeofday(&now, NULL);

		timeToWait.tv_sec = now.tv_sec;
		timeToWait.tv_nsec = now.tv_usec*1000UL + 10 * 1000000;

		pthread_mutex_lock(mutex);
		const int res = pthread_cond_timedwait(cond, mutex, &timeToWait);
		pthread_mutex_unlock(mutex);

		if (res == ETIMEDOUT)
			return THREAD_TIMEOUT;
#endif

		return THREAD_OK;
	}

	void mutex_signal(mutex_t * imutex) {
#if WINHEAD
		SetEvent ((HANDLE) imutex->thing1);
#else
		pthread_mutex_t * mutex = (pthread_mutex_t *) imutex->thing1;
		pthread_cond_t * cond = (pthread_cond_t *) imutex->thing2;

		pthread_mutex_lock(mutex);
		pthread_cond_signal(cond);
		pthread_mutex_unlock(mutex);
#endif
	}

	void mutex_free(mutex_t * imutex) {
#if WINHEAD
		CloseHandle ((HANDLE) imutex->thing1);
		DeleteCriticalSection((CRITICAL_SECTION *) imutex->thing2);
		free(imutex->thing2);
#else
		pthread_mutex_t * mutex = (pthread_mutex_t *) imutex->thing1;
		pthread_cond_t * cond = (pthread_cond_t *) imutex->thing2;

		pthread_mutex_destroy(mutex);
		pthread_cond_destroy(cond);
#endif
	}
