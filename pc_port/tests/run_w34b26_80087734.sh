#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34B26_BUILD_DIR:-$ROOT/pc_port/build_native/w34b26_80087734}"
SCRATCH="${W34B26_SCRATCH:-/tmp/grok-goal-21964a4dfe76/implementer}"
mkdir -p "$BUILD_DIR" "$SCRATCH"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DWM_87734_TEST_TRACE)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/src)
SRC=(pc_port/tests/w34b26_80087734_prod_test.c
     pc_port/src/world_map_scheduler.c)

check_slice() {
    local skip="$1" count="$2" want="$3" name="$4" got
    got="$(dd if=disc/world_map.bin bs=1 skip=$((skip)) count="$count" status=none | sha256sum | awk '{print $1}')"
    if [[ "$got" != "$want" ]]; then
        echo "ERROR: $name slice SHA mismatch: $got" >&2
        exit 1
    fi
}

require_retail() {
    local expected="4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70"
    local actual
    actual="$(sha256sum disc/world_map.bin | awk '{print $1}')"
    if [[ "$actual" != "$expected" ]]; then
        echo "ERROR: world_map.bin SHA mismatch: $actual" >&2
        exit 1
    fi
    # 0x80087734 - 0x8006FAF0 = 0x17C44, 172 bytes.
    check_slice 0x17C44 172 "930c20086d5082c3c42e39a2903e153a41b9e086e4a7b3afa1a8fbafc7787e3c" "0x80087734"
}

build_and_run() {
    local name="$1"
    shift
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" "${SRC[@]}" -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.raw" 2>"$BUILD_DIR/$name.stderr"
    grep -v 'PSX RAM emulation' "$BUILD_DIR/$name.raw" >"$BUILD_DIR/$name.stdout" || true
    grep -v 'worldmap-scheduler' "$BUILD_DIR/$name.stdout" >"$BUILD_DIR/$name.norm" || true
}

require_retail
echo "== focused production regimes =="
build_and_run focused_O0 -O0 -g
build_and_run focused_O2 -O2
build_and_run focused_ubsan -O2 -g -fsanitize=undefined -fno-sanitize-recover=undefined

for regime in focused_O0 focused_O2 focused_ubsan; do
    grep -q '^W34B26 0x80087734 focused oracle PASS' "$BUILD_DIR/$regime.stdout"
    grep -v -e 'worldmap-scheduler' -e 'worldmap-stub' "$BUILD_DIR/$regime.stderr" > "$BUILD_DIR/$regime.diag" || true
    test ! -s "$BUILD_DIR/$regime.diag"
done
cmp "$BUILD_DIR/focused_O0.norm" "$BUILD_DIR/focused_O2.norm"
cmp "$BUILD_DIR/focused_O0.norm" "$BUILD_DIR/focused_ubsan.norm"
echo "FOCUSED O0/O2/UBSAN PASS; normalized output identical"

cp "$BUILD_DIR/focused_O0.stdout" "$SCRATCH/80087734_o0.log"
cp "$BUILD_DIR/focused_O2.stdout" "$SCRATCH/80087734_o2.log"
cp "$BUILD_DIR/focused_ubsan.stdout" "$SCRATCH/80087734_ubsan.log"
cp "$BUILD_DIR/focused_ubsan.diag" "$SCRATCH/80087734_ubsan.diag"

mutants=(WRONG_AND_MASK ROT_DEST_SCRATCH RETURN_0 VZ_UNMASKED)
echo "== mutants (must fail) =="
killed=0
for m in "${mutants[@]}"; do
    name="mutant_$m"
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" -O0 -DWM_87734_MUTANT_"$m" \
        "${SRC[@]}" -o "$BUILD_DIR/$name"
    if "$BUILD_DIR/$name" >"$BUILD_DIR/$name.raw" 2>"$BUILD_DIR/$name.stderr"; then
        echo "ERROR: mutant $m did not fail" >&2
        exit 1
    fi
    echo "killed $m"
    killed=$((killed + 1))
done
echo "MUTANTS $killed/${#mutants[@]} KILLED"
echo "$killed/${#mutants[@]}" > "$SCRATCH/80087734_mutants.txt"
echo "W34B26 0x80087734 ALL REGIMES PASS"
