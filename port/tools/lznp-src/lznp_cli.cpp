/*
	lznp_cli.cpp — modern replacement for the 16-bit tools/lznp.exe, which
	cannot run on 64-bit Windows.

	Compresses with lznp_encode.cpp, written against the game's own decoder
	(source/utils/lznp.cpp, Nick Pelling's LZNP). Verified by round-trip
	through that exact decoder (roundtrip_test.cpp). Command line matches the
	vintage tool as used by the data pipeline:

		lznp [-Q] <infile> <outfile>

	-Q (quiet) is accepted and ignored. Build:
		g++ -O2 -static port/tools/lznp-src/lznp_cli.cpp \
		    port/tools/lznp-src/lznp_encode.cpp -o port/tools/lznp.exe
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lznp_encode.h"

int main(int argc, char *argv[])
{
	const char *inName = 0, *outName = 0;

	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-' || argv[i][0] == '/')
			continue;                       /* -Q etc: ignore */
		if (!inName)       inName = argv[i];
		else if (!outName) outName = argv[i];
	}

	if (!inName || !outName)
	{
		fprintf(stderr, "Usage: lznp [-Q] <infile> <outfile>\n");
		return 1;
	}

	FILE *f = fopen(inName, "rb");
	if (!f) { fprintf(stderr, "lznp: cannot open %s\n", inName); return 1; }
	fseek(f, 0, SEEK_END);
	long inSize = ftell(f);
	fseek(f, 0, SEEK_SET);
	unsigned char *inBuf = (unsigned char *)malloc(inSize ? inSize : 1);
	if (!inBuf || (long)fread(inBuf, 1, inSize, f) != inSize)
	{
		fprintf(stderr, "lznp: read error on %s\n", inName);
		return 1;
	}
	fclose(f);

	/* worst case: incompressible data grows by control bytes; be generous */
	unsigned char *outBuf = (unsigned char *)malloc(inSize + inSize / 2 + 4096);
	if (!outBuf) { fprintf(stderr, "lznp: out of memory\n"); return 1; }

	int outSize = LZNP_Encode(outBuf, inBuf, (int)inSize);

	f = fopen(outName, "wb");
	if (!f) { fprintf(stderr, "lznp: cannot create %s\n", outName); return 1; }
	if ((int)fwrite(outBuf, 1, outSize, f) != outSize)
	{
		fprintf(stderr, "lznp: write error on %s\n", outName);
		return 1;
	}
	fclose(f);

	free(inBuf);
	free(outBuf);
	return 0;
}
