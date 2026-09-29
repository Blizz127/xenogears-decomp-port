#!/usr/bin/env bash
# Retail certificate for SoundHandleError (SLUS_006.64 [8003F6B0,8003F738)).
#
# The SHIPPED production body from src/slus_006.64/system/sound.c is compiled
# and linked (not reimplemented). Its five callees are defined in the same TU,
# so -Wl,--wrap cannot intercept them; instead this runner builds a scratch copy
# of sound.c in which exactly those five definitions are marked weak, and the
# test supplies strong recording owners. Everything else in sound.c stays as
# shipped. Differential across -O0 / -O2 / UBSan, then one mutant in the real
# body that must BUILD and then FAIL at runtime with the named ASSERTION.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${SOUND_HANDLE_ERROR_BUILD_DIR:-$ROOT/pc_port/build_native/sound_handle_error}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

SLUS_SHA="dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119"
actual="$(sha256sum disc/SLUS_006.64 | awk '{print $1}')"
if [[ "$actual" != "$SLUS_SHA" ]]; then
    echo "ERROR: disc/SLUS_006.64 SHA-256 mismatch: $actual" >&2
    exit 1
fi

SLICE_SHA="$(dd if=disc/SLUS_006.64 bs=1 \
    skip=$((0x800 + 0x8003F6B0 - 0x80010000)) count=$((0x88)) status=none \
    | sha256sum | awk '{print $1}')"
if [[ "$SLICE_SHA" != "5352ec72557934cf546485dfdea2b3b15d9e392374046120cfc8aa2ce9fcf9fb" ]]; then
    echo "ERROR: retail SoundHandleError slice SHA-256 mismatch: $SLICE_SHA" >&2
    exit 1
fi
echo "retail 0x8003F6B0..0x8003F738 slice sha256: $SLICE_SHA"

# Scratch copy of the shipped TU with the five callee definitions weakened.
weaken_source() {
    local out="$1" mutate="${2:-}"
    python3 - "$out" "$mutate" <<'PY'
import sys

out, mutate = sys.argv[1], sys.argv[2]
src = open('src/slus_006.64/system/sound.c').read()

FIVE = [
    'SoundWDSEntry* SoundLoadWdsFile(SoundWDSEntry* pWdsFile, s32 mode) {',
    'void SoundAddSedsEntry(SoundFile* pSoundFile) {',
    'int SoundSpuMemoryFreeBlock(int targetAddress) {',
    'void func_80039E60(s32 packedId) {',
    's16 func_8003BDFC(s32 flags) {',
]
for sig in FIVE:
    line = '__attribute__((weak)) ' + sig
    assert src.count(sig) == 1, 'callee signature not unique: %s' % sig
    assert line not in src, 'already weakened: %s' % sig
    src = src.replace(sig, line, 1)

if mutate:
    old, new = '& 0x88', '& 0x80'
    assert src.count(old) == 1, 'mutant anchor not unique'
    src = src.replace(old, new, 1)

open(out, 'w').write(src)
PY
}

BASE=(-std=gnu17 -fno-pie -no-pie -m64 -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -fpermissive -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)

# sound.c keeps -fpermissive; the C test TU is held to -Wall -Wextra -Werror,
# and cc1 rejects -fpermissive for C under -Werror.
TEST_BASE=()
for f in "${BASE[@]}"; do
    [[ "$f" == "-fpermissive" ]] || TEST_BASE+=("$f")
done

build_and_run() {
    local name="$1"
    local linker="$CC"
    shift
    "${CC}" "${BASE[@]}" "${INC[@]}" -w -fno-inline -fno-sanitize=object-size \
        "$@" -c "$BUILD_DIR/$name.sound.c" -o "$BUILD_DIR/$name.sound.o"
    "${CC}" "${TEST_BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "$@" \
        -c pc_port/tests/sound_handle_error_retail_test.c \
        -o "$BUILD_DIR/$name.test.o"
    if [[ "$name" == "UBSan" ]] && command -v clang >/dev/null 2>&1; then
        linker=clang
    fi
    "$linker" -no-pie "$@" "$BUILD_DIR/$name.test.o" "$BUILD_DIR/$name.sound.o" \
        -Wl,--gc-sections -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" 2>"$BUILD_DIR/$name.stderr"
    test ! -s "$BUILD_DIR/$name.stderr"
}

weaken_source "$BUILD_DIR/O0.sound.c"
cp "$BUILD_DIR/O0.sound.c" "$BUILD_DIR/O2.sound.c"
cp "$BUILD_DIR/O0.sound.c" "$BUILD_DIR/UBSan.sound.c"

build_and_run O0 -O0 -g
build_and_run O2 -O2
build_and_run UBSan -O1 -g -fsanitize=undefined -fno-sanitize-recover=all

cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O2.stdout"
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/UBSan.stdout"
grep -qE '^SOUND HANDLE ERROR certificate PASS checks=[0-9]+$' "$BUILD_DIR/O0.stdout"
echo "SOUND HANDLE ERROR O0/O2/UBSAN PASS"

# Mutant: drop the 0x08 arm of the guard mask. The 0x0008 case then runs where
# retail is a no-op.
weaken_source "$BUILD_DIR/mutant_mask.sound.c" mutate
set +e
build_and_run mutant_mask -O0 -g
mutant_rc=$?
set -e
if [[ "$mutant_rc" -eq 0 ]] || \
   ! grep -qE '^ASSERTION sound.handleerror ' "$BUILD_DIR/mutant_mask.stderr"; then
    echo "GUARD-MASK MUTANT NOT DETECTED rc=$mutant_rc" >&2
    sed -n '1,40p' "$BUILD_DIR/mutant_mask.stderr" >&2 || true
    exit 1
fi
echo "GUARD-MASK MUTANT DETECTED"
echo "SOUND HANDLE ERROR CERTIFICATE PASS; MUTANT DETECTED"
