/*	libetc stubs (VSync/VSyncCallback live in pump.cpp).  */
#include "stub_log.h"

extern "C" {

int  ResetCallback(void)			{ return 0; }
int  CheckCallback(void)			{ return 0; }
int  RestartCallback(void)			{ return 0; }
int  StopCallback(void)				{ return 0; }

long GetVideoMode(void)				{ return 0; }		/* MODE_NTSC */
long SetVideoMode(long mode)
{
	extern void Port_SetVBlankHz(int hz);
	Port_SetVBlankHz(mode == 1 ? 50 : 60);
	return mode;
}

void PadInit(int mode)				{ (void)mode; PSYQ_STUB_ONCE(); }
unsigned long PadRead(int id)		{ (void)id; return 0; }
void PadStop(void)					{ }

}
