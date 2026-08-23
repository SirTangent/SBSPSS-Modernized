#!/bin/bash
# Build the original PlayStation executable (makefile.gaz) on modern Windows.
#
# Same harness as build-data.sh: the MSYS2 runtime (make/sh/coreutils) drives
# the vintage PSY-Q compilers, which still run on Win11 even though the 1999
# cygwin utilities do not. PSYQ_PATH goes on the make command line because
# globals.mak exports it empty; ccpsx looks for sn.ini in the cwd (its paths
# are repo-root-relative), so stage a copy if missing.
#
# Usage: port/build-psx.sh [TERRITORY] [VERSION]      # defaults: USA DEBUG
set -e

cd "$(dirname "$0")/.."

TERRITORY="${1:-USA}"
VERSION="${2:-DEBUG}"

[ -f sn.ini ] || cp tools/psyq/bin/egcs/sn.ini sn.ini

# The PSY-Q tools take DOS-style /flags; stop MSYS2 from rewriting them as paths.
export MSYS2_ARG_CONV_EXCL='*'

BUILD_PATH="/usr/bin:$PWD/port/tools:$PWD/tools:$PWD/tools/psyq/bin:$PWD/tools/psyq/bin/egcs"

# Targets 'dirs link' only: the full 'all' also runs 'cddata' (CPE->BIN,
# buildcd, ISO), which needs 16-bit tools (cpe2bin) and only matters for
# pressing a physical disc - the port consumes the linked output directly.
make -r -f makefile.gaz \
    VERSION="$VERSION" TERRITORY="$TERRITORY" USER_NAME=CDBUILD \
    "PATH=$BUILD_PATH" "Path=$BUILD_PATH" \
    "PSYQ_PATH=$PWD/tools/psyq/bin/egcs" \
    MKDIR=mkdir ECHO=echo MV=mv DATE=date SED=sed \
    RMDIR=rmdir LS=ls "ATTRIB=chmod +w" \
    dirs link

echo "PSX build complete: out/$TERRITORY/$VERSION/version/CD/Spongey.cpe"
