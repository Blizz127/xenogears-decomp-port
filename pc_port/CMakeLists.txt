cmake_minimum_required(VERSION 3.16)
project(xenogears_pc_port C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Phase 0 scaffold for a Shipwright-style native PC port.
#
# Architecture (mirrors the Silent Hill PC port):
#   Xenogears game logic (src/) --calls PsyQ API--> PsyCross (PSX HAL)
#     PsyCross: GTE in software, LibGPU -> OpenGL, LibSPU -> OpenAL,
#               LibCD -> BIN/CUE, controllers -> SDL2.
#
# Separate from the matching (gears/ninja/MIPS) build. We compile PsyCross's
# sources directly rather than using its bundled CMakeLists, which assumes
# SDL2/OpenAL are provided as subprojects and uses case-sensitive globs that
# miss the uppercase *.C PSX library files on Linux.
# ---------------------------------------------------------------------------

find_package(SDL2 REQUIRED)
find_package(OpenAL REQUIRED)
find_package(OpenGL REQUIRED)

set(PSYCROSS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/PsyCross)

file(GLOB_RECURSE PSYCROSS_SRCS CONFIGURE_DEPENDS
    "${PSYCROSS_DIR}/src/*.c"
    "${PSYCROSS_DIR}/src/*.C"
    "${PSYCROSS_DIR}/src/*.cpp"
)

add_library(psycross STATIC ${PSYCROSS_SRCS})

# Use the simple (non-PGXP) primitive pipeline. MUST match the game/port build's
# -DUSE_EXTENDED_PRIM_POINTERS=0 (see pc_port/build_port.sh) or the SPRT/POLY
# struct layouts diverge between the lib and the game TUs. The default extended/
# PGXP path routes 2D primitives through a perspective/offscreen path that does
# not render in this integration.
target_compile_definitions(psycross PRIVATE USE_EXTENDED_PRIM_POINTERS=0)

# PsyCross is Windows-first. On Linux/gcc, force-include a compat shim (by
# absolute path so it isn't shadowed by PsyCross's own include/psx/strings.h)
# and relax C++ narrowing/format diagnostics, rather than patch vendored sources.
if(NOT MSVC)
    target_compile_options(psycross PRIVATE
        "-include" "${CMAKE_CURRENT_SOURCE_DIR}/src/port_compat.h"
        -Wno-narrowing
        -Wno-format-security
        # PSX SDK code casts function pointers to int (32-bit assumption); on
        # 64-bit C++ this is an error. Downgrade for now (Phase 1 will address
        # 64-bit correctness of any callbacks that end up on the boot path).
        $<$<COMPILE_LANGUAGE:CXX>:-fpermissive>
    )
endif()

target_include_directories(psycross PUBLIC
    "${PSYCROSS_DIR}/include"
    "${PSYCROSS_DIR}/include/psx"
)
target_include_directories(psycross PRIVATE
    "${PSYCROSS_DIR}/src"
    ${SDL2_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIR}
)
target_link_libraries(psycross PUBLIC
    ${SDL2_LIBRARIES}
    ${OPENAL_LIBRARY}
    OpenGL::GL
)

add_executable(xeno-port src/port_main.c)
target_include_directories(xeno-port PRIVATE src ${SDL2_INCLUDE_DIRS})
target_link_libraries(xeno-port PRIVATE psycross)
