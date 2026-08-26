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
has been built - so the FINAL exe needs `SBSP_DATA_DIR` (see §3) or it exits 3
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


## 3. Environment variables

All optional, all read once at startup.

| Var | Meaning |
|---|---|
| `SBSP_DATA_DIR` | Override the CD data directory. Needed to run the FINAL exe: `out/USA/DEBUG/version/CD`. |
| `SBSP_PAD_SCRIPT` | Inject controller input at given vblanks (§4). |
| `SBSP_DUMP_FRAMES` | Comma-separated vblank numbers; writes the **displayed** VRAM region as a 24-bit BMP at each. Max 16. |
| `SBSP_DUMP_DIR` | Where those BMPs go (default `.`). Files are `sbsp_frame_<vblank>.bmp`. |
| `SBSP_EXIT_AFTER` | Clean `exit(0)` at that vblank. The game's `MainLoop` has no exit path of its own, so scripted runs need this. |
| `SBSP_CD_PACE=0` | Disable the emulated 150 sectors/s double-speed CD pacing -> instant loads. Handy to reach a screen fast; note it also makes the loading icon never appear (the game skips it when zero vblanks elapse between `StartLoad` and `StopLoad`). |

The frame dumps read emulated VRAM directly, so they work **even if Vulkan
fails to initialise** - that is the point of them. They capture the DISPENV
crop, i.e. exactly what should be on screen.

Implementation: `port/psyq/host/window.cpp` (dump/exit),
`port/psyq/cd/cd.cpp` (data dir, pacing).


## 4. `SBSP_PAD_SCRIPT` - scripted input

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

Live keyboard is the RetroArch layout: arrows = D-pad, Z/X/A/S =
Cross/Circle/Square/Triangle, Enter = Start, RShift = Select, Q/W = L1/R1,
E/R = L2/R2. A connected SDL gamepad ORs in on top and hotplugs.

Implementation: `port/psyq/host/input.cpp`.


## 5. Logging

Everything shim-side goes to **stderr** with a bracketed tag, so
`2>&1 | Select-String` filters it:

- `[shim] stub: <name>` - first call to an unimplemented SDK function, once
  per function. **This is the main "what is missing" signal**; if a screen
  misbehaves, check here first.
- `[gpu] unknown/unimplemented GP0 command 0xNN` - once per opcode.
- `[gte] ...`, `[input] gamepad connected: ...`,
  `[input] SBSP_PAD_SCRIPT: N entries`, `[host] wrote ....bmp`.

Game-side `SYSTEM_DBGMSG` goes to **stdout**, prefixed `file:line`. Only the
`DC_SYSTEM` channel is enabled by default (`source/system/dbg.cpp:74`); call
`setActiveDbgChannels(DC_ALL)` to get `DC_SOUND` / `DC_GUI` / `DC_MEMCARD`
chatter too.

`ASSERT` failures (DEBUG only) print the expression, draw it centred on screen
with the standard font, then hit `PSYQpause()` - which on PC is
`__builtin_trap()` (`port/include/libsn.h`), i.e. a hard trap straight into the
debugger.


## 6. Unit tests

Four self-contained exes per variant in `port\build\<variant>\`; each exits 0
on pass and prints its own failures:

| Exe | Covers |
|---|---|
| `gte_trig_test.exe` | exact fixed-point `rsin`/`rcos`/`ratan2`/`SquareRoot0` fixtures |
| `gte_test.exe` | software cop2 + libgte fixtures (RTPS/RTPT/NCLIP/MVMVA/AVSZ/UNR division, FLAG bits) |
| `gpu_test.exe` | VRAM rect ops, OT walker shape, rasterizer goldens, polyline terminator cases |
| `sbsp_headless.exe` | no window: dumps the 245-entry FAT, loads real lumps, CRCs them |

Run all four for both variants before any commit:

```powershell
foreach ($v in 'debug','final') { foreach ($t in 'gte_trig_test','gte_test','gpu_test') {
  & "port\build\$v\$t.exe"; "$v/$t -> $LASTEXITCODE" } }
$env:SBSP_DATA_DIR = "out/USA/DEBUG/version/CD"
port\build\final\sbsp_headless.exe
```


## 7. GDB

Use the MSYS2 one - it reads DWARF 5; the vintage `C:\MinGW\bin\gdb` does not:

```powershell
C:\msys64\mingw32\bin\gdb.exe --args port\build\debug\sbsp.exe
```

Useful breakpoints: `GTE_ExecuteCop2` (proves whether a screen touches the GTE
at all), `Port_Pump`, `DoAssert`, `GPU_DrawPrim`. Set the environment before
launching, or use `set environment SBSP_EXIT_AFTER 1300` inside gdb.


## 8. Gotcha worth memorising

**A vblank callback must never block on the pump.** A nested `Port_Pump` is a
complete no-op (that is what keeps the vblank counter and its callback in
lockstep), so a callback that spins waiting for the pump to advance hangs
forever. That is the shape of most mystery freezes in this codebase.
