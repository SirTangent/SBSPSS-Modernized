/*	Internal seams of the MDEC decoder (mdec.cpp) exposed for unit tests
	and for the VLC decoder (vlc3.cpp) - not part of the PSY-Q surface.

	The run-level stream between DecDCTvlc3 and DecDCTin mirrors the real
	MDEC(1) input format so buffers stay dump-diffable against emulators:
	  word 0     = (0x3800 << 16) | number-of-following-32bit-words
	  halfwords  = (run << 10) | (level & 0x3FF)   ;AC, level signed 10bit
	  block head = (qscale << 10) | (DC & 0x3FF)   ;first halfword per block
	  end/pad    = 0xFE00                          ;EOB, also inter-block pad
	Blocks per macroblock in order Cr, Cb, Y1, Y2, Y3, Y4; macroblocks in
	bitstream order (16px column strips, top to bottom - which is exactly
	the order fmv.cpp's 16px LoadImage slices consume).
*/
#ifndef PORT_MDEC_INTERNAL_H
#define PORT_MDEC_INTERNAL_H

#include <stdint.h>

#define MDEC_EOB			0xFE00u
#define MDEC_MAGIC			0x3800u
#define MDEC_MB_BYTES_24BPP	768			/* 16x16 px, 3 bytes each */
#define MDEC_MAX_MB			512			/* frame cap (320x240 needs 300) */

#ifdef __cplusplus
extern "C" {
#endif

/*	Dequant + un-zigzag one run-level block (no IDCT).  Advances *src.
	Returns 0 on success, -1 when only padding/end-of-stream remained
	(blk untouched in that case).  qtZigzag is 64 quant bytes in stream
	(zig-zag) order, as in DECDCTENV.  */
int Mdec_RlDecodeBlock(int16_t blk[64], const uint16_t **src,
					   const uint16_t *end, const uint8_t *qtZigzag);

/*	real_idct_core: two matrix passes against the scale table.  In place.  */
void Mdec_Idct(int16_t blk[64]);

/*	One 8x8 Y block + the shared Cr/Cb blocks -> 24bpp RGB quadrant
	(xx,yy in {0,8}) of a 16x16x3 macroblock buffer.  Unsigned output.  */
void Mdec_YuvToRgb24(const int16_t crBlk[64], const int16_t cbBlk[64],
					 const int16_t yBlk[64], int xx, int yy,
					 uint8_t rgb[16 * 16 * 3]);

/*	Test hooks into mdec.cpp state.  */
const uint8_t *Mdec_IqZigzagYForTest(void);
int Mdec_FrameBytesForTest(void);

#ifdef __cplusplus
}
#endif
#endif
