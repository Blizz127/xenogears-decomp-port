#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
CC="${CC:-gcc}"
OUT="${W34N45_OUT:-pc_port/build_native/w34n45-visibility-mask-cert}"
TEST="pc_port/tests/w34n45_visibility_mask_prod_test.c"
PROD=(pc_port/src/world_map_helper_983a0.c pc_port/src/world_map_helper_987ac.c)
BASE=(-std=gnu17 -DXENO_PC_PORT -DINCLUDE_ASM_USE_MACRO_INC=0
      -DWM_983A0_TEST_TRACE -DWM_987AC_TEST_TRACE -fno-pie
      -Ipc_port/include_shim -Ipc_port/extern/PsyCross/include
      -Ipc_port/extern/PsyCross/include/psx -Iinclude -Ipc_port/src)
WARN=(-Wall -Wextra -Werror -Wconversion -Wsign-conversion -Wshadow -Wundef)

mkdir -p "$OUT"

compile_and_run() {
    local name="$1"
    local rc=0
    shift
    rm -f "$OUT/$name" "$OUT/$name.stdout" "$OUT/$name.stderr"
    "$CC" "${BASE[@]}" "${WARN[@]}" "$@" "$TEST" "${PROD[@]}" \
        -no-pie -o "$OUT/$name"
    if "$OUT/$name" >"$OUT/$name.stdout" 2>"$OUT/$name.stderr"; then
        rc=0
    else
        rc=$?
    fi
    return "$rc"
}

compile_and_run O0 -O0 -g
compile_and_run O2 -O2 -g
compile_and_run UBSan -O2 -g -fsanitize=undefined \
    -fno-sanitize-recover=all

for regime in O0 O2 UBSan; do
    rg -q '^W34N45 0x800983A0/0x800987AC full-body certificate PASS$' \
        "$OUT/$regime.stdout"
    test ! -s "$OUT/$regime.stderr"
done
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan.stdout"
echo "CERTIFICATE O0/O2/UBSan PASS; strict warnings clean"

mutants=(
  'M1:WM_983A0_MUTANT_MATRIX_SOURCE:matrix-copy-source'
  'M2:WM_983A0_MUTANT_GRID_STEP:grid-cell-layout'
  'M3:WM_983A0_MUTANT_SKIP_SUBDIVISION:classify-call-count'
  'M4:WM_983A0_MUTANT_QUADRANT:boundary-table-selection'
  'M5:WM_983A0_MUTANT_REPLACE_MASK:boundary-mask-or'
  'M6:WM_987AC_MUTANT_SKIP_CORNER3:classifier-four-transforms'
  'M7:WM_987AC_MUTANT_OUTSIDE_ZERO:classifier-outside-tristate'
  'M8:WM_987AC_MUTANT_CORNER_ORDER:classifier-intersection-order'
  'M9:WM_987AC_MUTANT_WRONG_YZ_COMPONENT:classifier-intersection-order'
  'M10:WM_987AC_MUTANT_INSIDE_ZERO:classifier-inside-tristate'
)

for entry in "${mutants[@]}"; do
    label="${entry%%:*}"
    rest="${entry#*:}"
    define="${rest%%:*}"
    assertion="${rest#*:}"
    set +e
    compile_and_run "$label" -O0 -g -D"$define"
    rc=$?
    set -e
    if [[ "$rc" -eq 0 ]] ||
       ! rg -q "^ASSERTION $assertion$" "$OUT/$label.stderr"; then
        echo "$label FAILED mutant gate assertion=$assertion rc=$rc" >&2
        exit 1
    fi
    echo "$label DETECTED; ASSERTION $assertion"
done

echo "W34N45 VISIBILITY MASK CERTIFICATE PASS; M1-M10 DETECTED"
