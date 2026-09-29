#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${W34N28_OUT:-$ROOT/pc_port/build_tests/w34n28_terminal_zero}"
CC="${CC:-clang}"
mkdir -p "$OUT"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -DWM_710E4_TEST_HOOKS -ffunction-sections -fdata-sections
      -Wall -Wextra -Wconversion -Wsign-conversion -Werror
      -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
      -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
SRC=(pc_port/tests/w34n28_terminal_zero_prod_test.c
     pc_port/src/world_map_terminal_zero_710e4.c
     pc_port/src/world_map_main_loop_71034.c
     pc_port/tests/world_map_mode_lifecycle_stubs.c)

compile_and_run() {
    local name="$1"
    shift
    "$CC" "${BASE[@]}" "$@" "${SRC[@]}" -no-pie \
        -Wl,--gc-sections -o "$OUT/$name"
    "$OUT/$name" >"$OUT/$name.stdout" 2>"$OUT/$name.stderr"
    rg -q '^W34N28 terminal zero certificate PASS$' "$OUT/$name.stdout"
}

compile_and_run O0 -O0 -g
compile_and_run O2 -O2
compile_and_run UBSan -O2 -g -fsanitize=undefined \
    -fno-sanitize-recover=all
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan.stdout"

mutants=(
  'M1:W34N28_MUTANT_SWAP_OVERLAY_STATE:order.guard.skip'
  'M2:W34N28_MUTANT_INVERT_BBC4_GUARD:guard.bbc4.skips.outputs'
  'M3:W34N28_MUTANT_SKIP_HELPER:guard.type3.calls.helper'
  'M4:W34N28_MUTANT_WRONG_HELPER_ARG:helper.retail.arguments'
  'M5:W34N28_MUTANT_STALE_RECORD:outputs.reload.helper.record'
  'M6:W34N28_MUTANT_GUEST_OUTPUTS:outputs.native.authority'
  'M7:W34N28_MUTANT_WRONG_EF68_OFFSET:ef68.bd0c.plus.400'
  'M8:W34N28_MUTANT_SKIP_SYSTEM_BYTE_CLEAR:system.byte.native.authority'
  'M9:W34N28_MUTANT_SWAP_EPILOGUE:order.guard.skip'
  'M10:W34N28_MUTANT_IGNORE_RECORD_TYPE:guard.non3.skips.helper'
)
for spec in "${mutants[@]}"; do
    IFS=: read -r name define assertion <<<"$spec"
    set +e
    compile_and_run "$name" -O2 -D"$define"
    rc=$?
    set -e
    if [[ $rc -eq 0 ]] ||
       ! rg -q "^ASSERTION $assertion FAILED$" "$OUT/$name.stderr"; then
        echo "$name failed mutant gate rc=$rc expected=$assertion" >&2
        cat "$OUT/$name.stderr" >&2
        exit 1
    fi
    echo "$name DETECTED by ASSERTION $assertion"
done

rg -q 'world_map_terminal_zero_710e4.c' pc_port/build_port.sh
rg -q 'wm_71034_run_terminal_zero_lane\(\);' \
    pc_port/src/world_map_main_loop_71034.c
echo 'W34N28 TERMINAL ZERO O0/O2/UBSan PASS; strict warnings clean; M1-M10 DETECTED'
