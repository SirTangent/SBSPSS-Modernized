/*	Fixture tests for the software GTE (port/psyq/gte/) - hand-computed
	expectations from the psx-spx documented algorithms (M3 design decision:
	self-contained fixtures now, emulator A/B in M4 if visuals diverge).

	Covers: register read/write conversions, the UNR division (exact case
	and the overflow case), RTPS/RTPT through both the DMPSX tag path and
	the libgte C functions, NCLIP winding, MVMVA saturation + FLAG bits,
	AVSZ3, SQR, GPF colour FIFO, and the CPU matrix helpers.
*/
#include <cstdio>

#include <sys/types.h>
#include <libgte.h>
#include <inline_c.h>			/* the gte_* macros layertile3d itself uses */

#include "gte/gte_core.h"

extern "C" {
void GTEport_Op(u_long op);
u_long GTEport_GetData(int reg);
void GTEport_SetData(int reg, u_long v);
u_long GTEport_GetCtrl(int reg);
void GTEport_SetCtrl(int reg, u_long v);
}

static int g_failures;

static void check(bool ok, const char *what, long got, long want)
{
	if (!ok)
	{
		std::printf("FAIL: %s (got 0x%lX, want 0x%lX)\n", what, got, want);
		g_failures++;
	}
}
#define CHK(expr, want)	do { long g_ = (long)(expr); check(g_ == (long)(want), #expr, g_, (long)(want)); } while (0)

/*	identity rotation, zero translation, H=350, offsets/DQ zero  */
static void setupIdentity350(void)
{
	GTE_WriteCtrl(0, 0x00001000);	/* R11=4096 R12=0 */
	GTE_WriteCtrl(1, 0x00000000);	/* R13=0   R21=0 */
	GTE_WriteCtrl(2, 0x00001000);	/* R22=4096 R23=0 */
	GTE_WriteCtrl(3, 0x00000000);	/* R31=0   R32=0 */
	GTE_WriteCtrl(4, 0x00001000);	/* R33=4096 */
	for (int r = 5; r <= 7; r++)
		GTE_WriteCtrl(r, 0);		/* TR */
	GTE_WriteCtrl(24, 0);			/* OFX */
	GTE_WriteCtrl(25, 0);			/* OFY */
	GTE_WriteCtrl(26, 350);			/* H */
	GTE_WriteCtrl(27, 0);			/* DQA */
	GTE_WriteCtrl(28, 0);			/* DQB */
	GTE_WriteCtrl(29, 0x155);		/* ZSF3 */
	GTE_WriteCtrl(30, 0x100);		/* ZSF4 */
}

static void loadV(int v, int x, int y, int z)
{
	GTE_WriteData(v * 2, ((u_long)(unsigned short)x) | ((u_long)(unsigned short)y << 16));
	GTE_WriteData(v * 2 + 1, (u_long)(unsigned short)z);
}

int main()
{
	/* ---- register conversions ------------------------------------- */
	GTE_WriteData(1, 0x8000);						/* VZ0 write */
	CHK(GTE_ReadData(1), 0xFFFF8000ul);				/* sign-extended read */

	GTE_WriteData(15, 0x00010001);					/* SXYP pushes the fifo */
	GTE_WriteData(15, 0x00020002);
	GTE_WriteData(15, 0x00030003);
	CHK(GTE_ReadData(12), 0x00010001);
	CHK(GTE_ReadData(13), 0x00020002);
	CHK(GTE_ReadData(14), 0x00030003);
	CHK(GTE_ReadData(15), 0x00030003);				/* SXYP reads SXY2 */

	GTE_WriteData(28, 0x7FFF);						/* IRGB unpack */
	CHK(GTE_ReadData(9),  0xF80);					/* 0x1F * 0x80 */
	CHK(GTE_ReadData(10), 0xF80);
	CHK(GTE_ReadData(11), 0xF80);
	CHK(GTE_ReadData(29), 0x7FFF);					/* ORGB packs back */

	GTE_WriteData(30, 0);          CHK(GTE_ReadData(31), 32);	/* LZCS/LZCR */
	GTE_WriteData(30, 0xFFFFFFFF); CHK(GTE_ReadData(31), 32);
	GTE_WriteData(30, 1);          CHK(GTE_ReadData(31), 31);
	GTE_WriteData(30, 0x80000000); CHK(GTE_ReadData(31), 1);

	GTE_WriteCtrl(26, 0x8000);						/* H: cfc2 sign-extends */
	CHK(GTE_ReadCtrl(26), 0xFFFF8000ul);
	GTE_WriteCtrl(31, 0xFFFFFFFF);					/* FLAG write mask + bit31 */
	CHK(GTE_ReadCtrl(31), 0xFFFFF000ul);

	/* ---- RTPS: exact UNR division case (350/700 = 0.5 exactly) ---- */
	setupIdentity350();
	loadV(0, 100, -50, 700);
	GTEport_Op(0x0000007f);							/* rtps via DMPSX tag */
	CHK(GTE_ReadData(25), 100);						/* MAC1 */
	CHK(GTE_ReadData(9),  100);						/* IR1  */
	CHK(GTE_ReadData(10), 0xFFFFFFCEul);			/* IR2 = -50 */
	CHK(GTE_ReadData(19), 700);						/* SZ3  */
	CHK(GTE_ReadData(14), 0xFFE70032ul);			/* SXY2 = (50, -25) */
	CHK(GTE_ReadData(8),  0);						/* IR0 (DQA=DQB=0) */
	CHK(GTE_ReadCtrl(31), 0);						/* clean FLAG */

	/* ---- RTPS divide overflow: SZ3=1, H=350 ----------------------- */
	setupIdentity350();
	loadV(0, 100, 0, 1);
	GTEport_Op(0x0000007f);
	CHK(GTE_ReadData(19), 1);
	CHK((GTE_ReadCtrl(31) >> 17) & 1, 1);			/* divide overflow */
	CHK((GTE_ReadCtrl(31) >> 31) & 1, 1);			/* master bit */
	CHK((short)GTE_ReadData(14), 199);				/* SX2 = 131071*100 >> 16 */

	/* ---- RTPT: three vertices, fifo + last-vertex depth cue ------- */
	setupIdentity350();
	loadV(0, 100, -50, 700);
	loadV(1, -100, 50, 700);
	loadV(2, 0, 0, 350);
	GTEport_Op(0x000000bf);							/* rtpt */
	CHK(GTE_ReadData(12), 0xFFE70032ul);			/* SXY0 = (50,-25) */
	CHK(GTE_ReadData(13), 0x0019FFCEul);			/* SXY1 = (-50,25) */
	CHK(GTE_ReadData(14), 0);						/* SXY2 = (0,0)   */
	CHK(GTE_ReadData(17), 700);						/* SZ1 */
	CHK(GTE_ReadData(18), 700);						/* SZ2 */
	CHK(GTE_ReadData(19), 350);						/* SZ3 */

	/* ---- NCLIP winding -------------------------------------------- */
	GTE_WriteData(12, 0x00000000);					/* (0,0)   */
	GTE_WriteData(13, 0x0000000A);					/* (10,0)  */
	GTE_WriteData(14, 0x000A0000);					/* (0,10)  */
	GTEport_Op(0x0000117f);							/* nclip */
	CHK((long)GTE_ReadData(24), 100);				/* MAC0 */
	GTE_WriteData(13, 0x000A0000);					/* swap -> opposite winding */
	GTE_WriteData(14, 0x0000000A);
	GTEport_Op(0x0000117f);
	CHK((long)GTE_ReadData(24), -100);

	/* ---- AVSZ3 ---------------------------------------------------- */
	setupIdentity350();
	GTE_WriteData(17, 700);
	GTE_WriteData(18, 700);
	GTE_WriteData(19, 700);
	GTEport_Op(0x000011bf);							/* avsz3 */
	CHK((long)GTE_ReadData(24), 716100);			/* 0x155 * 2100 */
	CHK(GTE_ReadData(7), 174);						/* OTZ = >>12 */

	/* ---- SQR: sf variants + IR saturation ------------------------- */
	GTE_WriteData(9,  (u_long)(unsigned short)-100);
	GTE_WriteData(10, 50);
	GTE_WriteData(11, 3000);
	GTEport_Op(0x00000f3f);							/* sqr0 (sf=0) */
	CHK((long)GTE_ReadData(25), 10000);
	CHK((long)GTE_ReadData(26), 2500);
	CHK((long)GTE_ReadData(27), 9000000);
	CHK(GTE_ReadData(11), 0x7FFF);					/* IR3 saturated */
	CHK((GTE_ReadCtrl(31) >> 22) & 1, 1);			/* IR3 sat flag */
	GTE_WriteData(9,  (u_long)(unsigned short)-100);
	GTE_WriteData(10, 50);
	GTE_WriteData(11, 3000);
	GTEport_Op(0x00000eff);							/* sqr12 (sf=1) */
	CHK((long)GTE_ReadData(27), 2197);				/* 9000000 >> 12 */
	CHK(GTE_ReadData(11), 2197);
	CHK(GTE_ReadCtrl(31), 0);

	/* ---- MVMVA via the parameterised tag: 44-bit overflow + sat --- */
	setupIdentity350();
	GTE_WriteCtrl(5, 0x7FFFFFFF);					/* TRX huge */
	loadV(0, 2, 0, 0);
	/* mvmva(sf=1, mx=RT, v=V0, cv=TR, lm=0) */
	GTEport_Op(0x000013bfu | (1u << 25) | (0u << 23) | (0u << 21) | (0u << 19) | (0u << 18));
	CHK(GTE_ReadData(25), 0x80000001ul);			/* MAC1 wrapped through 44 bits */
	CHK(GTE_ReadData(9), 0xFFFF8000ul);				/* IR1 saturated low */
	CHK((GTE_ReadCtrl(31) >> 30) & 1, 1);			/* MAC1 positive overflow */
	CHK((GTE_ReadCtrl(31) >> 24) & 1, 1);			/* IR1 sat */

	/* same op through its preset tag (rt == mvmva sf=1 RT*V0+TR) */
	setupIdentity350();
	GTE_WriteCtrl(5, 0x7FFFFFFF);
	loadV(0, 2, 0, 0);
	GTEport_Op(0x000000ff);							/* rt */
	CHK(GTE_ReadData(25), 0x80000001ul);

	/* lm=1 clamps negatives to zero (ll preset has lm=1) */
	setupIdentity350();
	loadV(0, -100, 0, 0);
	GTEport_Op(0x000013bfu | (1u << 25) | (1u << 18));	/* mvmva sf=1 lm=1 */
	CHK(GTE_ReadData(9), 0);
	CHK((GTE_ReadCtrl(31) >> 24) & 1, 1);

	/* ---- GPF colour FIFO ------------------------------------------ */
	GTE_WriteData(6, 0x30000000);					/* RGBC: code byte 0x30 */
	GTE_WriteData(8, 0x800);						/* IR0 = 0.5 */
	GTE_WriteData(9, 0x1000);
	GTE_WriteData(10, 0x1000);
	GTE_WriteData(11, 0x1000);
	GTEport_Op(0x000012bf);							/* gpf12 */
	CHK((long)GTE_ReadData(25), 0x800);
	CHK(GTE_ReadData(22), 0x30808080ul);			/* RGB2 = code|80,80,80 */

	/* ---- libgte C functions over the core ------------------------- */
	{
		SVECTOR in;
		long sxy, p, flag, otz;
		setupIdentity350();
		in.vx = 100; in.vy = -50; in.vz = 700;
		otz = RotTransPers(&in, &sxy, &p, &flag);
		CHK(sxy, 0xFFE70032l);
		CHK(p, 0);
		CHK(flag, 0);
		CHK(otz, 175);								/* SZ3 >> 2 */

		SVECTOR out;
		GTE_WriteCtrl(5, 10);
		GTE_WriteCtrl(6, 20);
		GTE_WriteCtrl(7, 30);
		in.vx = 5; in.vy = 6; in.vz = 7;
		RotTransSV(&in, &out, &flag);
		CHK(out.vx, 15);
		CHK(out.vy, 26);
		CHK(out.vz, 37);
	}

	/* ---- CPU matrix helpers --------------------------------------- */
	{
		MATRIX m;
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				m.m[i][j] = (i == j) ? 4096 : 0;
		RotMatrixZ(1024, &m);						/* 90 degrees */
		CHK(m.m[0][0], 0);
		CHK(m.m[0][1], -4096);
		CHK(m.m[1][0], 4096);
		CHK(m.m[1][1], 0);
		CHK(m.m[2][2], 4096);

		VECTOR s = { 2048, 4096, 1024, 0 };			/* x0.5, x1, x0.25 */
		ScaleMatrix(&m, &s);
		CHK(m.m[1][0], 2048);
		CHK(m.m[0][1], -4096);

		SVECTOR v0 = { 100, 200, 400, 0 };
		VECTOR r;
		MATRIX ident;
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				ident.m[i][j] = (i == j) ? 4096 : 0;
		ApplyMatrix(&ident, &v0, &r);
		CHK(r.vx, 100); CHK(r.vy, 200); CHK(r.vz, 400);

		MATRIX t;
		TransposeMatrix(&m, &t);
		CHK(t.m[0][1], m.m[1][0]);
		CHK(t.m[1][0], m.m[0][1]);
	}

	/* ---- RotMatrix_gte: the X+Z composite (hbbarrel.cpp:195) ------- */
	/*	Every other caller in the tree rotates about ONE axis, where the
		library's nested truncation cannot show; hbbarrel is the only
		{X,0,Z} case, so it is the only place the evaluation ORDER is
		observable.  With vy=0 (s1=0, c1=ONE) every s1 term drops out and
		the result must collapse to a plain Rx*Rz with ONE >>12 per
		element - if a rewrite ever hoists or re-nests the shifts, the
		single-shift expectations below stop matching.  */
	{
		MATRIX		m;
		SVECTOR		r;
		const int	ax = 300, az = 700;			/* arbitrary, non-special */
		int			s0 = rsin(ax), c0 = rcos(ax);
		int			s2 = rsin(az), c2 = rcos(az);

		r.vx = ax; r.vy = 0; r.vz = az; r.pad = 0;
		RotMatrix_gte(&r, &m);

		CHK(m.m[0][0], (short)c2);				/* (c1*c2)>>12, c1==ONE */
		CHK(m.m[0][1], (short)(-s2));
		CHK(m.m[0][2], 0);						/* s1 == rsin(0) */
		CHK(m.m[1][0], (short)((c0 * s2) >> 12));
		CHK(m.m[1][1], (short)((c0 * c2) >> 12));
		CHK(m.m[1][2], (short)(-s0));			/* -((s0*ONE)>>12) */
		CHK(m.m[2][0], (short)((s0 * s2) >> 12));
		CHK(m.m[2][1], (short)((s0 * c2) >> 12));
		CHK(m.m[2][2], (short)c0);

		/*	90 degrees on both axes - closed form, independent of the
			trig table.  */
		r.vx = 1024; r.vy = 0; r.vz = 1024;
		RotMatrix_gte(&r, &m);
		CHK(m.m[0][0], 0);      CHK(m.m[0][1], -4096); CHK(m.m[0][2], 0);
		CHK(m.m[1][0], 0);      CHK(m.m[1][1], 0);     CHK(m.m[1][2], -4096);
		CHK(m.m[2][0], 4096);   CHK(m.m[2][1], 0);     CHK(m.m[2][2], 0);
	}

	/* ---- layertile3d's op sequence --------------------------------- */
	/*	The 3D action layer is the port's heaviest GTE consumer and uses a
		shape nothing else does.  These pin the three things that would
		silently corrupt the tile layer if the shim drifted.  */
	{
		/*	(a) CMX_SetRotMatrixXY (layertile3d.h) writes the flip matrix
			as two PACKED PAIRS into ctrl 0 and ctrl 2 - R11|R12<<16 and
			R22|R23<<16 - never touching R13/R21/R31/R32/R33.  A packing
			mismatch here shows up as tiles that fail to mirror.  */
		setupIdentity350();
		const short	Mtx[4] = { -4096, 0, 4096, 0 };		/* X mirror */
		GTE_WriteCtrl(0, (u_long)(unsigned short)Mtx[0]
						 | ((u_long)(unsigned short)Mtx[1] << 16));
		GTE_WriteCtrl(2, (u_long)(unsigned short)Mtx[2]
						 | ((u_long)(unsigned short)Mtx[3] << 16));
		CHK(GTE_ReadCtrl(0), 0x0000F000ul);
		CHK(GTE_ReadCtrl(2), 0x00001000ul);

		loadV(0, 100, -50, 700);
		GTEport_Op(0x0000007f);						/* rtps */
		CHK(GTE_ReadData(14), 0xFFE7FFCEul);		/* SX negated, SY intact */

		/*	(b) the pipelined batch (layertile3d.cpp:270-283): the game
			preloads the NEXT triangle with ldv3 before storing the
			CURRENT one with stsxy3c.  That is only valid if loading V0-V2
			leaves the SXY fifo alone - if it did not, every tile would
			take the following tile's screen coordinates.  */
		setupIdentity350();
		SVECTOR	a0 = { 100, -50, 700, 0 };
		SVECTOR	a1 = {   0,   0, 700, 0 };
		SVECTOR	a2 = { -100, 50, 700, 0 };
		SVECTOR	b  = { 200, 200, 700, 0 };
		u_long	out[3] = { 0, 0, 0 };

		gte_ldv3(&a0, &a1, &a2);
		gte_rtpt_b();
		gte_ldv3(&b, &b, &b);					/* preload, must not disturb */
		gte_stsxy3c(out);
		CHK(out[0], 0xFFE70032ul);				/* ( 50,-25) */
		CHK(out[1], 0x00000000ul);				/* (  0,  0) */
		CHK(out[2], 0x0019FFCEul);				/* (-50, 25) */

		/*	(c) ldsxy0/1/2 -> nclip_b -> stopz.  The layer draws a tile
			when MAC0 is NEGATIVE, and mirrored tiles flip that test by
			XORing the sign bit, so the sign convention is load-bearing in
			both directions.  stopz reads MAC0 (data reg 24).  */
		long	clipZ = 0;
		gte_ldsxy0(0x00000000);					/* (  0,  0) */
		gte_ldsxy1(0x00000064);					/* (100,  0) */
		gte_ldsxy2(0x00640000);					/* (  0,100) */
		gte_nclip_b();
		gte_stopz(&clipZ);
		CHK(clipZ, 10000);						/* one winding: positive */

		gte_ldsxy1(0x00640000);					/* swap 1 and 2 */
		gte_ldsxy2(0x00000064);
		gte_nclip_b();
		gte_stopz(&clipZ);
		CHK(clipZ, -10000);						/* the other: negative */
		CHK((u_long)clipZ >> 31, 1);			/* the bit the layer tests */
	}

	if (g_failures)
	{
		std::printf("gte_test: %d FAILURES\n", g_failures);
		return 1;
	}
	std::printf("gte_test: all passed\n");
	return 0;
}
