#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${W34N32_OUT:-$ROOT/pc_port/build_tests/w34n32_terminal_default}"
CC="${CC:-clang}"
mkdir -p "$OUT"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -DWM_71264_TEST_HOOKS -Wall -Wextra -Wconversion -Wsign-conversion
      -Werror -Ipc_port/include_shim -Iinclude
      -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx
      -Ipc_port/src)
SRC=(pc_port/tests/w34n32_terminal_default_prod_test.c
     pc_port/src/world_map_terminal_default_71264.c)

compile_and_run() {
    local name="$1"
    shift
    "$CC" "${BASE[@]}" "$@" "${SRC[@]}" -no-pie -o "$OUT/$name"
    "$OUT/$name" >"$OUT/$name.stdout" 2>"$OUT/$name.stderr"
    rg -q '^W34N32 terminal default certificate PASS$' "$OUT/$name.stdout"
}

compile_and_run O0 -O0 -g
compile_and_run O2 -O2
compile_and_run UBSan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan.stdout"

mutants=(
  'M1:W34N32_MUTANT_WRONG_STATE:state.change.zero'
  'M2:W34N32_MUTANT_WRONG_ORIGIN:clear.origin.zero'
  'M3:W34N32_MUTANT_WRONG_EXTENT:clear.extent.319.431'
  'M4:W34N32_MUTANT_WRONG_BLUE:clear.color.0.0.64'
  'M5:W34N32_MUTANT_SKIP_DIRECT_DRAW_SYNC:draw_sync.direct.once'
  'M6:W34N32_MUTANT_GUEST_SYSTEM_BYTE:system.byte.native.authority'
  'M7:W34N32_MUTANT_SWAP_EPILOGUE:order.retail.sequence'
  'M8:W34N32_MUTANT_WRONG_MAIN_ARG:main_loop.argument.zero'
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

rg -q 'world_map_terminal_default_71264.c' pc_port/build_port.sh
rg -q 'wm_71034_run_terminal_default_lane\(\);' \
    pc_port/src/world_map_main_loop_71034.c
echo 'W34N32 TERMINAL DEFAULT O0/O2/UBSan PASS; strict warnings clean; M1-M8 DETECTED'
