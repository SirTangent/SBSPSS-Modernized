# MSYS2 MinGW-w64 i686 toolchain for the SBSPSS Win11 port.
# The game must build 32-bit: the on-disc data formats embed 4-byte pointers
# that are patched in place at load time (see tools/Data/include/dstructs.h).
#
# Install once:  pacman -S --needed mingw-w64-i686-gcc mingw-w64-i686-cmake mingw-w64-i686-ninja

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

set(MINGW32_ROOT "C:/msys64/mingw32" CACHE PATH "MSYS2 mingw32 prefix")

set(CMAKE_C_COMPILER   "${MINGW32_ROOT}/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "${MINGW32_ROOT}/bin/g++.exe")
set(CMAKE_RC_COMPILER  "${MINGW32_ROOT}/bin/windres.exe")

# The compiler subprocesses (cc1plus etc.) need the mingw32 DLLs on PATH.
# port/build-pc.sh arranges that; guard configure-time checks here too.
set(CMAKE_FIND_ROOT_PATH "${MINGW32_ROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
