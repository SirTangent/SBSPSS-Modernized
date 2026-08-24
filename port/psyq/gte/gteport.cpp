/*	Software-GTE register interface (see source/system/asmport.h and the
	portable port/include/inline_c.h, whose macros funnel every cop2 access
	through these four calls).

	M1 scope: a passive register file - loads land in registers, stores read
	them back, operations log once and do nothing.  The real cop2 emulation
	(RTPS/RTPT/NCLIP/SQR/AVSZ/... with PS1 flag semantics) is milestone M3;
	freezing the interface now is the point.
*/
#include "system/types.h"
#include "stub_log.h"

extern "C" {
u32  GTEport_GetData(int reg);
void GTEport_SetData(int reg, u32 v);
u32  GTEport_GetCtrl(int reg);		/* cfc2 - declared by port/include/inline_c.h */
void GTEport_SetCtrl(int reg, u32 v);
void GTEport_Op(u32 op);
}

static u32 g_dataReg[32];
static u32 g_ctrlReg[32];

extern "C" u32 GTEport_GetData(int reg)
{
	return g_dataReg[reg & 31];
}

extern "C" void GTEport_SetData(int reg, u32 v)
{
	g_dataReg[reg & 31] = v;
}

extern "C" u32 GTEport_GetCtrl(int reg)
{
	return g_ctrlReg[reg & 31];
}

extern "C" void GTEport_SetCtrl(int reg, u32 v)
{
	g_ctrlReg[reg & 31] = v;
}

extern "C" void GTEport_Op(u32 op)
{
	(void)op;
	PSYQ_STUB_ONCE();	/* real GTE math lands in M3 */
}
