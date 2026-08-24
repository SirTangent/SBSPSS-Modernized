/*	libapi + kernel-adjacent stubs.

	SetSp/GetSp: InitSystem() relocates the MIPS stack pointer with
	SetSp(GetSp()|0x807f0000) - meaningless on PC, so GetSp hands back a
	value for which the OR is harmless and SetSp ignores it.

	EnterCriticalSection/ExitCriticalSection disable PS1 interrupts; the pump
	model is single-threaded so they are no-ops.  (The Win32 functions of the
	same name are stdcall and take a parameter - different decorated symbols
	on i686, so no collision.)

	The event/root-counter set backs system/clickcount.cpp's RCnt2 timer;
	M1 accepts the registration and never fires it (CClickCount arms itself
	lazily and nothing in the headless path reads it).
*/
#include "stub_log.h"

extern "C" {

long GetSp(void)					{ return 0; }
long SetSp(long newSp)				{ (void)newSp; return 0; }

void EnterCriticalSection(void)		{ }
void ExitCriticalSection(void)		{ }

long OpenEvent(unsigned long desc, long spec, long mode, long (*func)())
{
	(void)desc; (void)spec; (void)mode; (void)func;
	PSYQ_STUB_ONCE();
	return 1;	/* fake event handle */
}
long CloseEvent(long ev)			{ (void)ev; return 1; }
long EnableEvent(long ev)			{ (void)ev; return 1; }
long DisableEvent(long ev)			{ (void)ev; return 1; }

long SetRCnt(unsigned long spec, unsigned short target, long mode)
{
	(void)spec; (void)target; (void)mode;
	PSYQ_STUB_ONCE();
	return 1;
}
long StartRCnt(unsigned long spec)	{ (void)spec; return 1; }
long StopRCnt(unsigned long spec)	{ (void)spec; return 1; }
long GetRCnt(unsigned long spec)	{ (void)spec; return 0; }

void FlushCache(void)				{ }

}
