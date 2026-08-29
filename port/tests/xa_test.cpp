/*	Unit tests for the M6 XA path: the XA-ADPCM sector decoder
	(port/psyq/cd/xa_adpcm.cpp).

	Decoder oracles, three layers:
	  1. hand-checked basics: an all-zero sector decodes to silence;
	     filter-0 blocks are exactly the sign-extended nibble >> shift.
	  2. cross-oracle vs the golden-tested SPU ADPCM decoder: the per-sample
	     arithmetic is the same recurrence, only the bit layout differs - so
	     an XA group carrying the same nibble stream and params as a chain
	     of SPU blocks must decode identically, history and all.
	  3. real data: 4 consecutive channel-1 sectors extracted from
	     Track1.Ixa (xa_fixture_sectors.bin) against ffmpeg's adpcm_xa
	     decode (xa_fixture_golden.pcm).  Regenerate both from the repo
	     root with `py port/tests/make_xa_fixture.py` (wraps the sectors as
	     RIFF CDXA, ffmpeg -i fixture.xa -f s16le xa_fixture_golden.pcm).
	     Layer 3 needs the repo root as cwd; it skips with a note otherwise.
*/
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "cd/xa_adpcm.h"
#include "spu/spu_core.h"

static int g_failures;

static void check(bool ok, const char *what)
{
	if (!ok)
	{
		std::printf("FAIL: %s\n", what);
		g_failures++;
	}
}

/*	build one 128-byte XA sound group: params[8] and, per block, 28 nibbles  */
static void buildGroup(uint8_t *g, const uint8_t params[8],
					   const uint8_t nibbles[8][28])
{
	memset(g, 0, 128);
	for (int b = 0; b < 8; b++)
	{
		g[b < 4 ? b : b + 4] = params[b];		/* + the duplicate copies */
		g[(b < 4 ? b : b + 4) + 4] = params[b];
		for (int i = 0; i < 28; i++)
			g[16 + i * 4 + (b >> 1)] |= (uint8_t)(nibbles[b][i] << ((b & 1) * 4));
	}
}

int main(void)
{
	static uint8_t	user[XA_SECTOR_USER_BYTES];
	static int16_t	out[XA_SECTOR_SAMPLES];
	int32_t			h1, h2;

	/*	-------- 1a: silence in, silence out  */
	memset(user, 0, sizeof(user));
	h1 = h2 = 0;
	XaAdpcm_DecodeSector4bitMono(user, out, &h1, &h2);
	bool allZero = true;
	for (int i = 0; i < XA_SECTOR_SAMPLES; i++)
		allZero = allZero && out[i] == 0;
	check(allZero && h1 == 0 && h2 == 0, "zero sector decodes to silence");

	/*	-------- 1b: filter 0 = pure (nibble<<12 sign-extended) >> shift  */
	{
		uint8_t params[8]     = { 0, 1, 4, 12, 0, 2, 3, 5 };	/* filter 0, varied shifts */
		uint8_t nib[8][28];
		for (int b = 0; b < 8; b++)
			for (int i = 0; i < 28; i++)
				nib[b][i] = (uint8_t)((b * 5 + i) & 0x0F);
		memset(user, 0, sizeof(user));
		buildGroup(user, params, nib);

		h1 = h2 = 0;
		XaAdpcm_DecodeSector4bitMono(user, out, &h1, &h2);
		bool ok = true;
		for (int b = 0; b < 8 && ok; b++)
			for (int i = 0; i < 28; i++)
			{
				int expect = (int16_t)(nib[b][i] << 12) >> params[b];
				if (out[b * 28 + i] != expect)
				{
					std::printf("  block %d sample %d: got %d expect %d\n",
								b, i, out[b * 28 + i], expect);
					ok = false;
					break;
				}
			}
		check(ok, "filter-0 blocks are nibble>>shift exactly");
	}

	/*	-------- 2: cross-oracle vs the SPU ADPCM decoder.
		Feed the identical nibble stream + params through both layouts with
		threaded history; every sample must match.  Covers filters 1-3,
		block-to-block and group-to-group history continuity.  */
	{
		uint8_t params[8];
		uint8_t nib[8][28];
		unsigned seed = 0x2D5;
		for (int b = 0; b < 8; b++)
		{
			int filter = 1 + (b % 3);			/* 1,2,3 */
			int shift  = 2 + (b % 5);
			params[b] = (uint8_t)((filter << 4) | shift);
			for (int i = 0; i < 28; i++)
			{
				seed = seed * 1103515245 + 12345;
				nib[b][i] = (uint8_t)((seed >> 16) & 0x0F);
			}
		}
		/*	the same group in all 18 slots - the SPU chain below then runs
			18x8 blocks, so cross-group history continuity is covered too  */
		for (int grp = 0; grp < 18; grp++)
			buildGroup(user + grp * 128, params, nib);

		h1 = h2 = 0;
		XaAdpcm_DecodeSector4bitMono(user, out, &h1, &h2);

		int16_t spuOut[28];
		int16_t sh1 = 0, sh2 = 0;
		bool ok = true;
		for (int blk = 0; blk < 18 * 8 && ok; blk++)
		{
			int b = blk % 8;
			uint8_t block[16];
			block[0] = params[b];				/* same (filter<<4)|shift byte */
			block[1] = 0;
			memset(block + 2, 0, 14);
			for (int i = 0; i < 28; i++)
				block[2 + (i >> 1)] |= (uint8_t)(nib[b][i] << ((i & 1) * 4));
			SpuAdpcm_DecodeBlock(block, spuOut, &sh1, &sh2);
			for (int i = 0; i < 28; i++)
				if (out[blk * 28 + i] != spuOut[i])
				{
					std::printf("  block %d sample %d: xa %d spu %d\n",
								blk, i, out[blk * 28 + i], spuOut[i]);
					ok = false;
					break;
				}
		}
		check(ok, "XA layout matches the SPU decoder on the same nibble stream");
		check(h1 == sh1 && h2 == sh2, "history matches the SPU decoder");
	}

	/*	-------- 3: real Track1.Ixa sectors vs the ffmpeg golden  */
	FILE *fs = fopen("port/tests/xa_fixture_sectors.bin", "rb");
	FILE *fg = fopen("port/tests/xa_fixture_golden.pcm", "rb");
	if (!fs || !fg)
	{
		std::printf("xa_test: fixtures not found (run from the repo root) - "
					"real-data golden SKIPPED\n");
		if (fs) fclose(fs);
		if (fg) fclose(fg);
	}
	else
	{
		static uint8_t	sector[2336];
		static int16_t	golden[4 * XA_SECTOR_SAMPLES];
		size_t gn = fread(golden, 2, 4 * XA_SECTOR_SAMPLES, fg);
		fclose(fg);
		check(gn == 4 * XA_SECTOR_SAMPLES, "golden pcm is 4 sectors long");

		h1 = h2 = 0;
		int mismatches = 0, maxDiff = 0, firstBad = -1;
		for (int s = 0; s < 4; s++)
		{
			check(fread(sector, 1, 2336, fs) == 2336, "fixture sector read");
			check(sector[2] & 0x04, "fixture sector is XA audio");
			check(sector[3] == 0x04, "fixture coding is mono 18.9kHz 4-bit");
			/*	raw sector = [8-byte subheader][2328 data]; user area first  */
			XaAdpcm_DecodeSector4bitMono(sector + 8, out, &h1, &h2);
			for (int i = 0; i < XA_SECTOR_SAMPLES; i++)
			{
				int d = out[i] - golden[s * XA_SECTOR_SAMPLES + i];
				if (d)
				{
					if (firstBad < 0)
						firstBad = s * XA_SECTOR_SAMPLES + i;
					mismatches++;
					if (d < 0) d = -d;
					if (d > maxDiff) maxDiff = d;
				}
			}
		}
		fclose(fs);
		if (mismatches)
			std::printf("  real-data: %d/%d samples differ from ffmpeg, "
						"max |diff| %d, first at %d\n",
						mismatches, 4 * XA_SECTOR_SAMPLES, maxDiff, firstBad);
		check(mismatches == 0, "real Track1.Ixa sectors decode bit-identical to ffmpeg");
	}

	if (g_failures)
	{
		std::printf("xa_test: %d failure(s)\n", g_failures);
		return 1;
	}
	std::printf("xa_test: all passed\n");
	return 0;
}
