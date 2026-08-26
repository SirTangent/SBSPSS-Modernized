/*	Software GTE (cop2) core - hardware-word interface.

	GTE_ExecuteCop2 takes a real MIPS cop2 instruction word - the thing
	DMPSX patched into PS1 object files.  gteport.cpp translates the
	portable DMPSX *tag* words (what the game's gte_* macros emit) into
	these using the tag table lifted from DMPSX.EXE itself (offset 0x823,
	24-byte records), so the mapping cannot drift from the original tool.

	The register file is the raw hardware one: writes store words, reads
	convert (sign-extension, FIFO mirrors, ORGB packing) - exactly
	mtc2/mfc2/ctc2/cfc2 semantics.  Arithmetic follows the psx-spx GTE
	chapter: 44-bit MAC accumulation with per-step sign extension and
	overflow flags, saturation with FLAG bits, the FLAG master bit 31,
	and the UNR reciprocal division for RTPS/RTPT.
*/
#ifndef PORT_GTE_CORE_H
#define PORT_GTE_CORE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*	The two hardware cop2 words libgte's C functions issue directly
	(libgte_fns.cpp).  They live here rather than at the call site so they
	cannot drift from gteport.cpp's DMPSX tag table, which names these same
	constants for the corresponding tag entries.  */
#define COP2_RTPS	0x4A180001u		/* RTPS  */
#define COP2_RT		0x4A480012u		/* MVMVA sf=1: RT*V0 + TR */

void		GTE_ExecuteCop2(uint32_t inst);		/* hardware cop2 word */
uint32_t	GTE_ReadData(int reg);				/* mfc2 */
void		GTE_WriteData(int reg, uint32_t v);	/* mtc2 */
uint32_t	GTE_ReadCtrl(int reg);				/* cfc2 */
void		GTE_WriteCtrl(int reg, uint32_t v);	/* ctc2 */

#ifdef __cplusplus
}
#endif

#endif
