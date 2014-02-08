#ifndef _TSDRPluginLoader
#define _TSDRPluginLoader

#include <stdint.h>
#include <stdbool.h>

#include <windows.h>

typedef void(*pfnExtIOCallback) (int cnt, int status, float IQoffs, void *IQdata);

	struct extiosource {
		HINSTANCE fd;

		// mandatory functions that the dll must implement
		bool(__stdcall * InitHW) (char *name, char *model, int *hwtype); // for hwtype see enum extHWtypeT
		bool(__stdcall * OpenHW) (void);
		void(__stdcall * CloseHW) (void);
		int(__stdcall * StartHW) (long extLOfreq);
		void(__stdcall * StopHW) (void);
		void(__stdcall * SetCallback) (pfnExtIOCallback funcptr);
		int(__stdcall * SetHWLO) (long extLOfreq);   // see also SetHWLO64
		int(__stdcall * GetStatus) (void);

		// mandatory functions that tsdrrequires
		long(__stdcall * GetHWSR) (void);

		// completely optional functions
		int(__stdcall * SetAttenuator) (int atten_idx);
		int(__stdcall * GetAttenuators) (int atten_idx, float * attenuation);
		int(__stdcall * ShowGUI) (void);
		int(__stdcall * HideGUI) (void);

	} typedef extiosource_t;

	int extio_load(extiosource_t * plugin, const char *dlname);
	void extio_close(extiosource_t * plugin);

#endif
