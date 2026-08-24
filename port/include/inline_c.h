/*
 *	inline_c.h  --  portable shadow of tools/psyq/include/INLINE_C.H
 *
 *	Win32 port of the PSY-Q GTE inline-assembly macros ("DMPSX version 3",
 *	(C) 1996 Sony Computer Entertainment Inc.).  The PC build places
 *	port/include ahead of tools/psyq/include on the include path, so this
 *	file replaces the vintage header, whose macro bodies are MIPS inline
 *	asm.  Every macro is translated register-for-register into portable C
 *	through the software-GTE interface declared in source/system/asmport.h
 *	(GTEport_GetData / GTEport_SetData / GTEport_SetCtrl / GTEport_Op).
 *	The PS1 build never sees this file -- it keeps the vintage header.
 *
 *	The GTE command words passed to GTEport_Op() are the exact ".word"
 *	values from the vintage header (the DMPSX tag words); the software GTE
 *	decodes those.
 */

#ifndef _INLINE_C_PORT_H_
#define _INLINE_C_PORT_H_

#include "system/asmport.h"

/*	asmport.h declares reads of cop2 DATA registers only (mfc2/swc2).
	Several vintage macros also read cop2 CONTROL registers (cfc2) --
	gte_stflg, gte_ReadRotMatrix, gte_ReadGeomOffset, gte_FlipTRX, etc.
	GTEport_GetCtrl comes from system/asmport.h with the rest of the
	software-GTE interface (included above).
*/

/*	Private load/store helpers, named after the MIPS instructions they
	replace so each macro body can be eyeballed against the vintage asm.
	Little-endian x86 target; offsets are byte offsets exactly as in the
	original 'off( %k )' operands.
*/
#define _GTE_LW( p, off )	 (*(const u32 *)((const u8 *)(p) + (off)))
#define _GTE_LHU( p, off )	 ((u32)*(const u16 *)((const u8 *)(p) + (off)))
#define _GTE_LH( p, off )	 ((s32)*(const s16 *)((const u8 *)(p) + (off)))
#define _GTE_LBU( p, off )	 ((u32)*(const u8 *)((const u8 *)(p) + (off)))
#define _GTE_SW( p, off, v ) (*(u32 *)((u8 *)(p) + (off)) = (u32)(v))
#define _GTE_SH( p, off, v ) (*(u16 *)((u8 *)(p) + (off)) = (u16)(v))
#define _GTE_SB( p, off, v ) (*(u8 *)((u8 *)(p) + (off)) = (u8)(v))

/*
 * Type 1 functions
 */

#define gte_ldv0( r0 ) do {						\
	GTEport_SetData( 0, _GTE_LW( r0, 0 ) );				\
	GTEport_SetData( 1, _GTE_LW( r0, 4 ) );				\
} while (0)

#define gte_ldv1( r0 ) do {						\
	GTEport_SetData( 2, _GTE_LW( r0, 0 ) );				\
	GTEport_SetData( 3, _GTE_LW( r0, 4 ) );				\
} while (0)

#define gte_ldv2( r0 ) do {						\
	GTEport_SetData( 4, _GTE_LW( r0, 0 ) );				\
	GTEport_SetData( 5, _GTE_LW( r0, 4 ) );				\
} while (0)

#define gte_ldv3( r0, r1, r2 ) do {					\
	GTEport_SetData( 0, _GTE_LW( r0, 0 ) );				\
	GTEport_SetData( 1, _GTE_LW( r0, 4 ) );				\
	GTEport_SetData( 2, _GTE_LW( r1, 0 ) );				\
	GTEport_SetData( 3, _GTE_LW( r1, 4 ) );				\
	GTEport_SetData( 4, _GTE_LW( r2, 0 ) );				\
	GTEport_SetData( 5, _GTE_LW( r2, 4 ) );				\
} while (0)

#define gte_ldv3c( r0 ) do {						\
	GTEport_SetData( 0, _GTE_LW( r0, 0 ) );				\
	GTEport_SetData( 1, _GTE_LW( r0, 4 ) );				\
	GTEport_SetData( 2, _GTE_LW( r0, 8 ) );				\
	GTEport_SetData( 3, _GTE_LW( r0, 12 ) );			\
	GTEport_SetData( 4, _GTE_LW( r0, 16 ) );			\
	GTEport_SetData( 5, _GTE_LW( r0, 20 ) );			\
} while (0)

#define gte_ldv3c_vertc( r0 ) do {					\
	GTEport_SetData( 0, _GTE_LW( r0, 0 ) );				\
	GTEport_SetData( 1, _GTE_LW( r0, 4 ) );				\
	GTEport_SetData( 2, _GTE_LW( r0, 12 ) );			\
	GTEport_SetData( 3, _GTE_LW( r0, 16 ) );			\
	GTEport_SetData( 4, _GTE_LW( r0, 24 ) );			\
	GTEport_SetData( 5, _GTE_LW( r0, 28 ) );			\
} while (0)

#define gte_ldv01( r0, r1 ) do {					\
	GTEport_SetData( 0, _GTE_LW( r0, 0 ) );				\
	GTEport_SetData( 1, _GTE_LW( r0, 4 ) );				\
	GTEport_SetData( 2, _GTE_LW( r1, 0 ) );				\
	GTEport_SetData( 3, _GTE_LW( r1, 4 ) );				\
} while (0)

#define gte_ldv01c( r0 ) do {						\
	GTEport_SetData( 0, _GTE_LW( r0, 0 ) );				\
	GTEport_SetData( 1, _GTE_LW( r0, 4 ) );				\
	GTEport_SetData( 2, _GTE_LW( r0, 8 ) );				\
	GTEport_SetData( 3, _GTE_LW( r0, 12 ) );			\
} while (0)

#define gte_ldrgb( r0 )							\
	( GTEport_SetData( 6, _GTE_LW( r0, 0 ) ) )

#define gte_ldrgb3( r0, r1, r2 ) do {					\
	GTEport_SetData( 20, _GTE_LW( r0, 0 ) );			\
	GTEport_SetData( 21, _GTE_LW( r1, 0 ) );			\
	GTEport_SetData( 22, _GTE_LW( r2, 0 ) );			\
	GTEport_SetData( 6, _GTE_LW( r2, 0 ) );				\
} while (0)

#define gte_ldrgb3c( r0 ) do {						\
	GTEport_SetData( 20, _GTE_LW( r0, 0 ) );			\
	GTEport_SetData( 21, _GTE_LW( r0, 4 ) );			\
	GTEport_SetData( 22, _GTE_LW( r0, 8 ) );			\
	GTEport_SetData( 6, _GTE_LW( r0, 8 ) );				\
} while (0)

#define gte_ldlv0( r0 ) do {						\
	GTEport_SetData( 0, _GTE_LHU( r0, 0 ) | ( _GTE_LHU( r0, 4 ) << 16 ) ); \
	GTEport_SetData( 1, _GTE_LW( r0, 8 ) );				\
} while (0)

#define gte_ldlvl( r0 ) do {						\
	GTEport_SetData( 9, _GTE_LW( r0, 0 ) );				\
	GTEport_SetData( 10, _GTE_LW( r0, 4 ) );			\
	GTEport_SetData( 11, _GTE_LW( r0, 8 ) );			\
} while (0)

#define gte_ldsv( r0 ) do {						\
	GTEport_SetData( 9, _GTE_LHU( r0, 0 ) );			\
	GTEport_SetData( 10, _GTE_LHU( r0, 2 ) );			\
	GTEport_SetData( 11, _GTE_LHU( r0, 4 ) );			\
} while (0)

#define gte_ldbv( r0 ) do {						\
	GTEport_SetData( 9, _GTE_LBU( r0, 0 ) );			\
	GTEport_SetData( 10, _GTE_LBU( r0, 1 ) );			\
} while (0)

#define gte_ldcv( r0 ) do {						\
	GTEport_SetData( 9, _GTE_LBU( r0, 0 ) );			\
	GTEport_SetData( 10, _GTE_LBU( r0, 1 ) );			\
	GTEport_SetData( 11, _GTE_LBU( r0, 2 ) );			\
} while (0)

#define gte_ldclmv( r0 ) do {						\
	GTEport_SetData( 9, _GTE_LHU( r0, 0 ) );			\
	GTEport_SetData( 10, _GTE_LHU( r0, 6 ) );			\
	GTEport_SetData( 11, _GTE_LHU( r0, 12 ) );			\
} while (0)

#define gte_lddp( r0 )							\
	( GTEport_SetData( 8, (u32)( r0 ) ) )

#define gte_ldsxy0( r0 )						\
	( GTEport_SetData( 12, (u32)( r0 ) ) )

#define gte_ldsxy1( r0 )						\
	( GTEport_SetData( 13, (u32)( r0 ) ) )

#define gte_ldsxy2( r0 )						\
	( GTEport_SetData( 14, (u32)( r0 ) ) )

#define gte_ldsxy3( r0, r1, r2 ) do {					\
	GTEport_SetData( 12, (u32)( r0 ) );				\
	GTEport_SetData( 14, (u32)( r2 ) );				\
	GTEport_SetData( 13, (u32)( r1 ) );				\
} while (0)

#define gte_ldsxy3c( r0 ) do {						\
	GTEport_SetData( 12, _GTE_LW( r0, 0 ) );			\
	GTEport_SetData( 13, _GTE_LW( r0, 4 ) );			\
	GTEport_SetData( 14, _GTE_LW( r0, 8 ) );			\
} while (0)

#define gte_ldsz3( r0, r1, r2 ) do {					\
	GTEport_SetData( 17, (u32)( r0 ) );				\
	GTEport_SetData( 18, (u32)( r1 ) );				\
	GTEport_SetData( 19, (u32)( r2 ) );				\
} while (0)

#define gte_ldsz4( r0, r1, r2, r3 ) do {				\
	GTEport_SetData( 16, (u32)( r0 ) );				\
	GTEport_SetData( 17, (u32)( r1 ) );				\
	GTEport_SetData( 18, (u32)( r2 ) );				\
	GTEport_SetData( 19, (u32)( r3 ) );				\
} while (0)

#define gte_ldopv1( r0 ) do {						\
	GTEport_SetCtrl( 0, _GTE_LW( r0, 0 ) );				\
	GTEport_SetCtrl( 2, _GTE_LW( r0, 4 ) );				\
	GTEport_SetCtrl( 4, _GTE_LW( r0, 8 ) );				\
} while (0)

#define gte_ldopv2( r0 ) do {						\
	GTEport_SetData( 11, _GTE_LW( r0, 8 ) );			\
	GTEport_SetData( 9, _GTE_LW( r0, 0 ) );				\
	GTEport_SetData( 10, _GTE_LW( r0, 4 ) );			\
} while (0)

#define gte_ldlzc( r0 )							\
	( GTEport_SetData( 30, (u32)( r0 ) ) )

#define gte_SetRGBcd( r0 )						\
	( GTEport_SetData( 6, _GTE_LW( r0, 0 ) ) )

#define gte_ldbkdir( r0, r1, r2 ) do {					\
	GTEport_SetCtrl( 13, (u32)( r0 ) );				\
	GTEport_SetCtrl( 14, (u32)( r1 ) );				\
	GTEport_SetCtrl( 15, (u32)( r2 ) );				\
} while (0)

#define gte_SetBackColor( r0, r1, r2 ) do {				\
	GTEport_SetCtrl( 13, (u32)( r0 ) << 4 );			\
	GTEport_SetCtrl( 14, (u32)( r1 ) << 4 );			\
	GTEport_SetCtrl( 15, (u32)( r2 ) << 4 );			\
} while (0)

#define gte_ldfcdir( r0, r1, r2 ) do {					\
	GTEport_SetCtrl( 21, (u32)( r0 ) );				\
	GTEport_SetCtrl( 22, (u32)( r1 ) );				\
	GTEport_SetCtrl( 23, (u32)( r2 ) );				\
} while (0)

#define gte_SetFarColor( r0, r1, r2 ) do {				\
	GTEport_SetCtrl( 21, (u32)( r0 ) << 4 );			\
	GTEport_SetCtrl( 22, (u32)( r1 ) << 4 );			\
	GTEport_SetCtrl( 23, (u32)( r2 ) << 4 );			\
} while (0)

#define gte_SetGeomOffset( r0, r1 ) do {				\
	GTEport_SetCtrl( 24, (u32)( r0 ) << 16 );			\
	GTEport_SetCtrl( 25, (u32)( r1 ) << 16 );			\
} while (0)

#define gte_SetGeomScreen( r0 )						\
	( GTEport_SetCtrl( 26, (u32)( r0 ) ) )

#define gte_ldsvrtrow0( r0 ) do {					\
	GTEport_SetCtrl( 0, _GTE_LW( r0, 0 ) );				\
	GTEport_SetCtrl( 1, _GTE_LW( r0, 4 ) );				\
} while (0)

#define gte_SetRotMatrix( r0 ) do {					\
	GTEport_SetCtrl( 0, _GTE_LW( r0, 0 ) );				\
	GTEport_SetCtrl( 1, _GTE_LW( r0, 4 ) );				\
	GTEport_SetCtrl( 2, _GTE_LW( r0, 8 ) );				\
	GTEport_SetCtrl( 3, _GTE_LW( r0, 12 ) );			\
	GTEport_SetCtrl( 4, _GTE_LW( r0, 16 ) );			\
} while (0)

#define gte_ldsvllrow0( r0 ) do {					\
	GTEport_SetCtrl( 8, _GTE_LW( r0, 0 ) );				\
	GTEport_SetCtrl( 9, _GTE_LW( r0, 4 ) );				\
} while (0)

#define gte_SetLightMatrix( r0 ) do {					\
	GTEport_SetCtrl( 8, _GTE_LW( r0, 0 ) );				\
	GTEport_SetCtrl( 9, _GTE_LW( r0, 4 ) );				\
	GTEport_SetCtrl( 10, _GTE_LW( r0, 8 ) );			\
	GTEport_SetCtrl( 11, _GTE_LW( r0, 12 ) );			\
	GTEport_SetCtrl( 12, _GTE_LW( r0, 16 ) );			\
} while (0)

#define gte_ldsvlcrow0( r0 ) do {					\
	GTEport_SetCtrl( 16, _GTE_LW( r0, 0 ) );			\
	GTEport_SetCtrl( 17, _GTE_LW( r0, 4 ) );			\
} while (0)

#define gte_SetColorMatrix( r0 ) do {					\
	GTEport_SetCtrl( 16, _GTE_LW( r0, 0 ) );			\
	GTEport_SetCtrl( 17, _GTE_LW( r0, 4 ) );			\
	GTEport_SetCtrl( 18, _GTE_LW( r0, 8 ) );			\
	GTEport_SetCtrl( 19, _GTE_LW( r0, 12 ) );			\
	GTEport_SetCtrl( 20, _GTE_LW( r0, 16 ) );			\
} while (0)

#define gte_SetTransMatrix( r0 ) do {					\
	GTEport_SetCtrl( 5, _GTE_LW( r0, 20 ) );			\
	GTEport_SetCtrl( 6, _GTE_LW( r0, 24 ) );			\
	GTEport_SetCtrl( 7, _GTE_LW( r0, 28 ) );			\
} while (0)

#define gte_ldtr( r0, r1, r2 ) do {					\
	GTEport_SetCtrl( 5, (u32)( r0 ) );				\
	GTEport_SetCtrl( 6, (u32)( r1 ) );				\
	GTEport_SetCtrl( 7, (u32)( r2 ) );				\
} while (0)

#define gte_SetTransVector( r0 ) do {					\
	GTEport_SetCtrl( 5, _GTE_LW( r0, 0 ) );				\
	GTEport_SetCtrl( 6, _GTE_LW( r0, 4 ) );				\
	GTEport_SetCtrl( 7, _GTE_LW( r0, 8 ) );				\
} while (0)

#define gte_ld_intpol_uv0( r0 ) do {					\
	GTEport_SetCtrl( 21, _GTE_LBU( r0, 0 ) );			\
	GTEport_SetCtrl( 22, _GTE_LBU( r0, 1 ) );			\
} while (0)

#define gte_ld_intpol_uv1( r0 ) do {					\
	GTEport_SetData( 9, _GTE_LBU( r0, 0 ) );			\
	GTEport_SetData( 10, _GTE_LBU( r0, 1 ) );			\
} while (0)

#define gte_ld_intpol_bv0( r0 ) do {					\
	GTEport_SetCtrl( 21, _GTE_LBU( r0, 0 ) );			\
	GTEport_SetCtrl( 22, _GTE_LBU( r0, 1 ) );			\
} while (0)

#define gte_ld_intpol_bv1( r0 ) do {					\
	GTEport_SetData( 9, _GTE_LBU( r0, 0 ) );			\
	GTEport_SetData( 10, _GTE_LBU( r0, 1 ) );			\
} while (0)

#define gte_ld_intpol_sv0( r0 ) do {					\
	GTEport_SetCtrl( 21, (u32)_GTE_LH( r0, 0 ) );			\
	GTEport_SetCtrl( 22, (u32)_GTE_LH( r0, 2 ) );			\
	GTEport_SetCtrl( 23, (u32)_GTE_LH( r0, 4 ) );			\
} while (0)

#define gte_ld_intpol_sv1( r0 ) do {					\
	GTEport_SetData( 9, (u32)_GTE_LH( r0, 0 ) );			\
	GTEport_SetData( 10, (u32)_GTE_LH( r0, 2 ) );			\
	GTEport_SetData( 11, (u32)_GTE_LH( r0, 4 ) );			\
} while (0)

#define gte_ldfc( r0 ) do {						\
	GTEport_SetCtrl( 21, _GTE_LW( r0, 0 ) );			\
	GTEport_SetCtrl( 22, _GTE_LW( r0, 4 ) );			\
	GTEport_SetCtrl( 23, _GTE_LW( r0, 8 ) );			\
} while (0)

#define gte_ldopv2SV( r0 ) do {						\
	GTEport_SetData( 9, (u32)_GTE_LH( r0, 0 ) );			\
	GTEport_SetData( 10, (u32)_GTE_LH( r0, 2 ) );			\
	GTEport_SetData( 11, (u32)_GTE_LH( r0, 4 ) );			\
} while (0)

#define gte_ldopv1SV( r0 ) do {						\
	GTEport_SetCtrl( 0, (u32)_GTE_LH( r0, 0 ) );			\
	GTEport_SetCtrl( 2, (u32)_GTE_LH( r0, 2 ) );			\
	GTEport_SetCtrl( 4, (u32)_GTE_LH( r0, 4 ) );			\
} while (0)

/*
 * Type 2 functions
 */

#define gte_rtps()	( GTEport_Op( 0x0000007f ) )
#define gte_rtpt()	( GTEport_Op( 0x000000bf ) )
#define gte_rt()	( GTEport_Op( 0x000000ff ) )
#define gte_rtv0()	( GTEport_Op( 0x0000013f ) )
#define gte_rtv1()	( GTEport_Op( 0x0000017f ) )
#define gte_rtv2()	( GTEport_Op( 0x000001bf ) )
#define gte_rtir()	( GTEport_Op( 0x000001ff ) )
#define gte_rtir_sf0()	( GTEport_Op( 0x0000023f ) )
#define gte_rtv0tr()	( GTEport_Op( 0x0000027f ) )
#define gte_rtv1tr()	( GTEport_Op( 0x000002bf ) )
#define gte_rtv2tr()	( GTEport_Op( 0x000002ff ) )
#define gte_rtirtr()	( GTEport_Op( 0x0000033f ) )
#define gte_rtv0bk()	( GTEport_Op( 0x0000037f ) )
#define gte_rtv1bk()	( GTEport_Op( 0x000003bf ) )
#define gte_rtv2bk()	( GTEport_Op( 0x000003ff ) )
#define gte_rtirbk()	( GTEport_Op( 0x0000043f ) )
#define gte_ll()	( GTEport_Op( 0x0000057f ) )
#define gte_llv0()	( GTEport_Op( 0x000005bf ) )
#define gte_llv1()	( GTEport_Op( 0x000005ff ) )
#define gte_llv2()	( GTEport_Op( 0x0000063f ) )
#define gte_llir()	( GTEport_Op( 0x0000067f ) )
#define gte_llv0tr()	( GTEport_Op( 0x000006bf ) )
#define gte_llv1tr()	( GTEport_Op( 0x000006ff ) )
#define gte_llv2tr()	( GTEport_Op( 0x0000073f ) )
#define gte_llirtr()	( GTEport_Op( 0x0000077f ) )
#define gte_llv0bk()	( GTEport_Op( 0x000007bf ) )
#define gte_llv1bk()	( GTEport_Op( 0x000007ff ) )
#define gte_llv2bk()	( GTEport_Op( 0x0000083f ) )
#define gte_llirbk()	( GTEport_Op( 0x0000087f ) )
#define gte_lc()	( GTEport_Op( 0x000009bf ) )
#define gte_lcv0()	( GTEport_Op( 0x000009ff ) )
#define gte_lcv1()	( GTEport_Op( 0x00000a3f ) )
#define gte_lcv2()	( GTEport_Op( 0x00000a7f ) )
#define gte_lcir()	( GTEport_Op( 0x00000abf ) )
#define gte_lcv0tr()	( GTEport_Op( 0x00000aff ) )
#define gte_lcv1tr()	( GTEport_Op( 0x00000b3f ) )
#define gte_lcv2tr()	( GTEport_Op( 0x00000b7f ) )
#define gte_lcirtr()	( GTEport_Op( 0x00000bbf ) )
#define gte_lcv0bk()	( GTEport_Op( 0x00000bff ) )
#define gte_lcv1bk()	( GTEport_Op( 0x00000c3f ) )
#define gte_lcv2bk()	( GTEport_Op( 0x00000c7f ) )
#define gte_lcirbk()	( GTEport_Op( 0x00000cbf ) )
#define gte_dpcl()	( GTEport_Op( 0x00000dff ) )
#define gte_dpcs()	( GTEport_Op( 0x00000e3f ) )
#define gte_dpct()	( GTEport_Op( 0x00000e7f ) )
#define gte_intpl()	( GTEport_Op( 0x00000ebf ) )
#define gte_sqr12()	( GTEport_Op( 0x00000eff ) )
#define gte_sqr0()	( GTEport_Op( 0x00000f3f ) )
#define gte_ncs()	( GTEport_Op( 0x00000f7f ) )
#define gte_nct()	( GTEport_Op( 0x00000fbf ) )
#define gte_ncds()	( GTEport_Op( 0x00000fff ) )
#define gte_ncdt()	( GTEport_Op( 0x0000103f ) )
#define gte_nccs()	( GTEport_Op( 0x0000107f ) )
#define gte_ncct()	( GTEport_Op( 0x000010bf ) )
#define gte_cdp()	( GTEport_Op( 0x000010ff ) )
#define gte_cc()	( GTEport_Op( 0x0000113f ) )
#define gte_nclip()	( GTEport_Op( 0x0000117f ) )
#define gte_avsz3()	( GTEport_Op( 0x000011bf ) )
#define gte_avsz4()	( GTEport_Op( 0x000011ff ) )
#define gte_op12()	( GTEport_Op( 0x0000123f ) )
#define gte_op0()	( GTEport_Op( 0x0000127f ) )
#define gte_gpf12()	( GTEport_Op( 0x000012bf ) )
#define gte_gpf0()	( GTEport_Op( 0x000012ff ) )
#define gte_gpl12()	( GTEport_Op( 0x0000133f ) )
#define gte_gpl0()	( GTEport_Op( 0x0000137f ) )

#define gte_mvmva_core( r0 )	( GTEport_Op( (u32)( r0 ) ) )

#define gte_mvmva(sf,mx,v,cv,lm) gte_mvmva_core( 0x000013bf |	\
	((sf)<<25) | ((mx)<<23) | ((v)<<21) | ((cv)<<19) | ((lm)<<18))


/*
 * Type 2 functions without nop
 * (the nops only padded the MIPS pipeline; the portable forms are identical)
 */

#define gte_rtps_b()		( GTEport_Op( 0x0000007f ) )
#define gte_rtpt_b()		( GTEport_Op( 0x000000bf ) )
#define gte_rt_b()		( GTEport_Op( 0x000000ff ) )
#define gte_rtv0_b()		( GTEport_Op( 0x0000013f ) )
#define gte_rtv1_b()		( GTEport_Op( 0x0000017f ) )
#define gte_rtv2_b()		( GTEport_Op( 0x000001bf ) )
#define gte_rtir_b()		( GTEport_Op( 0x000001ff ) )
#define gte_rtir_sf0_b()	( GTEport_Op( 0x0000023f ) )
#define gte_rtv0tr_b()		( GTEport_Op( 0x0000027f ) )
#define gte_rtv1tr_b()		( GTEport_Op( 0x000002bf ) )
#define gte_rtv2tr_b()		( GTEport_Op( 0x000002ff ) )
#define gte_rtirtr_b()		( GTEport_Op( 0x0000033f ) )
#define gte_rtv0bk_b()		( GTEport_Op( 0x0000037f ) )
#define gte_rtv1bk_b()		( GTEport_Op( 0x000003bf ) )
#define gte_rtv2bk_b()		( GTEport_Op( 0x000003ff ) )
#define gte_rtirbk_b()		( GTEport_Op( 0x0000043f ) )
#define gte_ll_b()		( GTEport_Op( 0x0000057f ) )
#define gte_llv0_b()		( GTEport_Op( 0x000005bf ) )
#define gte_llv1_b()		( GTEport_Op( 0x000005ff ) )
#define gte_llv2_b()		( GTEport_Op( 0x0000063f ) )
#define gte_llir_b()		( GTEport_Op( 0x0000067f ) )
#define gte_llv0tr_b()		( GTEport_Op( 0x000006bf ) )
#define gte_llv1tr_b()		( GTEport_Op( 0x000006ff ) )
#define gte_llv2tr_b()		( GTEport_Op( 0x0000073f ) )
#define gte_llirtr_b()		( GTEport_Op( 0x0000077f ) )
#define gte_llv0bk_b()		( GTEport_Op( 0x000007bf ) )
#define gte_llv1bk_b()		( GTEport_Op( 0x000007ff ) )
#define gte_llv2bk_b()		( GTEport_Op( 0x0000083f ) )
#define gte_llirbk_b()		( GTEport_Op( 0x0000087f ) )
#define gte_lc_b()		( GTEport_Op( 0x000009bf ) )
#define gte_lcv0_b()		( GTEport_Op( 0x000009ff ) )
#define gte_lcv1_b()		( GTEport_Op( 0x00000a3f ) )
#define gte_lcv2_b()		( GTEport_Op( 0x00000a7f ) )
#define gte_lcir_b()		( GTEport_Op( 0x00000abf ) )
#define gte_lcv0tr_b()		( GTEport_Op( 0x00000aff ) )
#define gte_lcv1tr_b()		( GTEport_Op( 0x00000b3f ) )
#define gte_lcv2tr_b()		( GTEport_Op( 0x00000b7f ) )
#define gte_lcirtr_b()		( GTEport_Op( 0x00000bbf ) )
#define gte_lcv0bk_b()		( GTEport_Op( 0x00000bff ) )
#define gte_lcv1bk_b()		( GTEport_Op( 0x00000c3f ) )
#define gte_lcv2bk_b()		( GTEport_Op( 0x00000c7f ) )
#define gte_lcirbk_b()		( GTEport_Op( 0x00000cbf ) )
#define gte_dpcl_b()		( GTEport_Op( 0x00000dff ) )
#define gte_dpcs_b()		( GTEport_Op( 0x00000e3f ) )
#define gte_dpct_b()		( GTEport_Op( 0x00000e7f ) )
#define gte_intpl_b()		( GTEport_Op( 0x00000ebf ) )
#define gte_sqr12_b()		( GTEport_Op( 0x00000eff ) )
#define gte_sqr0_b()		( GTEport_Op( 0x00000f3f ) )
#define gte_ncs_b()		( GTEport_Op( 0x00000f7f ) )
#define gte_nct_b()		( GTEport_Op( 0x00000fbf ) )
#define gte_ncds_b()		( GTEport_Op( 0x00000fff ) )
#define gte_ncdt_b()		( GTEport_Op( 0x0000103f ) )
#define gte_nccs_b()		( GTEport_Op( 0x0000107f ) )
#define gte_ncct_b()		( GTEport_Op( 0x000010bf ) )
#define gte_cdp_b()		( GTEport_Op( 0x000010ff ) )
#define gte_cc_b()		( GTEport_Op( 0x0000113f ) )
#define gte_nclip_b()		( GTEport_Op( 0x0000117f ) )
#define gte_avsz3_b()		( GTEport_Op( 0x000011bf ) )
#define gte_avsz4_b()		( GTEport_Op( 0x000011ff ) )
#define gte_op12_b()		( GTEport_Op( 0x0000123f ) )
#define gte_op0_b()		( GTEport_Op( 0x0000127f ) )
#define gte_gpf12_b()		( GTEport_Op( 0x000012bf ) )
#define gte_gpf0_b()		( GTEport_Op( 0x000012ff ) )
#define gte_gpl12_b()		( GTEport_Op( 0x0000133f ) )
#define gte_gpl0_b()		( GTEport_Op( 0x0000137f ) )
#define gte_mvmva_core_b( r0 )	( GTEport_Op( (u32)( r0 ) ) )
#define gte_mvmva_b(sf,mx,v,cv,lm) gte_mvmva_core_b( 0x000013bf |\
	((sf)<<25) | ((mx)<<23) | ((v)<<21) | ((cv)<<19) | ((lm)<<18))

/*
 * Type 3 functions
 */

#define gte_stsxy( r0 )							\
	( _GTE_SW( r0, 0, GTEport_GetData( 14 ) ) )

#define gte_stsxy3( r0, r1, r2 ) do {					\
	_GTE_SW( r0, 0, GTEport_GetData( 12 ) );			\
	_GTE_SW( r1, 0, GTEport_GetData( 13 ) );			\
	_GTE_SW( r2, 0, GTEport_GetData( 14 ) );			\
} while (0)

#define gte_stsxy3c( r0 ) do {						\
	_GTE_SW( r0, 0, GTEport_GetData( 12 ) );			\
	_GTE_SW( r0, 4, GTEport_GetData( 13 ) );			\
	_GTE_SW( r0, 8, GTEport_GetData( 14 ) );			\
} while (0)

#define gte_stsxy2( r0 )						\
	( _GTE_SW( r0, 0, GTEport_GetData( 14 ) ) )

#define gte_stsxy1( r0 )						\
	( _GTE_SW( r0, 0, GTEport_GetData( 13 ) ) )

#define gte_stsxy0( r0 )						\
	( _GTE_SW( r0, 0, GTEport_GetData( 12 ) ) )

#define gte_stsxy01( r0, r1 ) do {					\
	_GTE_SW( r0, 0, GTEport_GetData( 12 ) );			\
	_GTE_SW( r1, 0, GTEport_GetData( 13 ) );			\
} while (0)

#define gte_stsxy01c( r0 ) do {						\
	_GTE_SW( r0, 0, GTEport_GetData( 12 ) );			\
	_GTE_SW( r0, 4, GTEport_GetData( 13 ) );			\
} while (0)

#define gte_stsxy3_f3( r0 ) do {					\
	_GTE_SW( r0, 8, GTEport_GetData( 12 ) );			\
	_GTE_SW( r0, 12, GTEport_GetData( 13 ) );			\
	_GTE_SW( r0, 16, GTEport_GetData( 14 ) );			\
} while (0)

#define gte_stsxy3_g3( r0 ) do {					\
	_GTE_SW( r0, 8, GTEport_GetData( 12 ) );			\
	_GTE_SW( r0, 16, GTEport_GetData( 13 ) );			\
	_GTE_SW( r0, 24, GTEport_GetData( 14 ) );			\
} while (0)

#define gte_stsxy3_ft3( r0 ) do {					\
	_GTE_SW( r0, 8, GTEport_GetData( 12 ) );			\
	_GTE_SW( r0, 16, GTEport_GetData( 13 ) );			\
	_GTE_SW( r0, 24, GTEport_GetData( 14 ) );			\
} while (0)

#define gte_stsxy3_gt3( r0 ) do {					\
	_GTE_SW( r0, 8, GTEport_GetData( 12 ) );			\
	_GTE_SW( r0, 20, GTEport_GetData( 13 ) );			\
	_GTE_SW( r0, 32, GTEport_GetData( 14 ) );			\
} while (0)

#define gte_stsxy3_f4( r0 ) do {					\
	_GTE_SW( r0, 8, GTEport_GetData( 12 ) );			\
	_GTE_SW( r0, 12, GTEport_GetData( 13 ) );			\
	_GTE_SW( r0, 16, GTEport_GetData( 14 ) );			\
} while (0)

#define gte_stsxy3_g4( r0 ) do {					\
	_GTE_SW( r0, 8, GTEport_GetData( 12 ) );			\
	_GTE_SW( r0, 16, GTEport_GetData( 13 ) );			\
	_GTE_SW( r0, 24, GTEport_GetData( 14 ) );			\
} while (0)

#define gte_stsxy3_ft4( r0 ) do {					\
	_GTE_SW( r0, 8, GTEport_GetData( 12 ) );			\
	_GTE_SW( r0, 16, GTEport_GetData( 13 ) );			\
	_GTE_SW( r0, 24, GTEport_GetData( 14 ) );			\
} while (0)

#define gte_stsxy3_gt4( r0 ) do {					\
	_GTE_SW( r0, 8, GTEport_GetData( 12 ) );			\
	_GTE_SW( r0, 20, GTEport_GetData( 13 ) );			\
	_GTE_SW( r0, 32, GTEport_GetData( 14 ) );			\
} while (0)

#define gte_stdp( r0 )							\
	( _GTE_SW( r0, 0, GTEport_GetData( 8 ) ) )

#define gte_stflg( r0 )							\
	( _GTE_SW( r0, 0, GTEport_GetCtrl( 31 ) ) )

#define gte_stflg_4( r0 )						\
	( _GTE_SW( r0, 0, GTEport_GetCtrl( 31 ) & 0x00040000 ) )

#define gte_stsz( r0 )							\
	( _GTE_SW( r0, 0, GTEport_GetData( 19 ) ) )

#define gte_stsz3( r0, r1, r2 ) do {					\
	_GTE_SW( r0, 0, GTEport_GetData( 17 ) );			\
	_GTE_SW( r1, 0, GTEport_GetData( 18 ) );			\
	_GTE_SW( r2, 0, GTEport_GetData( 19 ) );			\
} while (0)

#define gte_stsz4( r0, r1, r2, r3 ) do {				\
	_GTE_SW( r0, 0, GTEport_GetData( 16 ) );			\
	_GTE_SW( r1, 0, GTEport_GetData( 17 ) );			\
	_GTE_SW( r2, 0, GTEport_GetData( 18 ) );			\
	_GTE_SW( r3, 0, GTEport_GetData( 19 ) );			\
} while (0)

#define gte_stsz3c( r0 ) do {						\
	_GTE_SW( r0, 0, GTEport_GetData( 17 ) );			\
	_GTE_SW( r0, 4, GTEport_GetData( 18 ) );			\
	_GTE_SW( r0, 8, GTEport_GetData( 19 ) );			\
} while (0)

#define gte_stsz4c( r0 ) do {						\
	_GTE_SW( r0, 0, GTEport_GetData( 16 ) );			\
	_GTE_SW( r0, 4, GTEport_GetData( 17 ) );			\
	_GTE_SW( r0, 8, GTEport_GetData( 18 ) );			\
	_GTE_SW( r0, 12, GTEport_GetData( 19 ) );			\
} while (0)

#define gte_stszotz( r0 )						\
	( _GTE_SW( r0, 0, (u32)( (s32)GTEport_GetData( 19 ) >> 2 ) ) )

#define gte_stotz( r0 )							\
	( _GTE_SW( r0, 0, GTEport_GetData( 7 ) ) )

#define gte_stopz( r0 )							\
	( _GTE_SW( r0, 0, GTEport_GetData( 24 ) ) )

#define gte_stlvl( r0 ) do {						\
	_GTE_SW( r0, 0, GTEport_GetData( 9 ) );				\
	_GTE_SW( r0, 4, GTEport_GetData( 10 ) );			\
	_GTE_SW( r0, 8, GTEport_GetData( 11 ) );			\
} while (0)

#define gte_stlvnl( r0 ) do {						\
	_GTE_SW( r0, 0, GTEport_GetData( 25 ) );			\
	_GTE_SW( r0, 4, GTEport_GetData( 26 ) );			\
	_GTE_SW( r0, 8, GTEport_GetData( 27 ) );			\
} while (0)

#define gte_stlvnl0( r0 )						\
	( _GTE_SW( r0, 0, GTEport_GetData( 25 ) ) )

#define gte_stlvnl1( r0 )						\
	( _GTE_SW( r0, 0, GTEport_GetData( 26 ) ) )

#define gte_stlvnl2( r0 )						\
	( _GTE_SW( r0, 0, GTEport_GetData( 27 ) ) )

#define gte_stsv( r0 ) do {						\
	_GTE_SH( r0, 0, GTEport_GetData( 9 ) );				\
	_GTE_SH( r0, 2, GTEport_GetData( 10 ) );			\
	_GTE_SH( r0, 4, GTEport_GetData( 11 ) );			\
} while (0)

#define gte_stclmv( r0 ) do {						\
	_GTE_SH( r0, 0, GTEport_GetData( 9 ) );				\
	_GTE_SH( r0, 6, GTEport_GetData( 10 ) );			\
	_GTE_SH( r0, 12, GTEport_GetData( 11 ) );			\
} while (0)

#define gte_stbv( r0 ) do {						\
	_GTE_SB( r0, 0, GTEport_GetData( 9 ) );				\
	_GTE_SB( r0, 1, GTEport_GetData( 10 ) );			\
} while (0)

#define gte_stcv( r0 ) do {						\
	_GTE_SB( r0, 0, GTEport_GetData( 9 ) );				\
	_GTE_SB( r0, 1, GTEport_GetData( 10 ) );			\
	_GTE_SB( r0, 2, GTEport_GetData( 11 ) );			\
} while (0)

#define gte_strgb( r0 )							\
	( _GTE_SW( r0, 0, GTEport_GetData( 22 ) ) )

#define gte_strgb3( r0, r1, r2 ) do {					\
	_GTE_SW( r0, 0, GTEport_GetData( 20 ) );			\
	_GTE_SW( r1, 0, GTEport_GetData( 21 ) );			\
	_GTE_SW( r2, 0, GTEport_GetData( 22 ) );			\
} while (0)

#define gte_strgb3_g3( r0 ) do {					\
	_GTE_SW( r0, 4, GTEport_GetData( 20 ) );			\
	_GTE_SW( r0, 12, GTEport_GetData( 21 ) );			\
	_GTE_SW( r0, 20, GTEport_GetData( 22 ) );			\
} while (0)

#define gte_strgb3_gt3( r0 ) do {					\
	_GTE_SW( r0, 4, GTEport_GetData( 20 ) );			\
	_GTE_SW( r0, 16, GTEport_GetData( 21 ) );			\
	_GTE_SW( r0, 28, GTEport_GetData( 22 ) );			\
} while (0)

#define gte_strgb3_g4( r0 ) do {					\
	_GTE_SW( r0, 4, GTEport_GetData( 20 ) );			\
	_GTE_SW( r0, 12, GTEport_GetData( 21 ) );			\
	_GTE_SW( r0, 20, GTEport_GetData( 22 ) );			\
} while (0)

#define gte_strgb3_gt4( r0 ) do {					\
	_GTE_SW( r0, 4, GTEport_GetData( 20 ) );			\
	_GTE_SW( r0, 16, GTEport_GetData( 21 ) );			\
	_GTE_SW( r0, 28, GTEport_GetData( 22 ) );			\
} while (0)

#define gte_ReadGeomOffset( r0, r1 ) do {				\
	_GTE_SW( r0, 0, (u32)( (s32)GTEport_GetCtrl( 24 ) >> 16 ) );	\
	_GTE_SW( r1, 0, (u32)( (s32)GTEport_GetCtrl( 25 ) >> 16 ) );	\
} while (0)

#define gte_ReadGeomScreen( r0 )					\
	( _GTE_SW( r0, 0, GTEport_GetCtrl( 26 ) ) )

#define gte_ReadRotMatrix( r0 ) do {					\
	_GTE_SW( r0, 0, GTEport_GetCtrl( 0 ) );				\
	_GTE_SW( r0, 4, GTEport_GetCtrl( 1 ) );				\
	_GTE_SW( r0, 8, GTEport_GetCtrl( 2 ) );				\
	_GTE_SW( r0, 12, GTEport_GetCtrl( 3 ) );			\
	_GTE_SW( r0, 16, GTEport_GetCtrl( 4 ) );			\
	_GTE_SW( r0, 20, GTEport_GetCtrl( 5 ) );			\
	_GTE_SW( r0, 24, GTEport_GetCtrl( 6 ) );			\
	_GTE_SW( r0, 28, GTEport_GetCtrl( 7 ) );			\
} while (0)

#define gte_sttr( r0 ) do {						\
	_GTE_SW( r0, 0, GTEport_GetCtrl( 5 ) );				\
	_GTE_SW( r0, 4, GTEport_GetCtrl( 6 ) );				\
	_GTE_SW( r0, 8, GTEport_GetCtrl( 7 ) );				\
} while (0)

#define gte_ReadLightMatrix( r0 ) do {					\
	_GTE_SW( r0, 0, GTEport_GetCtrl( 8 ) );				\
	_GTE_SW( r0, 4, GTEport_GetCtrl( 9 ) );				\
	_GTE_SW( r0, 8, GTEport_GetCtrl( 10 ) );			\
	_GTE_SW( r0, 12, GTEport_GetCtrl( 11 ) );			\
	_GTE_SW( r0, 16, GTEport_GetCtrl( 12 ) );			\
	_GTE_SW( r0, 20, GTEport_GetCtrl( 13 ) );			\
	_GTE_SW( r0, 24, GTEport_GetCtrl( 14 ) );			\
	_GTE_SW( r0, 28, GTEport_GetCtrl( 15 ) );			\
} while (0)

#define gte_ReadColorMatrix( r0 ) do {					\
	_GTE_SW( r0, 0, GTEport_GetCtrl( 16 ) );			\
	_GTE_SW( r0, 4, GTEport_GetCtrl( 17 ) );			\
	_GTE_SW( r0, 8, GTEport_GetCtrl( 18 ) );			\
	_GTE_SW( r0, 12, GTEport_GetCtrl( 19 ) );			\
	_GTE_SW( r0, 16, GTEport_GetCtrl( 20 ) );			\
	_GTE_SW( r0, 20, GTEport_GetCtrl( 21 ) );			\
	_GTE_SW( r0, 24, GTEport_GetCtrl( 22 ) );			\
	_GTE_SW( r0, 28, GTEport_GetCtrl( 23 ) );			\
} while (0)

#define gte_stlzc( r0 )							\
	( _GTE_SW( r0, 0, GTEport_GetData( 31 ) ) )

#define gte_stfc( r0 ) do {						\
	_GTE_SW( r0, 0, GTEport_GetCtrl( 21 ) );			\
	_GTE_SW( r0, 4, GTEport_GetCtrl( 22 ) );			\
	_GTE_SW( r0, 8, GTEport_GetCtrl( 23 ) );			\
} while (0)

#define gte_mvlvtr() do {						\
	u32 _gte_x = GTEport_GetData( 25 );				\
	u32 _gte_y = GTEport_GetData( 26 );				\
	u32 _gte_z = GTEport_GetData( 27 );				\
	GTEport_SetCtrl( 5, _gte_x );					\
	GTEport_SetCtrl( 6, _gte_y );					\
	GTEport_SetCtrl( 7, _gte_z );					\
} while (0)

#define gte_nop()	( (void)0 )

/*	gte_subdvl / gte_subdvd / gte_adddvl / gte_adddvd:
	The originals push the two source words through IR1/IR2 (mtc2 then
	mfc2 of data regs 9/10) so that the GTE's 16-bit sign-extension of
	IR reads yields the sign-extended low halves, while the high halves
	are combined with sra 16.  The round trip through the software GTE
	is preserved so the semantics stay identical.
*/

#define gte_subdvl( r0, r1, r2 ) do {					\
	u32 _gte_a = _GTE_LW( r0, 0 );					\
	u32 _gte_b = _GTE_LW( r1, 0 );					\
	u32 _gte_hi;							\
	GTEport_SetData( 9, _gte_a );					\
	GTEport_SetData( 10, _gte_b );					\
	_gte_hi = (u32)( ( (s32)_gte_a >> 16 ) - ( (s32)_gte_b >> 16 ) ); \
	_gte_a = GTEport_GetData( 9 );					\
	_gte_b = GTEport_GetData( 10 );					\
	_GTE_SW( r2, 4, _gte_hi );					\
	_GTE_SW( r2, 0, _gte_a - _gte_b );				\
} while (0)

#define gte_subdvd( r0, r1, r2 ) do {					\
	u32 _gte_a = _GTE_LW( r0, 0 );					\
	u32 _gte_b = _GTE_LW( r1, 0 );					\
	u32 _gte_hi;							\
	GTEport_SetData( 9, _gte_a );					\
	GTEport_SetData( 10, _gte_b );					\
	_gte_hi = (u32)( ( (s32)_gte_a >> 16 ) - ( (s32)_gte_b >> 16 ) ); \
	_gte_a = GTEport_GetData( 9 );					\
	_gte_b = GTEport_GetData( 10 );					\
	_GTE_SH( r2, 2, _gte_hi );					\
	_GTE_SH( r2, 0, _gte_a - _gte_b );				\
} while (0)

#define gte_adddvl( r0, r1, r2 ) do {					\
	u32 _gte_a = _GTE_LW( r0, 0 );					\
	u32 _gte_b = _GTE_LW( r1, 0 );					\
	u32 _gte_hi;							\
	GTEport_SetData( 9, _gte_a );					\
	GTEport_SetData( 10, _gte_b );					\
	_gte_hi = (u32)( ( (s32)_gte_a >> 16 ) + ( (s32)_gte_b >> 16 ) ); \
	_gte_a = GTEport_GetData( 9 );					\
	_gte_b = GTEport_GetData( 10 );					\
	_GTE_SW( r2, 4, _gte_hi );					\
	_GTE_SW( r2, 0, _gte_a + _gte_b );				\
} while (0)

#define gte_adddvd( r0, r1, r2 ) do {					\
	u32 _gte_a = _GTE_LW( r0, 0 );					\
	u32 _gte_b = _GTE_LW( r1, 0 );					\
	u32 _gte_hi;							\
	GTEport_SetData( 9, _gte_a );					\
	GTEport_SetData( 10, _gte_b );					\
	_gte_hi = (u32)( ( (s32)_gte_a >> 16 ) + ( (s32)_gte_b >> 16 ) ); \
	_gte_a = GTEport_GetData( 9 );					\
	_gte_b = GTEport_GetData( 10 );					\
	_GTE_SH( r2, 2, _gte_hi );					\
	_GTE_SH( r2, 0, _gte_a + _gte_b );				\
} while (0)

/*	gte_FlipRotMatrixX: negates R11, R12 and R13 (the packed halfwords of
	rotation-matrix ctrl regs 0 and 1); the high half of ctrl reg 1 (R21)
	is repacked unchanged -- exactly as the original asm does.
*/
#define gte_FlipRotMatrixX() do {					\
	u32 _gte_c0 = GTEport_GetCtrl( 0 );				\
	u32 _gte_c1 = GTEport_GetCtrl( 1 );				\
	u32 _gte_lo;							\
	u32 _gte_hi;							\
	_gte_lo = (u32)( -(s32)(s16)(u16)_gte_c0 );	/* sll/sra/subu: -R11 */ \
	_gte_hi = (u32)( -( (s32)_gte_c0 >> 16 ) );	/* sra/subu:     -R12 */ \
	GTEport_SetCtrl( 0, ( _gte_lo & 0xffffu ) | ( _gte_hi << 16 ) ); \
	_gte_lo = (u32)( -(s32)(s16)(u16)_gte_c1 );	/* sll/sra/subu: -R13 */ \
	_gte_hi = (u32)( (s32)_gte_c1 >> 16 );		/* sra: R21 unchanged */ \
	GTEport_SetCtrl( 1, ( _gte_lo & 0xffffu ) | ( _gte_hi << 16 ) ); \
} while (0)

#define gte_FlipTRX() do {						\
	u32 _gte_tr = GTEport_GetCtrl( 5 );				\
	GTEport_SetCtrl( 5, (u32)0 - _gte_tr );		/* subu $0,TRX */ \
} while (0)

#endif	/* _INLINE_C_PORT_H_ */
