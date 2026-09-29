#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34B22_I5_BUILD_DIR:-$ROOT/pc_port/build_native/w34b22_i5}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
      -DWM_94A5C_TEST_TRACE -DWM_94A5C_TEST_HOOK)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/src)
SRC=(pc_port/tests/w34b22_i5_94a5c_integration_test.c
     pc_port/src/psx_memory.c
     pc_port/src/world_map_func_94a5c.c)

build_and_run() {
    local name="$1"
    shift
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" "${SRC[@]}" -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.raw" 2>"$BUILD_DIR/$name.stderr"
    rg -v 'PSX RAM emulation' "$BUILD_DIR/$name.raw" >"$BUILD_DIR/$name.stdout" || true
}

build_and_run focused_O0 -O0 -g
build_and_run focused_O2 -O2
build_and_run focused_ubsan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all
for name in focused_O0 focused_O2 focused_ubsan; do
    rg -q '^W34B22-I5 0x80094A5C PASS cases=9 routing\+tail verified$' "$BUILD_DIR/$name.stdout"
    test ! -s "$BUILD_DIR/$name.stderr"
done
cmp "$BUILD_DIR/focused_O0.stdout" "$BUILD_DIR/focused_O2.stdout"
cmp "$BUILD_DIR/focused_O0.stdout" "$BUILD_DIR/focused_ubsan.stdout"
echo "FOCUSED O0/O2/UBSAN PASS; normalized output identical"

mutants=(
    SRA11 THRESHOLD10 SWAP_PATTERN WRONG_MASK TAIL_NOCOPY
)
killed=0
for mutant in "${mutants[@]}"; do
    name="mutant_$mutant"
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" -O0 \
        -DWM_94A5C_MUTANT_"$mutant" \
        "${SRC[@]}" -o "$BUILD_DIR/$name"
    set +e
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" 2>"$BUILD_DIR/$name.stderr"
    rc=$?
    set -e
    if [ "$rc" -eq 0 ] || ! rg -q '^ASSERTION ' "$BUILD_DIR/$name.stderr"; then
        echo "$mutant FAILED mutant gate rc=$rc" >&2
        tail -30 "$BUILD_DIR/$name.stderr" >&2 || true
        exit 1
    fi
    echo "$mutant KILLED rc=$rc $(rg -m1 '^ASSERTION ' "$BUILD_DIR/$name.stderr")"
    killed=$((killed + 1))
done
echo "MUTANTS $killed/${#mutants[@]} KILLED"
