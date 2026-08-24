#!/bin/bash
# Configure + build the PC (Win32) port: both DEBUG and FINAL by default.
#
#   port/build-pc.sh [debug|final|all] [extra ninja args...]
#
# Requires the MSYS2 mingw32 toolchain:
#   pacman -S --needed mingw-w64-i686-gcc mingw-w64-i686-cmake mingw-w64-i686-ninja
# and the M0 generated headers (run port/build-data.cmd first).

set -e
# presets live in port/, and cmake resolves --preset from the cwd
cd "$(dirname "$0")"

# cc1plus/ninja need the mingw32 runtime DLLs on PATH.
export PATH="/c/msys64/mingw32/bin:$PATH"

what="${1:-all}"
shift 2>/dev/null || true

build_one()
{
    preset="$1"
    echo "=== configure+build: $preset ==="
    cmake --preset "$preset"
    cmake --build --preset "$preset" "$@"
}

case "$what" in
    debug|final) build_one "$what" "$@" ;;
    all)         build_one debug "$@"; build_one final "$@" ;;
    *) echo "usage: port/build-pc.sh [debug|final|all] [ninja args]" >&2; exit 1 ;;
esac

echo "PC build complete."
