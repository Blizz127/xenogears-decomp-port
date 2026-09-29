#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
CC="${CC:-gcc}"
OUT="${W34N41_OUT:-pc_port/build_native/w34n41-99bfc-cert}"
TEST="pc_port/tests/w34n41_99bfc_prod_test.c"
PROD="pc_port/src/world_map_helper_99bfc.c"
BASE=(-std=gnu17 -DXENO_PC_PORT -DINCLUDE_ASM_USE_MACRO_INC=0 -fno-pie
      -Ipc_port/include_shim -Ipc_port/extern/PsyCross/include
      -Ipc_port/extern/PsyCross/include/psx -Iinclude -Ipc_port/src)
WARN=(-Wall -Wextra -Werror -Wconversion -Wsign-conversion -Wshadow -Wundef)

mkdir -p "$OUT"
compile_and_run() {
    local name="$1" rc=0
    shift
    rm -f "$OUT/$name" "$OUT/$name.stdout" "$OUT/$name.stderr"
    "$CC" "${BASE[@]}" "${WARN[@]}" "$@" "$TEST" "$PROD" \
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
compile_and_run UBSan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all
for regime in O0 O2 UBSan; do
    rg -q '^W34N41 0x80099BFC full-body certificate PASS$' \
        "$OUT/$regime.stdout"
    test ! -s "$OUT/$regime.stderr"
done
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan.stdout"
echo "CERTIFICATE O0/O2/UBSan PASS; focused warnings clean"

mutants=(
  'M1:WM_99BFC_MUTANT_OUTPUT_LIMIT_256:retail-output-ceiling'
  'M2:WM_99BFC_MUTANT_NO_CAMERA_SUBTRACT:camera-relative-wrap-and-z-sign'
  'M3:WM_99BFC_MUTANT_POSITIVE_Z:camera-relative-wrap-and-z-sign'
  'M4:WM_99BFC_MUTANT_SKIP_FLAG_GATE:retail-rejection-and-compaction'
  'M5:WM_99BFC_MUTANT_SKIP_SCREEN_GATE:retail-rejection-and-compaction'
  'M6:WM_99BFC_MUTANT_DEPTH_0C00:retail-rejection-and-compaction'
  'M7:WM_99BFC_MUTANT_NO_SHADE_CLAMP:shade-clamp-and-clut-selection'
  'M8:WM_99BFC_MUTANT_NO_PUBLICATION:retail-rejection-and-compaction'
  'M9:WM_99BFC_MUTANT_PACKET_STRIDE_24:retail-rejection-and-compaction'
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
echo "W34N41 0x80099BFC FULL CERTIFICATE PASS; M1-M9 DETECTED"
