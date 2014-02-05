#ifndef _TSDRPluginLoader
#define _TSDRPluginLoader

// A platform independed dynamic library loader

#include <stdint.h>
#include <stdbool.h>
#include "osdetect.h"

#if WINHEAD
	#include <windows.h>
#else
	#include <dlfcn.h>
#endif

	typedef int (* pfnExtIOCallback) (int cnt, int status, float IQoffs, void *IQdata);

	struct extiosource {
		void * fd;

		// mandatory functions that the dll must implement
		bool (* InitHW) (char *name, char *model, int *hwtype); // for hwtype see enum extHWtypeT
		bool (* OpenHW) (void);
		void (* CloseHW) (void);
		int (* StartHW) (long extLOfreq);
		void (* StopHW) (void);
		void (* SetCallback) (pfnExtIOCallback funcptr);
		int (* SetHWLO) (long extLOfreq);   // see also SetHWLO64
		int (* GetStatus) (void);

		// optional functions
		void (* RawDataReady) (long samprate, void *Ldata, void *Rdata, int numsamples);
		void (* ShowGUI) (void);
		void (* HideGUI)  (void);

	} typedef extiosource_t;

	int extio_load(extiosource_t * plugin, const char *dlname);
	void extio_close(extiosource_t * plugin);

#endif
