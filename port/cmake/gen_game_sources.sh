#!/bin/bash
# Regenerates port/cmake/game_sources.cmake from makefile.gaz's *_src lists
# (the authoritative object manifest of the vintage CD link).  Run after any
# TU is added to or removed from makefile.gaz, then commit both files:
#
#   C:\msys64\usr\bin\bash.exe -l port/cmake/gen_game_sources.sh
#
# The $(FILE_SYSTEM)_FILESYS_SRC indirection resolves to cdfile for the CD
# build; system/main.cpp is split out so test exes can supply main().
set -e
cd "$(dirname "$0")/../.."

LIST=$(awk -f port/cmake/extract_tus.awk makefile.gaz | grep -v 'FILESYS_SRC' | grep -v '_src_mip'; echo "fileio/cdfile")
LIST=$(printf '%s\n' "$LIST" | sort -u)

missing=0
while read -r f; do
    [ -f "source/$f.cpp" ] || { echo "ERROR: makefile.gaz lists source/$f.cpp but it does not exist" >&2; missing=1; }
done <<< "$LIST"
[ "$missing" -eq 0 ] || exit 1

OUT=port/cmake/game_sources.cmake
{
    echo "# Game translation-unit manifest, generated from the *_src lists in makefile.gaz"
    echo "# by port/cmake/gen_game_sources.sh - regenerate with that script, do not edit"
    echo "# by hand.  $(printf '%s\n' "$LIST" | wc -l | tr -d ' ') TUs: all but system/main.cpp in the static lib (main is split"
    echo "# out so test exes can supply their own main())."
    echo "# Dead .cpp files under source/ are deliberately absent - do NOT glob."
    echo ""
    echo "set(SBSP_GAME_MAIN_SOURCE \${SBSP_ROOT}/source/system/main.cpp)"
    echo ""
    echo "set(SBSP_GAME_SOURCES"
    printf '%s\n' "$LIST" | grep -v '^system/main$' | sed 's|^|    ${SBSP_ROOT}/source/|; s|$|.cpp|'
    echo ")"
} > "$OUT"

echo "wrote $OUT ($(grep -c 'cpp$' "$OUT") lib TUs + main)"
