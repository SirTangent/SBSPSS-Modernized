/*
	lznp_encode.cpp — LZNP compressor, written against the format that the
	game's decoder (source/utils/lznp.cpp, (c) Nick Pelling 1993-1994) consumes.
	Replaces the 16-bit tools/lznp.exe, which cannot run on 64-bit Windows.

	Token forms (each costs one flag bit, LSB-first per control byte):
	  flag 0:  literal            -> 1 data byte
	  flag 1:  byte i >= 0x60     -> copy 2 bytes  from offset (0x100 - i), 1..160
	  flag 1:  i < 0x60, j=i>>4:
	           j in 0..4          -> copy (j+3)    from offset ((i&15)<<8)|next, 1..4095
	           j == 5             -> copy (8+next) from 12-bit offset (superlength, 8..262)
	  flag 1:  bytes 0x00 0x00    -> terminator (offset 0)

	Greedy longest-match search. Brute-force window scan - correctness first;
	the data pipeline's inputs are small (sprite frames) or few (screens).
*/

#include "lznp_encode.h"

static const int WINDOW      = 4095;   /* 12-bit offset */
static const int PAIR_WINDOW = 160;    /* 0x100 - 0x60 */
static const int MAX_MATCH   = 262;    /* 8 + 254 superlength */

struct BitPacker
{
	unsigned char	*out;
	unsigned char	*ctrl;      /* position of current control byte */
	int				bitCount;

	void init(unsigned char *dest)
	{
		out = dest;
		ctrl = 0;
		bitCount = 8;
	}

	void putFlag(int bit)
	{
		if (bitCount == 8)
		{
			ctrl = out++;
			*ctrl = 0;
			bitCount = 0;
		}
		if (bit)
			*ctrl |= (unsigned char)(1 << bitCount);
		bitCount++;
	}

	void putByte(unsigned char b)	{ *out++ = b; }
};

int LZNP_Encode(unsigned char *Dest, const unsigned char *Src, int insize)
{
	BitPacker	bp;
	int			pos = 0;

	bp.init(Dest);

	while (pos < insize)
	{
		int	bestLen = 0, bestOff = 0;

		int	windowStart = pos - WINDOW;
		if (windowStart < 0) windowStart = 0;

		int	maxLen = insize - pos;
		if (maxLen > MAX_MATCH) maxLen = MAX_MATCH;

		if (maxLen >= 2)
		{
			for (int cand = pos - 1; cand >= windowStart; cand--)
			{
				if (Src[cand] != Src[pos])
					continue;
				/* a candidate can only beat bestLen if it also matches at
				   position bestLen - skipping the rest is output-identical
				   (nearest-first scan: later ties never replace bestOff) */
				if (bestLen && Src[cand + bestLen] != Src[pos + bestLen])
					continue;
				int len = 1;
				while (len < maxLen && Src[cand + len] == Src[pos + len])
					len++;
				if (len > bestLen)
				{
					bestLen = len;
					bestOff = pos - cand;
					if (len == maxLen)
						break;
				}
			}
		}

		if (bestLen >= 3)
		{
			bp.putFlag(1);
			if (bestLen <= 7)
			{	/* short form: j in 0..4 */
				bp.putByte((unsigned char)(((bestLen - 3) << 4) | (bestOff >> 8)));
				bp.putByte((unsigned char)(bestOff & 0xff));
			}
			else
			{	/* superlength form: j == 5 */
				bp.putByte((unsigned char)((5 << 4) | (bestOff >> 8)));
				bp.putByte((unsigned char)(bestOff & 0xff));
				bp.putByte((unsigned char)(bestLen - 8));
			}
			pos += bestLen;
		}
		else if (bestLen == 2 && bestOff <= PAIR_WINDOW)
		{
			bp.putFlag(1);
			bp.putByte((unsigned char)(0x100 - bestOff));
			pos += 2;
		}
		else
		{
			bp.putFlag(0);
			bp.putByte(Src[pos++]);
		}
	}

	/* terminator: flagged token with 12-bit offset 0 */
	bp.putFlag(1);
	bp.putByte(0x00);
	bp.putByte(0x00);

	return (int)(bp.out - Dest);
}
