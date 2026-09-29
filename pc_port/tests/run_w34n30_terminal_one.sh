#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${W34N30_OUT:-$ROOT/pc_port/build_tests/w34n30_terminal_one}"
CC="${CC:-clang}"
mkdir -p "$OUT"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -DWM_711B0_TEST_HOOKS -Wall -Wextra -Wconversion -Wsign-conversion
      -Werror -Ipc_port/include_shim -Iinclude
      -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx
      -Ipc_port/src)
SRC=(pc_port/tests/w34n30_terminal_one_prod_test.c
     pc_port/src/world_map_terminal_one_711b0.c)

compile_and_run() {
    local name="$1"
    shift
    "$CC" "${BASE[@]}" "$@" "${SRC[@]}" -no-pie -o "$OUT/$name"
    "$OUT/$name" >"$OUT/$name.stdout" 2>"$OUT/$name.stderr"
    rg -q '^W34N30 terminal one certificate PASS$' "$OUT/$name.stdout"
}

compile_and_run O0 -O0 -g
compile_and_run O2 -O2
compile_and_run UBSan -O2 -g -fsanitize=undefined \
    -fno-sanitize-recover=all
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan.stdout"

mutants=(
  'M1:W34N30_MUTANT_SWAP_OVERLAY_STATE:order.retail.sequence'
  'M2:W34N30_MUTANT_GUEST_594F8:byte594f8.native.authority'
  'M3:W34N30_MUTANT_WRONG_PARTY_STRIDE:party.halfword.stride'
  'M4:W34N30_MUTANT_SKIP_SOUND_CLEANUP:sound.cleanup.once'
  'M5:W34N30_MUTANT_WRONG_ARCHIVE_ID:archive.id.from.bcc8'
  'M6:W34N30_MUTANT_WRONG_COPY_SOURCE:copy.source.exact'
  'M7:W34N30_MUTANT_SKIP_OLD_MANAGER_SAVE:manager.old.saved'
  'M8:W34N30_MUTANT_SKIP_NEW_MANAGER_PUBLISH:manager.new.published'
  'M9:W34N30_MUTANT_WRONG_CONFIGURE_LEVEL:manager.configure.127.0'
  'M10:W34N30_MUTANT_SWAP_EPILOGUE:order.retail.sequence'
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

rg -q 'world_map_terminal_one_711b0.c' pc_port/build_port.sh
rg -q 'wm_71034_run_terminal_one_lane\(\);' \
    pc_port/src/world_map_main_loop_71034.c
echo 'W34N30 TERMINAL ONE O0/O2/UBSan PASS; strict warnings clean; M1-M10 DETECTED'
