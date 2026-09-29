#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34B25_BUILD_DIR:-$ROOT/pc_port/build_native/w34b25_800907f4}"
SCRATCH="${W34B25_SCRATCH:-/tmp/grok-goal-21964a4dfe76/implementer}"
mkdir -p "$BUILD_DIR" "$SCRATCH"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DWM_907F4_TEST_TRACE)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/src)
SRC=(pc_port/tests/w34b25_800907f4_prod_test.c
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
    local fixture="disc/world_map.bin"
    local expected="4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70"
    local actual
    if [[ ! -f "$fixture" ]]; then
        echo "ERROR: missing $fixture" >&2
        exit 1
    fi
    if [[ "$(stat -Lc '%s' "$fixture")" != "180422" ]]; then
        echo "ERROR: $fixture size is not 180422" >&2
        exit 1
    fi
    actual="$(sha256sum "$fixture" | awk '{print $1}')"
    if [[ "$actual" != "$expected" ]]; then
        echo "ERROR: world_map.bin SHA mismatch: $actual" >&2
        exit 1
    fi
    # 0x800907F4 - 0x8006FAF0 = 0x20D04, 548 bytes.
    check_slice 0x20D04 548 "bd2946bda218185bd796696ef10c845d09f2ebfb9a99814d090e303ae46cc91b" "0x800907F4"
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
    grep -q '^W34B25 0x800907F4 focused oracle PASS' "$BUILD_DIR/$regime.stdout"
    grep -v -e 'worldmap-scheduler' -e 'worldmap-stub' "$BUILD_DIR/$regime.stderr" > "$BUILD_DIR/$regime.diag" || true
    test ! -s "$BUILD_DIR/$regime.diag"
done
cmp "$BUILD_DIR/focused_O0.norm" "$BUILD_DIR/focused_O2.norm"
cmp "$BUILD_DIR/focused_O0.norm" "$BUILD_DIR/focused_ubsan.norm"
echo "FOCUSED O0/O2/UBSAN PASS; normalized output identical"

cp "$BUILD_DIR/focused_O0.stdout" "$SCRATCH/800907f4_o0.log"
cp "$BUILD_DIR/focused_O2.stdout" "$SCRATCH/800907f4_o2.log"
cp "$BUILD_DIR/focused_ubsan.stdout" "$SCRATCH/800907f4_ubsan.log"
cp "$BUILD_DIR/focused_ubsan.diag" "$SCRATCH/800907f4_ubsan.log.diag"
test ! -s "$BUILD_DIR/focused_ubsan.diag"

mutants=(
    SLOT20_OFFSET
    INC_THRESHOLD
    MODE0_SKIP_5C
    ROT_DEST_SCRATCH
    RETURN_0
    VZ_ZERO
    FLAG_SKIP
)

echo "== mutants (must fail) =="
killed=0
for m in "${mutants[@]}"; do
    name="mutant_$m"
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" -O0 -DWM_907F4_MUTANT_"$m" \
        "${SRC[@]}" -o "$BUILD_DIR/$name"
    if "$BUILD_DIR/$name" >"$BUILD_DIR/$name.raw" 2>"$BUILD_DIR/$name.stderr"; then
        echo "ERROR: mutant $m did not fail" >&2
        exit 1
    fi
    echo "killed $m"
    killed=$((killed + 1))
done
echo "MUTANTS $killed/${#mutants[@]} KILLED"
echo "$killed/${#mutants[@]}" > "$SCRATCH/800907f4_mutants.txt"
echo "W34B25 0x800907F4 ALL REGIMES PASS"
