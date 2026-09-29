#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${SOUND_CEF0_BUILD_DIR:-$ROOT/pc_port/build_native/sound_voice_record_cef0}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"
# /tmp here is a 16G tmpfs with a per-user quota; keep compiler temporaries out.
export TMPDIR="${TMPDIR:-/var/tmp}"

# Retail func_8003CEF0 slice: file offset 0x2D6F0 (vaddr 0x8003CEF0), 72 bytes.
SLUS_SHA="dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119"
CEF0_SHA="8859c93cd23efd47e7e4cf3ecf6216477139e82ec670d20e18547d3eb9a79a1c"
actual="$(sha256sum disc/SLUS_006.64 | awk '{print $1}')"
if [[ "$actual" != "$SLUS_SHA" ]]; then
    echo "ERROR: disc/SLUS_006.64 SHA-256 mismatch: $actual" >&2
    exit 1
fi
actual="$(dd if=disc/SLUS_006.64 bs=1 skip=$((0x2D6F0)) count=$((0x48)) \
    status=none | sha256sum | awk '{print $1}')"
if [[ "$actual" != "$CEF0_SHA" ]]; then
    echo "ERROR: retail func_8003CEF0 slice SHA-256 mismatch: $actual" >&2
    exit 1
fi

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -fpermissive -ffunction-sections -fdata-sections)
TEST_BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
           -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)

build_and_run() {
    local name="$1"
    local tuc="$2"
    local linker="$CC"
    shift 2
    "$CC" "${BASE[@]}" "${INC[@]}" -w "$@" \
        -c "$tuc" -o "$BUILD_DIR/$name.sound.o"
    "$CC" "${TEST_BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "$@" \
        -c pc_port/tests/sound_voice_record_cef0_retail_test.c \
        -o "$BUILD_DIR/$name.test.o"
    if [[ "$name" == "UBSan" ]] && command -v clang >/dev/null 2>&1; then
        linker=clang
    fi
    "$linker" -fno-pie -no-pie "$@" \
        "$BUILD_DIR/$name.test.o" "$BUILD_DIR/$name.sound.o" \
        -Wl,--gc-sections -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" \
        2>"$BUILD_DIR/$name.stderr"
    test ! -s "$BUILD_DIR/$name.stderr"
}

SOUND_TU="src/slus_006.64/system/sound.c"
build_and_run O0 "$SOUND_TU" -O0 -g
build_and_run O2 "$SOUND_TU" -O2
build_and_run UBSan "$SOUND_TU" -O2 -g -fsanitize=undefined -fno-sanitize-recover=all

cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O2.stdout"
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/UBSan.stdout"
grep -Eq '^SOUND VOICE RECORD CEF0 certificate PASS checks=[0-9]+$' \
    "$BUILD_DIR/O0.stdout"

echo "SOUND VOICE RECORD CEF0 O0/O2/UBSAN PASS"

# Mutant: move the record base by one slot (0x9C -> 0x98) in the shipped TU.
set +e
sed '/^u8\* func_8003CEF0(/,/^}/ s/0x9C)/0x98)/' "$SOUND_TU" \
    > "$BUILD_DIR/mutant.c"
grep -q '0x98)' "$BUILD_DIR/mutant.c"
build_and_run badbase "$BUILD_DIR/mutant.c" -O0 -g
mutant_rc=$?
set -e
if [[ "$mutant_rc" -eq 0 ]] || \
   ! grep -Eq '^ASSERTION cef0.result ' "$BUILD_DIR/badbase.stderr"; then
    echo "CEF0 BAD-BASE MUTANT NOT DETECTED rc=$mutant_rc" >&2
    sed -n '1,40p' "$BUILD_DIR/badbase.stderr" >&2 || true
    exit 1
fi

echo "SOUND VOICE RECORD CEF0 MUTANT DETECTED"
