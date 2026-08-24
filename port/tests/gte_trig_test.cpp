/*	Fixture test for the shim's libgte trig (port/psyq/gte/trig.cpp).
	Pins the 12-bit fixed-point conventions and boundary values; M3 extends
	this with emulator-captured fixtures for bit-exactness.
*/
#include <cstdio>
#include <cstdlib>

extern "C" {
int  rsin(int a);
int  rcos(int a);
long ratan2(long y, long x);
long SquareRoot0(long a);
long SquareRoot12(long a);
}

static int g_failures;

static void check(bool ok, const char *what, long got, long want)
{
	if (!ok)
	{
		std::printf("FAIL: %s (got %ld, want %ld)\n", what, got, want);
		g_failures++;
	}
}

int main()
{
	/* cardinal points: ONE == 4096 == full circle */
	check(rsin(0) == 0,        "rsin(0)",    rsin(0), 0);
	check(rsin(1024) == 4096,  "rsin(1024)", rsin(1024), 4096);
	check(rsin(2048) == 0,     "rsin(2048)", rsin(2048), 0);
	check(rsin(3072) == -4096, "rsin(3072)", rsin(3072), -4096);
	check(rcos(0) == 4096,     "rcos(0)",    rcos(0), 4096);
	check(rcos(2048) == -4096, "rcos(2048)", rcos(2048), -4096);

	/* wrap + quadrant identities */
	check(rsin(4096) == rsin(0), "rsin wraps at 4096", rsin(4096), rsin(0));
	check(rsin(-1024) == -4096,  "rsin(-1024)", rsin(-1024), -4096);
	for (int a = 0; a < 4096; a += 37)
	{
		if (rcos(a) != rsin(a + 1024))
		{
			check(false, "rcos(a)==rsin(a+1024)", rcos(a), rsin(a + 1024));
			break;
		}
	}

	/* sine is monotone on the first quarter */
	for (int a = 1; a <= 1024; a++)
	{
		if (rsin(a) < rsin(a - 1))
		{
			check(false, "rsin monotone on [0,1024]", a, 0);
			break;
		}
	}

	/* ratan2: octant centres, range (-2048, 2048] */
	check(ratan2(0, 1000) == 0,      "ratan2(0,+x)",  ratan2(0, 1000), 0);
	check(ratan2(1000, 0) == 1024,   "ratan2(+y,0)",  ratan2(1000, 0), 1024);
	check(ratan2(0, -1000) == -2048 || ratan2(0, -1000) == 2048,
		  "ratan2(0,-x) == half turn", ratan2(0, -1000), 2048);
	check(ratan2(-1000, 0) == -1024, "ratan2(-y,0)",  ratan2(-1000, 0), -1024);
	check(ratan2(1000, 1000) == 512, "ratan2(+y,+x) diagonal", ratan2(1000, 1000), 512);
	check(ratan2(0, 0) == 0,         "ratan2(0,0)",   ratan2(0, 0), 0);

	/* integer square roots: exact on perfect squares, floor in between */
	check(SquareRoot0(0) == 0,           "SquareRoot0(0)", SquareRoot0(0), 0);
	check(SquareRoot0(1) == 1,           "SquareRoot0(1)", SquareRoot0(1), 1);
	check(SquareRoot0(65536) == 256,     "SquareRoot0(65536)", SquareRoot0(65536), 256);
	check(SquareRoot0(65535) == 255,     "SquareRoot0(65535) floors", SquareRoot0(65535), 255);
	for (long r = 0; r < 46000; r += 997)
	{
		if (SquareRoot0(r * r) != r)
		{
			check(false, "SquareRoot0 exact on squares", SquareRoot0(r * r), r);
			break;
		}
	}
	check(SquareRoot12(4096) == 4096,    "SquareRoot12(1.0) == 1.0", SquareRoot12(4096), 4096);
	check(SquareRoot12(16384) == 8192,   "SquareRoot12(4.0) == 2.0", SquareRoot12(16384), 8192);

	if (g_failures)
	{
		std::printf("gte trig test FAILED (%d)\n", g_failures);
		return 1;
	}
	std::printf("gte trig test PASSED\n");
	return 0;
}
