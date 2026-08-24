/*	Replaces source/system/except_a.mip (the R3000 exception-handler thunk).
	The PC runs no MIPS exception handler; install/uninstall are no-ops and
	reg_lst points at a zeroed 34-word register frame so except.cpp's
	printouts stay harmless if ever reached (installExceptionHandler is only
	called under __USER_paul__).
*/
static int g_regFrame[34];

int	*reg_lst = g_regFrame;
int	dev_kit  = 1;	/* SONY_PCI, matches lnkopt */

extern "C" void install_exc(void)	{ }
extern "C" void uninstall_exc(void)	{ }
