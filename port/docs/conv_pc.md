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

10. **No-op `typedef` keywords dropped** (13 sites): `typedef struct/enum NAME
    {...};` with no declarator name — the keyword was silently ignored by EGCS
    and warned per-including-TU by modern GCC (~2000 warnings).
    `source/sound/xmplay.h:115,123`, `source/sound/sound.h:44,57,62,189,201,207`,
    `source/sound/spu.h:32`, `source/gfx/bubicles.h:47`,
    `source/sound/sound.cpp:54,61,65`.

11. **Default arguments repeated on definitions removed** (4 more sites, same
    class as #6; the declarations keep them): `sound.cpp:638` (`playSfx`),
    `layercollision.cpp:134` (`getHeightFromGroundExcluding`),
    `saveload.cpp:152` (`startSave`), `player.cpp:1732` (`addSpatula`).

12. **`source/system/asmport.h` (post-review)** — the software-GTE contract
    corrected and completed before freezing: `GTEport_Op` documented as
    receiving the DMPSX tag words the vintage `INLINE_C.H` macros embed (not
    the raw 25-bit cop2 immediate the first draft claimed); `GTEport_GetCtrl`
    (cfc2) moved here from the shadow `inline_c.h` so the interface has one
    home; `PORT_Scratchpad` declared here (portable branch only) so game code
    and shim share one declaration instead of three raw externs.

13. **Warning-free pass — two latent bugs fixed** (the PC build is now 0
    warnings in both variants; these are behavioural fixes, not cosmetics):
    - `source/hazard/hrckshrd.cpp:127` and `source/platform/pfishhk.cpp:172`
      — `getThinkBBox()` returned `&objThinkBox`, the address of a stack
      local.  Dangling on PS1 too; it worked only because every caller reads
      through the pointer before the stack is reused.  Now `static`, matching
      the base class (`thing.h:191` returns the member `m_collisionArea`).
    - `source/game/gameslot.h:184` — `getHighestLevelOpen()` wrote its
      out-params only inside `if(isLevelOpen(...))`, so with no level open
      the callers (`start.cpp:302`, `map.h:51`) read uninitialised stack
      ints.  Defaults to chapter 0 / level 0 now.
    - `source/gfx/animtex.cpp:61` — the `default:` (unknown pixel depth) case
      only `ASSERT`ed, and `ASSERT` compiles away in FINAL, leaving
      `PixPerWord` uninitialised as the divisor two lines later.  Sets a safe
      `PixPerWord=1` first.  *(Surfaced only by the FINAL variant's heavier
      inlining — a good argument for building both.)*

14. **Warning-free pass — non-behavioural** (same batch as #13):
    - `source/gui/gui.h:65` — `virtual ~CGUIObject() {}` added: `shutdown()`
      does `delete this` on a polymorphic hierarchy.  No class in the
      hierarchy declares a destructor, so this only fixes dispatch.
    - `source/fileio/fileio.cpp:384,399` — `loadDataBank`/`dumpDataBank`
      index `DataBank[DATABANK_MAX]` where `DATABANK_MAX==0` (a zero-length
      array).  Neither function has any caller; a `if (DATABANK_MAX==0)
      return;` guard makes that explicit and folds the dead indexing away.
    - `source/backend/credits.cpp:85` — `enum {...} CREDIT_CONTROL;`
      accidentally declared a *global variable* of unnamed enum type (used
      nowhere; only the `CC_*` constants are).  Now `enum CREDIT_CONTROL
      {...};`.
    - `source/system/clickcount.cpp:64` — the `OpenEvent` handler cast is
      dual-pathed on `PSX_MIPS_ASM`: EGCS insists on `(long (*)(...))`,
      which modern GCC rejects, and vice versa.

## Game-source changes (M3)

15. **`source/system/main.cpp:89` (USE_SCREEN_UTILS gate)** � the DEBUG
    screen utils (SELECT=VRamViewer, L2+START=SaveScreen) were gated on
    `__FILE_SYSTEM__==PC && !__USER_CDBUILD__`, i.e. compiled out of every
    CD build.  Both outer conditions gained a `|| !defined(PSX_MIPS_ASM)`
    arm so the Win32 port's DEBUG variant keeps them (the shim implements
    the libsn `PC*` file calls SaveScreen needs - `port/psyq/sn/
    pcfile.cpp`).  On the PlayStation build `PSX_MIPS_ASM` is defined, so
    both conditions reduce to the originals - verified by the PSX
    regression build.

## Not changed (accepted by `-fpermissive -std=gnu++98`)

- String-literal → `char*` conversions (pervasive; `-Wno-write-strings`).
- Zero-length array `DataBank[DATABANK_MAX==0]` (`fileio.h:68`) — a GNU
  extension modern GCC still accepts in gnu++ mode.
- `operator new(size_t, const char* = NULL)` overload set (`mem/memory.h`) —
  feared ambiguous against the implicit `::operator new`, but GCC 16 resolves
  the game's `new ("name") T` and plain `new T` correctly under gnu++98.
- Backslash `#include` paths and mixed-case generated-header names (NTFS).

## Warning policy (PC game target)

Style classes pervasive in the 1999 code are suppressed on the game target
only (see `SBSP_GAME_CXX_FLAGS` in `port/CMakeLists.txt`): narrowing,
overloaded-virtual, non-c-typedef-for-linkage, parentheses, char-subscripts,
sign-compare, misleading-indentation, int-in-bool-context, dangling-else,
header-guard, unused, write-strings.  The shim keeps plain `-Wall`.

Every warning those flags do *not* suppress has since been fixed at its
source (entries #13/#14 above), so **both variants build with 0 warnings and
0 errors**.  Keep it that way: a new warning now means new code, not
inherited noise.
