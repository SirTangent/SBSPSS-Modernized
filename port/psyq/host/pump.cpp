/*	Emulated vblank clock (QPC-driven) + the libetc VSync surface.

	Game-visible semantics (libetc):
	  VSync(0)   - wait for the next vblank, return the vblank counter
	  VSync(n>0) - wait until n vblanks have passed since the previous VSync
	  VSync(n<0) - return the counter without waiting
	  VSyncCallback(f) - f fires once per vblank (from inside the pump)
*/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "pump.h"

extern "C" {
int  VSync(int mode);
int  VSyncCallback(void (*f)(void));
}

static void			(*g_vsyncCallback)(void);
static unsigned long	g_vblank;			/* emulated vblank counter */
static unsigned long	g_lastVSyncVblank;
static int				g_hz = 60;
static LARGE_INTEGER	g_qpcFreq;
static LARGE_INTEGER	g_qpcBase;
static int				g_clockInit;

static void clockInit(void)
{
	if (!g_clockInit)
	{
		QueryPerformanceFrequency(&g_qpcFreq);
		QueryPerformanceCounter(&g_qpcBase);
		g_clockInit = 1;
	}
}

static unsigned long wallVblank(void)
{
	LARGE_INTEGER now;
	clockInit();
	QueryPerformanceCounter(&now);
	return (unsigned long)(((now.QuadPart - g_qpcBase.QuadPart) * g_hz) / g_qpcFreq.QuadPart);
}

extern "C" void Port_SetVBlankHz(int hz)
{
	g_hz = (hz == 50) ? 50 : 60;
}

extern "C" unsigned long Port_VBlankCount(void)
{
	return g_vblank;
}

extern "C" void Port_Pump(void)
{
	unsigned long target = wallVblank();
	while (g_vblank < target)
	{
		g_vblank++;
		if (g_vsyncCallback)
			g_vsyncCallback();
	}
}

extern "C" int VSync(int mode)
{
	Port_Pump();
	if (mode < 0)
		return (int)g_vblank;

	unsigned long until = (mode == 0) ? g_vblank + 1
									  : g_lastVSyncVblank + (unsigned long)mode;
	while (g_vblank < until)
	{
		Sleep(1);
		Port_Pump();
	}
	g_lastVSyncVblank = g_vblank;
	return (int)g_vblank;
}

extern "C" int VSyncCallback(void (*f)(void))
{
	g_vsyncCallback = f;
	return 0;
}
