/*	libspu over the software SPU core (M5).

	The public PSY-Q surface: init, the synchronous transfer set (a memcpy
	into the 512KB model - SpuIsTransferCompleted is truthfully 1), the
	SpuInitMalloc/SpuMalloc/SpuFree allocator (first-fit + coalescing,
	records kept in the caller-supplied table: SPU_MALLOC_RECSIZ bytes each,
	address-sorted, addr bit 31 = free - the SDK documents the table as an
	opaque work area), voice attributes/keys over the mixer, common/CD
	volumes, and the state-recording reverb family (the game only ever runs
	SPU_REV_MODE_OFF - see port/docs).

	Everything that touches voices, RAM, or the master/CD volumes locks;
	callers of this API never hold the SPU lock themselves.
*/
#include <stdio.h>
#include <string.h>

#include <libspu.h>

#include "spu/spu_core.h"

namespace
{

/*	first byte the allocator may hand out - below this live the hardware's
	CD/voice capture areas and the customary silent block  */
const uint32_t kAllocBase = 0x1010;

uint32_t g_transferAddr;
long g_transferMode = SPU_TRANSFER_BY_DMA;
SpuTransferCallbackProc g_transferCallback;

/*	allocator records, living in the game-owned SpuInitMalloc table  */
struct MallocRec
{
	uint32_t	addr;		/* bit 31 set = free block */
	uint32_t	size;
};
const uint32_t kFreeBit = 0x80000000u;

MallocRec *g_mallocTable;
long g_mallocMax;
long g_mallocCount;

/* reverb + env state: recorded, never rendered (deferred - game runs OFF) */
long g_reverbOn;
long g_reverbMode;
unsigned long g_reverbVoiceBits;
short g_reverbDepthL, g_reverbDepthR;
long g_reverbDelay, g_reverbFeedback;
SpuEnv g_env;

void logOnce(int *flag, const char *msg)
{
	if (!*flag)
	{
		*flag = 1;
		fprintf(stderr, "[spu] %s\n", msg);
	}
}

}	/* namespace */

extern "C" {

void SpuInit(void)
{
	Spu_Lock();
	memset(g_spuVoice, 0, sizeof(SpuVoiceState) * SPU_NVOICES);
	g_spuMasterVolL = g_spuMasterVolR = 0;
	g_spuCdVolL = g_spuCdVolR = 0;
	g_spuCdMixOn = 0;
	Spu_Unlock();
	g_transferAddr = 0;
	g_transferMode = SPU_TRANSFER_BY_DMA;
	g_transferCallback = 0;
	g_mallocTable = 0;
	g_mallocMax = g_mallocCount = 0;
	g_reverbOn = SPU_OFF;
	g_reverbMode = SPU_REV_MODE_OFF;
	g_reverbVoiceBits = 0;
}

/* ---- transfer set ------------------------------------------------------- */

long SpuSetTransferMode(long mode)
{
	g_transferMode = mode;
	return mode;
}

unsigned long SpuSetTransferStartAddr(unsigned long addr)
{
	if (addr >= SPU_RAM_SIZE)
		return 0;
	g_transferAddr = (uint32_t)addr & ~7u;	/* hardware works in 8-byte units */
	return g_transferAddr;
}

unsigned long SpuWrite(unsigned char *addr, unsigned long size)
{
	static int clampWarned;
	if (g_transferAddr + size > SPU_RAM_SIZE)
	{
		logOnce(&clampWarned, "SpuWrite clamped at end of sound RAM");
		size = SPU_RAM_SIZE - g_transferAddr;
	}
	Spu_Lock();
	memcpy(&g_spuRam[g_transferAddr], addr, size);
	Spu_Unlock();
	g_transferAddr += (uint32_t)size;
	if (g_transferCallback)
		g_transferCallback();
	return size;
}

unsigned long SpuWrite0(unsigned long size)
{
	static int clampWarned;
	if (g_transferAddr + size > SPU_RAM_SIZE)
	{
		logOnce(&clampWarned, "SpuWrite0 clamped at end of sound RAM");
		size = SPU_RAM_SIZE - g_transferAddr;
	}
	Spu_Lock();
	memset(&g_spuRam[g_transferAddr], 0, size);
	Spu_Unlock();
	g_transferAddr += (uint32_t)size;
	if (g_transferCallback)
		g_transferCallback();
	return size;
}

long SpuIsTransferCompleted(long flag)
{
	(void)flag;
	return 1;					/* transfers are synchronous memcpys */
}

SpuTransferCallbackProc SpuSetTransferCallback(SpuTransferCallbackProc func)
{
	SpuTransferCallbackProc old = g_transferCallback;
	g_transferCallback = func;
	return old;
}

/* ---- SPU RAM allocator -------------------------------------------------- */

long SpuInitMalloc(long num, char *top)
{
	if (num <= 0 || !top)
		return 0;
	g_mallocTable = (MallocRec *)top;
	g_mallocMax = num;
	g_mallocCount = 1;
	g_mallocTable[0].addr = kAllocBase | kFreeBit;
	g_mallocTable[0].size = SPU_RAM_SIZE - kAllocBase;
	return num;
}

long SpuMalloc(long size)
{
	static int noTable, exhausted;
	if (!g_mallocTable)
	{
		logOnce(&noTable, "SpuMalloc before SpuInitMalloc");
		return -1;
	}
	uint32_t need = ((uint32_t)size + 15u) & ~15u;	/* ADPCM block granularity */
	if (need == 0)
		need = 16;
	for (long i = 0; i < g_mallocCount; i++)
	{
		MallocRec &r = g_mallocTable[i];
		if (!(r.addr & kFreeBit) || r.size < need)
			continue;
		uint32_t addr = r.addr & ~kFreeBit;
		if (r.size == need)
		{
			r.addr = addr;		/* whole block used */
			return (long)addr;
		}
		/* split: insert the used part before the shrunk free remainder */
		if (g_mallocCount >= g_mallocMax)
		{
			logOnce(&exhausted, "SpuMalloc management table full");
			return -1;
		}
		memmove(&g_mallocTable[i + 1], &g_mallocTable[i],
				(size_t)(g_mallocCount - i) * sizeof(MallocRec));
		g_mallocCount++;
		g_mallocTable[i].addr = addr;
		g_mallocTable[i].size = need;
		g_mallocTable[i + 1].addr = (addr + need) | kFreeBit;
		g_mallocTable[i + 1].size -= need;
		return (long)addr;
	}
	logOnce(&exhausted, "SpuMalloc: out of sound RAM");
	return -1;
}

void SpuFree(unsigned long addr)
{
	static int badFree;
	for (long i = 0; i < g_mallocCount; i++)
	{
		MallocRec &r = g_mallocTable[i];
		if (r.addr != (uint32_t)addr)
			continue;
		r.addr |= kFreeBit;
		/* coalesce with the next record, then with the previous one */
		if (i + 1 < g_mallocCount && (g_mallocTable[i + 1].addr & kFreeBit))
		{
			r.size += g_mallocTable[i + 1].size;
			memmove(&g_mallocTable[i + 1], &g_mallocTable[i + 2],
					(size_t)(g_mallocCount - i - 2) * sizeof(MallocRec));
			g_mallocCount--;
		}
		if (i > 0 && (g_mallocTable[i - 1].addr & kFreeBit))
		{
			g_mallocTable[i - 1].size += g_mallocTable[i].size;
			memmove(&g_mallocTable[i], &g_mallocTable[i + 1],
					(size_t)(g_mallocCount - i - 1) * sizeof(MallocRec));
			g_mallocCount--;
		}
		return;
	}
	logOnce(&badFree, "SpuFree of an address that was never allocated");
}

/* ---- voices ------------------------------------------------------------- */

void SpuSetVoiceAttr(SpuVoiceAttr *arg)
{
	static int maskWarned;
	const unsigned long handled =
		SPU_VOICE_VOLL | SPU_VOICE_VOLR | SPU_VOICE_PITCH | SPU_VOICE_WDSA |
		SPU_VOICE_LSAX | SPU_VOICE_ADSR_ADSR1 | SPU_VOICE_ADSR_ADSR2;
	if (arg->mask & ~handled)
		logOnce(&maskWarned, "SpuSetVoiceAttr: unimplemented mask bits ignored"
							 " (NOTE/volmode/individual-rate family)");

	Spu_Lock();
	for (int i = 0; i < SPU_NVOICES; i++)
	{
		if (!(arg->voice & SPU_VOICECH(i)))
			continue;
		SpuVoiceState &v = g_spuVoice[i];
		if (arg->mask & SPU_VOICE_VOLL)
			v.volL = arg->volume.left;
		if (arg->mask & SPU_VOICE_VOLR)
			v.volR = arg->volume.right;
		if (arg->mask & SPU_VOICE_PITCH)
			v.pitch = arg->pitch;
		if (arg->mask & SPU_VOICE_WDSA)
			v.startAddr = arg->addr & ~15u;
		if (arg->mask & SPU_VOICE_LSAX)
			v.repeatAddr = arg->loop_addr & ~15u;
		if (arg->mask & SPU_VOICE_ADSR_ADSR1)
			v.adsr1 = arg->adsr1;
		if (arg->mask & SPU_VOICE_ADSR_ADSR2)
			v.adsr2 = arg->adsr2;
	}
	Spu_Unlock();
}

void SpuSetVoiceVolume(int vNum, short volL, short volR)
{
	static int badVoice;
	if (vNum < 0 || vNum >= SPU_NVOICES)
	{
		logOnce(&badVoice, "SpuSetVoiceVolume/Pitch: voice out of range");
		return;
	}
	Spu_Lock();
	g_spuVoice[vNum].volL = volL;
	g_spuVoice[vNum].volR = volR;
	Spu_Unlock();
}

void SpuSetVoicePitch(int vNum, unsigned short pitch)
{
	static int badVoice;
	if (vNum < 0 || vNum >= SPU_NVOICES)
	{
		logOnce(&badVoice, "SpuSetVoiceVolume/Pitch: voice out of range");
		return;
	}
	Spu_Lock();
	g_spuVoice[vNum].pitch = pitch;
	Spu_Unlock();
}

void SpuSetKey(long on_off, unsigned long voice_bit)
{
	Spu_Lock();
	for (int i = 0; i < SPU_NVOICES; i++)
	{
		if (!(voice_bit & SPU_VOICECH(i)))
			continue;
		if (on_off == SPU_ON)
			Spu_KeyOn(i);
		else
			Spu_KeyOff(i);
	}
	Spu_Unlock();
}

void SpuGetAllKeysStatus(char *status)
{
	Spu_Lock();
	for (int i = 0; i < SPU_NVOICES; i++)
	{
		const SpuVoiceState &v = g_spuVoice[i];
		if (v.envPhase == SPU_ENV_OFF)
			status[i] = SPU_OFF;
		else if (v.envPhase == SPU_ENV_RELEASE)
			status[i] = SPU_OFF_ENV_ON;
		else
			status[i] = SPU_ON;
	}
	Spu_Unlock();
}

/* ---- common attributes -------------------------------------------------- */

void SpuSetCommonMasterVolume(short mvol_left, short mvol_right)
{
	Spu_Lock();
	g_spuMasterVolL = mvol_left;
	g_spuMasterVolR = mvol_right;
	Spu_Unlock();
}

void SpuSetCommonCDVolume(short cd_left, short cd_right)
{
	Spu_Lock();
	g_spuCdVolL = cd_left;
	g_spuCdVolR = cd_right;
	Spu_Unlock();
}

void SpuSetCommonCDMix(long cd_mix)
{
	Spu_Lock();
	g_spuCdMixOn = (cd_mix == SPU_ON);
	Spu_Unlock();
}

void SpuSetCommonAttr(SpuCommonAttr *attr)
{
	Spu_Lock();
	if (attr->mask & SPU_COMMON_MVOLL)
		g_spuMasterVolL = attr->mvol.left;
	if (attr->mask & SPU_COMMON_MVOLR)
		g_spuMasterVolR = attr->mvol.right;
	if (attr->mask & SPU_COMMON_CDVOLL)
		g_spuCdVolL = attr->cd.volume.left;
	if (attr->mask & SPU_COMMON_CDVOLR)
		g_spuCdVolR = attr->cd.volume.right;
	if (attr->mask & SPU_COMMON_CDMIX)
		g_spuCdMixOn = (attr->cd.mix == SPU_ON);
	Spu_Unlock();
	/* reverb/ext routing bits: recorded nowhere - nothing consumes them */
}

void SpuSetEnv(SpuEnv *env)
{
	g_env = *env;
}

/* ---- reverb: state-recording no-ops (deferred - game only runs OFF) ----- */

long SpuSetReverb(long on_off)
{
	if (on_off == SPU_ON || on_off == SPU_OFF)
		g_reverbOn = on_off;
	return g_reverbOn;
}

long SpuSetReverbModeType(long mode)
{
	if (mode == SPU_REV_MODE_CHECK)
		return g_reverbMode;
	long m = mode & ~SPU_REV_MODE_CLEAR_WA;
	if (m < 0 || m >= SPU_REV_MODE_MAX)
		return SPU_INVALID_ARGS;
	g_reverbMode = m;
	return SPU_SUCCESS;
}

long SpuReserveReverbWorkArea(long on_off)
{
	/*	the real call fences the top of sound RAM off from SpuMalloc; with
		reverb deferred nothing renders from that area, so the model keeps
		the full 512KB and just reports the flag  */
	(void)on_off;
	return g_reverbOn;
}

unsigned long SpuSetReverbVoice(long on_off, unsigned long voice_bit)
{
	if (on_off == SPU_BIT)
		g_reverbVoiceBits = voice_bit;
	else if (on_off == SPU_ON)
		g_reverbVoiceBits |= voice_bit;
	else if (on_off == SPU_OFF)
		g_reverbVoiceBits &= ~voice_bit;
	return g_reverbVoiceBits;
}

void SpuSetReverbModeDepth(short depth_left, short depth_right)
{
	g_reverbDepthL = depth_left;
	g_reverbDepthR = depth_right;
}

void SpuSetReverbModeDelayTime(long delay)
{
	g_reverbDelay = delay;
}

void SpuSetReverbModeFeedback(long feedback)
{
	g_reverbFeedback = feedback;
}

}	/* extern "C" */
