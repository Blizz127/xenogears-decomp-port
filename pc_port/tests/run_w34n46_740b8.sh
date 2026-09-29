#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
CC="${CC:-gcc}"
OUT="${W34N46_OUT:-pc_port/build_native/w34n46-740b8-cert}"
TEST="pc_port/tests/w34n46_740b8_prod_test.c"
PROD="pc_port/src/world_map_helper_740b8.c"
BASE=(-std=gnu17 -DXENO_PC_PORT -DINCLUDE_ASM_USE_MACRO_INC=0
      -DWM_740B8_TEST_TRACE -fno-pie
      -Ipc_port/include_shim -Ipc_port/extern/PsyCross/include
      -Ipc_port/extern/PsyCross/include/psx -Iinclude -Ipc_port/src)
WARN=(-Wall -Wextra -Werror -Wconversion -Wsign-conversion -Wshadow -Wundef)

mkdir -p "$OUT"

compile_and_run() {
    local name="$1"
    local rc=0
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
compile_and_run UBSan -O2 -g -fsanitize=undefined \
    -fno-sanitize-recover=all
for regime in O0 O2 UBSan; do
    rg -q '^W34N46 0x800740B8 full-body certificate PASS$' \
        "$OUT/$regime.stdout"
    test ! -s "$OUT/$regime.stderr"
done
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan.stdout"
echo "CERTIFICATE O0/O2/UBSan PASS; strict warnings clean"

mutants=(
  'M1:WM_740B8_MUTANT_PACKET_STRIDE:project-packet-stride'
  'M2:WM_740B8_MUTANT_VERTEX_STRIDE:project-vertex-stride'
  'M3:WM_740B8_MUTANT_VERTEX_ORDER:project-vertex-order'
  'M4:WM_740B8_MUTANT_POSITION_OFFSET:matrix-translation'
  'M5:WM_740B8_MUTANT_SKIP_PROJECTED_LINKS:link-count-and-order'
  'M6:WM_740B8_MUTANT_MASK_LEFT:link-count-and-order'
  'M7:WM_740B8_MUTANT_SPECIAL_SOURCE:special-coordinates'
  'M8:WM_740B8_MUTANT_DEFAULT_COORD:default-coordinate'
  'M9:WM_740B8_MUTANT_SKIP_MODE_PACKET:link-count-and-order'
  'M10:WM_740B8_MUTANT_SKIP_FINAL_PACKET:link-count-and-order'
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

echo "W34N46 0x800740B8 FULL CERTIFICATE PASS; M1-M10 DETECTED"
