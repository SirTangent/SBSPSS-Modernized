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
static unsigned long	g_vblankBase;		/* wall count at the last rate change */
static int				g_hz = 60;
static LARGE_INTEGER	g_qpcFreq;
static LARGE_INTEGER	g_qpcBase;		/* vblank epoch - REBASED by Port_SetVBlankHz */
static LARGE_INTEGER	g_qpcOrigin;	/* wall-clock epoch - fixed for the whole run */
static int				g_clockInit;

static void clockInit(void)
{
	if (!g_clockInit)
	{
		QueryPerformanceFrequency(&g_qpcFreq);
		QueryPerformanceCounter(&g_qpcBase);
		g_qpcOrigin = g_qpcBase;
		g_clockInit = 1;
	}
}

static unsigned long wallVblank(void)
{
	LARGE_INTEGER now;
	clockInit();
	QueryPerformanceCounter(&now);
	return g_vblankBase +
		   (unsigned long)(((now.QuadPart - g_qpcBase.QuadPart) * g_hz) / g_qpcFreq.QuadPart);
}

extern "C" void Port_SetVBlankHz(int hz)
{
	/*	rebase the epoch so already-elapsed wall time is not retroactively
		reinterpreted at the new rate (60->50 would stall VSync until the
		wall clock caught back up; 50->60 would burst callbacks)  */
	g_vblankBase = wallVblank();
	QueryPerformanceCounter(&g_qpcBase);
	g_hz = (hz == 50) ? 50 : 60;
}

extern "C" unsigned long Port_VBlankCount(void)
{
	return g_vblank;
}

/*	Wall clock for deadline arithmetic (CD pacing).  It must run off the
	FIXED origin, never off g_qpcBase: Port_SetVBlankHz rebases that epoch at
	the game's SetVideoMode, which would step this clock backwards by however
	long the boot took and freeze every read deadline set before it.  */
extern "C" double Port_NowSeconds(void)
{
	LARGE_INTEGER now;
	clockInit();
	QueryPerformanceCounter(&now);
	return (double)(now.QuadPart - g_qpcOrigin.QuadPart) / (double)g_qpcFreq.QuadPart;
}

extern "C" void Port_Pump(void)
{
	extern void Host_VBlank(unsigned long vblankNo);	/* host/window.cpp */
	extern void Port_RCnt2Vblank(int vblankHz);			/* api/libapi_stubs.cpp */

	unsigned long target = wallVblank();

	/*	after a long stall (debugger, laptop sleep) don't fire thousands of
		catch-up callbacks - drop the missed vblanks and replay a few  */
	if (target > g_vblank + 8)
		g_vblank = target - 8;

	/*	AT MOST ONE vblank per pump call.  On PS1 the vblank is an interrupt:
		game code waiting on its side effects always runs between two firings
		and can observe every intermediate state.  StopLoad (vid.cpp:125)
		depends on that - it spins `while(LoadTime) VSync(0)` and LoadTime
		WRAPS to zero, so if one wait iteration ever fires two callbacks the
		zero can be stepped over forever (seen live: the FIFO present paces
		the loop at ~1 real vsync, one pending vblank accumulates per
		iteration, and a catch-up burst here made every VSync(0) fire twice).
		The counter still tracks the wall clock - pending vblanks drain one
		per call through the many pump sites a game frame passes.  */
	if (g_vblank < target)
	{
		g_vblank++;
		if (g_vsyncCallback)
			g_vsyncCallback();		/* game vblank work first (loading icon...) */
		Port_RCnt2Vblank(g_hz);
		Host_VBlank(g_vblank);		/* ...then events + present + tooling */
	}
}

/*	Wait-loop body: yield the core for a tick, then pump.  Every blocking SDK
	call spins on some deadline (a vblank number, a CD read completion); doing
	that without the Sleep pins a core at 100% for the whole wait.  */
extern "C" void Port_PumpIdle(void)
{
	Sleep(1);
	Port_Pump();
}

extern "C" int VSync(int mode)
{
	if (mode < 0)
	{
		Port_Pump();
		return (int)g_vblank;
	}

	/*	mode 0: `until` is computed BEFORE any pumping, so one call consumes
		exactly one vblank callback even when a pending vblank has already
		accumulated - the StopLoad invariant (see Port_Pump).  */
	unsigned long until = (mode == 0) ? g_vblank + 1
									  : g_lastVSyncVblank + (unsigned long)mode;
	Port_Pump();
	while (g_vblank < until)
		Port_PumpIdle();
	g_lastVSyncVblank = g_vblank;
	return (int)g_vblank;
}

extern "C" int VSyncCallback(void (*f)(void))
{
	g_vsyncCallback = f;
	return 0;
}
