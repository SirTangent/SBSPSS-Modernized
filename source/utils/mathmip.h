/*****************/
/*** Math Mips ***/
/*****************/

#ifndef __MISCMIP_HEADER__
#define __MISCMIP_HEADER__

#include	"system\asmport.h"
#include	"system\types.h"

/*****************************************************************************/
#ifdef	PSX_MIPS_ASM

extern "C" s32	FixedMul(s32 a, s32 b);

#else	/* PSX_MIPS_ASM */

/* Portable fallback for the 4.12 fixed-point multiply in mathmip.mip */
inline s32	FixedMul(s32 a, s32 b)	{return((s32)(((s64)a*(s64)b)>>12));}

#endif	/* PSX_MIPS_ASM */

#endif
