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

/*	blocking GPU sync joins the cooperative pump like VSync/CdReadSync */
long DrawSync(long mode)
{
	(void)mode;
	Port_Pump();
	return 0;
}

/*	The M7 FMV surface is real now: the St functions, CdRead2 and
	StCdIntrFlag live in psyq/cd/str_stream.cpp, DecDCT* in psyq/mdec/.  */

}
