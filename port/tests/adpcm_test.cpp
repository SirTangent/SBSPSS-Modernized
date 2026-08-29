/*	Unit tests for the SPU ADPCM block decoder (port/psyq/spu/spu_adpcm.cpp).
	Golden vectors hand-computed from the psx-spx decode algorithm: nibble
	order, shift, every filter, +32 arithmetic-shift rounding, s16 clamping
	with the clamped value entering the history, the shift 13..15 == 9 rule,
	and history continuity across consecutive blocks.
*/
#include <cstdio>
#include <cstring>

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

static void checkSamples(const int16_t *got, const int16_t *want, int n,
						 const char *what)
{
	for (int i = 0; i < n; i++)
	{
		if (got[i] != want[i])
		{
			std::printf("FAIL: %s - sample %d = %d, want %d\n",
						what, i, got[i], want[i]);
			g_failures++;
			return;
		}
	}
}

/*	build a block: shift/filter byte, flag byte, 14 data bytes (may be short -
	rest zero-filled)  */
static void makeBlock(uint8_t *b, int filter, int shift, int flags,
					  const uint8_t *data, int nData)
{
	memset(b, 0, 16);
	b[0] = (uint8_t)((filter << 4) | shift);
	b[1] = (uint8_t)flags;
	for (int i = 0; i < nData; i++)
		b[2 + i] = data[i];
}

int main()
{
	/* --- nibble order + shift 0, filter 0: raw nibble<<12 ----------------- */
	{
		/*	low nibble decodes first: 0x21 -> +1 then +2; 0x87 -> +7 then -8  */
		static const uint8_t data[] = { 0x21, 0x43, 0x65, 0x87,
										0xA9, 0xCB, 0xED, 0x0F };
		uint8_t blk[16];
		makeBlock(blk, 0, 0, 0, data, sizeof(data));
		int16_t out[28], h1 = 0, h2 = 0;
		SpuAdpcm_DecodeBlock(blk, out, &h1, &h2);
		static const int16_t want[16] = {
			4096, 8192, 12288, 16384, 20480, 24576, 28672, -32768,
			-28672, -24576, -20480, -16384, -12288, -8192, -4096, 0,
		};
		checkSamples(out, want, 16, "filter 0 shift 0: nibble<<12, low first");
		for (int i = 16; i < 28; i++)
			check(out[i] == 0, "filter 0 shift 0: zero tail");
		check(h1 == 0 && h2 == 0, "history holds last two (zero-tail) outputs");
	}

	/* --- shift 12, filter 0: bare signed nibbles -------------------------- */
	{
		static const uint8_t data[] = { 0x21, 0x43, 0x65, 0x87,
										0xA9, 0xCB, 0xED, 0x0F };
		uint8_t blk[16];
		makeBlock(blk, 0, 12, 0, data, sizeof(data));
		int16_t out[28], h1 = 0, h2 = 0;
		SpuAdpcm_DecodeBlock(blk, out, &h1, &h2);
		static const int16_t want[16] = {
			1, 2, 3, 4, 5, 6, 7, -8, -7, -6, -5, -4, -3, -2, -1, 0,
		};
		checkSamples(out, want, 16, "filter 0 shift 12: signed nibbles");
	}

	/* --- filter 1 impulse response: s += (h1*60 + 32) >> 6 ---------------- */
	{
		static const uint8_t data[] = { 0x01 };		/* one +1 nibble, rest 0 */
		uint8_t blk[16];
		makeBlock(blk, 1, 6, 0, data, sizeof(data));
		int16_t out[28], h1 = 0, h2 = 0;
		SpuAdpcm_DecodeBlock(blk, out, &h1, &h2);
		/*	s0 = 1<<12>>6 = 64; then s = (prev*60+32)>>6: 3872>>6=60,
			3632>>6=56, 3392>>6=53, 3212>>6=50, 3032>>6=47, 2852>>6=44,
			2672>>6=41  */
		static const int16_t want[8] = { 64, 60, 56, 53, 50, 47, 44, 41 };
		checkSamples(out, want, 8, "filter 1 impulse decay");
	}

	/* --- filter 2 impulse response: (h1*115 - h2*52 + 32) >> 6 ------------ */
	{
		static const uint8_t data[] = { 0x01 };
		uint8_t blk[16];
		makeBlock(blk, 2, 12, 0, data, sizeof(data));
		int16_t out[28], h1 = 0, h2 = 0;
		SpuAdpcm_DecodeBlock(blk, out, &h1, &h2);
		/*	s0=1; 147>>6=2; (230-52+32)>>6=3; (345-104+32)>>6=4;
			(460-156+32)>>6=5; (575-208+32)>>6=6; (690-260+32)>>6=7;
			(805-312+32)>>6=8  */
		static const int16_t want[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
		checkSamples(out, want, 8, "filter 2 impulse rise");
	}

	/* --- positive clamp, clamped value enters history --------------------- */
	{
		uint8_t data[14];
		memset(data, 0x77, sizeof(data));			/* all nibbles +7 */
		uint8_t blk[16];
		makeBlock(blk, 1, 0, 0, data, sizeof(data));
		int16_t out[28], h1 = 0, h2 = 0;
		SpuAdpcm_DecodeBlock(blk, out, &h1, &h2);
		/*	s0 = 28672; s1 = 28672 + (28672*60+32)>>6 = 28672+26880 -> clamp;
			s2 already uses the CLAMPED 32767 in the filter and clamps again  */
		check(out[0] == 28672, "clamp: first sample below the rail");
		for (int i = 1; i < 28; i++)
			check(out[i] == 32767, "clamp: pinned at +32767");
		check(h1 == 32767 && h2 == 32767, "clamp: history holds clamped value");
	}

	/* --- negative clamp (arithmetic-shift rounding of a negative sum) ----- */
	{
		uint8_t data[14];
		memset(data, 0x88, sizeof(data));			/* all nibbles -8 */
		uint8_t blk[16];
		makeBlock(blk, 1, 0, 0, data, sizeof(data));
		int16_t out[28], h1 = 0, h2 = 0;
		SpuAdpcm_DecodeBlock(blk, out, &h1, &h2);
		check(out[0] == -32768, "negative clamp: exact rail on first sample");
		for (int i = 1; i < 28; i++)
			check(out[i] == -32768, "negative clamp: pinned at -32768");
	}

	/* --- shift 13..15 behave as shift 9 ----------------------------------- */
	{
		static const uint8_t data[] = { 0x93, 0x5C, 0x2F, 0x71, 0x84, 0x6B };
		for (int badShift = 13; badShift <= 15; badShift++)
		{
			uint8_t blkBad[16], blkRef[16];
			makeBlock(blkBad, 3, badShift, 0, data, sizeof(data));
			makeBlock(blkRef, 3, 9, 0, data, sizeof(data));
			int16_t outBad[28], outRef[28];
			int16_t h1 = 0, h2 = 0;
			SpuAdpcm_DecodeBlock(blkBad, outBad, &h1, &h2);
			h1 = h2 = 0;
			SpuAdpcm_DecodeBlock(blkRef, outRef, &h1, &h2);
			check(memcmp(outBad, outRef, sizeof(outBad)) == 0,
				  "shift 13..15 decodes exactly like shift 9");
		}
	}

	/* --- history continuity across blocks --------------------------------- */
	{
		static const uint8_t data[] = { 0x01 };
		uint8_t blkA[16], blkB[16];
		makeBlock(blkA, 2, 12, 0, data, sizeof(data));
		makeBlock(blkB, 2, 12, 0, 0, 0);			/* all-zero data */
		int16_t outA[28], outB[28];
		int16_t h1 = 0, h2 = 0;
		SpuAdpcm_DecodeBlock(blkA, outA, &h1, &h2);
		check(h1 == outA[27] && h2 == outA[26],
			  "history equals the last two block outputs");
		SpuAdpcm_DecodeBlock(blkB, outB, &h1, &h2);
		/*	first sample of block B continues the same filter recurrence
			from block A's last two outputs (the one place the test applies
			the spec formula itself)  */
		int expect = (outA[27] * 115 + outA[26] * -52 + 32) >> 6;
		if (expect > 32767) expect = 32767;
		if (expect < -32768) expect = -32768;
		check(outB[0] == (int16_t)expect,
			  "block B continues block A's filter state");

		/*	fresh history must NOT reproduce the continuation  */
		int16_t f1 = 0, f2 = 0;
		int16_t outFresh[28];
		SpuAdpcm_DecodeBlock(blkB, outFresh, &f1, &f2);
		check(memcmp(outFresh, outB, sizeof(outB)) != 0,
			  "zero-history decode differs from continued decode");
	}

	/* --- flag byte is carried verbatim in byte 1 --------------------------- */
	{
		uint8_t blk[16];
		makeBlock(blk, 0, 0, SPU_ADPCM_LOOP_START | SPU_ADPCM_LOOP_END, 0, 0);
		check(blk[1] == 5, "flag byte layout: START|END == 5");
		check(SPU_ADPCM_LOOP_END == 1 && SPU_ADPCM_LOOP_REPEAT == 2 &&
			  SPU_ADPCM_LOOP_START == 4, "flag bit values");
	}

	if (g_failures)
	{
		std::printf("adpcm test FAILED (%d)\n", g_failures);
		return 1;
	}
	std::printf("adpcm test PASSED\n");
	return 0;
}
