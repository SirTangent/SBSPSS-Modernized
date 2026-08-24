/*	Software rasterizer - triangles, rects, lines into the 15-bit VRAM.

	Conventions (PS1):
	- Right/bottom edges excluded (top-left fill rule); quads arrive as two
	  triangles (012, 123) from the interpreter, matching hardware.
	- A triangle whose bounding box exceeds 1023x511 is not drawn.
	- Texel 0x0000 is fully transparent.  Modulation is
	  out5 = min(31, (tex5 * col8) >> 7); flat colour converts 8->5 bits.
	- Semi-transparency (four ABR modes) applies to all pixels of untextured
	  prims, and only to texels with the STP bit for textured ones.
	- Written pixels carry the texel's STP bit (0 for untextured).
	- Dithering: not yet (M3 - nothing in the M2 window needs it).
*/
#include <string.h>

#include "gpu/gpu_core.h"

/*****************************************************************************/
static inline uint16_t sampleTexel(int u, int v, const RasterCfg *cfg)
{
	u &= 0xFF;
	v &= 0xFF;
	int ty = (cfg->texBaseY + v) & 0x1FF;
	switch (cfg->texDepth)
	{
	case 0:	/* 4bpp: 4 texels per halfword */
	{
		uint16_t cell = g_vram[ty][(cfg->texBaseX + (u >> 2)) & 0x3FF];
		int idx = (cell >> ((u & 3) << 2)) & 0xF;
		return g_vram[cfg->clutY][(cfg->clutX + idx) & 0x3FF];
	}
	case 1:	/* 8bpp: 2 texels per halfword */
	{
		uint16_t cell = g_vram[ty][(cfg->texBaseX + (u >> 1)) & 0x3FF];
		int idx = (cell >> ((u & 1) << 3)) & 0xFF;
		return g_vram[cfg->clutY][(cfg->clutX + idx) & 0x3FF];
	}
	default:	/* 15bpp direct */
		return g_vram[ty][(cfg->texBaseX + u) & 0x3FF];
	}
}

static inline int mod5(int tex5, int col8)
{
	int v = (tex5 * col8) >> 7;
	return v > 31 ? 31 : v;
}

static inline uint16_t blendSemi(uint16_t back, int fr, int fg, int fb, int mode)
{
	int br = back & 0x1F, bg = (back >> 5) & 0x1F, bb = (back >> 10) & 0x1F;
	int r, g, b;
	switch (mode)
	{
	case 0:  r = (br + fr) >> 1;  g = (bg + fg) >> 1;  b = (bb + fb) >> 1;  break;
	case 1:  r = br + fr;         g = bg + fg;         b = bb + fb;         break;
	case 2:  r = br - fr;         g = bg - fg;         b = bb - fb;         break;
	default: r = br + (fr >> 2);  g = bg + (fg >> 2);  b = bb + (fb >> 2);  break;
	}
	if (r < 0) r = 0; else if (r > 31) r = 31;
	if (g < 0) g = 0; else if (g > 31) g = 31;
	if (b < 0) b = 0; else if (b > 31) b = 31;
	return (uint16_t)(r | (g << 5) | (b << 10));
}

/*	The shared per-pixel pipeline.  Returns without writing when the pixel
	is transparent.  cr/cg/cb are the 8-bit vertex colour at this pixel.  */
static inline void shadePixel(int x, int y, int u, int v,
							  int cr, int cg, int cb, const RasterCfg *cfg)
{
	int fr, fg, fb, stp = 0;

	if (cfg->textured)
	{
		uint16_t tex = sampleTexel(u, v, cfg);
		if (tex == 0)
			return;						/* transparent black */
		stp = tex >> 15;
		int tr = tex & 0x1F, tg = (tex >> 5) & 0x1F, tb = (tex >> 10) & 0x1F;
		if (cfg->rawTex)
		{
			fr = tr;  fg = tg;  fb = tb;
		}
		else
		{
			fr = mod5(tr, cr);  fg = mod5(tg, cg);  fb = mod5(tb, cb);
		}
	}
	else
	{
		fr = cr >> 3;  fg = cg >> 3;  fb = cb >> 3;
	}

	uint16_t out;
	if (cfg->semi && (!cfg->textured || stp))
		out = (uint16_t)(blendSemi(g_vram[y][x], fr, fg, fb, cfg->semiMode)
						 | (stp << 15));
	else
		out = (uint16_t)(fr | (fg << 5) | (fb << 10) | (stp << 15));

	g_vram[y][x] = out;
}

/*****************************************************************************/
void Raster_Triangle(const RasterVtx *v0, const RasterVtx *v1, const RasterVtx *v2,
					 const RasterCfg *cfg)
{
	const RasterVtx *a = v0, *b = v1, *c = v2;

	/* size reject on the raw bounding box */
	{
		int minx = a->x < b->x ? (a->x < c->x ? a->x : c->x) : (b->x < c->x ? b->x : c->x);
		int maxx = a->x > b->x ? (a->x > c->x ? a->x : c->x) : (b->x > c->x ? b->x : c->x);
		int miny = a->y < b->y ? (a->y < c->y ? a->y : c->y) : (b->y < c->y ? b->y : c->y);
		int maxy = a->y > b->y ? (a->y > c->y ? a->y : c->y) : (b->y > c->y ? b->y : c->y);
		if (maxx - minx > 1023 || maxy - miny > 511)
			return;
	}

	/* wind consistently (PS1 draws both windings) */
	long area = (long)(b->x - a->x) * (c->y - a->y)
			  - (long)(b->y - a->y) * (c->x - a->x);
	if (area == 0)
		return;
	if (area < 0)
	{
		const RasterVtx *t = b;  b = c;  c = t;
		area = -area;
	}

	int minx = a->x < b->x ? (a->x < c->x ? a->x : c->x) : (b->x < c->x ? b->x : c->x);
	int maxx = a->x > b->x ? (a->x > c->x ? a->x : c->x) : (b->x > c->x ? b->x : c->x);
	int miny = a->y < b->y ? (a->y < c->y ? a->y : c->y) : (b->y < c->y ? b->y : c->y);
	int maxy = a->y > b->y ? (a->y > c->y ? a->y : c->y) : (b->y > c->y ? b->y : c->y);

	if (minx < g_gpu.clipX0) minx = g_gpu.clipX0;
	if (miny < g_gpu.clipY0) miny = g_gpu.clipY0;
	if (maxx > g_gpu.clipX1) maxx = g_gpu.clipX1;
	if (maxy > g_gpu.clipY1) maxy = g_gpu.clipY1;
	if (minx < 0) minx = 0;
	if (miny < 0) miny = 0;
	if (maxx > VRAM_W - 1) maxx = VRAM_W - 1;
	if (maxy > VRAM_H - 1) maxy = VRAM_H - 1;
	if (minx > maxx || miny > maxy)
		return;

	/*	Edge functions (interior positive).  Top-left fill rule in y-down
		coordinates: LEFT edges have A > 0 (interior to their right), TOP
		edges are horizontal with the interior below (A == 0, B > 0) - those
		are inclusive; right/bottom edges get a -1 bias so shared boundary
		pixels belong to exactly one primitive.  */
	long A0 = b->y - c->y, B0 = c->x - b->x, C0 = (long)b->x * c->y - (long)b->y * c->x;
	long A1 = c->y - a->y, B1 = a->x - c->x, C1 = (long)c->x * a->y - (long)c->y * a->x;
	long A2 = a->y - b->y, B2 = b->x - a->x, C2 = (long)a->x * b->y - (long)a->y * b->x;

	long bias0 = (A0 > 0 || (A0 == 0 && B0 > 0)) ? 0 : -1;
	long bias1 = (A1 > 0 || (A1 == 0 && B1 > 0)) ? 0 : -1;
	long bias2 = (A2 > 0 || (A2 == 0 && B2 > 0)) ? 0 : -1;

	for (int y = miny; y <= maxy; y++)
	{
		long w0 = A0 * minx + B0 * y + C0 + bias0;
		long w1 = A1 * minx + B1 * y + C1 + bias1;
		long w2 = A2 * minx + B2 * y + C2 + bias2;

		for (int x = minx; x <= maxx; x++, w0 += A0, w1 += A1, w2 += A2)
		{
			if ((w0 | w1 | w2) < 0)
				continue;

			/* barycentric weights of a,b,c (undo the bias for interpolation) */
			long ua = w0 - bias0, ub = w1 - bias1, uc = w2 - bias2;

			int u = 0, v = 0, cr, cg, cb;
			if (cfg->textured)
			{
				u = (int)((ua * a->u + ub * b->u + uc * c->u) / area);
				v = (int)((ua * a->v + ub * b->v + uc * c->v) / area);
			}
			if (cfg->gouraud)
			{
				cr = (int)((ua * a->r + ub * b->r + uc * c->r) / area);
				cg = (int)((ua * a->g + ub * b->g + uc * c->g) / area);
				cb = (int)((ua * a->b + ub * b->b + uc * c->b) / area);
			}
			else
			{
				cr = a->r;  cg = a->g;  cb = a->b;
			}

			shadePixel(x, y, u, v, cr, cg, cb, cfg);
		}
	}
}

/*****************************************************************************/
void Raster_Rect(int x, int y, int w, int h, int u0, int v0,
				 uint8_t r, uint8_t g, uint8_t b, const RasterCfg *cfg)
{
	if (w > 1023 || h > 511)
		return;

	int x0 = x, y0 = y;
	int x1 = x + w - 1, y1 = y + h - 1;
	int ustart = u0, vstart = v0;

	if (x0 < g_gpu.clipX0) { ustart += g_gpu.clipX0 - x0; x0 = g_gpu.clipX0; }
	if (y0 < g_gpu.clipY0) { vstart += g_gpu.clipY0 - y0; y0 = g_gpu.clipY0; }
	if (x1 > g_gpu.clipX1) x1 = g_gpu.clipX1;
	if (y1 > g_gpu.clipY1) y1 = g_gpu.clipY1;
	if (x0 < 0) { ustart += -x0; x0 = 0; }
	if (y0 < 0) { vstart += -y0; y0 = 0; }
	if (x1 > VRAM_W - 1) x1 = VRAM_W - 1;
	if (y1 > VRAM_H - 1) y1 = VRAM_H - 1;

	for (int py = y0, v = vstart; py <= y1; py++, v++)
		for (int px = x0, u = ustart; px <= x1; px++, u++)
			shadePixel(px, py, u, v, r, g, b, cfg);
}

/*****************************************************************************/
void Raster_Line(const RasterVtx *pa, const RasterVtx *pb, const RasterCfg *cfg)
{
	int x0 = pa->x, y0 = pa->y, x1 = pb->x, y1 = pb->y;

	int dx = x1 > x0 ? x1 - x0 : x0 - x1;
	int dy = y1 > y0 ? y1 - y0 : y0 - y1;
	if (dx > 1023 || dy > 511)
		return;

	int sx = x0 < x1 ? 1 : -1;
	int sy = y0 < y1 ? 1 : -1;
	int err = dx - dy;
	int steps = (dx > dy ? dx : dy);
	int n = 0;

	for (;;)
	{
		if (x0 >= g_gpu.clipX0 && x0 <= g_gpu.clipX1 &&
			y0 >= g_gpu.clipY0 && y0 <= g_gpu.clipY1 &&
			x0 >= 0 && x0 < VRAM_W && y0 >= 0 && y0 < VRAM_H)
		{
			int cr, cg, cb;
			if (cfg->gouraud && steps > 0)
			{
				cr = pa->r + (pb->r - pa->r) * n / steps;
				cg = pa->g + (pb->g - pa->g) * n / steps;
				cb = pa->b + (pb->b - pa->b) * n / steps;
			}
			else
			{
				cr = pa->r;  cg = pa->g;  cb = pa->b;
			}
			int fr = cr >> 3, fg = cg >> 3, fb = cb >> 3;
			uint16_t out;
			if (cfg->semi)
				out = blendSemi(g_vram[y0][x0], fr, fg, fb, cfg->semiMode);
			else
				out = (uint16_t)(fr | (fg << 5) | (fb << 10));
			g_vram[y0][x0] = out;
		}

		if (x0 == x1 && y0 == y1)
			break;
		int e2 = err << 1;
		if (e2 > -dy) { err -= dy; x0 += sx; }
		if (e2 <  dx) { err += dx; y0 += sy; }
		n++;
	}
}
