#!/usr/bin/env bash
# W34C2 — PS-X static data load path certificate (O0/O2/UBSan + loader mutation controls).
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34C2_BUILD_DIR:-$ROOT/pc_port/build_native/w34c2-static-data}"
mkdir -p "$BUILD_DIR"

cd "$ROOT"
INC=(-Ipc_port/src)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
BASE=(-std=gnu17 -DXENO_PC_PORT)
SRC=(pc_port/tests/w34c2_static_data_prod_test.c pc_port/src/psx_memory.c)
image=disc/SLUS_006.64
digest=$(sha256sum < "$image")
[[ ${digest%% *} == dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119 ]]

build_and_run() {
    local name="$1"; shift
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" "${SRC[@]}" \
       -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name"
}

echo "== focused production regimes =="
build_and_run w34c2_static_data_O0 -O0 -g
build_and_run w34c2_static_data_O2 -O2
build_and_run w34c2_static_data_ubsan -O2 -g -fsanitize=undefined \
    -fno-sanitize-recover=all

echo "== mutants (each must fail a named oracle assertion) =="
declare -A MUT=(
    [M1]="-DPSX_EXE_MUTANT_M1"            # loader copies .text as well
    [M2]="-DPSX_EXE_MUTANT_M2"            # loader copies the ._49AC0 island
    [M4]="-DPSX_EXE_MUTANT_M4"            # sdata range extended into .sbss
    [M5]="-DPSX_EXE_MUTANT_M5"            # rodata range short by one page
)
for m in M1 M2 M4 M5; do
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" ${MUT[$m]} -O0 "${SRC[@]}" \
       -o "$BUILD_DIR/w34c2_static_data_mutant_$m"
    set +e
    "$BUILD_DIR/w34c2_static_data_mutant_$m" \
        >"$BUILD_DIR/w34c2_static_data_mutant_$m.log" 2>&1
    rc=$?
    set -e
    if [ "$rc" -eq 0 ] || ! grep -q 'ASSERTION ' \
        "$BUILD_DIR/w34c2_static_data_mutant_$m.log"; then
        echo "$m FAILED mutant gate (rc=$rc)" >&2
        exit 1
    fi
    echo "$m: DETECTED rc=$rc $(grep -o 'ASSERTION [a-z_0-9]* FAILED' "$BUILD_DIR/w34c2_static_data_mutant_$m.log" | head -1)"
done

echo "W34C2 static data focused certificate PASS"
