#ifndef __REPLACE_H__
#define __REPLACE_H__

#include	"system\asmport.h"

#ifdef	PSX_MIPS_ASM

extern "C" void	MCmemcpy(void *Dst,void *Src,int Length);

#else	/* PSX_MIPS_ASM */

/* Portable fallback for the hand-written MIPS copy in replace.mip */
#include	<string.h>
#define	MCmemcpy(Dst,Src,Length)	memcpy((Dst),(Src),(size_t)(Length))

#endif	/* PSX_MIPS_ASM */

#endif