#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34B_73B04_BUILD_DIR:-$ROOT/pc_port/build_native}"
PSYX_CMAKE_LIB="$(find "$ROOT/pc_port/build" "$ROOT/pc_port/build_tsan" -name 'libpsycross.a' 2>/dev/null | head -1 || true)"
PSYCROSS_LIB="${W34B_73B04_PSYCROSS_LIB:-${PSYX_CMAKE_LIB:-$BUILD_DIR/libpsycross.a}}"
if [[ ! -f "$PSYCROSS_LIB" ]]; then
    echo "ERROR: libpsycross.a not found; run pc_port/build_port.sh first" >&2
    exit 1
fi
mkdir -p "$BUILD_DIR"

cd "$ROOT"
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C)
LIBS=("$PSYCROSS_LIB" -lSDL2 -lopenal -lGL -lm -ldl -lpthread)
SRC=(pc_port/tests/w34b_r4world_73b04_prod_test.c
     pc_port/src/psx_memory.c pc_port/src/world_map_helper_73b04.c)

build_and_run() {
    local name="$1"; shift
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" "${SRC[@]}" "${LIBS[@]}" \
        -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name"
}

echo "== focused production regimes =="
build_and_run w34b_r4world_73b04_O0 -O0 -g
build_and_run w34b_r4world_73b04_O2 -O2
build_and_run w34b_r4world_73b04_ubsan -O2 -g -fsanitize=undefined \
    -fno-sanitize-recover=all

echo "== copied-source mutants M1-M23 (each must fail named oracle assertion) =="
for mutant in $(seq 1 23); do
    trace=()
    if [ "$mutant" -eq 17 ]; then
        trace=(-DWM_73B04_TRACE)
    fi
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "${trace[@]}" \
        -DWM_73B04_MUTANT_M"$mutant" -O0 "${SRC[@]}" "${LIBS[@]}" \
        -o "$BUILD_DIR/w34b_r4world_73b04_mutant_M$mutant"
    set +e
    "$BUILD_DIR/w34b_r4world_73b04_mutant_M$mutant" \
        >"$BUILD_DIR/w34b_r4world_73b04_mutant_M$mutant.log" 2>&1
    rc=$?
    set -e
    if [ "$rc" -eq 0 ] || ! rg -q 'ASSERTION ' \
        "$BUILD_DIR/w34b_r4world_73b04_mutant_M$mutant.log"; then
        echo "M$mutant FAILED mutant gate (rc=$rc)" >&2
        exit 1
    fi
    echo "M$mutant: compile PASS, run rc=$rc, named assertion PASS"
done

echo "W34-R4WORLD-B 0x80073B04 focused certificate PASS"
