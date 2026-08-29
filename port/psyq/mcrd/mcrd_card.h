/*	PS1 memory-card image (M6): a standard 128KB raw card image, the format
	DuckStation (the project's reference emulator) reads and writes natively,
	so the PC port's saves can be cross-checked in its memory-card editor and
	real PS1 saves imported by dropping a file in.

	Pure card filesystem over an in-memory image - no libmcrd types or
	protocol here (mcrd_shim.cpp maps results to McErr codes), so it is unit
	testable in isolation (mcrd_test.cpp).
*/
#ifndef PORT_MCRD_CARD_H
#define PORT_MCRD_CARD_H

#include <stdint.h>

struct DIRENTRY;					/* kernel.h */

enum CardResult
{
	CARD_OK = 0,
	CARD_IO_ERROR,					/* host file cannot be created/written */
	CARD_NOT_FORMATTED,
	CARD_NO_FILE,
	CARD_FILE_EXISTS,
	CARD_FULL,
};

static const int CARD_FRAME_SIZE  = 128;
static const int CARD_BLOCK_SIZE  = 8192;
static const int CARD_DATA_BLOCKS = 15;
static const int CARD_IMAGE_SIZE  = 16 * CARD_BLOCK_SIZE;	/* system block + 15 */

/*	Resolve the image path ($SBSP_SAVE_DIR else %APPDATA%\SBSPSS, file
	card0.mcd), then load it - or create a freshly formatted image on first
	run.  Idempotent; every other call implies it.  CARD_IO_ERROR means the
	host location is unusable (the shim reports "no card").  */
CardResult	Card_Open(void);

int			Card_IsFormatted(void);
CardResult	Card_Format(void);		/* fresh format, persisted */
CardResult	Card_Unformat(void);	/* wipe the header frame, persisted */

/*	Fill KERNEL.H DIRENTRYs for every file on the card (first-link directory
	frames); returns the count, at most maxEntries.  `size` is in bytes, as
	the game's free-block arithmetic expects.  */
long		Card_Dirents(struct DIRENTRY *out, long maxEntries);

CardResult	Card_CreateFile(const char *name, long blocks);
CardResult	Card_DeleteFile(const char *name);
CardResult	Card_ReadFile(const char *name, void *dst, long ofs, long bytes);
CardResult	Card_WriteFile(const char *name, const void *src, long ofs, long bytes);

/*	Test access: the raw image and the resolved path (NULL before Card_Open).  */
uint8_t		*Card_ImageForTest(void);
const char	*Card_PathForTest(void);

#endif
