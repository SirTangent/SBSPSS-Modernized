/*	libmcrd over a host card image (M6).  M3's "no card present" protocol
	shim, now wired to the real 128KB card image in mcrd_card.cpp.

	The game's MemCard state machine (source/memcard/memcard.cpp) drives
	everything by polling s_syncStatus = MemCardSync(1,&cmds,&rslt):
	-1 means "no process active" (issue the next command), anything else
	means the queued command finished with `rslt`.  Completion model here is
	immediate: every command executes synchronously at call time and latches
	its result for the next Sync poll.  The state machine accepts next-poll
	latching by design (its handlers are two-phase on exactly that), and the
	save/load UI has 60-frame settle timers, so simulated card latency would
	buy nothing.

	Split of blocking vs async follows the game's usage, not the SDK docs:
	  - Exist/Accept/ReadFile/WriteFile are polled via Sync; their return is
	    "was the command REGISTERED" and MUST be nonzero - the game treats 0
	    as "could not register" and abandons the operation without waiting
	    for a completion (memcard.cpp:970 HandleCmd_ReadFile and :1077
	    HandleCmd_WriteFile drop straight back to CmdNone on 0, leaving the
	    UI waiting on a callback that never comes).  Card absence is
	    reported through the completion result, never this return.
	  - CreateFile/DeleteFile/Format/Unformat/GetDirentry are called
	    blocking and their McErr code is checked directly.

	Only chan 0 (slot 1, USE_SLOT_ONE_ONLY) is backed; any other chan
	reports McErrCardNotExist, which the game turns into the shipped
	"no memory card" screens.
*/
#include <sys/types.h>
#include <libmcrd.h>

#include "stub_log.h"
#include "mcrd_card.h"

static int	s_pending;			/* one command deep - all the game needs */
static long	s_pendingCmd;
static long	s_pendingResult;

static long queueCmd(long cmd, long result)
{
	s_pending       = 1;
	s_pendingCmd    = cmd;
	s_pendingResult = result;
	return 1;					/* command registered */
}

/*	CardResult -> the McErr the game's handlers branch on  */
static long mapResult(CardResult r)
{
	switch (r)
	{
	case CARD_OK:			return McErrNone;
	case CARD_NOT_FORMATTED:return McErrNotFormat;
	case CARD_NO_FILE:		return McErrFileNotExist;
	case CARD_FILE_EXISTS:	return McErrAlreadyExist;
	case CARD_FULL:			return McErrBlockFull;
	case CARD_IO_ERROR:
	default:				return McErrCardNotExist;
	}
}

static int chanBacked(long chan)
{
	return chan == 0 && Card_Open() == CARD_OK;
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
	return queueCmd(McFuncExist,
					chanBacked(chan) ? McErrNone : McErrCardNotExist);
}

long MemCardAccept(long chan)
{
	long result = McErrCardNotExist;
	if (chanBacked(chan))
		result = Card_IsFormatted() ? McErrNone : McErrNotFormat;
	return queueCmd(McFuncAccept, result);
}

long MemCardReadFile(long chan, char *file, unsigned long *adrs, long ofs, long bytes)
{
	long result = McErrCardNotExist;
	if (chanBacked(chan))
		result = mapResult(Card_ReadFile(file, adrs, ofs, bytes));
	return queueCmd(McFuncReadFile, result);
}

long MemCardWriteFile(long chan, char *file, unsigned long *adrs, long ofs, long bytes)
{
	long result = McErrCardNotExist;
	if (chanBacked(chan))
		result = mapResult(Card_WriteFile(file, adrs, ofs, bytes));
	return queueCmd(McFuncWriteFile, result);
}

/*	The blocking calls report their McErr directly.  */
long MemCardCreateFile(long chan, char *file, long blocks)
{
	if (!chanBacked(chan))
		return McErrCardNotExist;
	return mapResult(Card_CreateFile(file, blocks));
}

long MemCardDeleteFile(long chan, char *file)
{
	if (!chanBacked(chan))
		return McErrCardNotExist;
	return mapResult(Card_DeleteFile(file));
}

long MemCardFormat(long chan)
{
	if (!chanBacked(chan))
		return McErrCardNotExist;
	return mapResult(Card_Format());
}

long MemCardUnformat(long chan)
{
	if (!chanBacked(chan))
		return McErrCardNotExist;
	return mapResult(Card_Unformat());
}

long MemCardGetDirentry(long chan, char *name, struct DIRENTRY *dir,
						long *files, long ofs, long max)
{
	(void)name;					/* the game only ever passes "*" */
	(void)ofs;
	if (!chanBacked(chan))
	{
		if (files)
			*files = 0;
		return McErrCardNotExist;
	}
	long count = Card_Dirents(dir, max);
	if (files)
		*files = count;
	return McErrNone;
}

}
