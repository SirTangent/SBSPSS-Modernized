/*	Software PS1 SPU - state (M5).

	This TU owns the 512KB sound-RAM model and the voice register file.  The
	mixer (gaussian interpolation, ADSR, render-under-lock) lands next; until
	then nothing reads this state, so no locking is needed yet.
*/
#include "spu/spu_core.h"

uint8_t g_spuRam[SPU_RAM_SIZE];
SpuVoiceState g_spuVoice[SPU_NVOICES];
