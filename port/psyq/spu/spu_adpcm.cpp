/*	PS1 SPU ADPCM block decoder (M5).

	Semantics per psx-spx "CDROM/SPU ADPCM samples": each 16-byte block holds
	28 4-bit samples; a sample is the nibble sign-extended to 16 bits, shifted
	right, plus a two-tap IIR filter over the previous two OUTPUT samples with
	the fixed coefficient table below, rounded by +32 then an arithmetic shift
	(not a divide - negative sums round toward -inf like the hardware), and
	clamped to s16.  The clamped value is what enters the history.
*/
#include "spu/spu_core.h"

namespace
{
const int kFiltPos[5] = { 0, 60, 115, 98, 122 };
const int kFiltNeg[5] = { 0, 0, -52, -55, -60 };
}

void SpuAdpcm_DecodeBlock(const uint8_t *block, int16_t *out28,
						  int16_t *hist1, int16_t *hist2)
{
	int shift = block[0] & 0x0F;
	int filter = (block[0] >> 4) & 0x0F;
	if (shift > 12)
		shift = 9;			/* hardware: 13..15 behave as 9 */
	if (filter > 4)
		filter = 4;			/* 5..15 reserved; nothing shipped uses them */

	const int f0 = kFiltPos[filter];
	const int f1 = kFiltNeg[filter];
	int h1 = *hist1;
	int h2 = *hist2;

	for (int i = 0; i < 28; i++)
	{
		int nibble = (block[2 + (i >> 1)] >> ((i & 1) * 4)) & 0x0F;
		int s = (int16_t)(nibble << 12) >> shift;
		s += (h1 * f0 + h2 * f1 + 32) >> 6;
		if (s > 32767)
			s = 32767;
		else if (s < -32768)
			s = -32768;
		h2 = h1;
		h1 = s;
		out28[i] = (int16_t)s;
	}

	*hist1 = (int16_t)h1;
	*hist2 = (int16_t)h2;
}
