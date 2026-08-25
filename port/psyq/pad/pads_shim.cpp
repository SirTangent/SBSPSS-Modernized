/*	libpad with real signatures (M2).  Behaviour is still "no controller
	connected" - identical to the M1 stubs, which is the proven-benign boot
	state (PadInitShock early-returns on PadData[].Active==0, and
	ReadController sees PadStateDiscon and stays inactive).  The SDL3
	gamepad/keyboard mapping lands in M3: it will fill the two 34-byte
	buffers captured here in PS1 packet layout (byte0=0 connected, byte1
	0x41 digital, bytes 2-3 ACTIVE-LOW button mask - 0xFF/0xFF = idle) and
	return PadStateStable(6) from PadGetState.
*/
#include "stub_log.h"

unsigned char *Port_PadBuffer[2];	/* captured for M3 */

extern "C" {

void PadInitDirect(unsigned char *pad1, unsigned char *pad2)
{
	Port_PadBuffer[0] = pad1;
	Port_PadBuffer[1] = pad2;
	if (pad1) pad1[0] = 1;		/* byte0 != 0: no data (disconnected) */
	if (pad2) pad2[0] = 1;
}

void PadStartCom(void)	{ }
void PadStopCom(void)	{ }

int PadGetState(int port)
{
	(void)port;
	return 0;	/* PadStateDiscon */
}

int PadInfoMode(int port, int term, int id)
{
	(void)port; (void)term; (void)id;
	return 0;
}

int PadSetMainMode(int port, int offs, int lock)
{
	(void)port; (void)offs; (void)lock;
	return 0;
}

int PadSetAct(int port, unsigned char *data, int len)
{
	(void)port; (void)data; (void)len;
	return 0;
}

int PadSetActAlign(int port, unsigned char *data)
{
	(void)port; (void)data;
	return 0;	/* keeps Pad->Send NULL and CanVibrate false in pads.cpp */
}

}
