/*	Replaces source/system/gp.mip (MIPS $gp register save/reload).
	x86 has no small-data pointer; these are no-ops.  Called from
	system/main.cpp (InitSystem) and system/clickcount.cpp (IRQ wrapper).
*/
#include "system/types.h"
#include "system/gp.h"

extern "C" void	SaveGP(void)		{ }
extern "C" u32	ReloadGP(void)		{ return 0; }
extern "C" void	SetGP(u32 GpVal)	{ (void)GpVal; }
