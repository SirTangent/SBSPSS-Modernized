/*	libgte C functions over the software cop2 core.

	The GTE-backed ones (RotTransPers/RotTransSV/RotTrans, SetRotMatrix &c)
	go through gte_core exactly like the MIPS originals went through cop2,
	so saturation/FLAG behaviour is shared with the inline gte_* macro path
	by construction.  The pure-CPU ones (RotMatrixZ, ScaleMatrix,
	ApplyMatrix, RotMatrix_gte) follow the PSY-Q library's fixed-point
	evaluation order (matched-decompilation form: every product truncated
	with an arithmetic >>12 exactly where the original did).

	RotMatrix_gte composes m = Rx(vx)*Ry(vy)*Rz(vz); its only caller is
	in-game 3D model rendering (CModelGfx::RenderElem), so M4's emulator
	A/B pass is the final authority on it - flagged there.
*/
#include <sys/types.h>
#include <libgte.h>

#include "gte_core.h"

extern "C" int rsin(int a);
extern "C" int rcos(int a);

/*	cop2 words (from the DMPSX table in gteport.cpp)  */
#define COP2_RTPS	0x4A180001u
#define COP2_RT		0x4A480012u		/* MVMVA sf=1: RT*V0 + TR */

static void loadV0(const SVECTOR *v)
{
	GTE_WriteData(0, ((uint32_t)(uint16_t)v->vx) | ((uint32_t)(uint16_t)v->vy << 16));
	GTE_WriteData(1, (uint32_t)(uint16_t)v->vz);
}

/*****************************************************************************/
extern "C" void InitGeom(void)
{
	/*	Classic library defaults; the game overrides H and the offset right
		after (vid.cpp: SetGeomScreen(350), SetGeomOffset).  ZSF3/4 give the
		conventional OTZ = averageZ/4 scaling.  */
	GTE_WriteCtrl(24, 0);					/* OFX */
	GTE_WriteCtrl(25, 0);					/* OFY */
	GTE_WriteCtrl(26, 1000);				/* H   */
	GTE_WriteCtrl(27, 0);					/* DQA */
	GTE_WriteCtrl(28, 0);					/* DQB */
	GTE_WriteCtrl(29, 0x155);				/* ZSF3 = 4096/12 */
	GTE_WriteCtrl(30, 0x100);				/* ZSF4 = 4096/16 */
}

extern "C" void SetGeomOffset(long ofx, long ofy)
{
	GTE_WriteCtrl(24, (uint32_t)ofx << 16);
	GTE_WriteCtrl(25, (uint32_t)ofy << 16);
}

extern "C" void SetGeomScreen(long h)
{
	GTE_WriteCtrl(26, (uint32_t)h);
}

/*****************************************************************************/
static uint32_t pairLE(short lo, short hi)
{
	return (uint32_t)(uint16_t)lo | ((uint32_t)(uint16_t)hi << 16);
}

extern "C" void SetRotMatrix(MATRIX *m)
{
	GTE_WriteCtrl(0, pairLE(m->m[0][0], m->m[0][1]));
	GTE_WriteCtrl(1, pairLE(m->m[0][2], m->m[1][0]));
	GTE_WriteCtrl(2, pairLE(m->m[1][1], m->m[1][2]));
	GTE_WriteCtrl(3, pairLE(m->m[2][0], m->m[2][1]));
	GTE_WriteCtrl(4, (uint32_t)(uint16_t)m->m[2][2]);
}

extern "C" void SetTransMatrix(MATRIX *m)
{
	GTE_WriteCtrl(5, (uint32_t)m->t[0]);
	GTE_WriteCtrl(6, (uint32_t)m->t[1]);
	GTE_WriteCtrl(7, (uint32_t)m->t[2]);
}

/*****************************************************************************/
extern "C" long RotTransPers(SVECTOR *v0, long *sxy, long *p, long *flag)
{
	loadV0(v0);
	GTE_ExecuteCop2(COP2_RTPS);
	*sxy  = (long)GTE_ReadData(14);			/* SXY2 */
	*p    = (long)GTE_ReadData(8);			/* IR0 (depth interpolation) */
	*flag = (long)GTE_ReadCtrl(31);
	return (long)(GTE_ReadData(19) >> 2);	/* OTZ = SZ3 >> 2 */
}

extern "C" void RotTransSV(SVECTOR *v0, SVECTOR *v1, long *flag)
{
	loadV0(v0);
	GTE_ExecuteCop2(COP2_RT);
	v1->vx = (short)(int32_t)GTE_ReadData(9);	/* IR1 */
	v1->vy = (short)(int32_t)GTE_ReadData(10);	/* IR2 */
	v1->vz = (short)(int32_t)GTE_ReadData(11);	/* IR3 */
	*flag  = (long)GTE_ReadCtrl(31);
}

extern "C" void RotTrans(SVECTOR *v0, VECTOR *v1, long *flag)
{
	loadV0(v0);
	GTE_ExecuteCop2(COP2_RT);
	v1->vx = (long)(int32_t)GTE_ReadData(25);	/* MAC1 - unclamped */
	v1->vy = (long)(int32_t)GTE_ReadData(26);	/* MAC2 */
	v1->vz = (long)(int32_t)GTE_ReadData(27);	/* MAC3 */
	*flag  = (long)GTE_ReadCtrl(31);
}

/*****************************************************************************/
/*	CPU matrix helpers - PSY-Q fixed-point evaluation order throughout
	(each >>12 lands exactly where the original library truncated).  */

extern "C" MATRIX *RotMatrixZ(long r, MATRIX *m)
{
	short s = (short)rsin((int)r);
	short c = (short)rcos((int)r);

	for (int j = 0; j < 3; j++)
	{
		short t1 = m->m[0][j];
		short t2 = m->m[1][j];
		m->m[0][j] = (short)((t1 * c - t2 * s) >> 12);
		m->m[1][j] = (short)((t1 * s + t2 * c) >> 12);
	}
	return m;
}

extern "C" MATRIX *ScaleMatrix(MATRIX *m, VECTOR *v)
{
	for (int i = 0; i < 3; i++)
	{
		m->m[i][0] = (short)(((int)m->m[i][0] * v->vx) >> 12);
		m->m[i][1] = (short)(((int)m->m[i][1] * v->vy) >> 12);
		m->m[i][2] = (short)(((int)m->m[i][2] * v->vz) >> 12);
	}
	return m;
}

extern "C" MATRIX *TransposeMatrix(MATRIX *m0, MATRIX *m1)
{
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			m1->m[i][j] = m0->m[j][i];
	return m1;
}

extern "C" VECTOR *ApplyMatrix(MATRIX *m, SVECTOR *v0, VECTOR *v1)
{
	v1->vx = ((int)m->m[0][0] * v0->vx + (int)m->m[0][1] * v0->vy
			+ (int)m->m[0][2] * v0->vz) >> 12;
	v1->vy = ((int)m->m[1][0] * v0->vx + (int)m->m[1][1] * v0->vy
			+ (int)m->m[1][2] * v0->vz) >> 12;
	v1->vz = ((int)m->m[2][0] * v0->vx + (int)m->m[2][1] * v0->vy
			+ (int)m->m[2][2] * v0->vz) >> 12;
	return v1;
}

extern "C" MATRIX *RotMatrix_gte(SVECTOR *r, MATRIX *m)
{
	/*	m = Rx(vx) * Ry(vy) * Rz(vz), the library's rotation order, with
		its nested-truncation evaluation.  M4's emulator A/B pass pins
		this against the real library (in-game models are its only user).  */
	int s0 = rsin(r->vx), c0 = rcos(r->vx);
	int s1 = rsin(r->vy), c1 = rcos(r->vy);
	int s2 = rsin(r->vz), c2 = rcos(r->vz);

	m->m[0][0] = (short)((c1 * c2) >> 12);
	m->m[0][1] = (short)(-((c1 * s2) >> 12));
	m->m[0][2] = (short)s1;

	m->m[1][0] = (short)(((((s1 * s0) >> 12) * c2) >> 12) + ((c0 * s2) >> 12));
	m->m[1][1] = (short)(((c0 * c2) >> 12) - ((((s1 * s0) >> 12) * s2) >> 12));
	m->m[1][2] = (short)(-((s0 * c1) >> 12));

	m->m[2][0] = (short)(((s0 * s2) >> 12) - ((((s1 * c0) >> 12) * c2) >> 12));
	m->m[2][1] = (short)(((((s1 * c0) >> 12) * s2) >> 12) + ((s0 * c2) >> 12));
	m->m[2][2] = (short)((c0 * c1) >> 12);
	return m;
}
