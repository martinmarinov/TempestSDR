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
		bool (__stdcall * InitHW) (char *name, char *model, int *hwtype); // for hwtype see enum extHWtypeT
		bool (__stdcall * OpenHW) (void);
		void (__stdcall * CloseHW) (void);
		int (__stdcall * StartHW) (long extLOfreq);
		void (__stdcall * StopHW) (void);
		void (__stdcall * SetCallback) (pfnExtIOCallback funcptr);
		int (__stdcall * SetHWLO) (long extLOfreq);   // see also SetHWLO64
		int (__stdcall * GetStatus) (void);

		// optional functions
		void (__stdcall * RawDataReady) (long samprate, void *Ldata, void *Rdata, int numsamples);
		void (__stdcall * ShowGUI) (void);
		void (__stdcall * HideGUI)  (void);

	} typedef extiosource_t;

	int extio_load(extiosource_t * plugin, const char *dlname);
	void extio_close(extiosource_t * plugin);

#endif
