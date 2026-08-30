/*	CD-XA ADPCM sector decoder (M6).

	Layout per psx-spx "CDROM XA Audio ADPCM Compression": the 2328-byte
	Form2 user area starts with 18 sound groups of 128 bytes (the tail is
	unused/EDC).  A group is 16 header bytes + 112 data bytes.  In 4-bit
	mode a group holds 8 blocks ("sound units") of 28 samples:

	  header:  bytes 0-3 = params for blocks 0-3, bytes 4-7 = a duplicate,
	           bytes 8-11 = params for blocks 4-7, bytes 12-15 = duplicate
	           (param = shift in the low nibble, filter 0-3 in the high)
	  data:    sample i of block b lives in byte 16 + i*4 + b/2,
	           low nibble for even b, high nibble for odd b

	The per-sample arithmetic is exactly the SPU ADPCM recurrence
	(spu_adpcm.cpp, filters 0-3 of the same table): nibble sign-extended
	from bit 15, >> shift, plus the two-tap IIR over previous OUTPUTS with
	+32 >>6 rounding (arithmetic shift, so negative sums round toward -inf
	like the hardware), clamped to s16; the clamped value enters the
	history.  History flows across blocks, groups and sectors.
*/
#include <stdio.h>

#include "xa_adpcm.h"

namespace
{
/*	filters 0-3 (XA has no 4) - the same family as spu_adpcm.cpp:14-15  */
const int kFiltPos[4] = { 0, 60, 115, 98 };
const int kFiltNeg[4] = { 0, 0, -52, -55 };

/*	One 28-sample sound unit into `out` with stride (1 = mono buffer,
	2 = one channel of an interleaved pair buffer).  */
inline void decodeUnit(const uint8_t *g, int b, int16_t *out, int stride,
					   int *h1, int *h2)
{
	int param  = g[b < 4 ? b : b + 4];
	int shift  = param & 0x0F;
	int filter = (param >> 4) & 0x03;			/* XA: 2-bit filter field */
	if (shift > 12)
		shift = 9;								/* 13..15 behave as 9 */

	const int f0 = kFiltPos[filter];
	const int f1 = kFiltNeg[filter];
	const uint8_t *data = g + 16 + (b >> 1);
	const int nibShift  = (b & 1) * 4;

	for (int i = 0; i < 28; i++)
	{
		int nibble = (data[i * 4] >> nibShift) & 0x0F;
		int s = (int16_t)(nibble << 12) >> shift;
		s += (*h1 * f0 + *h2 * f1 + 32) >> 6;
		if (s > 32767)
			s = 32767;
		else if (s < -32768)
			s = -32768;
		*h2 = *h1;
		*h1 = s;
		out[i * stride] = (int16_t)s;
	}
}
}	/* namespace */

void XaAdpcm_DecodeSector4bitMono(const uint8_t *user2304, int16_t *out4032,
								  int32_t *hist1, int32_t *hist2)
{
	int h1 = (int)*hist1;
	int h2 = (int)*hist2;
	int16_t *out = out4032;

	for (int group = 0; group < 18; group++)
	{
		const uint8_t *g = user2304 + group * 128;
		for (int b = 0; b < 8; b++)
		{
			decodeUnit(g, b, out, 1, &h1, &h2);
			out += 28;
		}
	}

	*hist1 = h1;
	*hist2 = h2;
}

void XaAdpcm_DecodeSector4bitStereo(const uint8_t *user2304,
									int16_t *outPairs2016x2,
									int32_t *histL1, int32_t *histL2,
									int32_t *histR1, int32_t *histR2)
{
	int hL1 = (int)*histL1, hL2 = (int)*histL2;
	int hR1 = (int)*histR1, hR2 = (int)*histR2;
	int16_t *out = outPairs2016x2;

	for (int group = 0; group < 18; group++)
	{
		const uint8_t *g = user2304 + group * 128;
		/*	units pair up in time: (0,1), (2,3), (4,5), (6,7); even = L  */
		for (int u = 0; u < 8; u += 2)
		{
			decodeUnit(g, u,     out,     2, &hL1, &hL2);
			decodeUnit(g, u + 1, out + 1, 2, &hR1, &hR2);
			out += 28 * 2;
		}
	}

	*histL1 = hL1;
	*histL2 = hL2;
	*histR1 = hR1;
	*histR2 = hR2;
}
