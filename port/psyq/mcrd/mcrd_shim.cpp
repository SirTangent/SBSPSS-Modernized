/*	libmcrd "no card present" protocol (M3).

	The game's MemCard state machine (source/memcard/memcard.cpp) drives
	everything by polling s_syncStatus = MemCardSync(1,&cmds,&rslt):
	-1 means "no process active" (issue the next command), anything else
	means the queued command finished with `rslt`.  The M1/M2 stubs
	returned -1 forever, so MemCardExist never completed and the options
	screen's load/save pages would sit in SS_Scanning for good.

	Here every queued command completes on the next Sync with
	McErrCardNotExist, which drives HandleCmd_Exist's error branch ->
	InvalidateCard -> CS_NoCard: exactly the graceful "no memory card"
	screens the game shipped with.  The real file-backed implementation
	lands in M6.
*/
#include <sys/types.h>
#include <libmcrd.h>

#include "stub_log.h"

static int	s_pending;			/* one command deep - all the game needs */
static long	s_pendingCmd;
static long	s_pendingResult;

static long queueCmd(long cmd, long result)
{
	s_pending       = 1;
	s_pendingCmd    = cmd;
	s_pendingResult = result;
	return 0;					/* command accepted */
}

extern "C" {

void MemCardInit(long val)		{ (void)val; }
void MemCardEnd(void)			{ }
void MemCardStart(void)			{ }
void MemCardStop(void)			{ }
void MemCardClose(void)			{ }

long MemCardSync(long mode, long *cmds, long *rslt)
{
	(void)mode;					/* 0 blocking / 1 polling: both resolve now */
	if (!s_pending)
		return -1;				/* no process active */
	s_pending = 0;
	if (cmds)
		*cmds = s_pendingCmd;
	if (rslt)
		*rslt = s_pendingResult;
	return 1;					/* command completed */
}

long MemCardExist(long chan)
{
	(void)chan;
	return queueCmd(McFuncExist, McErrCardNotExist);
}

long MemCardAccept(long chan)
{
	(void)chan;
	return queueCmd(McFuncAccept, McErrCardNotExist);
}

long MemCardReadFile(long chan, char *file, unsigned long *adrs, long ofs, long bytes)
{
	(void)chan; (void)file; (void)adrs; (void)ofs; (void)bytes;
	return queueCmd(McFuncReadFile, McErrCardNotExist);
}

long MemCardWriteFile(long chan, char *file, unsigned long *adrs, long ofs, long bytes)
{
	(void)chan; (void)file; (void)adrs; (void)ofs; (void)bytes;
	return queueCmd(McFuncWriteFile, McErrCardNotExist);
}

/*	The synchronous calls report the same absence directly.  */
long MemCardCreateFile(long chan, char *file, long blocks)
{
	(void)chan; (void)file; (void)blocks;
	return McErrCardNotExist;
}

long MemCardDeleteFile(long chan, char *file)
{
	(void)chan; (void)file;
	return McErrCardNotExist;
}

long MemCardFormat(long chan)
{
	(void)chan;
	return McErrCardNotExist;
}

long MemCardUnformat(long chan)
{
	(void)chan;
	return McErrCardNotExist;
}

long MemCardGetDirentry(long chan, char *name, struct DIRENTRY *dir,
						long *files, long ofs, long max)
{
	(void)chan; (void)name; (void)dir; (void)ofs; (void)max;
	if (files)
		*files = 0;
	return McErrCardNotExist;
}

}
