#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${SOUND_DBE4_BUILD_DIR:-$ROOT/pc_port/build_native/sound_vibrato_ramp_dbe4}"
CC="${CC:-gcc}"
mkdir -p "$OUT"
cd "$ROOT"
export TMPDIR="${TMPDIR:-/var/tmp}"

SLUS_SHA="dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119"
DBE4_SHA="$(dd if=disc/SLUS_006.64 bs=1 skip=$((0x2e3e4)) count=$((0x6c)) status=none | sha256sum | awk '{print $1}')"
test "$(sha256sum disc/SLUS_006.64 | awk '{print $1}')" = "$SLUS_SHA"
test "$DBE4_SHA" = "a0faad89df4b5b3dd60e6bd6a1ee5c3e2ebe660c7a166bfcb79c1b4415021421"

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -fpermissive -ffunction-sections -fdata-sections)
TEST_BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
           -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)

build_and_run() {
    local name="$1" source="src/slus_006.64/system/sound.c" compiler="$CC" linker="$CC"
    shift
    if [[ $# -gt 0 && -f "$1" ]]; then source="$1"; shift; fi
    if [[ "$name" == UBSan ]]; then linker="${UBSAN_CC:-gcc}"; fi
    "$compiler" "${BASE[@]}" "${INC[@]}" -w "$@" -c "$source" -o "$OUT/$name.sound.o"
    "$compiler" "${TEST_BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "$@" -c pc_port/tests/sound_vibrato_ramp_dbe4_retail_test.c -o "$OUT/$name.test.o"
    "$linker" -fno-pie -no-pie "$@" "$OUT/$name.test.o" "$OUT/$name.sound.o" -Wl,--gc-sections -o "$OUT/$name"
    "$OUT/$name" >"$OUT/$name.stdout" 2>"$OUT/$name.stderr"
    test ! -s "$OUT/$name.stderr"
}

build_and_run O0 -O0 -g
build_and_run O2 -O2
build_and_run UBSan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan.stdout"
grep -Eq '^SOUND VIBRATO RAMP DBE4 certificate PASS checks=125$' "$OUT/O0.stdout"
echo 'SOUND VIBRATO RAMP DBE4 O0/O2/UBSAN PASS'

set +e
sed '/^u8\* func_8003DBE4(/,/^}/ s/0xFFF7/0xFFEF/' src/slus_006.64/system/sound.c >"$OUT/mutant.c"
build_and_run mutant "$OUT/mutant.c" -O0 -g
rc=$?
set -e
if [[ "$rc" -eq 0 ]] || ! grep -Eq '^SOUND DBE4 FAIL flags' "$OUT/mutant.stderr"; then
    echo "DBE4 FLAG MUTANT NOT DETECTED rc=$rc" >&2
    exit 1
fi
echo 'SOUND VIBRATO RAMP DBE4 MUTANT DETECTED'
