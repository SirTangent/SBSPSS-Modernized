# PC (Win32) conversion log — mechanical game-source changes

Companion to `Docs/conv_mips.md` (local notes dir, untracked; it covers the GNU/MIPS inline-asm dual-pathing).
This file logs every change made to game code (`source/`, `tools/`) for the PC
build, per the "mechanical fixes only, logged" policy.  Every entry compiles
identically on the PS1 toolchain — `port/build-psx.cmd` is re-run after each
batch as the regression guard.

The PC build itself lives in `port/` (CMake + MSYS2 MinGW-w64 i686 GCC; see
`port/CMakeLists.txt`).  Shim/runtime code and the shadow SDK headers are
new files under `port/` and are not listed here.

## Include strategy (context for the entries below)

The PC compile keeps Sony's own SDK headers authoritative for type layouts:
`tools/psyq/include` sits at the *end* of the search path (`-idirafter`), so
MinGW's real `stddef.h`/`stdlib.h`/`stdio.h` win, and `port/include` shadows
only the headers that cannot work on x86:

| shadow | reason |
|---|---|
| `inline_c.h` | MIPS `lwc2/swc2/mtc2/cop2` macro bodies → portable `GTEport_*` translation (all 245 macros, register-for-register; `gtemac.h` is pure composition over these and passes through unchanged) |
| `libsn.h` | `pollhost()`/`PSYQpause()` are MIPS `break` instructions → no-op / `__builtin_trap()` |
| `libapi.h` | declares `rename()` and `_get_errno()` at signatures that clash with the MinGW CRT (game calls neither) → renamed out of the way, rest passes through |
| `strings.h`, `memory.h` | vintage headers declare `memcpy`/`strcpy`/`strlen` with empty parameter lists ("to avoid conflicting"), which C++ reads as zero-arg → forward to `<string.h>`; `strings.h` also restores the vintage guarantee that `sys/types.h` precedes the SDK block, and adds `abs(unsigned)` overloads reproducing the vintage `abs(int)` implicit conversion |
| `sys/types.h` | provides the BSD `u_char/u_short/u_int/u_long/ushort` typedefs the SDK headers use, without the vintage `time_t`/`off_t`/`size_t` clashes |

## Game-source changes (M1)

1. **`source/system/asmport.h`, `source/system/gte.h`, `source/utils/cmxmacro.h`,
   `source/level/layertile3d.h`** — portable GTE shim widths `unsigned long`→`u32`,
   magnitude stores through the caller's pointer type, `GTEport_Op` declared
   (issues #12/#13; commit `M1: fix portable GTE shim widths to u32`).

2. **`source/system/global.h`** — `SCRATCH_RAM` gated on `PSX_MIPS_ASM`:
   the PS1 keeps the literal `0x1f800000`; the PC path points at the shim's
   real 1KB buffer (`PORT_Scratchpad`).  Needed because `layertile3d.cpp`
   bakes `(DVECTOR*)SCRATCH_RAM + n` into namespace-scope initialisers, so it
   must stay an address-constant expression.

3. **`source/gfx/animtex.h:18`** — `AddAnimTex(sFrameHdr *Frame,int Frame,...)`
   declared two parameters both named `Frame` (EGCS tolerated it); the second
   is now `FrameNo`, matching the definition in `animtex.cpp:37`.

4. **`source/player/player.h:355`** — `const struct AnimFrameSfx *` named a
   typedef of an anonymous struct after `struct` (ill-formed); dropped the
   `struct` keyword.

5. **`source/player/player.h` (prompt block)** — `sPromptData`/`sPromptTable`
   moved from `private` to `public`: `player.cpp` defines the prompt tables at
   namespace scope (`player.cpp:3281+`), which EGCS let reach private nested
   types.  No layout or behaviour change.

6. **`source/player/player.cpp:1686`** — `getHeightFromGroundNoPlatform`
   repeated the `_maxHeight=32` default argument on the definition (already on
   the declaration, `player.h:250`); removed from the definition.

7. **`source/pickups/pickup.h:81`** — `const struct DVECTOR *` → `const
   DVECTOR *` (same typedef-after-`struct` issue as #4).

8. **`source/game/gamebubs.h:53`** — `static struct BubicleEmitterData` →
   `static BubicleEmitterData` (same).

9. **`tools/vlc/include/VLC_BIT.H:13`** — `void DecDCTvlcBuild3();` given its
   real prototype `(unsigned short *table)`: `fmv.cpp:199` passes the table,
   and in C++ the empty parens mean zero-arg.  Matches the MIPS `VLC_BIT.O`
   ABI (table pointer in `$a0`).

## Not changed (accepted by `-fpermissive -std=gnu++98`)

- String-literal → `char*` conversions (pervasive; `-Wno-write-strings`).
- Zero-length array `DataBank[DATABANK_MAX==0]` (`fileio.h:68`) — a GNU
  extension modern GCC still accepts in gnu++ mode.
- `operator new(size_t, const char* = NULL)` overload set (`mem/memory.h`) —
  feared ambiguous against the implicit `::operator new`, but GCC 16 resolves
  the game's `new ("name") T` and plain `new T` correctly under gnu++98.
- Backslash `#include` paths and mixed-case generated-header names (NTFS).

## Residual PC-only diagnostics worth knowing about

- `-Wnarrowing` warnings (C++11-hostile `{int → short}` initialisers) — a
  handful across the tree, warnings only under gnu++98.
- `xmplay.h:115` "typedef was ignored" — vintage header quirk, harmless.
