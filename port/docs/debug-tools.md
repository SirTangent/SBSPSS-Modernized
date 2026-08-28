# Debug &amp; diagnostic tools (PC port)

Everything the Win32/Vulkan port offers for looking inside a running game, in
the order you would normally reach for it. Commands are PowerShell, run from
the repository root.

Related: [`conv_pc.md`](conv_pc.md) (the log of mechanical `source/` edits).


## 1. Which build to run

```powershell
port\build-pc.cmd debug      # or: final, all
```

Only the **DEBUG** variant (`port\build\debug\sbsp.exe`) defines
`__VERSION_DEBUG__`, which is what gates `ASSERT`, the `SYSTEM_DBGMSG`
channel, VRamViewer and SaveScreen. `final\sbsp.exe` is the shipping build and
has none of them.

Run from the **repo root**: the CD path is relative
(`out/<TERRITORY>/<VERSION>/version/CD/BIGLUMP.BIN`). FINAL looks for
`out/USA/FINAL/...`, which does not exist in this tree - only the DEBUG data
has been built - so the FINAL exe needs `SBSP_DATA_DIR` (see §4) or it exits 3
at `CdInit`.


## 2. In-game keys (DEBUG only)

Polled at the bottom of `MainLoop` (`source/system/main.cpp:243-246`), so they
work only while the game is in its normal loop, not during a blocking load.

| Key | PS1 button | Effect |
|---|---|---|
| **RShift** (hold) | SELECT | **VRamViewer** - displays raw VRAM; arrows pan the 512x256 window over the 1024x512 atlas. Release RShift to exit; the previous display origin is restored. |
| **E** + **Enter** | L2 + START | **SaveScreen** - writes `SBSP0000.tga`, `SBSP0001.tga`, ... to the **current working directory**: 24-bit TGA of the current draw clip (512x256). |

VRamViewer's own loop (`source/system/vid.cpp:461`) calls no `VSync`; it stays
alive only because the shim's `PadGetState` pumps. It spins a core while open -
expected, it is a debug tool.


## 3. Boot arguments (M4)

`sbsp.exe --help` prints the full surface.  The headline one:

```powershell
port\build\debug\sbsp.exe --level 1-2        # boot straight into Chapter 1 Level 2
```

`--level C-L` takes chapter 1-5, level 1-5 (level 5 is that chapter's bonus
level; `6-N` also addresses bonus level N), or a raw `LvlTable` index 0-24.
It skips the frontend entirely - `InitSystem` runs, then the scene select
jumps straight to `GameScene` (the same shape as the vintage
`__USER_daveo__` dev build).  Env equivalent: `SBSP_BOOT_LEVEL`.

The remaining arguments are aliases for the environment variables in §4 -
same names without the prefix: `--data-dir`, `--pad-script`,
`--dump-frames`, `--dump-dir`, `--exit-after`, `--dump-audio`,
`--no-cd-pace`, `--no-audio`, and `--pace-log` (§6).  Both `--flag value`
and `--flag=value` work; an argument overrides an inherited env var.
Unknown arguments warn and are ignored.

Implementation: `port/psyq/host/args.cpp` (parsed before any static
initialiser consumes its configuration) + the `Port_BootLevel` hook in
`source/system/main.cpp` (conv_pc.md entry 16).

## 4. Environment variables

All optional, all read once at startup.

| Var | Meaning |
|---|---|
| `SBSP_DATA_DIR` | Override the CD data directory. Needed to run the FINAL exe: `out/USA/DEBUG/version/CD`. |
| `SBSP_PAD_SCRIPT` | Inject controller input at given vblanks (§5). |
| `SBSP_DUMP_FRAMES` | Comma-separated vblank numbers; writes the **displayed** VRAM region as a 24-bit BMP at each. Max 16. |
| `SBSP_DUMP_DIR` | Where those BMPs go (default `.`). Files are `sbsp_frame_<vblank>.bmp`. |
| `SBSP_EXIT_AFTER` | Clean `exit(0)` at that vblank. The game's `MainLoop` has no exit path of its own, so scripted runs need this. |
| `SBSP_PRIM_LOG` | Print the prim-pool high-water mark each time it rises (§6). Use it to measure headroom against `MAX_PRIMS` after any change that adds primitives per frame. |
| `SBSP_CD_PACE=0` | Disable the emulated 150 sectors/s double-speed CD pacing -> instant loads. Handy to reach a screen fast; note it also makes the loading icon never appear (the game skips it when zero vblanks elapse between `StartLoad` and `StopLoad`). |
| `SBSP_DUMP_AUDIO` | (M5) Write the SPU mixer output to this WAV path instead of opening a playback device: exactly `44100/hz` frames per emulated vblank, rendered after that vblank's `XM_Update`. Works headless and with no sound device. Two identical runs produce **bit-identical audio modulo a ±1-vblank start offset** (window bring-up races the wall-clock vblank counter) - compare runs aligned at the first non-zero sample, not by raw file hash. Never combine with real playback: each render advances the mixer, so two consumers would each get half the samples (which is why the device is disabled in dump mode). |
| `SBSP_NO_AUDIO=1` | Skip the audio device entirely (no dump either). The SPU/XM machinery still runs. |

The frame dumps read emulated VRAM directly, so they work **even if Vulkan
fails to initialise** - that is the point of them. They capture the DISPENV
crop, i.e. exactly what should be on screen.

Implementation: `port/psyq/host/window.cpp` (dump/exit),
`port/psyq/cd/cd.cpp` (data dir, pacing).


## 5. `SBSP_PAD_SCRIPT` - scripted input

Format: `"vblank:HEXMASK[,vblank:HEXMASK...]"`, up to 64 entries.

The mask is the **active-high** 16-bit `(Button1<<8)|Button2` hardware word
(LIBETC.H order):

| | | | |
|---|---|---|---|
| SELECT `0100` | START `0800` | UP `1000` | RIGHT `2000` |
| DOWN `4000` | LEFT `8000` | L2 `0001` | R2 `0002` |
| L1 `0004` | R1 `0008` | TRIANGLE `0010` | CIRCLE `0020` |
| CROSS `0040` | SQUARE `0080` | | |

An entry is in force from its vblank until a **later-timed** one comes due -
selection is by vblank value, never by position in the string, so an
out-of-order script still behaves sanely.

**Keep taps short - about 12 vblanks.** Menu typematic repeat (DELAY 30 /
RATE 15, scaled by `getFramesSinceLast` which is ~6 in the frontend) fires
`selectNextItem` repeatedly on a long hold, and two-item menus wrap straight
back to where they started.

Full example - boot, skip to the main titles, press START, move down, select,
and capture three frames:

```powershell
$env:SBSP_PAD_SCRIPT = "900:0800,912:0000,1000:4000,1012:0000,1100:0040,1112:0000"
$env:SBSP_DUMP_FRAMES = "880,960,1200"
$env:SBSP_DUMP_DIR    = "$env:TEMP\sbspdump"
$env:SBSP_EXIT_AFTER  = "1300"
port\build\debug\sbsp.exe
```

Clear the variables afterwards or they leak into the next interactive run - a
stuck START is a confusing five minutes:

```powershell
Remove-Item Env:SBSP_PAD_SCRIPT, Env:SBSP_DUMP_FRAMES, Env:SBSP_DUMP_DIR, Env:SBSP_EXIT_AFTER
```

**Do not combine a long pad script with `--no-cd-pace`.** Vblanks are
wall-clock paced, so an instant load consumes however many vblanks the real
file I/O happened to take - which varies run to run. A script that reaches
the map on one run can stall at the title on the next. With CD pacing left
ON, loads take a fixed number of *emulated* vblanks and a script replays
identically; the full frontend route below reproduces exactly that way.
Scripts that only need a few hundred vblanks in one scene are unaffected.

Route that reaches gameplay from a cold boot (new game -> intro FMA ->
map -> Chapter 1 Level 1), alternating START and CROSS every 220 vblanks so
the exact arrival time of each prompt does not matter:

```powershell
$e = @(); $t = 800
for ($n = 0; $n -lt 26; $n++) {
  $m = if ($n % 2 -eq 0) { "0800" } else { "0040" }
  $e += "${t}:$m"; $e += "$($t+12):0000"; $t += 220
}
$env:SBSP_PAD_SCRIPT = ($e -join ",")
port\build\debug\sbsp.exe --exit-after 6600
```

Scene transitions are printed by the game itself on stdout
(`GameState: Opening new scene '...'`), which is the cheapest way to see
how far a scripted run got: expect `FrontEnd -> FMA -> Map -> Game`.

Live keyboard is the RetroArch layout: arrows = D-pad, Z/X/A/S =
Cross/Circle/Square/Triangle, Enter = Start, RShift = Select, Q/W = L1/R1,
E/R = L2/R2. A connected SDL gamepad ORs in on top and hotplugs.

Implementation: `port/psyq/host/input.cpp`.


## 6. Logging

Everything shim-side goes to **stderr** with a bracketed tag, so
`2>&1 | Select-String` filters it:

- `[shim] stub: <name>` - first call to an unimplemented SDK function, once
  per function. **This is the main "what is missing" signal**; if a screen
  misbehaves, check here first.
- `[gpu] unknown/unimplemented GP0 command 0xNN` - once per opcode.
- `[gte] ...`, `[input] gamepad connected: ...`,
  `[input] SBSP_PAD_SCRIPT: N entries`, `[host] wrote ....bmp`,
  `[args] boot level: ...`, `[cd] XA stream chan N: end-of-stream ...`.
- `[spu]` (M5) - software-SPU anomalies: SpuMalloc exhaustion/table-full,
  out-of-range transfers, unimplemented `SpuSetVoiceAttr` mask bits.
- `[xm]` (M5) - XMPlayer anomalies: bad PXM/VH data, slot exhaustion,
  an XM effect outside the implemented (XMPLAY.LIB) set, the deferred
  looping-sample path being reached.  A clean run prints **neither**
  `[spu]` nor `[xm]` lines.
- `[gpu] prim pool high-water N/M bytes` (with `SBSP_PRIM_LOG=1`) - the
  peak bytes used in one prim buffer, sampled in `DrawOTag`, i.e. at the
  exact peak of a frame's fill. **A `[gpu] WARNING: prim pool ...` line
  prints in BOTH variants** once usage passes 87.5%: the game's own
  overflow check (`prim.cpp`'s `ASSERT(!"PRIM OVERFLOW")`) is post-hoc and
  compiles away in FINAL, so without this a pool overrun would silently
  corrupt the heap in the shipping build. Measured peaks are in §10.
- `--pace-log` (or `SBSP_PACE_LOG=1`): every 300 vblanks, `[pace]` prints
  delivered vblanks vs `VSync(0)` waits - `vbl/frame 1.00` is locked
  full-rate; the M3 frontend read ~6 before the present was decoupled from
  emulated time.

Game-side `SYSTEM_DBGMSG` goes to **stdout**, prefixed `file:line`. Only the
`DC_SYSTEM` channel is enabled by default (`source/system/dbg.cpp:74`); call
`setActiveDbgChannels(DC_ALL)` to get `DC_SOUND` / `DC_GUI` / `DC_MEMCARD`
chatter too.

`ASSERT` failures (DEBUG only) print the expression, draw it centred on screen
with the standard font, then hit `PSYQpause()` - which on PC is
`__builtin_trap()` (`port/include/libsn.h`), i.e. a hard trap straight into the
debugger.


## 7. Unit tests

Seven self-contained exes per variant in `port\build\<variant>\`; each exits 0
on pass and prints its own failures:

| Exe | Covers |
|---|---|
| `gte_trig_test.exe` | exact fixed-point `rsin`/`rcos`/`ratan2`/`SquareRoot0` fixtures |
| `gte_test.exe` | software cop2 + libgte fixtures (RTPS/RTPT/NCLIP/MVMVA/AVSZ/UNR division, FLAG bits) |
| `gpu_test.exe` | VRAM rect ops, OT walker shape, rasterizer goldens, polyline terminator cases |
| `adpcm_test.exe` | (M5) SPU ADPCM decoder golden vectors: filters, shift rules, clamping, history |
| `spu_test.exe` | (M5) voice mixer properties (pan, pitch, ADSR release, one-shot mute), the libspu API, SpuMalloc fragmentation incl. the VAB-swap pattern |
| `xm_test.exe` | (M5) parses the real PXMs and cross-checks **every pattern** against the pristine `.xm` files beside them; VAB upload byte-exactness; then plays the title theme + a one-shot SFX end-to-end through the mixer (run from the repo root or it skips) |
| `sbsp_headless.exe` | no window: dumps the 245-entry FAT, loads real lumps, CRCs them |

Run all for both variants before any commit:

```powershell
foreach ($v in 'debug','final') { foreach ($t in 'gte_trig_test','gte_test','gpu_test',
    'adpcm_test','spu_test','xm_test') {
  & "port\build\$v\$t.exe"; "$v/$t -> $LASTEXITCODE" } }
$env:SBSP_DATA_DIR = "out/USA/DEBUG/version/CD"
port\build\final\sbsp_headless.exe
```


## 8. GDB

Use the MSYS2 one - it reads DWARF 5; the vintage `C:\MinGW\bin\gdb` does not:

```powershell
C:\msys64\mingw32\bin\gdb.exe --args port\build\debug\sbsp.exe
```

Useful breakpoints: `GTE_ExecuteCop2` (proves whether a screen touches the GTE
at all), `Port_Pump`, `DoAssert`, `GPU_DrawPrim`. Set the environment before
launching, or use `set environment SBSP_EXIT_AFTER 1300` inside gdb.


## 9. Measured budgets (M4 sweep)

Baselines to compare against after any change that adds primitives or grows
a render window. Re-measure with:

```powershell
$env:SBSP_PRIM_LOG = "1"
port\build\debug\sbsp.exe --level 1-1 --no-cd-pace --pad-script "240:2000,600:0000" --exit-after 600
```

**Prim pool** - the budget is `MAX_PRIMS * MAX_PRIM_SIZE`, which the PC
build doubles to **163,840 bytes per buffer** (`source/gfx/prim.h`,
conv_pc.md #20; the console keeps 81,920). It is a *byte* budget, not a
primitive count, so mixed primitive sizes matter.

**Scripted input understates the real peak by ~1.4x.** Two full C1L1 play
sessions of different lengths peaked at 39,180 and 39,276 bytes, against
28,516 for the scripted walk - so treat every figure in the table below as
a floor, not a bound. A partial (unfinished) human run of Chapter 4 Level 3
reached 59,544, already above its scripted 52,308. That gap, and the fact
that proving a real bound would mean playing all 25 levels to completion,
is why the PC pool was doubled rather than measured further.

Sweeping all 25 bootable levels (600 vblanks each, walking right), against
the ORIGINAL 81,920-byte budget:

| | level | peak | of budget |
|---|---|---|---|
| worst | Chapter 4 Level 3 (`--level 4-3`) | 52,308 B | 63.9% |
| next | Chapter 5 bonus (`--level 6-5`) | 50,392 B | 61.5% |
| next | Chapter 1 bonus (`--level 6-1`) | 48,600 B | 59.3% |
| typical | most levels | 26-38 KB | 32-46% |
| lightest | Chapter 4 Level 1 | 17,380 B | 21.2% |

Kelp World (bonus) levels are the dense ones, as expected - they are the
vehicle levels. Against the doubled PC budget the worst scripted level sits
at 31.9% and the worst measured human run at 36.3%, so the 87.5% warning
has a wide margin. Re-measure with a *played* session, not a script, if you
ever change what the renderer emits per frame.

**Scratchpad**: C1L1 peaks at 240 B; the worst in the tree is 996 B
(`CHAPTER06_LEVEL01`) against the 1 KB `PORT_Scratchpad`.


## 10. Gotcha worth memorising

**A vblank callback must never block on the pump.** A nested `Port_Pump` is a
complete no-op (that is what keeps the vblank counter and its callback in
lockstep), so a callback that spins waiting for the pump to advance hangs
forever. That is the shape of most mystery freezes in this codebase.
