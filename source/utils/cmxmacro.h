/*********************/
/*** Climax Macros ***/
/*********************/

#ifndef	__CMX_MACROS_HEADER__
#define	__CMX_MACROS_HEADER__

#include	"system\asmport.h"

#ifdef	PSX_MIPS_ASM

/*****************************************************************************/
// Get the summed magnitude of the vector XYZ (after sqr)
#define CMX_StVecXYZMag(r0) __asm__ volatile (		\
		"mfc2	$12, $25;"							\
		"mfc2	$13, $26;"							\
		"mfc2	$14, $27;"							\
		"add	$12, $12, $13;"						\
		"add	$12, $12, $14;"						\
		"sw		$12, 0( %0 );"						\
		:											\
		: "r"( r0 )									\
			: "$12", "$13", "$14", "memory" )

/*---------------------------------------------------------------------------*/
// Get the summed magnitude of the vector XZ (after sqr)
#define CMX_StVecXZMag(r0) __asm__ volatile (		\
		"mfc2	$12, $25;"							\
		"mfc2	$13, $27;"							\
		"add	$12, $12, $13;"						\
		"sw		$12, 0( %0 );"						\
		:											\
		: "r"( r0 )									\
		: "$12", "$13", "memory" )

/*---------------------------------------------------------------------------*/
// Load IR0,IR1,IR2 with values (for SQR)
#define CMX_ldXYZ(r0,r1,r2) __asm__  (			\
		"mtc2   %0,$9;"							\
		"mtc2   %1,$10;"						\
		"mtc2   %2,$11"							\
		:										\
		: "r"( r0 ),"r"( r1 ),"r"( r2 ) )

/*---------------------------------------------------------------------------*/
// Load IR0,IR1,IR2 with values (for SQR)
#define CMX_ldXZ(r0,r1) __asm__  (				\
		"mtc2   %0,$9;"							\
		"mtc2   %1,$11"							\
		:										\
		: "r"( r0 ),"r"( r1 ))


/*****************************************************************************/
#define	CMX_SetTransMtxXY(r0) __asm__ (			\
		"lw	$12, 0( %0 );"						\
		"lw	$13, 4( %0 );"						\
		"ctc2	$12, $5;"						\
		"ctc2	$13, $6;"						\
		:										\
		: "r"( r0 )								\
		: "$12", "$13" )

#else	/* PSX_MIPS_ASM */

/*****************************************************************************/
/*	Portable equivalents, register for register with the asm above.
	Data regs: 9,10,11 = IR1,IR2,IR3; 25,26,27 = MAC1,MAC2,MAC3.
	Ctrl regs: 5,6 = TRX,TRY.
*/
// Get the summed magnitude of the vector XYZ (after sqr)
inline void	CMX_StVecXYZMagF(long *r0)
{
	*r0=(long)(GTEport_GetData(25)+GTEport_GetData(26)+GTEport_GetData(27));
}
#define CMX_StVecXYZMag(r0)		CMX_StVecXYZMagF((long*)(r0))

/*---------------------------------------------------------------------------*/
// Get the summed magnitude of the vector XZ (after sqr)
inline void	CMX_StVecXZMagF(long *r0)
{
	*r0=(long)(GTEport_GetData(25)+GTEport_GetData(27));
}
#define CMX_StVecXZMag(r0)		CMX_StVecXZMagF((long*)(r0))

/*---------------------------------------------------------------------------*/
// Load IR0,IR1,IR2 with values (for SQR)
#define CMX_ldXYZ(r0,r1,r2)		(GTEport_SetData(9,(unsigned long)(r0)),	\
								 GTEport_SetData(10,(unsigned long)(r1)),	\
								 GTEport_SetData(11,(unsigned long)(r2)))

/*---------------------------------------------------------------------------*/
// Load IR0,IR1,IR2 with values (for SQR)
#define CMX_ldXZ(r0,r1)			(GTEport_SetData(9,(unsigned long)(r0)),	\
								 GTEport_SetData(11,(unsigned long)(r1)))

/*****************************************************************************/
inline void	CMX_SetTransMtxXYF(const void *r0)
{
const unsigned long	*w=(const unsigned long*)r0;
	GTEport_SetCtrl(5,w[0]);
	GTEport_SetCtrl(6,w[1]);
}
#define	CMX_SetTransMtxXY(r0)	CMX_SetTransMtxXYF((const void*)(r0))

#endif	/* PSX_MIPS_ASM */

/*****************************************************************************/
/*** Smaller Translation Macros (no return flags) ****************************/
/*****************************************************************************/
#define CMX_RotTransPers(r1,r2)						\
{		gte_ldv0(r1);								\
		gte_rtps();									\
		gte_stsxy(r2);								\
}

/*---------------------------------------------------------------------------*/
#define CMX_RotTransPers3(r1,r2,r3,r4,r5,r6)		\
{		gte_ldv3(r1,r2,r3);							\
		gte_rtpt();									\
		gte_stsxy3(r4,r5,r6);						\
}

/*---------------------------------------------------------------------------*/
#define CMX_RotTransPers4(r1,r2,r3,r4,r5,r6,r7,r8)	\
{		gte_ldv0(r4);								\
		gte_rtps();									\
		gte_ldv3(r1,r2,r3);							\
		gte_stsxy(r8);								\
		gte_rtpt();									\
		gte_stsxy3(r5,r6,r7);						\
}

/*---------------------------------------------------------------------------*/
#define CMX_RotTrans(r1,r2)							\
{		gte_ldv0(r1);								\
		gte_rt();									\
		gte_stlvnl(r2);								\
}

/*****************************************************************************/
#endif