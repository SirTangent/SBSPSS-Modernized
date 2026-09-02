/*	XA streaming engine (M6) - see xa_stream.cpp.  cd.cpp delegates the
	stream-relevant CD commands here; the engine owns the per-vblank sector
	clock, the de-interleave/filter routing, the ADPCM decode into the SPU
	CD-input ring, and the staged sector CdGetSector serves.  */
#ifndef PORT_XA_STREAM_H
#define PORT_XA_STREAM_H

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <libcd.h>

/*	engine entry points, called from cd.cpp's command dispatch  */
void XaStream_SetFilter(int file, int chan);
void XaStream_GetFilter(int *file, int *chan);	/* str_stream's SF gate */
void XaStream_SetMode(int mode);
void XaStream_ReadS(const CdlLOC *pos);
void XaStream_Pause(void);
void XaStream_Serve(uint32_t *madr, int sizeWords);	/* CdGetSector body */

/*	once per emulated vblank, from Port_Pump  */
extern "C" void Port_CdVblank(int vblankHz);

/*	provided by cd.cpp  */
extern CdlCB g_cdReadyCallback;
int Port_CdXaTrackInfo(FILE **fp, long *startLBA, long *sectors);
int Port_CdFileForLBA(long lba, FILE **fp, long *startLBA, long *sectors,
					  int *bytesPerSector, const char **name);

/*	test support: drop the cached track binding and any playing stream
	(used with cd.cpp's Port_CdRebuildDirForTest after re-pointing
	SBSP_DATA_DIR at a synthetic disc)  */
void XaStream_ResetForTest(void);

#endif
