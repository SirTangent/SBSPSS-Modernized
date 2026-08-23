/* LZNP encoder — counterpart to the game's decoder (source/utils/lznp.cpp,
   Nick Pelling's format). Returns the packed size written to Dest.
   Dest must have room for the worst case: insize + insize/8 + 16. */

#ifndef __LZNP_ENCODE_H__
#define __LZNP_ENCODE_H__

int LZNP_Encode(unsigned char *Dest, const unsigned char *Src, int insize);

#endif
