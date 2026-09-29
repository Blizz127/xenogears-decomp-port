#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/pc_port/extern/PsyCross/src/render/PsyX_render.cpp"
PATCH="$ROOT/pc_port/patches/psycross_vram_copy_wrap.patch"
BUILD="$ROOT/pc_port/build_port.sh"

body="$(sed -n '/^void GR_CopyRGBAFramebufferToVRAM(/,/^}/p' "$SRC")"
if grep -Fq 'assert(x >= 0)' <<<"$body"; then
    echo 'FAIL: framebuffer copy still asserts x >= 0' >&2
    exit 1
fi
if ! grep -Fq '_xeno_vram_copy_wrap' <<<"$body"; then
    echo 'FAIL: framebuffer copy is missing the VRAM wrap marker' >&2
    exit 1
fi
if ! grep -Fq '(x + fx) & (VRAM_WIDTH - 1)' <<<"$body"; then
    echo 'FAIL: framebuffer copy does not mask destination X' >&2
    exit 1
fi
grep -Fq '_xeno_vram_copy_wrap' "$PATCH"
grep -Fq 'psycross_vram_copy_wrap.patch' "$BUILD"
echo 'PASS: framebuffer copy wraps PS1 VRAM coordinates'
