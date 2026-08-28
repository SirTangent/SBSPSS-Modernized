/*	Unit tests for the SPU voice mixer (port/psyq/spu/spu_core.cpp).
	Property checks over a synthetic looping square-wave sample: silence
	before key-on, pan sidedness, pitch halving doubles the waveform period,
	ADSR release decays to silence and frees the voice, one-shot samples
	(LOOP_END without REPEAT) self-mute, master volume scales the output,
	and rendering is deterministic for identical programmed state.
*/
#include <cstdio>
#include <cstring>
#include <cstdlib>

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

/*	±28672 square wave, period 8 samples, as two looping ADPCM blocks
	(filter 0, shift 0: nibble 0x7 = +28672, 0x9 = -28672)  */
static void writeSquareSample(uint32_t addr)
{
	static const uint8_t b0Data[14] = { 0x77, 0x77, 0x99, 0x99, 0x77, 0x77,
										0x99, 0x99, 0x77, 0x77, 0x99, 0x99,
										0x77, 0x77 };
	static const uint8_t b1Data[14] = { 0x99, 0x99, 0x77, 0x77, 0x99, 0x99,
										0x77, 0x77, 0x99, 0x99, 0x77, 0x77,
										0x99, 0x99 };
	uint8_t *p = &g_spuRam[addr];
	p[0] = 0x00;
	p[1] = SPU_ADPCM_LOOP_START;
	memcpy(p + 2, b0Data, 14);
	p[16] = 0x00;
	p[17] = SPU_ADPCM_LOOP_END | SPU_ADPCM_LOOP_REPEAT;
	memcpy(p + 18, b1Data, 14);
}

/*	single one-shot block: 28 samples of +28672, END without REPEAT  */
static void writeOneShotSample(uint32_t addr)
{
	uint8_t *p = &g_spuRam[addr];
	p[0] = 0x00;
	p[1] = SPU_ADPCM_LOOP_END;
	memset(p + 2, 0x77, 14);
}

static void resetSpu(void)
{
	memset(g_spuRam, 0, SPU_RAM_SIZE);
	memset(g_spuVoice, 0, sizeof(SpuVoiceState) * SPU_NVOICES);
	g_spuMasterVolL = 0x3FFF;
	g_spuMasterVolR = 0x3FFF;
}

/*	program + key a voice with an effectively instant envelope
	(attack shift 0 step +7, sustain level max, sustain pinned)  */
static void keyVoice(int i, uint32_t addr, int pitch, int volL, int volR)
{
	SpuVoiceState &v = g_spuVoice[i];
	memset(&v, 0, sizeof(v));
	v.startAddr = addr;
	v.repeatAddr = addr;
	v.pitch = (uint16_t)pitch;
	v.volL = (int16_t)volL;
	v.volR = (int16_t)volR;
	v.adsr1 = 0x000F;
	v.adsr2 = 0x0000;
	Spu_KeyOn(i);
}

static int countTransitions(const int16_t *stereo, int nFrames, int ch)
{
	int n = 0;
	int prev = 0;
	for (int i = 0; i < nFrames; i++)
	{
		int s = stereo[i * 2 + ch];
		if (s == 0)
			continue;
		if (prev != 0 && ((s > 0) != (prev > 0)))
			n++;
		prev = s;
	}
	return n;
}

static int peakAbs(const int16_t *stereo, int nFrames, int ch)
{
	int peak = 0;
	for (int i = 0; i < nFrames; i++)
	{
		int a = std::abs((int)stereo[i * 2 + ch]);
		if (a > peak)
			peak = a;
	}
	return peak;
}

int main()
{
	static int16_t buf[8192 * 2];
	static int16_t buf2[8192 * 2];

	/* --- all voices off: silence ------------------------------------------ */
	{
		resetSpu();
		memset(buf, 0x5A, sizeof(buf));
		Spu_RenderFrames(buf, 256);
		bool silent = true;
		for (int i = 0; i < 256 * 2; i++)
			silent = silent && buf[i] == 0;
		check(silent, "no keyed voices -> silence");
	}

	/* --- keyed voice: sound on the panned side only ----------------------- */
	{
		resetSpu();
		writeSquareSample(0x1000);
		keyVoice(0, 0x1000, 0x1000, 0x3FFF, 0);
		Spu_RenderFrames(buf, 1024);
		check(peakAbs(buf, 1024, 0) > 20000,
			  "hard-left voice: strong left signal");
		check(peakAbs(buf, 1024, 1) == 0, "hard-left voice: right silent");
		check(g_spuVoice[0].envPhase == SPU_ENV_SUSTAIN,
			  "instant envelope reached sustain");
	}

	/* --- pitch halving doubles the period --------------------------------- */
	{
		resetSpu();
		writeSquareSample(0x1000);
		keyVoice(0, 0x1000, 0x1000, 0x3FFF, 0x3FFF);
		Spu_RenderFrames(buf, 64);						/* warm-up */
		Spu_RenderFrames(buf, 4096);
		int tFull = countTransitions(buf, 4096, 0);

		keyVoice(0, 0x1000, 0x0800, 0x3FFF, 0x3FFF);
		Spu_RenderFrames(buf, 64);
		Spu_RenderFrames(buf, 4096);
		int tHalf = countTransitions(buf, 4096, 0);

		/*	period 8 at 0x1000 -> ~1024 transitions in 4096 frames  */
		check(tFull > 920 && tFull < 1130,
			  "pitch 0x1000: ~2 transitions per 8 samples");
		check(tHalf > 460 && tHalf < 565, "pitch 0x0800: half the transitions");
		if (!(tFull > 920 && tFull < 1130 && tHalf > 460 && tHalf < 565))
			std::printf("  (transitions: full=%d half=%d)\n", tFull, tHalf);
	}

	/* --- key-off: release ramps to silence and frees the voice ------------ */
	{
		resetSpu();
		writeSquareSample(0x1000);
		keyVoice(0, 0x1000, 0x1000, 0x3FFF, 0x3FFF);
		Spu_RenderFrames(buf, 256);
		g_spuVoice[0].adsr2 = 0x0005;	/* linear release, shift 5: -512/sample */
		Spu_KeyOff(0);
		Spu_RenderFrames(buf, 256);
		check(peakAbs(buf, 32, 0) > 0, "release: audible tail after key-off");
		bool tailSilent = true;
		for (int i = 128; i < 256; i++)
			tailSilent = tailSilent && buf[i * 2] == 0 && buf[i * 2 + 1] == 0;
		check(tailSilent, "release: silent within ~64 samples at shift 5");
		check(g_spuVoice[0].envPhase == SPU_ENV_OFF,
			  "release: voice freed at level 0");
	}

	/* --- one-shot sample (END without REPEAT) self-mutes ------------------ */
	{
		resetSpu();
		writeOneShotSample(0x2000);
		keyVoice(0, 0x2000, 0x1000, 0x3FFF, 0x3FFF);
		Spu_RenderFrames(buf, 128);
		check(peakAbs(buf, 32, 0) > 0, "one-shot: audible while block plays");
		bool tailSilent = true;
		for (int i = 40; i < 128; i++)
			tailSilent = tailSilent && buf[i * 2] == 0;
		check(tailSilent, "one-shot: silent after the 28-sample block");
		check(g_spuVoice[0].envPhase == SPU_ENV_OFF, "one-shot: voice freed");
		check(g_spuVoice[0].endx == 1, "one-shot: ENDX latched");
	}

	/* --- master volume scales the mix ------------------------------------- */
	{
		resetSpu();
		writeSquareSample(0x1000);
		keyVoice(0, 0x1000, 0x1000, 0x3FFF, 0x3FFF);
		Spu_RenderFrames(buf, 512);
		int peakFull = peakAbs(buf, 512, 0);

		keyVoice(0, 0x1000, 0x1000, 0x3FFF, 0x3FFF);
		g_spuMasterVolL = g_spuMasterVolR = 0x2000;
		Spu_RenderFrames(buf, 512);
		int peakHalf = peakAbs(buf, 512, 0);
		check(peakHalf > peakFull * 2 / 5 && peakHalf < peakFull * 3 / 5,
			  "master volume 0x2000 halves the peak");
	}

	/* --- determinism: identical state renders identical output ------------ */
	{
		resetSpu();
		writeSquareSample(0x1000);
		keyVoice(0, 0x1000, 0x0C00, 0x3FFF, 0x2000);
		keyVoice(1, 0x1000, 0x1000, 0x1000, 0x3FFF);
		Spu_RenderFrames(buf, 2048);

		keyVoice(0, 0x1000, 0x0C00, 0x3FFF, 0x2000);
		keyVoice(1, 0x1000, 0x1000, 0x1000, 0x3FFF);
		Spu_RenderFrames(buf2, 2048);
		check(memcmp(buf, buf2, 2048 * 2 * sizeof(int16_t)) == 0,
			  "re-keyed identical state renders bit-identical audio");
	}

	if (g_failures)
	{
		std::printf("spu test FAILED (%d)\n", g_failures);
		return 1;
	}
	std::printf("spu test PASSED\n");
	return 0;
}
