/*	Software-GTE register interface (see source/system/asmport.h and the
	portable port/include/inline_c.h, whose macros funnel every cop2 access
	through these four calls).

	GTEport_Op receives DMPSX TAG WORDS (op index = tag>>6; gte_mvmva ORs
	its sf/mx/v/cv/lm fields in at <<25/<<23/<<21/<<19/<<18).  The table
	below maps each tag index to the hardware cop2 instruction word - it
	was extracted from DMPSX.EXE itself (tools/psyq/bin, table at file
	offset 0x823, 24-byte records), so it is the same mapping the original
	MIPS build was patched with.  gte_core.cpp executes the hardware word.
*/
#include "system/types.h"
#include "stub_log.h"
#include "gte_core.h"

extern "C" {
u32  GTEport_GetData(int reg);
void GTEport_SetData(int reg, u32 v);
u32  GTEport_GetCtrl(int reg);		/* cfc2 - declared by port/include/inline_c.h */
void GTEport_SetCtrl(int reg, u32 v);
void GTEport_Op(u32 op);
}

/*	Tag index -> cop2 word.  Index 0 unused; 17-20/34-37/51-54 are the
	far-colour MVMVA presets DMPSX defines but the portable inline_c.h
	never names; 78 is the parameterised gte_mvmva base.  */
static const uint32_t s_tagToCop2[79] =
{
	0,
	COP2_RTPS,	/*  1 rtps      */
	0x4A280030,	/*  2 rtpt      */
	COP2_RT,	/*  3 rt        = MVMVA sf=1 RT*V0+TR  */
	0x4A486012,	/*  4 rtv0      */
	0x4A48E012,	/*  5 rtv1      */
	0x4A496012,	/*  6 rtv2      */
	0x4A49E012,	/*  7 rtir      */
	0x4A41E012,	/*  8 rtir_sf0  */
	COP2_RT,	/*  9 rtv0tr    */
	0x4A488012,	/* 10 rtv1tr    */
	0x4A490012,	/* 11 rtv2tr    */
	0x4A498012,	/* 12 rtirtr    */
	0x4A482012,	/* 13 rtv0bk    */
	0x4A48A012,	/* 14 rtv1bk    */
	0x4A492012,	/* 15 rtv2bk    */
	0x4A49A012,	/* 16 rtirbk    */
	0x4A484012,	/* 17 rtv0fc    */
	0x4A48C012,	/* 18 rtv1fc    */
	0x4A494012,	/* 19 rtv2fc    */
	0x4A49C012,	/* 20 rtirfc    */
	0x4A4A6412,	/* 21 ll        = MVMVA sf=1 LLM*V0 lm=1 */
	0x4A4A6012,	/* 22 llv0      */
	0x4A4AE012,	/* 23 llv1      */
	0x4A4B6012,	/* 24 llv2      */
	0x4A4BE012,	/* 25 llir      */
	0x4A4A0012,	/* 26 llv0tr    */
	0x4A4A8012,	/* 27 llv1tr    */
	0x4A4B0012,	/* 28 llv2tr    */
	0x4A4B8012,	/* 29 llirtr    */
	0x4A4A2012,	/* 30 llv0bk    */
	0x4A4AA012,	/* 31 llv1bk    */
	0x4A4B2012,	/* 32 llv2bk    */
	0x4A4BA012,	/* 33 llirbk    */
	0x4A4A4012,	/* 34 llv0fc    */
	0x4A4AC012,	/* 35 llv1fc    */
	0x4A4B4012,	/* 36 llv2fc    */
	0x4A4BC012,	/* 37 llirfc    */
	0x4A4DA412,	/* 38 lc        = MVMVA sf=1 LCM*IR+BK lm=1 */
	0x4A4C6012,	/* 39 lcv0      */
	0x4A4CE012,	/* 40 lcv1      */
	0x4A4D6012,	/* 41 lcv2      */
	0x4A4DE012,	/* 42 lcir      */
	0x4A4C0012,	/* 43 lcv0tr    */
	0x4A4C8012,	/* 44 lcv1tr    */
	0x4A4D0012,	/* 45 lcv2tr    */
	0x4A4D8012,	/* 46 lcirtr    */
	0x4A4C2012,	/* 47 lcv0bk    */
	0x4A4CA012,	/* 48 lcv1bk    */
	0x4A4D2012,	/* 49 lcv2bk    */
	0x4A4DA012,	/* 50 lcirbk    */
	0x4A4C4012,	/* 51 lcv0fc    */
	0x4A4CC012,	/* 52 lcv1fc    */
	0x4A4D4012,	/* 53 lcv2fc    */
	0x4A4DC012,	/* 54 lcirfc    */
	0x4A680029,	/* 55 dpcl      */
	0x4A780010,	/* 56 dpcs      */
	0x4AF8002A,	/* 57 dpct      */
	0x4A980011,	/* 58 intpl     */
	0x4AA80428,	/* 59 sqr12     */
	0x4AA00428,	/* 60 sqr0      */
	0x4AC8041E,	/* 61 ncs       */
	0x4AD80420,	/* 62 nct       */
	0x4AE80413,	/* 63 ncds      */
	0x4AF80416,	/* 64 ncdt      */
	0x4B08041B,	/* 65 nccs      */
	0x4B18043F,	/* 66 ncct      */
	0x4B280414,	/* 67 cdp       */
	0x4B38041C,	/* 68 cc        */
	0x4B400006,	/* 69 nclip     */
	0x4B58002D,	/* 70 avsz3     */
	0x4B68002E,	/* 71 avsz4     */
	0x4B78000C,	/* 72 op12      */
	0x4B70000C,	/* 73 op0       */
	0x4B98003D,	/* 74 gpf12     */
	0x4B90003D,	/* 75 gpf0      */
	0x4BA8003E,	/* 76 gpl12     */
	0x4BA0003E,	/* 77 gpl0      */
	0x4A400012,	/* 78 mvmva base - fields ORed in from the tag */
};

extern "C" u32 GTEport_GetData(int reg)
{
	return GTE_ReadData(reg);
}

extern "C" void GTEport_SetData(int reg, u32 v)
{
	GTE_WriteData(reg, v);
}

extern "C" u32 GTEport_GetCtrl(int reg)
{
	return GTE_ReadCtrl(reg);
}

extern "C" void GTEport_SetCtrl(int reg, u32 v)
{
	GTE_WriteCtrl(reg, v);
}

extern "C" void GTEport_Op(u32 op)
{
	unsigned idx = (op >> 6) & 0x7F;	/* mvmva field bits land above bit 12 */
	if (idx == 0 || idx > 78)
	{
		PSYQ_STUB_ONCE();				/* not a DMPSX tag - nothing emits these */
		return;
	}
	uint32_t inst = s_tagToCop2[idx];
	if (idx == 78)
		inst |= ((op >> 25) & 1) << 19		/* sf */
			  | ((op >> 23) & 3) << 17		/* mx */
			  | ((op >> 21) & 3) << 15		/* v  */
			  | ((op >> 19) & 3) << 13		/* cv */
			  | ((op >> 18) & 1) << 10;		/* lm */
	GTE_ExecuteCop2(inst);
}
