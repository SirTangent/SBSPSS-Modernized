/*********************************/
/*** Inline Assembly Selection ***/
/*********************************/

#ifndef __SYSTEM_ASMPORT_H__
#define __SYSTEM_ASMPORT_H__

/*	The original PlayStation build (ccpsx / EGCS targeting MIPS) compiles the
	hand-written GNU inline assembly exactly as it always did.  Any other
	compiler gets portable C++ equivalents instead.

	The vintage compiler predefines 'mips' (verified against cpppsx -dM);
	modern MIPS compilers define '__mips__'.  Define PSX_NO_ASM to force the
	portable paths onto a MIPS compiler (to test them against the originals).
*/

#if (defined(mips) || defined(__mips__)) && !defined(PSX_NO_ASM)
#define	PSX_MIPS_ASM	1
#endif

#ifndef	PSX_MIPS_ASM
#include "system/types.h"
/*	Software-GTE register interface.  The portable equivalents of the
	coprocessor-2 macros funnel through these calls, which a PC port must
	implement in its software GTE.  Register numbers are the raw cop2
	register numbers from the original asm (mfc2/mtc2 = data, cfc2/ctc2 =
	control).

	GTEport_Op receives the DMPSX TAG WORD exactly as embedded in the
	vintage INLINE_C.H macros (e.g. 0x0000007f for rtps, 0x000000bf for
	rtpt) - NOT the raw 25-bit cop2 instruction immediate.  gte_mvmva ORs
	its sf/mx/v/cv/lm fields into the tag at <<25/<<23/<<21/<<19/<<18, so
	the software GTE must decode that packing, not the hardware encoding.
*/
extern "C"
{
u32		GTEport_GetData(int reg);			/* mfc2 */
void	GTEport_SetData(int reg,u32 v);		/* mtc2 */
u32		GTEport_GetCtrl(int reg);			/* cfc2 */
void	GTEport_SetCtrl(int reg,u32 v);		/* ctc2 */
void	GTEport_Op(u32 op);					/* cop2 command (DMPSX tag word) */

/*	The PS1 scratchpad (1KB of fast RAM at 0x1f800000) becomes a real
	buffer on PC, supplied by the PsyQ shim; SCRATCH_RAM in system/global.h
	points at it.  Declared here so game code and shim share one contract.
*/
extern unsigned char	PORT_Scratchpad[1024];
}
#endif

#endif
