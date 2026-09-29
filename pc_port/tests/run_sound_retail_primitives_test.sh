#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${XENO_SOUND_PRIMITIVES_BUILD_DIR:-$ROOT/pc_port/build_native/sound_retail_primitives}"
CC_BIN="${CC:-cc}"

mkdir -p "$BUILD_DIR"

# sound.c now defines the real SoundHandleError (it is no longer guarded out of
# the port build). This test deliberately supplies its OWN fail-fast
# SoundHandleError that trips if the error path is reached while the primitives
# are exercised, so weaken the shipped definition in a scratch copy for this
# link only. Everything else in sound.c stays as shipped.
SOUND_WEAK="$BUILD_DIR/sound_weak.c"
python3 - "$SOUND_WEAK" <<'PY'
import sys
src = open('src/slus_006.64/system/sound.c').read()
sig = 'void SoundHandleError(s32 errorId)'
assert src.count(sig) == 1, 'SoundHandleError signature not unique'
open(sys.argv[1], 'w').write(src.replace(sig, '__attribute__((weak)) ' + sig, 1))
PY

build_and_run() {
    local mode="$1"
    shift
    "$CC_BIN" -std=gnu17 -include assert.h -Wall -Wextra -fpermissive \
        -m64 -fno-builtin \
        -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -ffunction-sections -fdata-sections \
        -I"$ROOT/include" -I"$ROOT/pc_port/include" \
        "$@" \
        "$ROOT/pc_port/tests/sound_retail_primitives_test.c" \
        "$SOUND_WEAK" \
        "$ROOT/pc_port/src/psyq_cd_mix.c" \
        -Wl,--gc-sections -lm \
        -o "$BUILD_DIR/test-$mode" 2>"$BUILD_DIR/$mode.warnings"
    if grep -Eq "In function '(SoundSpuMemoryAllocateBlock|SoundTransferWdsPart|SoundSetupCdMix)':|psyq_cd_mix\\.c:" \
        "$BUILD_DIR/$mode.warnings"; then
        echo "sound retail primitives: FAIL: target-owned compiler diagnostic ($mode)" >&2
        grep -E "In function '(SoundSpuMemoryAllocateBlock|SoundTransferWdsPart|SoundSetupCdMix)':|psyq_cd_mix\\.c:" \
            "$BUILD_DIR/$mode.warnings" >&2
        exit 1
    fi
    "$BUILD_DIR/test-$mode"
}

build_and_run O0 -O0
build_and_run O2 -O2
build_and_run UBSan -O1 -fsanitize=undefined -fno-sanitize-recover=all

nm -C "$ROOT/pc_port/build_native/xeno-port" > "$BUILD_DIR/native.nm"
for symbol in CdMix SoundSetupCdMix SoundSpuMemoryAllocateBlock SoundTransferWdsPart; do
    if grep -Eq "^[[:space:]]*(long|int|void)[[:space:]]+${symbol}\\(" \
        "$ROOT/pc_port/build_native/stubs.c" 2>/dev/null; then
        echo "sound retail primitives: FAIL: generated fallback remains for $symbol" >&2
        exit 1
    fi
    if ! grep -Eq " [Tt] ${symbol}$" "$BUILD_DIR/native.nm"; then
        echo "sound retail primitives: FAIL: native owner missing for $symbol" >&2
        exit 1
    fi
done

transfer_sha=$(dd if="$ROOT/disc/SLUS_006.64" bs=1 \
    skip=$((0x800 + 0x8003827c - 0x80010000)) count=$((0x94)) status=none |
    sha256sum | awk '{print $1}')
mix_sha=$(dd if="$ROOT/disc/SLUS_006.64" bs=1 \
    skip=$((0x800 + 0x8003885c - 0x80010000)) count=$((0x78)) status=none |
    sha256sum | awk '{print $1}')
allocator_sha=$(dd if="$ROOT/disc/SLUS_006.64" bs=1 \
    skip=$((0x800 + 0x800393b8 - 0x80010000)) count=$((0x100)) status=none |
    sha256sum | awk '{print $1}')
cdmix_sha=$(dd if="$ROOT/disc/SLUS_006.64" bs=1 \
    skip=$((0x800 + 0x8004138c - 0x80010000)) count=$((0x20)) status=none |
    sha256sum | awk '{print $1}')
test "$transfer_sha" = 488e928c9eae2726cd8e7d359aafffc4432066fe811c09420b8003f53c2604d0
test "$mix_sha" = 045ca7300d7b5076a7c2b9b5c2d9800cd400441d9adffb912e036ed0b814a51e
test "$allocator_sha" = cf6569f7fe8bae597637de59da0d4317cc6873b47102b3d9868ec10211c994a3
test "$cdmix_sha" = 344bf8a32638701db5bae1222a9e7c13d50b4d54fb0476ae8e9f63674c41c1a9

echo "sound retail primitives native ownership: PASS"
echo "sound retail primitive bytes: PASS"
