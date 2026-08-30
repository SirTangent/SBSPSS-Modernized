/*	CD-XA ADPCM sector decoder (M6) - see xa_adpcm.cpp.  */
#ifndef PORT_XA_ADPCM_H
#define PORT_XA_ADPCM_H

#include <stdint.h>

static const int XA_SECTOR_USER_BYTES = 2304;	/* 18 x 128-byte sound groups */
static const int XA_SECTOR_SAMPLES    = 4032;	/* mono 4-bit: 18 x 8 x 28 */
static const int XA_SECTOR_PAIRS      = 2016;	/* stereo 4-bit: per channel */

/*	Decode one mono 4-bit XA sector (coding-info 0x04, the only mode the
	game's speech uses: 18.9kHz mono).  `user2304` is the first 2304 bytes of
	the sector's user data; emits 4032 samples.  `hist1/hist2` carry the IIR
	filter history across blocks, groups AND sectors of one stream - reset
	them to 0 when a stream (re)starts.  */
void XaAdpcm_DecodeSector4bitMono(const uint8_t *user2304, int16_t *out4032,
								  int32_t *hist1, int32_t *hist2);

/*	Decode one stereo 4-bit XA sector (coding-info 0x01: 37.8kHz stereo -
	what the STR movies interleave, M7).  Sound units alternate L/R (even =
	left), so a sector yields 2016 interleaved L,R pairs.  Separate history
	per channel, same carry rules as mono.  */
void XaAdpcm_DecodeSector4bitStereo(const uint8_t *user2304,
									int16_t *outPairs2016x2,
									int32_t *histL1, int32_t *histL2,
									int32_t *histR1, int32_t *histR2);

#endif
