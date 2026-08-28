/*	Software PS1 SPU - shared internal state (M5).

	Sound RAM is the real thing: a flat 512KB byte array addressed exactly as
	the hardware is (the game and the XM player only ever pass integer SPU
	addresses around, so no game-visible pointer ever aliases this).  The
	voice register file mirrors what SpuSetVoiceAttr / the XM player program.
	Unlike the GPU, this state is shared with the SDL audio thread - the
	mixer (spu_core.cpp) owns the lock.
*/
#ifndef PORT_SPU_CORE_H
#define PORT_SPU_CORE_H

#include <stdint.h>

#define SPU_RAM_SIZE	(512 * 1024)
#define SPU_NVOICES		24

extern uint8_t g_spuRam[SPU_RAM_SIZE];

struct SpuVoiceState
{
	/* programmed registers */
	uint32_t	startAddr;		/* byte address in g_spuRam (16-byte blocks) */
	uint32_t	repeatAddr;		/* loop address (flag bit 2 rewrites it) */
	uint16_t	pitch;			/* 0x1000 = 44100 Hz */
	int16_t		volL, volR;		/* plain volume mode only - sweep never used */
	uint16_t	adsr1, adsr2;	/* raw hardware ADSR register halves */
};

extern SpuVoiceState g_spuVoice[SPU_NVOICES];

/*	spu_adpcm.cpp: decode one 16-byte SPU ADPCM block into 28 PCM samples.
	byte 0 = shift (low nibble; 13..15 act as 9) | filter (high nibble, 0..4),
	byte 1 = loop flags, bytes 2..15 = 4-bit samples, low nibble first.
	hist1/hist2 are the two previous output samples and carry across
	consecutive blocks of one voice.  Pure function - no SPU state.  */
void SpuAdpcm_DecodeBlock(const uint8_t *block, int16_t *out28,
						  int16_t *hist1, int16_t *hist2);

/*	loop flags in block byte 1  */
#define SPU_ADPCM_LOOP_END		1	/* jump to repeatAddr after this block */
#define SPU_ADPCM_LOOP_REPEAT	2	/* with END: keep playing; without: mute */
#define SPU_ADPCM_LOOP_START	4	/* set repeatAddr to this block */

#endif
