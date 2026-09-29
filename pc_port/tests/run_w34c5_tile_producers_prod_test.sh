#!/usr/bin/env bash
# W34C5 — tile producers certificate (981C8 / 97DC0 / 980D4), O0/O2/UBSan + mutants.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34C5_BUILD_DIR:-$ROOT/pc_port/build_native}"
PSYX_CMAKE_LIB="$(find "$ROOT/pc_port/build" "$ROOT/pc_port/build_tsan" -name 'libpsycross.a' 2>/dev/null | head -1 || true)"
PSYCROSS_LIB="${W34C5_PSYCROSS_LIB:-${PSYX_CMAKE_LIB:-$BUILD_DIR/libpsycross.a}}"
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
SRC=(pc_port/tests/w34c5_tile_producers_prod_test.c pc_port/src/psx_memory.c
     pc_port/src/world_map_helper_981c8.c pc_port/src/world_map_helper_97dc0.c
     pc_port/src/world_map_helper_980d4.c)
build_and_run() { local name="$1"; shift
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" "${SRC[@]}" "${LIBS[@]}" -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name"; }
echo "== focused production regimes =="
build_and_run w34c5_tile_O0 -O0 -g
build_and_run w34c5_tile_O2 -O2
build_and_run w34c5_tile_ubsan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all
echo "== mutants (each must fail a named oracle assertion) =="
declare -A MUT=(
    [M1]="-DWM_981C8_MUTANT_M1"   # old shape: 8x8 window
    [M2]="-DWM_981C8_MUTANT_M2"   # no tz wrap against height
    [M3]="-DWM_97DC0_MUTANT_M3"   # old shape: (i + j) window index
    [M4]="-DWM_980D4_MUTANT_M4"   # old shape: queue-counter stub
    [M5]="-DWM_97DC0_MUTANT_M5"   # host pointer published in C184
    [M6]="-DWM_97DC0_MUTANT_M6"   # centre pass skipped
)
for m in M1 M2 M3 M4 M5 M6; do
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" ${MUT[$m]} -O0 "${SRC[@]}" "${LIBS[@]}" -o "$BUILD_DIR/w34c5_tile_mutant_$m"
    set +e; "$BUILD_DIR/w34c5_tile_mutant_$m" >"$BUILD_DIR/w34c5_tile_mutant_$m.log" 2>&1; rc=$?; set -e
    if [ "$rc" -eq 0 ] || ! grep -q 'ASSERTION ' "$BUILD_DIR/w34c5_tile_mutant_$m.log"; then
        echo "$m FAILED mutant gate (rc=$rc)" >&2; exit 1; fi
    echo "$m: DETECTED rc=$rc $(grep -o 'ASSERTION [a-z_0-9]* FAILED' "$BUILD_DIR/w34c5_tile_mutant_$m.log" | head -1)"
done
echo "W34C5 tile producers focused certificate PASS"
