#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34CB1_90A84_BUILD_DIR:-/tmp/w34cb1-90a84-build}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -DWM_90A84_TEST_TRACE)
SRC=(pc_port/tests/w34cb1_90a84_prod_test.c
     pc_port/src/psx_memory.c pc_port/src/world_map_helper_90a84.c
     pc_port/src/battle_mips_adapter.c)

build_and_run() {
    local name="$1"; shift
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" "${SRC[@]}" \
        -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name"
}

build_and_run w34cb1_90a84_O0 -O0 -g
build_and_run w34cb1_90a84_O2 -O2
build_and_run w34cb1_90a84_ubsan -O2 -g -fsanitize=undefined \
    -fno-sanitize-recover=all

echo "== copied-source mutants M1-M16 (each must fail a named oracle assertion) =="
for mutant in $(seq 1 16); do
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" \
        -DWM_90A84_MUTANT_M"$mutant" -O0 "${SRC[@]}" \
        -o "$BUILD_DIR/w34cb1_90a84_mutant_M$mutant"
    set +e
    "$BUILD_DIR/w34cb1_90a84_mutant_M$mutant" \
        >"$BUILD_DIR/w34cb1_90a84_mutant_M$mutant.log" 2>&1
    rc=$?
    set -e
    if [ "$rc" -eq 0 ] || ! grep -q 'ASSERTION ' \
        "$BUILD_DIR/w34cb1_90a84_mutant_M$mutant.log"; then
        echo "M$mutant FAILED mutant gate (rc=$rc)" >&2
        exit 1
    fi
    echo "M$mutant: compile PASS, run rc=$rc, named assertion PASS"
done

echo "W34-CB1-8A72C-NP 0x80090A84 focused certificate PASS"
