#
# SBSPSS Win11 port - PC build (M1: scaffold + shim + headless file-I/O proof).
#
# Configure via the presets in port/CMakePresets.json, or directly:
#   cmake -S port -B port/build/debug -G Ninja \
#         --toolchain cmake/mingw32-toolchain.cmake -DSBSP_VARIANT=DEBUG
#
cmake_minimum_required(VERSION 3.28)
project(sbspss LANGUAGES C CXX)

set(SBSP_VARIANT   "DEBUG" CACHE STRING "Game build variant: DEBUG or FINAL")
set(SBSP_TERRITORY "USA"   CACHE STRING "Territory: USA or EUR")
set_property(CACHE SBSP_VARIANT   PROPERTY STRINGS DEBUG FINAL)
set_property(CACHE SBSP_TERRITORY PROPERTY STRINGS USA EUR)

get_filename_component(SBSP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# The generated headers gate everything (BigLump.h / Sprites.h / trans.h / ...).
if(NOT EXISTS "${SBSP_ROOT}/out/${SBSP_TERRITORY}/include/BigLump.h")
    message(FATAL_ERROR
        "Generated headers missing: ${SBSP_ROOT}/out/${SBSP_TERRITORY}/include/BigLump.h\n"
        "Run port/build-data.cmd (M0 data pipeline) first.")
endif()

include(cmake/game_sources.cmake)

# ---------------------------------------------------------------------------
# Compile environment shared by all game code (mirrors build/ccopt.mak +
# build/parse.mak for the CD/CDBUILD configuration; -Werror and the vintage
# warning set are dropped for the modern compiler).
# ---------------------------------------------------------------------------
set(SBSP_GAME_DEFINES
    PSX
    _LANGUAGE_C_PLUS_PLUS
    __TERRITORY_${SBSP_TERRITORY}__
    __VERSION_${SBSP_VARIANT}__
    __USER_CDBUILD__
    PC=0
    CD=1
    __FILE_SYSTEM__=CD
    SONY_ISA=0
    SONY_PCI=1
    CLIMAX=2
    TPWART=3
    __DEV_KIT__=SONY_PCI
)

set(SBSP_GAME_CXX_FLAGS
    -std=gnu++98
    -fpermissive
    -fno-exceptions
    -fno-rtti
    -fno-strict-aliasing
    -fno-builtin
    -Wall
    -Wno-unused
    -Wno-write-strings
    # style noise pervasive in the 1999 code - suppressed on the game target
    # only; genuine bug-smell classes (-Wmaybe-uninitialized,
    # -Wreturn-local-addr, -Wdelete-non-virtual-dtor) stay visible
    -Wno-narrowing
    -Wno-overloaded-virtual
    -Wno-non-c-typedef-for-linkage
    -Wno-parentheses
    -Wno-char-subscripts
    -Wno-sign-compare
    -Wno-misleading-indentation
    -Wno-int-in-bool-context
    -Wno-dangling-else
    -Wno-header-guard
    -g
    -O2
)

# Include order is load-bearing (mirrors ccopt.mak); port/include shadows the
# few PSY-Q headers that cannot compile on x86, and tools/psyq/include sits
# BEHIND the system headers (-idirafter) so MinGW's real stddef/stdlib/stdio
# win while the SDK's libgpu/libgte/libcd/... layouts stay authoritative.
set(SBSP_GAME_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SBSP_ROOT}
    ${SBSP_ROOT}/tools/mod/include
    ${SBSP_ROOT}/source
    ${SBSP_ROOT}/tools/Data/include
    ${SBSP_ROOT}/data
    ${SBSP_ROOT}/out/${SBSP_TERRITORY}/include
)
set(SBSP_PSYQ_DIRAFTER -idirafter ${SBSP_ROOT}/tools/psyq/include)

function(sbsp_game_flavour target)
    target_compile_definitions(${target} PRIVATE ${SBSP_GAME_DEFINES})
    target_compile_options(${target} PRIVATE ${SBSP_GAME_CXX_FLAGS} ${SBSP_PSYQ_DIRAFTER})
    target_include_directories(${target} PRIVATE ${SBSP_GAME_INCLUDES})
endfunction()

# ---------------------------------------------------------------------------
# Game code (unmodified PS1 sources)
# ---------------------------------------------------------------------------
add_library(sbsp_game STATIC ${SBSP_GAME_SOURCES})
sbsp_game_flavour(sbsp_game)

# system/main.cpp compiles like everything else but is kept out of the lib so
# test executables can supply their own main(); the real game exe (M2+) links
# this object in.
add_library(sbsp_game_main OBJECT ${SBSP_GAME_MAIN_SOURCE})
sbsp_game_flavour(sbsp_game_main)

# ---------------------------------------------------------------------------
# PsyQ-on-PC shim runtime (modern C++)
# ---------------------------------------------------------------------------
file(GLOB SBSP_SHIM_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/psyq/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/psyq/*/*.cpp
)
add_library(psyq_shim STATIC ${SBSP_SHIM_SOURCES})
target_compile_features(psyq_shim PRIVATE cxx_std_17)
target_compile_options(psyq_shim PRIVATE -Wall -g -O2 ${SBSP_PSYQ_DIRAFTER})
target_compile_definitions(psyq_shim PRIVATE
    SBSP_INFO_VERSION="${SBSP_VARIANT}"
    SBSP_INFO_TERRITORY="${SBSP_TERRITORY}"
    SBSP_INFO_FILESYSTEM="CD"
)
target_include_directories(psyq_shim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/psyq
    ${SBSP_ROOT}
    ${SBSP_ROOT}/source
    ${SBSP_ROOT}/out/${SBSP_TERRITORY}/include
)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
# Headless file-I/O proof: whole-archive the game lib so EVERY game TU's
# undefined symbols hit the linker - that is the link-completeness proof.
add_executable(sbsp_headless tests/headless.cpp)
sbsp_game_flavour(sbsp_headless)
target_link_options(sbsp_headless PRIVATE -static)
target_link_libraries(sbsp_headless PRIVATE
    -Wl,--whole-archive sbsp_game -Wl,--no-whole-archive
    psyq_shim
)

add_executable(gte_trig_test tests/gte_trig_test.cpp)
target_compile_features(gte_trig_test PRIVATE cxx_std_17)
target_include_directories(gte_trig_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/psyq)
target_link_options(gte_trig_test PRIVATE -static)
target_link_libraries(gte_trig_test PRIVATE psyq_shim)
