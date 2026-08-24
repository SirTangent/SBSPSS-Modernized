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
	register numbers from the original asm (mfc2/mtc2 = data, ctc2 = control);
	GTEport_Op takes the 25-bit cop2 command word (the immediate field of the
	original cop2 instruction, e.g. 0x0280030 for rtpt).
*/
extern "C"
{
u32		GTEport_GetData(int reg);			/* mfc2 */
void	GTEport_SetData(int reg,u32 v);		/* mtc2 */
void	GTEport_SetCtrl(int reg,u32 v);		/* ctc2 */
void	GTEport_Op(u32 op);					/* cop2 command */
}
#endif

#endif
