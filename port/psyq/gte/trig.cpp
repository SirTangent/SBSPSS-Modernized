/*	libgte fixed-point trig: rsin/rcos/ratan2/SquareRoot0/SquareRoot12.

	These are real implementations (not stubs) - they are linked by ~100
	gameplay TUs and drive movement/collision everywhere.  Conventions
	(PSY-Q): angles are 12-bit fixed with ONE==4096 == a full circle;
	rsin/rcos return 12-bit fixed in [-4096, 4096]; ratan2(y,x) returns an
	angle in (-2048, 2048]; SquareRoot0 is the integer square root,
	SquareRoot12 the 20.12 square root.

	The sine table is computed once with round-to-nearest - deterministic
	across runs.  Bit-exactness against the MIPS libgte gets validated in M3
	with emulator-captured fixtures; the unit test (port/tests/
	gte_trig_test.cpp) pins the identities and boundary values.
*/
#include <math.h>
#include <stdint.h>

extern "C" {
int  rsin(int a);
int  rcos(int a);
long ratan2(long y, long x);
long SquareRoot0(long a);
long SquareRoot12(long a);
}

static int		g_sinInit;
static short	g_sinTab[4096];

static void sinInit(void)
{
	if (g_sinInit)
		return;
	for (int i = 0; i < 4096; i++)
		g_sinTab[i] = (short)lround(sin((double)i * (3.14159265358979323846 * 2.0 / 4096.0)) * 4096.0);
	g_sinInit = 1;
}

extern "C" int rsin(int a)
{
	sinInit();
	return g_sinTab[a & 4095];
}

extern "C" int rcos(int a)
{
	sinInit();
	return g_sinTab[(a + 1024) & 4095];
}

extern "C" long ratan2(long y, long x)
{
	if (x == 0 && y == 0)
		return 0;
	long v = (long)lround(atan2((double)y, (double)x) * (4096.0 / (3.14159265358979323846 * 2.0)));
	return ((v + 2048) & 4095) - 2048;
}

static long isqrt64(uint64_t v)
{
	uint64_t r = (uint64_t)sqrt((double)v);
	/* correct the double rounding at the edges */
	while (r * r > v)
		r--;
	while ((r + 1) * (r + 1) <= v)
		r++;
	return (long)r;
}

extern "C" long SquareRoot0(long a)
{
	return isqrt64((uint32_t)a);
}

extern "C" long SquareRoot12(long a)
{
	return isqrt64((uint64_t)(uint32_t)a << 12);
}
