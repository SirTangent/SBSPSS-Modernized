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

echo "Data build complete: out/$TERRITORY"
