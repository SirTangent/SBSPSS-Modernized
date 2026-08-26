/*	Link-demand-driven stubs for the PSY-Q surface M1 does not implement
	(libgpu, libgte matrix path, libspu, libpad, libmcrd, libpress/St*,
	libsnd, XMPlayer).  Generated from the undefined-symbol list of the
	whole-archive headless link; each logs its first call and returns 0.

	Declared as no-prototype C functions on purpose: i686 cdecl is caller-
	cleanup, so an arg-count mismatch cannot corrupt the stack, and the
	real signatures land with the real implementations (M2-M7).
*/
#include "stub_log.h"
#include "host/pump.h"

extern "C" {

/*	fmv.cpp polls this streaming flag (libpress/libcd CD-interrupt path) */
long StCdIntrFlag;

/*	blocking GPU sync joins the cooperative pump like VSync/CdReadSync */
long DrawSync(long mode)
{
	(void)mode;
	Port_Pump();
	return 0;
}

/*	Stubs whose RETURN VALUE is part of a caller protocol - each of these is
	polled in a loop or gates a state machine, so the generated return-0
	default would hang or crash the caller (found by /code-review on M1):
*/

/*	spu.cpp:73 spins while(!SpuIsTransferCompleted(...)) - 1 == complete  */
long SpuIsTransferCompleted() { PSYQ_STUB_ONCE(); return 1; }

/*	fmv.cpp strNext() treats 0 as "sector ready" and dereferences the
	never-written out-param; nonzero drains its timeout and playFMV exits
	cleanly  */
long StGetNext() { PSYQ_STUB_ONCE(); return 1; }

/*	memcard.cpp:1242 runs its completion handlers only when != 0
	(0 == still processing would poll forever); -1 == error resolves every
	operation to the failure path ("no card")  */
long MemCardSync() { PSYQ_STUB_ONCE(); return -1; }

/*	Real prototype (tools/vlc/include/VLC_BIT.H): the caller expects its
	table filled through the pointer.  M7 must implement Build3 and vlc3
	TOGETHER - a real DecDCTvlc3 over this no-op walks an unbuilt table.  */
void DecDCTvlcBuild3(unsigned short *table)
{
	(void)table;
	PSYQ_STUB_ONCE();
}

long CdRead2() { PSYQ_STUB_ONCE(); return 0; }
long DecDCTReset() { PSYQ_STUB_ONCE(); return 0; }
long DecDCTin() { PSYQ_STUB_ONCE(); return 0; }
long DecDCTout() { PSYQ_STUB_ONCE(); return 0; }
long DecDCToutCallback() { PSYQ_STUB_ONCE(); return 0; }
long DecDCTvlc3() { PSYQ_STUB_ONCE(); return 0; }
long DecDCTvlcSize3() { PSYQ_STUB_ONCE(); return 0; }
long InitXMData() { PSYQ_STUB_ONCE(); return 0; }
long MemCardAccept() { PSYQ_STUB_ONCE(); return 0; }
long MemCardInit() { PSYQ_STUB_ONCE(); return 0; }
long MemCardEnd() { PSYQ_STUB_ONCE(); return 0; }
long MemCardClose() { PSYQ_STUB_ONCE(); return 0; }
long MemCardCreateFile() { PSYQ_STUB_ONCE(); return 0; }
long MemCardDeleteFile() { PSYQ_STUB_ONCE(); return 0; }
long MemCardExist() { PSYQ_STUB_ONCE(); return 0; }
long MemCardFormat() { PSYQ_STUB_ONCE(); return 0; }
long MemCardGetDirentry() { PSYQ_STUB_ONCE(); return 0; }
long MemCardReadFile() { PSYQ_STUB_ONCE(); return 0; }
long MemCardStart() { PSYQ_STUB_ONCE(); return 0; }
long MemCardStop() { PSYQ_STUB_ONCE(); return 0; }
long MemCardUnformat() { PSYQ_STUB_ONCE(); return 0; }
long MemCardWriteFile() { PSYQ_STUB_ONCE(); return 0; }
long SpuInit() { PSYQ_STUB_ONCE(); return 0; }
long SpuInitMalloc() { PSYQ_STUB_ONCE(); return 0; }
long SpuReserveReverbWorkArea() { PSYQ_STUB_ONCE(); return 0; }
long SpuSetCommonAttr() { PSYQ_STUB_ONCE(); return 0; }
long SpuSetCommonCDMix() { PSYQ_STUB_ONCE(); return 0; }
long SpuSetCommonCDVolume() { PSYQ_STUB_ONCE(); return 0; }
long SpuSetCommonMasterVolume() { PSYQ_STUB_ONCE(); return 0; }
long SpuSetEnv() { PSYQ_STUB_ONCE(); return 0; }
long SpuSetReverb() { PSYQ_STUB_ONCE(); return 0; }
long SpuSetReverbModeDelayTime() { PSYQ_STUB_ONCE(); return 0; }
long SpuSetReverbModeDepth() { PSYQ_STUB_ONCE(); return 0; }
long SpuSetReverbModeFeedback() { PSYQ_STUB_ONCE(); return 0; }
long SpuSetReverbModeType() { PSYQ_STUB_ONCE(); return 0; }
long SpuSetReverbVoice() { PSYQ_STUB_ONCE(); return 0; }
long SpuSetTransferCallback() { PSYQ_STUB_ONCE(); return 0; }
long SpuSetTransferMode() { PSYQ_STUB_ONCE(); return 0; }
long SpuSetTransferStartAddr() { PSYQ_STUB_ONCE(); return 0; }
long SpuSetVoiceAttr() { PSYQ_STUB_ONCE(); return 0; }
long SpuWrite0() { PSYQ_STUB_ONCE(); return 0; }
long StCdInterrupt() { PSYQ_STUB_ONCE(); return 0; }
long StFreeRing() { PSYQ_STUB_ONCE(); return 0; }
long StSetRing() { PSYQ_STUB_ONCE(); return 0; }
long StSetStream() { PSYQ_STUB_ONCE(); return 0; }
long StUnSetRing() { PSYQ_STUB_ONCE(); return 0; }
long XM_ClearSFXRange() { PSYQ_STUB_ONCE(); return 0; }
long XM_CloseVAB() { PSYQ_STUB_ONCE(); return 0; }
long XM_GetFeedback() { PSYQ_STUB_ONCE(); return 0; }
long XM_GetFileHeaderSize() { PSYQ_STUB_ONCE(); return 0; }
long XM_GetSampleAddress() { PSYQ_STUB_ONCE(); return 0; }
long XM_GetSongSize() { PSYQ_STUB_ONCE(); return 0; }
long XM_Init() { PSYQ_STUB_ONCE(); return 0; }
long XM_OnceOffInit() { PSYQ_STUB_ONCE(); return 0; }
long XM_PlaySample() { PSYQ_STUB_ONCE(); return 0; }
long XM_PlayStop() { PSYQ_STUB_ONCE(); return 0; }
long XM_Quit() { PSYQ_STUB_ONCE(); return 0; }
long XM_SetFileHeaderAddress() { PSYQ_STUB_ONCE(); return 0; }
long XM_SetMasterPan() { PSYQ_STUB_ONCE(); return 0; }
long XM_SetMasterVol() { PSYQ_STUB_ONCE(); return 0; }
long XM_SetMono() { PSYQ_STUB_ONCE(); return 0; }
long XM_SetSongAddress() { PSYQ_STUB_ONCE(); return 0; }
long XM_SetStereo() { PSYQ_STUB_ONCE(); return 0; }
long XM_StopSample() { PSYQ_STUB_ONCE(); return 0; }
long XM_Update() { PSYQ_STUB_ONCE(); return 0; }
long XM_VABInit() { PSYQ_STUB_ONCE(); return 0; }

}
