#!/bin/bash
# Build the SBSPSS game data (BigLump.Bin + generated headers) on modern Windows.
#
# Runs the original makefile.gfx with the MSYS2 runtime (make/sh/coreutils/perl)
# driving the repo's original Win32 converter EXEs. The PATH assignment on the
# make command line overrides the vintage-cygwin PATH that build/globals.mak:141
# force-exports (those 1999 cygwin binaries crash on Windows 10/11).
#
# Usage (from an MSYS2 shell, or: C:\msys64\usr\bin\bash.exe -l <this script>):
#   port/build-data.sh [TERRITORY] [VERSION]      # defaults: USA DEBUG
set -e

cd "$(dirname "$0")/.."

TERRITORY="${1:-USA}"
VERSION="${2:-DEBUG}"

GAME_DATA_SCR="data/DataCache.scr"
BIGLUMP_BIN="out/$TERRITORY/$VERSION/version/CD/BIGLUMP.BIN"
BIGLUMP_INC="out/$TERRITORY/include/BigLump.h"

# makefile.gfx:709 makes BigLump.Bin depend on $(GFX_DATA_OUT) alone, not on
# DataCache.scr.  So editing that script - or repairing its line endings -
# leaves the previous BigLump.Bin/BigLump.h in place and make skips the rule
# without a word, keeping stale file equates.  Drop them when the script is
# newer so the generated equates always match it.
if [ -f "$BIGLUMP_BIN" ] && [ "$GAME_DATA_SCR" -nt "$BIGLUMP_BIN" ]; then
    echo "$GAME_DATA_SCR is newer than $BIGLUMP_BIN - forcing a BigLump rebuild"
    rm -f "$BIGLUMP_BIN" "$BIGLUMP_INC"
fi

# port/tools first: its modern lznp.exe must shadow the 16-bit tools/lznp.exe.
# PATH and Path both overridden (globals.mak exports both spellings), and every
# tool variable globals.mak pins to the vintage tools/cygwin binaries is
# redirected to the MSYS2 equivalents - the 1999 cygwin ones crash on Win11.
BUILD_PATH="/usr/bin:$PWD/port/tools:$PWD/tools:$PWD/tools/Data/bin:$PWD/tools/psyq/bin"
make -r -f makefile.gfx \
    VERSION="$VERSION" TERRITORY="$TERRITORY" USER_NAME=CDBUILD \
    "PATH=$BUILD_PATH" "Path=$BUILD_PATH" \
    MKDIR=mkdir ECHO=echo MV=mv DATE=date SED=sed \
    RMDIR=rmdir LS=ls "ATTRIB=chmod +w"

# MkData exits 0 even when it parses nothing.  Its script reader is CRLF-only
# (Utils/MkData/MkData.cpp GotoNextLine scans for CR then skips two bytes), so
# an LF-only data/DataCache.scr makes it bail after the opening bank line and
# emit a stub BigLump.h holding just SYSTEM_CACHE.  That surfaces much later as
# hundreds of "'SCRIPTS_..._DAT' was not declared in this scope" errors in the
# PC/PSX build, so fail here instead, where the cause is still visible.
EQUATES=$(tr -cd ',' < "$BIGLUMP_INC" 2>/dev/null | wc -c)
if [ "$EQUATES" -lt 100 ]; then
    echo "ERROR: $BIGLUMP_INC has only $EQUATES file equates - MkData read" >&2
    echo "       almost nothing from $GAME_DATA_SCR." >&2
    echo "       That file must have CRLF line endings.  Check with:" >&2
    echo "         od -c $GAME_DATA_SCR | head -2" >&2
    echo "       and restore it with:" >&2
    echo "         rm $GAME_DATA_SCR && git checkout -- $GAME_DATA_SCR" >&2
    exit 1
fi

# Stage the XA speech stream next to BIGLUMP.BIN (M6) - on PSX this is
# makefile.gaz's cddata rule copying it into the CD image. Guard against an
# unmaterialised Git-LFS pointer file (a few hundred bytes, not ~127MB).
IXA_SRC="data/CDData/Track1.Ixa"
IXA_DST="out/$TERRITORY/$VERSION/version/CD/TRACK1.IXA"
if [ ! -f "$IXA_SRC" ] || [ "$(stat -c%s "$IXA_SRC")" -lt 1048576 ]; then
    echo "ERROR: $IXA_SRC is missing or is an unmaterialised Git-LFS pointer." >&2
    echo "       Run: git lfs pull" >&2
    exit 1
fi
if [ ! -f "$IXA_DST" ] || [ "$IXA_SRC" -nt "$IXA_DST" ]; then
    cp "$IXA_SRC" "$IXA_DST"
    echo "Staged $IXA_DST"
fi

# Stage the FMV movies (M7) - raw-XA .STR files, same layout as the IXA.
# Not LFS-tracked, but keep the same size sanity guard for uniformity.
for movie in thq climax intro demo; do
    STR_SRC="data/CDData/$movie.str"
    STR_DST="out/$TERRITORY/$VERSION/version/CD/$(echo "$movie" | tr a-z A-Z).STR"
    if [ ! -f "$STR_SRC" ] || [ "$(stat -c%s "$STR_SRC")" -lt 1048576 ]; then
        echo "ERROR: $STR_SRC is missing or truncated." >&2
        exit 1
    fi
    if [ ! -f "$STR_DST" ] || [ "$STR_SRC" -nt "$STR_DST" ]; then
        cp "$STR_SRC" "$STR_DST"
        echo "Staged $STR_DST"
    fi
done

echo "Data build complete: out/$TERRITORY"
