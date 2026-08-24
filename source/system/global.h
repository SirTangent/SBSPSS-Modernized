/**********************/
/*** Global Defines ***/
/**********************/

#ifndef _GLOBAL_HEADER_
#define _GLOBAL_HEADER_

#include 	<stddef.h>
#include 	<stdlib.h>
#include 	<stdio.h>
#include 	<strings.h>
#include 	<libetc.h>
#include 	<libgte.h>
#include 	<libgpu.h>
#include 	<libsn.h>
#include 	<libcd.h>
#include 	<libspu.h>
#include 	<libapi.h>
#include 	<inline_c.h>
#include 	<sys\types.h>
#include 	"utils\replace.h"
#include 	<gtemac.h>

/*****************************************************************************/
/*	The PS1 scratchpad is 1KB of fast RAM at a fixed physical address.  The
	PC port supplies a real 1KB buffer instead (defined by the PsyQ shim);
	several users cast SCRATCH_RAM in namespace-scope initialisers, so it
	must stay an address-constant expression.
*/
#include	"system\asmport.h"
#ifdef	PSX_MIPS_ASM
#define SCRATCH_RAM 		0x1f800000
#else
#define SCRATCH_RAM 		((unsigned char*)PORT_Scratchpad)
#endif
#define	FAST_STACK			(SCRATCH_RAM+0x3f0)

/*****************************************************************************/
#include	"system\types.h"

#include 	"mem\memory.h"
#include 	"system\gte.h"
#include	"utils\cmxmacro.h"
#include 	"utils\fixed.h"

#include 	"system\dbg.h"
#include	"system\info.h"
#include	"system\lnkopt.h"

/*****************************************************************************/

#endif
