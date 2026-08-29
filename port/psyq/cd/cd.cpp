/*	libcd over the host filesystem: the M1 synchronous subset.

	Model: a virtual disc directory of the files filetab.cpp knows about
	(BIGLUMP.BIN, TRACK1.IXA, *.STR), each with a fabricated LBA range;
	BIGLUMP.BIN sits at LBA 0 so that with FileStart==0 BigLump sector N maps
	to byte N*2048 of the host file - exactly what CCDFileIO expects.

	This shim also plays PsxBoot's role: on the retail (__USER_CDBUILD__)
	build, filetab.cpp is compiled out and the game reads its file-position
	table from the scratchpad (CFileIO::GetAllFilePos), which the boot
	program pre-filled on real hardware.  Here a static initialiser fills
	PORT_Scratchpad with the virtual LBAs before main() runs.

	Reads are synchronous; CdReadSync pumps the vblank clock once so
	callback-time work (loading icon, XM_Update in later milestones) still
	happens "during" loads.
*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <libcd.h>

#include "stub_log.h"
#include "host/pump.h"

#include "system/types.h"
#include "system/asmport.h"		/* PORT_Scratchpad */
#include "system/info.h"		/* INF_Version/Territory/FileSystem */

/*	Virtual disc directory - names and order must match FilenameList in
	source/fileio/filetab.cpp (FILEPOS_* enum order).  DEMO.STR is EUR-only
	but harmlessly present here.
*/
struct VirtFile
{
	const char	*name;
	long		startLBA;
	long		sizeBytes;		/* 0 if the host file is absent */
	FILE		*fp;
	int			bytesPerSector;	/* 2048 data; 2336 for the raw-XA TRACK1.IXA
								   (8-byte subheader + 2328 data per sector,
								   no sync/header - see xa_stream.cpp) */
};

static VirtFile g_files[] =
{
	{ "BIGLUMP.BIN", 0, 0, NULL, 2048 },
	{ "TRACK1.IXA",  0, 0, NULL, 2336 },
	{ "THQ.STR",     0, 0, NULL, 2048 },
	{ "CLIMAX.STR",  0, 0, NULL, 2048 },
	{ "INTRO.STR",   0, 0, NULL, 2048 },
	{ "DEMO.STR",    0, 0, NULL, 2048 },
};
static const int	g_fileCount = sizeof(g_files) / sizeof(g_files[0]);
static const int	SECTOR = 2048;

static int		g_inited;
static long		g_curLBA;
static double	g_readDeadline;	/* CD pacing: when the in-flight read "completes" */
static int		g_pace = -1;	/* -1 unparsed; SBSP_CD_PACE=0 disables */

/*	XA streaming (M4 degree: end-immediately).  Real sector delivery is M6;
	until then an armed CdlReadS answers with ONE fabricated terminator
	header so CXAStream reaches XA_MODE_END instead of pinning
	isSpeechPlaying() true forever (which wedges GameOverScene's
	speech-before-exit state and permanently ducks the mixer).
	XACDReadyCallback (cdxa.cpp:53-91) ends the stream on CdlDataReady when
	sector word 3 carries ID 352 ("video terminator") and the filter's
	channel in bits 10-14 of the high halfword.  */
static int		g_xaFilterChan = -1;	/* last CdlSetfilter chan (param[1]) */
static int		g_xaEndIn;				/* vblanks until the terminator fires */
static int		g_xaServeTerminator;	/* CdGetSector serves the header now */

static void dataPath(char *dst, size_t dstSize, const char *name)
{
	const char *root = getenv("SBSP_DATA_DIR");
	if (root)
		snprintf(dst, dstSize, "%s/%s", root, name);
	else
		snprintf(dst, dstSize, "out/%s/%s/version/%s/%s",
				 INF_Territory, INF_Version, INF_FileSystem, name);
}

static void cdBuildDir(void)
{
	long lba = 0;
	for (int i = 0; i < g_fileCount; i++)
	{
		char path[512];
		dataPath(path, sizeof(path), g_files[i].name);
		FILE *f = fopen(path, "rb");
		long size = 0;
		if (f)
		{
			fseek(f, 0, SEEK_END);
			size = ftell(f);
		}
		g_files[i].fp        = f;
		g_files[i].sizeBytes = size;
		g_files[i].startLBA  = lba;
		long bps     = g_files[i].bytesPerSector;
		long sectors = (size + bps - 1) / bps;
		if (sectors < 16) sectors = 16;			/* keep ranges distinct */
		lba += (sectors + 15) & ~15;			/* 16-sector aligned */
	}

	/*	PsxBoot protocol: int FilePosList[FILEPOS_MAX] at the scratchpad,
		each entry CdPosToInt() of the file's start position (== start LBA).
		FILEPOS_MAX is 5 on USA, 6 on EUR; writing all 6 is harmless.  */
	int *pos = (int *)PORT_Scratchpad;
	for (int i = 0; i < g_fileCount; i++)
		pos[i] = (int)g_files[i].startLBA;
}

static VirtFile *fileForLBA(long lba)
{
	for (int i = 0; i < g_fileCount; i++)
	{
		long bps     = g_files[i].bytesPerSector;
		long sectors = (g_files[i].sizeBytes + bps - 1) / bps;
		if (lba >= g_files[i].startLBA && lba < g_files[i].startLBA + sectors)
			return &g_files[i];
	}
	return NULL;
}

namespace { struct CdBoot { CdBoot() { cdBuildDir(); g_inited = 1; } }; static CdBoot g_cdBoot; }

/*****************************************************************************/
/*	BCD conversion: use the SDK's own itob/btoi macros from LIBCD.H  */

extern "C" int CdInit(void)
{
	if (!g_inited)
	{
		cdBuildDir();
		g_inited = 1;
	}

	/*	The game cannot run without its data lump; a wrong path must fail
		HERE, loudly, not as zero-filled reads parsed far from the cause.
		(The static CdBoot stays soft so data-less tools like gte_trig_test
		can still link the shim.)  */
	if (!g_files[0].fp)
	{
		char path[512];
		dataPath(path, sizeof(path), g_files[0].name);
		fprintf(stderr, "[shim] CdInit: cannot open %s\n"
						"       run port/build-data.cmd, or point SBSP_DATA_DIR at the directory holding it\n",
				path);
		abort();
	}
	return 1;
}

extern "C" CdlLOC *CdIntToPos(int i, CdlLOC *p)
{
	i += 150;								/* 2-second lead-in */
	p->sector = (u_char)itob(i % 75);
	i /= 75;
	p->second = (u_char)itob(i % 60);
	p->minute = (u_char)itob(i / 60);
	p->track  = 0;
	return p;
}

extern "C" int CdPosToInt(CdlLOC *p)
{
	return ((btoi(p->minute) * 60 + btoi(p->second)) * 75 + btoi(p->sector)) - 150;
}

extern "C" CdlFILE *CdSearchFile(CdlFILE *fp, char *name)
{
	/*	filetab.cpp searches "\NAME;1" - strip the path and version chars  */
	const char *base = name;
	while (*base == '\\' || *base == '/')
		base++;

	char clean[32];
	size_t n = 0;
	while (base[n] && base[n] != ';' && n < sizeof(clean) - 1)
	{
		clean[n] = base[n];
		n++;
	}
	clean[n] = 0;

	for (int i = 0; i < g_fileCount; i++)
	{
		if (_stricmp(clean, g_files[i].name) == 0)
		{
			CdIntToPos((int)g_files[i].startLBA, &fp->pos);
			fp->size = (u_long)g_files[i].sizeBytes;
			strncpy(fp->name, g_files[i].name, sizeof(fp->name) - 1);
			fp->name[sizeof(fp->name) - 1] = 0;
			return fp;
		}
	}
	fprintf(stderr, "[shim] CdSearchFile: unknown file '%s'\n", clean);
	return NULL;
}

extern "C" int CdControl(u_char com, u_char *param, u_char *result)
{
	return CdControlB(com, param, result);
}

extern "C" int CdControlB(u_char com, u_char *param, u_char *result)
{
	(void)result;
	switch (com)
	{
	case CdlSetloc:
		g_curLBA = CdPosToInt((CdlLOC *)param);
		return 1;
	case CdlSetmode:
	case CdlNop:
	case CdlPause:
	case CdlMute:
	case CdlDemute:
		return 1;
	case CdlSetfilter:
		g_xaFilterChan = param ? param[1] : -1;	/* CdlFILTER.chan */
		return 1;
	case CdlReadS:
		/*	XA streaming read - arm the end-immediately terminator (see the
			g_xa* block above).  2 vblanks lets ControlXA finish its own
			START->PLAY transition before the callback ends the stream.  */
		g_xaEndIn = 2;
		return 1;
	default:
		PSYQ_STUB_ONCE();
		return 1;
	}
}

extern "C" int CdControlF(u_char com, u_char *param)
{
	return CdControlB(com, param, 0);
}

extern "C" int CdRead(int sectors, u_long *buf, int mode)
{
	(void)mode;
	unsigned char *dst = (unsigned char *)buf;
	int paced = 0;

	while (sectors > 0)
	{
		VirtFile *vf = fileForLBA(g_curLBA);
		if (vf && vf->bytesPerSector != SECTOR)
		{
			/*	CdRead is the 2048-byte data path; landing in the raw-XA
				range means a broken position, not a recoverable read  */
			fprintf(stderr, "[shim] CdRead: LBA %ld is inside %s (raw XA, "
							"%d-byte sectors) - data reads cannot go there\n",
					g_curLBA, vf->name, vf->bytesPerSector);
			abort();
		}
		if (!vf || !vf->fp)
		{
			/*	Success + zeros here would defeat the callers' entire error
				path (cdfile.cpp retries while(!Error) forever on 0, and
				parses whatever we hand it on 1) - a data-configuration
				problem is unrecoverable, so stop at the cause.  */
			fprintf(stderr, "[shim] CdRead: LBA %ld maps to no host file "
							"(missing data file or wrong SBSP_DATA_DIR)\n", g_curLBA);
			abort();
		}

		long offset = (g_curLBA - vf->startLBA) * SECTOR;
		fseek(vf->fp, offset, SEEK_SET);
		size_t got = fread(dst, 1, SECTOR, vf->fp);
		if (got < (size_t)SECTOR)
			memset(dst + got, 0, SECTOR - got);	/* zero-fill the EOF tail sector */

		dst += SECTOR;
		g_curLBA++;
		sectors--;
		paced++;
	}

	/*	CD pacing: the data is already in the buffer, but CdReadSync reports
		"still reading" until a double-speed drive would have delivered it
		(150 sectors/s).  This is what gives the loading icon its window -
		with instant reads, zero vblanks elapse between StartLoad and
		StopLoad and the game itself skips the icon.  SBSP_CD_PACE=0 turns
		it off for instant loads.  */
	if (g_pace < 0)
	{
		const char *e = getenv("SBSP_CD_PACE");
		g_pace = !(e && *e == '0');
	}
	if (g_pace)
	{
		double now = Port_NowSeconds();
		if (g_readDeadline < now)
			g_readDeadline = now;
		g_readDeadline += (double)paced / 150.0;
	}
	return 1;
}

extern "C" int CdReadSync(int mode, u_char *result)
{
	(void)result;
	Port_Pump();		/* PS1 interrupt-time work happens during reads */
	if (mode == 0)
	{	/* blocking wait - PumpIdle, not Pump: a bare spin burns a whole core
		   for the duration of every load */
		while (Port_NowSeconds() < g_readDeadline)
			Port_PumpIdle();
		return 0;
	}

	if (Port_NowSeconds() < g_readDeadline)
	{
		/*	the live caller (cdfile.cpp:45) is `while (CdReadSync(1,0) > 0);` -
			a bare spin.  Yield a tick before reporting busy so a paced load
			costs milliseconds of one core rather than all of it.  */
		Port_PumpIdle();
		return 1;
	}
	return 0;
}

extern "C" int CdSync(int mode, u_char *result)
{
	(void)mode;
	(void)result;
	return CdlComplete;
}

extern "C" int CdGetSector(void *madr, int size)
{
	/*	Real XA/FMV sector payloads are M6/M7.  The one thing served today is
		the armed terminator header: word 3 = (chan<<10)<<16 | 352, exactly
		what XACDReadyCallback's end test reads (cdxa.cpp:71-89).  */
	uint32_t *w = (uint32_t *)madr;
	for (int i = 0; i < size; i++)
		w[i] = 0;
	if (g_xaServeTerminator && size >= 4)
		w[3] = ((uint32_t)(g_xaFilterChan & 31) << 26) | 352u;
	return 1;
}

extern "C" int CdMix(CdlATV *vol)
{
	(void)vol;
	PSYQ_STUB_ONCE();
	return 1;
}

extern "C" int CdSetDebug(int level)
{
	(void)level;
	return 0;
}

/*	Callback registration is kept, not dropped: CXAStream::XACDReadyCallback
	(cdxa.cpp:150) is the only writer of XA_MODE_END, so losing it would pin
	isSpeechPlaying() true forever.  Until M6 routes real sector delivery
	through the pump, the only firing is Port_CdVblank's one-shot terminator
	(below); CdReadCallback stays registration-only (M7).  */
static CdlCB g_readCallback;
static CdlCB g_readyCallback;

/*	Once per emulated vblank, from Port_Pump.  Counts down an armed CdlReadS
	and delivers the fabricated end-of-stream sector: one CdlDataReady with
	the terminator header staged for CdGetSector.  */
extern "C" void Port_CdVblank(void)
{
	static u_char	result[8];

	/*	The countdown only runs while there is someone to deliver to.
		CFmvScene clears the ready callback at both ends of a movie
		(source/fmv/fmv.cpp:186,267), and consuming the arming into a NULL
		callback would drop this stream's terminator for good: cdxa.cpp is
		the only writer of XA_MODE_END, so isSpeechPlaying() would stick
		true forever - the exact wedge this delivery exists to prevent.  */
	if (!g_readyCallback || g_xaEndIn <= 0)
		return;
	if (--g_xaEndIn != 0)
		return;

	fprintf(stderr, "[cd] XA stream chan %d: end-of-stream delivered (audio is M6)\n",
			g_xaFilterChan);
	g_xaServeTerminator = 1;
	g_readyCallback(CdlDataReady, result);
	g_xaServeTerminator = 0;
}

extern "C" CdlCB CdReadCallback(CdlCB func)
{
	CdlCB old = g_readCallback;
	g_readCallback = func;
	PSYQ_STUB_ONCE();	/* registered but not yet fired (M7) */
	return old;
}

extern "C" CdlCB CdReadyCallback(CdlCB func)
{
	CdlCB old = g_readyCallback;
	g_readyCallback = func;
	PSYQ_STUB_ONCE();	/* registered but not yet fired (M6) */
	return old;
}
