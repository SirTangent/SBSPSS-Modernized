#version 450
// Samples emulated VRAM (R16_UINT, 1024x512) and unpacks the PS1 15-bit
// MBBBBBGGGGGRRRRR layout (R in the low bits).  disp = DISPENV rect in VRAM;
// mask = SetDispMask state (0 -> video blanked).
layout(binding = 0) uniform usampler2D vram;
layout(push_constant) uniform PC { ivec4 disp; int mask; } pc;

layout(location = 0) in  vec2 uv;
layout(location = 0) out vec4 col;

void main()
{
    if (pc.mask == 0)
    {
        col = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    ivec2 p = ivec2(uv * vec2(pc.disp.zw));
    p = clamp(p, ivec2(0), pc.disp.zw - ivec2(1));
    ivec2 v = (pc.disp.xy + p) & ivec2(1023, 511);
    uint px = texelFetch(vram, v, 0).r;
    col = vec4(float( px         & 31u) / 31.0,
               float((px >>  5u) & 31u) / 31.0,
               float((px >> 10u) & 31u) / 31.0,
               1.0);
}
