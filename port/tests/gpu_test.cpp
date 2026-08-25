/*	Unit tests for the software GPU (port/psyq/gpu/) - no window, no game.
	Pins the semantics the game depends on: transfer rect masking, OT chain
	shape, the GP0 interpreter, FT4 texture sampling + modulation, clipping,
	drawing offset, and semi-transparency mode 0.
*/
#include <cstdio>
#include <cstring>

#include <sys/types.h>
#include <libgte.h>
#include <libgpu.h>

#include "gpu/gpu_core.h"

static int g_failures;

static void check(bool ok, const char *what)
{
	if (!ok)
	{
		std::printf("FAIL: %s\n", what);
		g_failures++;
	}
}

static void checkPx(int x, int y, uint16_t want, const char *what)
{
	if (g_vram[y][x] != want)
	{
		std::printf("FAIL: %s - vram[%d][%d] = %04x, want %04x\n",
					what, y, x, g_vram[y][x], want);
		g_failures++;
	}
}

/*	fresh full-VRAM draw environment  */
static void resetEnv(void)
{
	memset(g_vram, 0, sizeof(g_vram));
	DRAWENV env;
	SetDefDrawEnv(&env, 0, 0, 1024, 512);
	PutDrawEnv(&env);
}

int main()
{
	/* --- ClearOTagR / ClearOTag chain shape ------------------------------- */
	{
		static u_long ot[16];
		ClearOTagR(ot, 16);
		check(ot[0] == 0x00FFFFFF, "ClearOTagR: ot[0] is the terminator");
		for (int i = 1; i < 16; i++)
			check((ot[i] & 0xFFFFFF) == ((u_long)(uintptr_t)&ot[i - 1] & 0xFFFFFF)
				  && (ot[i] >> 24) == 0,
				  "ClearOTagR: reverse links with len 0");

		ClearOTag(ot, 16);
		check(ot[15] == 0x00FFFFFF, "ClearOTag: last entry terminated");
		check((ot[0] & 0xFFFFFF) == ((u_long)(uintptr_t)&ot[1] & 0xFFFFFF),
			  "ClearOTag: forward links");
	}

	/* --- LoadImage / StoreImage round trip -------------------------------- */
	{
		resetEnv();
		static uint16_t src[8 * 4], back[8 * 4];
		for (int i = 0; i < 8 * 4; i++)
			src[i] = (uint16_t)(0x1234 + i);
		RECT r = { 100, 200, 8, 4 };
		LoadImage(&r, (u_long *)src);
		checkPx(100, 200, 0x1234, "LoadImage top-left");
		checkPx(107, 203, (uint16_t)(0x1234 + 31), "LoadImage bottom-right");
		StoreImage(&r, (u_long *)back);
		check(memcmp(src, back, sizeof(src)) == 0, "StoreImage round trip");
	}

	/* --- MoveImage -------------------------------------------------------- */
	{
		RECT r = { 100, 200, 8, 4 };
		MoveImage(&r, 300, 400);
		checkPx(300, 400, 0x1234, "MoveImage dest top-left");
		checkPx(307, 403, (uint16_t)(0x1234 + 31), "MoveImage dest bottom-right");
		checkPx(100, 200, 0x1234, "MoveImage source intact");
	}

	/* --- ClearImage with a hardware-oversized rect (actor.cpp:299) -------- */
	{
		resetEnv();
		RECT big = { 512, 256, 2048, 254 };
		ClearImage(&big, 255, 0, 0);
		/*	hardware masks the fill width to 10 bits: 2048 & 0x3FF == 0, so
			this call is a no-op on the console too - it must neither crash
			nor overrun here, and must not fill anything  */
		checkPx(512, 256, 0x0000, "ClearImage w=2048 masks to a no-op (hardware)");
		checkPx(0, 0, 0x0000, "ClearImage did not touch origin");

		RECT ok = { 512, 256, 64, 32 };
		ClearImage(&ok, 255, 0, 0);
		checkPx(512, 256, 0x001F, "ClearImage in-range rect fills");
	}

	/* --- POLY_F4 flat fill + drawing offset + clip ------------------------ */
	{
		resetEnv();
		DRAWENV env;
		SetDefDrawEnv(&env, 0, 256, 512, 256);	/* Screen[1]-style: ofs (0,256) */
		PutDrawEnv(&env);

		POLY_F4 q;
		setPolyF4(&q);
		setRGB0(&q, 255, 255, 255);
		setXYWH(&q, 10, 10, 16, 16);
		DrawPrim(&q);

		checkPx(10, 266, 0x7FFF, "F4: offset applied (10,10)->(10,266)");
		checkPx(25, 281, 0x7FFF, "F4: bottom-right inside (exclusive edges)");
		checkPx(26, 282, 0x0000, "F4: right/bottom edges excluded");
		checkPx(10, 10, 0x0000, "F4: clip kept it out of the top half");
	}

	/* --- POLY_FT4 4bpp CLUT texture + modulation -------------------------- */
	{
		resetEnv();
		/*	texture page 0 at VRAM (0,0); put a 4bpp pattern at texel row 0:
			indices 1,2,3,4 -> one halfword 0x4321  */
		static uint16_t texRow[4] = { 0x4321, 0x4321, 0x4321, 0x4321 };
		RECT tr = { 0, 0, 4, 1 };
		LoadImage(&tr, (u_long *)texRow);
		/*	CLUT at (0,500): entry 0 transparent-black, 1..4 solid colours  */
		static uint16_t clut[16];
		memset(clut, 0, sizeof(clut));
		clut[1] = 0x001F;	/* red   */
		clut[2] = 0x03E0;	/* green */
		clut[3] = 0x7C00;	/* blue  */
		clut[4] = 0x7FFF;	/* white */
		RECT cr = { 0, 500, 16, 1 };
		LoadImage(&cr, (u_long *)clut);

		DRAWENV env;
		SetDefDrawEnv(&env, 0, 0, 1024, 512);
		PutDrawEnv(&env);

		POLY_FT4 p;
		setPolyFT4(&p);
		setRGB0(&p, 128, 128, 128);			/* 128 = identity modulation */
		setXYWH(&p, 200, 100, 4, 1);
		setUVWH(&p, 0, 0, 4, 1);
		p.tpage = getTPage(0, 0, 0, 0);		/* 4bpp, page (0,0) */
		p.clut  = getClut(0, 500);
		DrawPrim(&p);

		checkPx(200, 100, 0x001F, "FT4: texel 1 -> CLUT red");
		checkPx(201, 100, 0x03E0, "FT4: texel 2 -> CLUT green");
		checkPx(202, 100, 0x7C00, "FT4: texel 3 -> CLUT blue");
		checkPx(203, 100, 0x7FFF, "FT4: texel 4 -> CLUT white");

		/* modulation at half brightness: 64/128 halves each 5-bit channel */
		setRGB0(&p, 64, 64, 64);
		setXYWH(&p, 210, 100, 4, 1);
		DrawPrim(&p);
		checkPx(213, 100, (uint16_t)(15 | (15 << 5) | (15 << 10)),
				"FT4: colour 64 halves white to 15/15/15");

		/* texel 0 (CLUT entry 0 = 0x0000) is transparent */
		g_vram[220][100] = 0x1234;			/* y=100? row is [y][x] */
		g_vram[100][220] = 0x1234;
		POLY_FT4 t0 = p;
		setRGB0(&t0, 128, 128, 128);
		setXYWH(&t0, 220, 100, 1, 1);
		setUVWH(&t0, 4, 0, 1, 1);			/* texel index 0 lives at u=4..7 (0x4321 nibbles) */
		/* u=4 -> halfword 1 (0x4321), nibble 0 -> index 1... build a real zero:
		   use u beyond the loaded row: VRAM is zeroed there -> index 0 */
		setUVWH(&t0, 32, 0, 1, 1);
		DrawPrim(&t0);
		checkPx(220, 100, 0x1234, "FT4: texel 0000 leaves dest untouched");
	}

	/*	--- POLY_G3 gouraud interpolation ------------------------------------
		Pins the barycentric path (which interpolates through a precomputed
		reciprocal rather than a per-pixel divide).  Right-angled triangle
		a=(100,100) red, b=(164,100) green, c=(100,164) blue: the vertex pixel
		must be the vertex colour exactly, and each edge midpoint the exact
		mean of its two endpoints.  */
	{
		resetEnv();

		uint32_t g3[7];
		g3[0] = 0x06000000;						/* tag: len 6 */
		g3[1] = 0x30000000u | 0x0000FF;			/* G3 + colour0 = red   (BGR word) */
		g3[2] = (100 << 16) | 100;
		g3[3] = 0x00FF00;						/* colour1 = green */
		g3[4] = (100 << 16) | 164;
		g3[5] = 0xFF0000;						/* colour2 = blue  */
		g3[6] = (164 << 16) | 100;
		DrawPrim(g3);

		checkPx(100, 100, 0x001F, "G3: vertex 0 pixel is vertex 0 colour exactly");
		checkPx(132, 100, (uint16_t)(15 | (15 << 5)),
				"G3: a-b midpoint is (red+green)/2");
		checkPx(100, 132, (uint16_t)(15 | (15 << 10)),
				"G3: a-c midpoint is (red+blue)/2");
		checkPx(163, 163, 0x0000, "G3: outside the hypotenuse stays clear");
	}

	/*	--- gouraud at the maximum triangle size ------------------------------
		The interpolation reciprocal is only exact while area*(area*255) fits
		its shift; the biggest triangle the size-reject allows is the worst
		case, so pin an exact vertex colour there too.  */
	{
		resetEnv();

		uint32_t g3[7];
		g3[0] = 0x06000000;
		g3[1] = 0x30000000u | 0x0000FF;			/* red   at (0,0)     */
		g3[2] = 0;
		g3[3] = 0x00FF00;						/* green at (0,511)   */
		g3[4] = (511 << 16) | 0;
		g3[5] = 0xFF0000;						/* blue  at (1023,0)  */
		g3[6] = 1023;
		DrawPrim(g3);

		checkPx(0, 0, 0x001F, "G3 (1023x511): vertex colour still exact");
	}

	/* --- semi-transparency mode 0 (B/2 + F/2) via TPOLY-style E1+poly ----- */
	{
		resetEnv();
		/* background: mid grey 16/16/16 */
		RECT r = { 50, 50, 16, 16 };
		ClearImage(&r, 128, 128, 128);
		uint16_t bg = g_vram[50][50];

		/*	packet: E1 word (abr mode 0) then semi-transparent flat white quad
			- exactly the TPOLY_F4 wire format  */
		uint32_t packet[7];
		packet[0] = 0x06000000;								/* tag: len 6 (ignored addr) */
		packet[1] = 0xE1000200;								/* draw mode: dtd, abr 0 */
		packet[2] = 0x2A000000u | (255) | (255 << 8) | (255 << 16);	/* F4 + semi */
		packet[3] = (50 << 16) | 50;
		packet[4] = (50 << 16) | 66;
		packet[5] = (66 << 16) | 50;
		packet[6] = (66 << 16) | 66;
		DrawPrim(packet);

		int br = bg & 31, fr = 31;
		uint16_t want = (uint16_t)(((br + fr) >> 1) * 0x421);	/* same on all channels */
		checkPx(50, 50, want, "semi mode 0: (B+F)/2");
	}

	if (g_failures)
	{
		std::printf("gpu test FAILED (%d)\n", g_failures);
		return 1;
	}
	std::printf("gpu test PASSED\n");
	return 0;
}
