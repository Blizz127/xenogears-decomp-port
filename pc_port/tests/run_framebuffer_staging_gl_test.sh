#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/framebuffer_staging_gl_test
mode=${1:-all}
if [ "$mode" != --run-only ]; then
    mkdir -p "$OUT"
    sed -n \
        -e '/^void GR_CopyRGBAFramebufferToVRAM(/,/^}/p' \
        -e '/^void GR_StoreFrameBuffer(/,/^}/p' \
        -e '/^static void GR_XenoReadBackbufferToVRAM(int x, int y, int w, int h)$/,/^}/p' \
        -e '/^void GR_UpdateVRAM()/,/^}/p' \
        -e '/^void GR_MaterializeFramebufferRect(/,/^}/p' \
        pc_port/extern/PsyCross/src/render/PsyX_render.cpp > "$OUT/framebuffer_staging_body.inc"
    # Bazzite's immutable host has GCC's libubsan linker script but not its
    # target; symlink clang's ABI-compatible standalone archive like the
    # field_script_vm_* harnesses do.
    UBSAN_ARCHIVE="/home/linuxbrew/.linuxbrew/Cellar/llvm/22.1.8/lib/clang/22/lib/linux/libclang_rt.ubsan_standalone-x86_64.a"
    UBSAN_EXTRA=()
    if [[ -f "$UBSAN_ARCHIVE" ]]; then
        ln -sf "$UBSAN_ARCHIVE" "$OUT/libubsan.a"
        UBSAN_EXTRA=(-L"$OUT")
    fi
    for opt in O0 O2 UBSan; do
        flags=(-"$opt")
        if [ "$opt" = UBSan ]; then
            # Build in the toolchain container, run visibly on the host.
            flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all "${UBSAN_EXTRA[@]}" -static-libubsan)
        fi
        g++ -std=c++17 "${flags[@]}" -g -Wall -Wextra -I"$OUT" \
            pc_port/tests/framebuffer_staging_gl_test.cpp \
            $(pkg-config --cflags --libs sdl2 gl) -o "$OUT/$opt.test"
    done
    mkdir -p "$OUT/mutant"
    sed '/^\tglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_fbTexture, 0);$/d' \
        "$OUT/framebuffer_staging_body.inc" > "$OUT/mutant/framebuffer_staging_body.inc"
    g++ -std=c++17 -O2 -I"$OUT/mutant" pc_port/tests/framebuffer_staging_gl_test.cpp \
        $(pkg-config --cflags --libs sdl2 gl) -o "$OUT/mutant.test"
    mkdir -p "$OUT/packed-mutant"
    sed '/^void GR_MaterializeFramebufferRect(/,/^}/ {
        s/u_int r = (p \& 31) \* 255 \/ 31;/u_int r = p \& 255;/
        s/u_int g = ((p >> 5) \& 31) \* 255 \/ 31;/u_int g = (p >> 8) \& 255;/
        s/u_int b = ((p >> 10) \& 31) \* 255 \/ 31;/u_int b = 0;/
    }' "$OUT/framebuffer_staging_body.inc" > "$OUT/packed-mutant/framebuffer_staging_body.inc"
    g++ -std=c++17 -O2 -I"$OUT/packed-mutant" pc_port/tests/framebuffer_staging_gl_test.cpp \
        $(pkg-config --cflags --libs sdl2 gl) -o "$OUT/packed-mutant.test"
    mkdir -p "$OUT/stretch-mutant"
    sed 's/left, top, right, bottom,/0, g_windowHeight, g_windowWidth, 0,/' \
        "$OUT/framebuffer_staging_body.inc" > "$OUT/stretch-mutant/framebuffer_staging_body.inc"
    g++ -std=c++17 -O2 -I"$OUT/stretch-mutant" pc_port/tests/framebuffer_staging_gl_test.cpp \
        $(pkg-config --cflags --libs sdl2 gl) -o "$OUT/stretch-mutant.test"
fi
if [ "$mode" != --build-only ]; then
    for opt in O0 O2 UBSan; do
        echo "FRAMEBUFFER STAGING configuration=$opt"
        "$OUT/$opt.test"
    done
    if "$OUT/mutant.test" > "$OUT/mutant.log" 2>&1; then
        echo 'FRAMEBUFFER STAGING FAIL attachment-removal mutant survived' >&2
        exit 1
    fi
    rg -q 'FRAMEBUFFER STAGING FAIL after-materialize' "$OUT/mutant.log"
    echo 'FRAMEBUFFER STAGING attachment-removal mutant rejected'
    if "$OUT/packed-mutant.test" > "$OUT/packed-mutant.log" 2>&1; then
        echo 'FRAMEBUFFER MATERIALIZE FAIL packed-byte mutant survived' >&2
        exit 1
    fi
    rg -q 'FRAMEBUFFER MATERIALIZE FAIL color=' "$OUT/packed-mutant.log"
    echo 'FRAMEBUFFER MATERIALIZE packed-byte mutant rejected'
    if "$OUT/stretch-mutant.test" > "$OUT/stretch-mutant.log" 2>&1; then
        echo 'FRAMEBUFFER MATERIALIZE FAIL full-screen-stretch mutant survived' >&2
        exit 1
    fi
    rg -q 'FRAMEBUFFER MATERIALIZE FAIL placement' "$OUT/stretch-mutant.log"
    echo 'FRAMEBUFFER MATERIALIZE full-screen-stretch mutant rejected'
fi
