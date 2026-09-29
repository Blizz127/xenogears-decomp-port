#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34B21_C2B_BUILD_DIR:-$ROOT/pc_port/build_native/w34b21_c2b}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

INC=(-Iinclude -Ipc_port/src)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -DWM_74794_TEST_TRACE)
SRC=(pc_port/tests/w34b21_c2b_74794_prod_test.c
     pc_port/src/world_map_helper_74794.c pc_port/src/psx_memory.c)

build_and_run() {
    local name="$1"
    shift
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" "${SRC[@]}" \
        -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name"
}

echo "== focused production regimes =="
build_and_run helper_74794_O0 -O0 -g
build_and_run helper_74794_O2 -O2
build_and_run helper_74794_ubsan -O2 -g -fsanitize=undefined \
    -fno-sanitize-recover=all

echo "== required mutants =="
mutants=(WRONG_MASK WRONG_STRIDE WRONG_X_OFFSET WRONG_Z_OFFSET WRONG_SHIFT
         UNSIGNED_SHIFT WRONG_TAG_STORE INDEX_BEFORE_WRITES STORE_U32)
for mutant in "${mutants[@]}"; do
    exe="$BUILD_DIR/helper_74794_mutant_${mutant}"
    log="$exe.log"
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" -O2 \
        -DWM_74794_MUTANT_${mutant} "${SRC[@]}" -o "$exe"
    set +e
    "$exe" >"$log" 2>&1
    rc=$?
    set -e
    if [ "$rc" -eq 0 ] || ! rg -q '^ASSERTION ' "$log"; then
        echo "$mutant FAILED mutant gate (rc=$rc)" >&2
        exit 1
    fi
    echo "$mutant: rejected by named assertion"
done

echo "W34B21-C2B 0x80074794 focused certificate PASS"
