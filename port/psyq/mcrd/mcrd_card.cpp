/*	Standard 128KB PS1 memory-card image (see mcrd_card.h).

	Layout (per no$psx and what DuckStation reads):
	  block 0 = system block, 64 x 128-byte frames:
	    frame 0      header: "MC", zero fill, XOR checksum in byte 127
	    frames 1-15  directory frames for data blocks 1-15:
	                   +0x00 u32  state: 0xA0 free, 0x51/52/53 in-use
	                              first/middle/last, 0xA1/A2/A3 freed
	                   +0x04 u32  file size in BYTES (first-link frame only)
	                   +0x08 u16  next block's dir index, 0xFFFF = end
	                   +0x0A      filename, 20 chars + NUL
	                   +0x7F      XOR checksum of bytes 0..126
	    frames 16-35 broken-sector list: sector u32 0xFFFFFFFF = none
	    frames 36-55 broken-sector replacement data, 0xFF fill (no checksum)
	    frames 56-62 unused, 0xFF fill
	    frame 63     write-test frame: a copy of frame 0
	  blocks 1-15 = file data, 8192 bytes each.

	The whole image lives in memory; every mutation rewrites the host file
	via a temp-file + rename so a crash mid-write cannot corrupt the save.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <sys/types.h>
#include <kernel.h>				/* DIRENTRY */

#include "mcrd_card.h"

static uint8_t	g_card[CARD_IMAGE_SIZE];
static char		g_cardPath[512];
static int		g_opened;			/* 0 = never, 1 = ok, -1 = host unusable */

/*****************************************************************************/
/*	little-endian field access inside a frame  */

static void st16(uint8_t *p, unsigned v)	{ p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void st32(uint8_t *p, unsigned v)	{ st16(p, v); st16(p + 2, v >> 16); }
static unsigned ld16(const uint8_t *p)		{ return p[0] | (p[1] << 8); }
static unsigned ld32(const uint8_t *p)		{ return ld16(p) | (ld16(p + 2) << 16); }

static uint8_t *frame(int n)		{ return &g_card[n * CARD_FRAME_SIZE]; }
static uint8_t *dirFrame(int block)	{ return frame(block); }	/* dir frame n <-> block n */
static uint8_t *blockData(int block){ return &g_card[block * CARD_BLOCK_SIZE]; }

static void frameChecksum(uint8_t *f)
{
	uint8_t x = 0;
	for (int i = 0; i < CARD_FRAME_SIZE - 1; i++)
		x ^= f[i];
	f[CARD_FRAME_SIZE - 1] = x;
}

static int stateInUse(unsigned s)	{ return (s & 0xF0) == 0x50; }
static int stateFirst(unsigned s)	{ return s == 0x51; }

/*****************************************************************************/
/*	host file  */

static void resolvePath(void)
{
	char dir[448];
	const char *env = getenv("SBSP_SAVE_DIR");
	if (env)
		snprintf(dir, sizeof(dir), "%s", env);
	else
	{
		const char *appdata = getenv("APPDATA");
		snprintf(dir, sizeof(dir), "%s\\SBSPSS", appdata ? appdata : ".");
	}

	/*	create the directory chain (the first mkdir in the port)  */
	char part[448];
	size_t n = 0;
	for (const char *p = dir; ; p++)
	{
		if (*p && *p != '\\' && *p != '/')
		{
			if (n < sizeof(part) - 1)
				part[n++] = *p;
			continue;
		}
		part[n] = 0;
		/*	skip drive roots ("C:") and empty segments  */
		if (n && !(n == 2 && part[1] == ':'))
			_mkdir(part);
		if (!*p)
			break;
		if (n < sizeof(part) - 1)
			part[n++] = '\\';
	}

	snprintf(g_cardPath, sizeof(g_cardPath), "%s\\card0.mcd", dir);
}

/*	"flush" itself is a PSY-Q macro (R3000.H) - hence the name  */
static CardResult cardFlush(void)
{
	char tmp[520];
	snprintf(tmp, sizeof(tmp), "%s.tmp", g_cardPath);

	FILE *f = fopen(tmp, "wb");
	if (!f)
	{
		fprintf(stderr, "[mcrd] cannot write %s\n", tmp);
		return CARD_IO_ERROR;
	}
	size_t wrote = fwrite(g_card, 1, CARD_IMAGE_SIZE, f);
	if (fclose(f) != 0 || wrote != CARD_IMAGE_SIZE)
	{
		fprintf(stderr, "[mcrd] short write to %s\n", tmp);
		remove(tmp);
		return CARD_IO_ERROR;
	}
	remove(g_cardPath);				/* Windows rename() will not overwrite */
	if (rename(tmp, g_cardPath) != 0)
	{
		fprintf(stderr, "[mcrd] cannot rename %s into place\n", tmp);
		return CARD_IO_ERROR;
	}
	return CARD_OK;
}

/*****************************************************************************/

static void formatImage(void)
{
	memset(g_card, 0, CARD_IMAGE_SIZE);

	uint8_t *hdr = frame(0);
	hdr[0] = 'M';
	hdr[1] = 'C';
	frameChecksum(hdr);

	for (int b = 1; b <= CARD_DATA_BLOCKS; b++)
	{
		uint8_t *d = dirFrame(b);
		st32(d + 0x00, 0xA0);		/* free */
		st32(d + 0x04, 0);
		st16(d + 0x08, 0xFFFF);
		frameChecksum(d);
	}
	for (int i = 16; i <= 35; i++)	/* broken-sector list: none */
	{
		uint8_t *f = frame(i);
		st32(f, 0xFFFFFFFF);
		frameChecksum(f);
	}
	for (int i = 36; i <= 62; i++)	/* replacement data + unused */
		memset(frame(i), 0xFF, CARD_FRAME_SIZE);
	memcpy(frame(63), frame(0), CARD_FRAME_SIZE);	/* write-test frame */
}

int Card_IsFormatted(void)
{
	const uint8_t *hdr = frame(0);
	return hdr[0] == 'M' && hdr[1] == 'C';
}

CardResult Card_Open(void)
{
	if (g_opened)
		return g_opened > 0 ? CARD_OK : CARD_IO_ERROR;

	resolvePath();

	FILE *f = fopen(g_cardPath, "rb");
	if (f)
	{
		size_t got = fread(g_card, 1, CARD_IMAGE_SIZE, f);
		fclose(f);
		if (got == CARD_IMAGE_SIZE)
		{
			g_opened = 1;
			return CARD_OK;			/* may still be unformatted - that is a
									   state the game handles, not an error */
		}
		fprintf(stderr, "[mcrd] %s is %u bytes, not a 128KB card image - "
						"reformatting\n", g_cardPath, (unsigned)got);
	}

	formatImage();
	if (cardFlush() != CARD_OK)
	{
		g_opened = -1;				/* host unusable -> "no card" */
		return CARD_IO_ERROR;
	}
	fprintf(stderr, "[mcrd] created card image %s\n", g_cardPath);
	g_opened = 1;
	return CARD_OK;
}

CardResult Card_Format(void)
{
	formatImage();
	return cardFlush();
}

CardResult Card_Unformat(void)
{
	memset(frame(0), 0, CARD_FRAME_SIZE);
	return cardFlush();
}

/*****************************************************************************/
/*	directory  */

static int findFile(const char *name)
{
	for (int b = 1; b <= CARD_DATA_BLOCKS; b++)
	{
		const uint8_t *d = dirFrame(b);
		if (stateFirst(ld32(d)) &&
			strncmp((const char *)d + 0x0A, name, 20) == 0)
			return b;
	}
	return 0;
}

long Card_Dirents(struct DIRENTRY *out, long maxEntries)
{
	long count = 0;
	for (int b = 1; b <= CARD_DATA_BLOCKS && count < maxEntries; b++)
	{
		const uint8_t *d = dirFrame(b);
		if (!stateFirst(ld32(d)))
			continue;
		struct DIRENTRY *e = &out[count++];
		memset(e, 0, sizeof(*e));
		memcpy(e->name, d + 0x0A, sizeof(e->name));
		e->size = (long)ld32(d + 0x04);
		e->attr = 0x50;				/* what a real card reports for a save */
		e->head = b - 1;
	}
	return count;
}

CardResult Card_CreateFile(const char *name, long blocks)
{
	if (!Card_IsFormatted())
		return CARD_NOT_FORMATTED;
	if (blocks < 1 || findFile(name))
		return blocks < 1 ? CARD_NO_FILE : CARD_FILE_EXISTS;

	/*	collect enough free blocks (0xA0 fresh or 0xA1-A3 freed)  */
	int chain[CARD_DATA_BLOCKS];
	int got = 0;
	for (int b = 1; b <= CARD_DATA_BLOCKS && got < blocks; b++)
		if (!stateInUse(ld32(dirFrame(b))))
			chain[got++] = b;
	if (got < blocks)
		return CARD_FULL;

	for (int i = 0; i < blocks; i++)
	{
		uint8_t *d = dirFrame(chain[i]);
		memset(d, 0, CARD_FRAME_SIZE);
		st32(d + 0x00, i == 0 ? 0x51 : (i == blocks - 1 ? 0x53 : 0x52));
		st16(d + 0x08, i == blocks - 1 ? 0xFFFF : (unsigned)(chain[i + 1] - 1));
		if (i == 0)
		{
			st32(d + 0x04, (unsigned)(blocks * CARD_BLOCK_SIZE));
			strncpy((char *)d + 0x0A, name, 20);
		}
		frameChecksum(d);
		memset(blockData(chain[i]), 0, CARD_BLOCK_SIZE);
	}
	return cardFlush();
}

CardResult Card_DeleteFile(const char *name)
{
	int b = findFile(name);
	if (!b)
		return CARD_NO_FILE;

	while (b)
	{
		uint8_t *d = dirFrame(b);
		unsigned state = ld32(d);
		unsigned next  = ld16(d + 0x08);
		st32(d, (state & 0x0F) | 0xA0);		/* 0x51/52/53 -> 0xA1/A2/A3 */
		frameChecksum(d);
		b = (next == 0xFFFF) ? 0 : (int)next + 1;
	}
	return cardFlush();
}

/*****************************************************************************/
/*	file data access, walking the block chain  */

static CardResult fileSpan(const char *name, long ofs, long bytes,
						   uint8_t *dst, const uint8_t *src)
{
	int b = findFile(name);
	if (!b)
		return CARD_NO_FILE;

	long pos = 0;					/* file offset of the current block */
	while (b && bytes > 0)
	{
		if (ofs < pos + CARD_BLOCK_SIZE)
		{
			long start = ofs > pos ? ofs - pos : 0;
			long n     = CARD_BLOCK_SIZE - start;
			if (n > bytes)
				n = bytes;
			if (dst)
				memcpy(dst, blockData(b) + start, n);
			else
				memcpy(blockData(b) + start, src, n);
			dst   += dst ? n : 0;
			src   += src ? n : 0;
			ofs   += n;
			bytes -= n;
		}
		pos += CARD_BLOCK_SIZE;
		unsigned next = ld16(dirFrame(b) + 0x08);
		b = (next == 0xFFFF) ? 0 : (int)next + 1;
	}
	return bytes > 0 ? CARD_NO_FILE : CARD_OK;	/* ran off the chain */
}

CardResult Card_ReadFile(const char *name, void *dst, long ofs, long bytes)
{
	return fileSpan(name, ofs, bytes, (uint8_t *)dst, NULL);
}

CardResult Card_WriteFile(const char *name, const void *src, long ofs, long bytes)
{
	CardResult r = fileSpan(name, ofs, bytes, NULL, (const uint8_t *)src);
	if (r != CARD_OK)
		return r;
	return cardFlush();
}

/*****************************************************************************/

uint8_t *Card_ImageForTest(void)	{ return g_card; }
const char *Card_PathForTest(void)	{ return g_opened ? g_cardPath : NULL; }
