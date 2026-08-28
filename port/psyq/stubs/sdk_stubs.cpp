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

/*	fmv.cpp strNext() treats 0 as "sector ready" and dereferences the
	never-written out-param; nonzero drains its timeout and playFMV exits
	cleanly  */
long StGetNext() { PSYQ_STUB_ONCE(); return 1; }

/*	Real prototype (tools/vlc/include/VLC_BIT.H): the caller expects its
	table filled through the pointer.  M7 must implement Build3 and vlc3
	TOGETHER - a real DecDCTvlc3 over this no-op walks an unbuilt table.  */
void DecDCTvlcBuild3(unsigned short *table)
{
	(void)table;
	PSYQ_STUB_ONCE();
}

/*	CdRead2 must report "streaming started" (nonzero): fmv.cpp's strKickCD
	spins `while(CdRead2(...)==0)` with no pump, so a 0 here hard-hangs the
	THQ FMV scene right after the Nick logo.  With 1, StGetNext's bounded
	no-data countdown makes strNextVlc return -1 and FMV_play falls through
	its whole decode loop - both FMV scenes skip cleanly to the main titles.
	Real streaming lands in M7.  */
long CdRead2() { PSYQ_STUB_ONCE(); return 1; }
long DecDCTReset() { PSYQ_STUB_ONCE(); return 0; }
long DecDCTin() { PSYQ_STUB_ONCE(); return 0; }
long DecDCTout() { PSYQ_STUB_ONCE(); return 0; }
long DecDCToutCallback() { PSYQ_STUB_ONCE(); return 0; }
long DecDCTvlc3() { PSYQ_STUB_ONCE(); return 0; }
long DecDCTvlcSize3() { PSYQ_STUB_ONCE(); return 0; }
long InitXMData() { PSYQ_STUB_ONCE(); return 0; }
long StCdInterrupt() { PSYQ_STUB_ONCE(); return 0; }
long StFreeRing() { PSYQ_STUB_ONCE(); return 0; }
long StSetRing() { PSYQ_STUB_ONCE(); return 0; }
long StSetStream() { PSYQ_STUB_ONCE(); return 0; }
long StUnSetRing() { PSYQ_STUB_ONCE(); return 0; }
long XM_ClearSFXRange() { PSYQ_STUB_ONCE(); return 0; }
long XM_CloseVAB() { PSYQ_STUB_ONCE(); return 0; }
/*	Nonzero = "this song/SFX has finished" (xmplay.cpp:208-215): the caller
	marks the channel SILENT and XM_Quits it, freeing it for reuse.  The old
	0 return meant "still playing" forever - channels leaked one one-shot
	SFX at a time until playSfx() permanently returned NOT_PLAYING (~10
	plays in).  With no audio until M5, everything finishing instantly is
	the correct degenerate behaviour.  */
long XM_GetFeedback() { PSYQ_STUB_ONCE(); return 1; }
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
