#ifndef _included_gte_
	#define _included_gte_

	#include "system\asmport.h"

#ifdef PSX_MIPS_ASM

	//
	// Store individual components of sv
	//

	#define gte_stsv_x( r0 ) __asm__ volatile (			\
		"mfc2	$12, $9;"					\
		"sh	$12, 0( %0 );"					\
		:							\
		: "r"( r0 )						\
		: "$12", "memory" )

	#define gte_stsv_y( r0 ) __asm__ volatile (			\
		"mfc2	$12, $10;"					\
		"sh	$12, 0( %0 );"					\
		:							\
		: "r"( r0 )						\
		: "$12", "memory" )

	#define gte_stsv_z( r0 ) __asm__ volatile (			\
		"mfc2	$12, $11;"					\
		"sh	$12, 0( %0 );"					\
		:							\
		: "r"( r0 )						\
		: "$12", "memory" )

	#define gte_stlvl_x( r0 ) __asm__ volatile (			\
		"swc2	$9, 0( %0 );"					\
		:							\
		: "r"( r0 )						\
		: "memory" )

	#define gte_stlvl_y( r0 ) __asm__ volatile (			\
		"swc2	$10, 0( %0 );"					\
		:							\
		: "r"( r0 )						\
		: "memory" )

	#define gte_stlvl_z( r0 ) __asm__ volatile (			\
		"swc2	$11, 0( %0 );"					\
		:							\
		: "r"( r0 )						\
		: "memory" )

#else	/* PSX_MIPS_ASM */

	/* Portable equivalents: data regs 9,10,11 = IR1,IR2,IR3 */

	#define gte_stsv_x( r0 )	(*(s16*)(r0)=(s16)GTEport_GetData(9))
	#define gte_stsv_y( r0 )	(*(s16*)(r0)=(s16)GTEport_GetData(10))
	#define gte_stsv_z( r0 )	(*(s16*)(r0)=(s16)GTEport_GetData(11))

	#define gte_stlvl_x( r0 )	(*(s32*)(r0)=(s32)GTEport_GetData(9))
	#define gte_stlvl_y( r0 )	(*(s32*)(r0)=(s32)GTEport_GetData(10))
	#define gte_stlvl_z( r0 )	(*(s32*)(r0)=(s32)GTEport_GetData(11))

#endif	/* PSX_MIPS_ASM */

#endif
