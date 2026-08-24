/***************************/
/*** 3d Tile Layer Class ***/
/***************************/

#ifndef __LAYER_TILE_3D_Hx__
#define __LAYER_TILE_3D_Hx__


#include	"system\asmport.h"

#ifdef	PSX_MIPS_ASM

#define CMX_SetRotMatrixXY( r0 ) __asm__  (       \
    "lw $12, 0( %0 );"                  \
    "lw $13, 4( %0 );"                  \
    "ctc2   $12, $0;"                   \
    "ctc2   $13, $2;"                   \
    :                           \
    : "r"( r0 )                     \
    : "$12", "$13")

#else	/* PSX_MIPS_ASM */

/*	Portable equivalent: ctrl regs 0,2 = packed R11R12 and R22R23 pairs.
	Reads s16 Mtx[4] as two packed 32-bit words like the original lw pairs
	(reinterpreting read: the game target builds -fno-strict-aliasing).  */
inline void	CMX_SetRotMatrixXYF(const void *r0)
{
const u32	*w=(const u32*)r0;
	GTEport_SetCtrl(0,w[0]);
	GTEport_SetCtrl(2,w[1]);
}
#define CMX_SetRotMatrixXY(r0)	CMX_SetRotMatrixXYF((const void*)(r0))

#endif	/* PSX_MIPS_ASM */

struct	sFlipTable
{
	s16		Mtx[4];				// 8
	DVECTOR	*DeltaTab[8];		// 32
	s32		ClipCode;			// 4
	s8		Pad[20];
};
extern	sFlipTable	FlipTable[];

/*****************************************************************************/
class	FontBank;
class CLayerTile3d : public CLayerTile
{
public:
		CLayerTile3d(sLevelHdr *LevelHdr,sLayerHdr *Hdr,u8 *_RGBMap,u8 *_RGBTable);
		~CLayerTile3d();

		void			init(DVECTOR &MapPos,int Shift);
		void			shutdown();
		void			think(DVECTOR &MapPos);
		void			render();

protected:
		void			CacheElemVtx(sElem3d *Elem);
		void			CalcDelta();

		sElem3d			*ElemBank3d;
		sTri			*TriList;
		sQuad			*QuadList;
		sVtx			*VtxList;
		u16				*VtxIdxList;
		DVECTOR			RenderOfs;
		u8				*RGBMap;
		u8				*RGBTable;

		s16				*DeltaTableX[16];
		s16				*DeltaTableY[16];
//		s16				*BTableX[16];
//		s16				*BTableY[16];
		DVECTOR			DeltaFOfs;
		DVECTOR			DeltaBOfs;
		s16				DeltaF,DeltaB;


};



/*****************************************************************************/

#endif