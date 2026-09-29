#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${SOUND_D13C_BUILD_DIR:-$ROOT/pc_port/build_native/sound_master_volume_fade_d13c}"
CC="${CC:-gcc}"
mkdir -p "$OUT"
cd "$ROOT"
export TMPDIR="${TMPDIR:-/var/tmp}"

SLUS_SHA="dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119"
D13C_SHA="$(dd if=disc/SLUS_006.64 bs=1 skip=$((0x2d93c)) count=$((0x3c)) status=none | sha256sum | awk '{print $1}')"
test "$(sha256sum disc/SLUS_006.64 | awk '{print $1}')" = "$SLUS_SHA"
test "$D13C_SHA" = "0cabf1db975006c2b4d2e2b576c2537765238a8136da6e59989af0ba2e65c3d7"

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -fpermissive -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
TEST_BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
           -ffunction-sections -fdata-sections)

build_and_run() {
    local name="$1" source="src/slus_006.64/system/sound.c" compiler="$CC" linker="$CC"
    shift
    if [[ $# -gt 0 && -f "$1" ]]; then source="$1"; shift; fi
    if [[ "$name" == UBSan ]]; then linker="${UBSAN_CC:-clang}"; fi
    "$compiler" "${BASE[@]}" "${INC[@]}" -w "$@" -c "$source" -o "$OUT/$name.sound.o"
    "$compiler" "${TEST_BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "$@" -c pc_port/tests/sound_master_volume_fade_d13c_retail_test.c -o "$OUT/$name.test.o"
    "$linker" -fno-pie -no-pie "$@" "$OUT/$name.test.o" "$OUT/$name.sound.o" -Wl,--gc-sections -o "$OUT/$name"
    "$OUT/$name" >"$OUT/$name.stdout" 2>"$OUT/$name.stderr"
    test ! -s "$OUT/$name.stderr"
}

build_and_run O0 -O0 -g
build_and_run O2 -O2
build_and_run UBSan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan.stdout"
grep -Eq '^SOUND MASTER VOLUME FADE D13C certificate PASS checks=125$' "$OUT/O0.stdout"
echo 'SOUND MASTER VOLUME FADE D13C O0/O2/UBSAN PASS'

set +e
sed '/^u8\* func_8003D13C(/,/^}/ s/if (steps != 0 && diff != 0)/if (steps != 0)/' src/slus_006.64/system/sound.c >"$OUT/mutant.c"
build_and_run mutant "$OUT/mutant.c" -O0 -g
rc=$?
set -e
if [[ "$rc" -eq 0 ]] || ! grep -Eq '^SOUND D13C FAIL zero guard' "$OUT/mutant.stderr"; then
    echo "D13C ZERO-GUARD MUTANT NOT DETECTED rc=$rc" >&2
    exit 1
fi
echo 'SOUND MASTER VOLUME FADE D13C MUTANT DETECTED'
