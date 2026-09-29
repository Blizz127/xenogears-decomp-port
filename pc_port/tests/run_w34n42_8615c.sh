#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
CC="${CC:-gcc}"
OUT="${W34N42_OUT:-pc_port/build_native/w34n42-8615c-cert}"
TEST=pc_port/tests/w34n42_8615c_prod_test.c
PROD=pc_port/src/world_map_helper_8615c.c
BASE=(-std=gnu17 -DXENO_PC_PORT -DINCLUDE_ASM_USE_MACRO_INC=0 -fno-pie
      -Ipc_port/include_shim -Ipc_port/extern/PsyCross/include
      -Ipc_port/extern/PsyCross/include/psx -Iinclude -Ipc_port/src)
WARN=(-Wall -Wextra -Werror -Wconversion -Wsign-conversion -Wshadow -Wundef)
mkdir -p "$OUT"
run_one() {
  local name="$1" rc=0; shift
  "$CC" "${BASE[@]}" "${WARN[@]}" "$@" "$TEST" "$PROD" -no-pie -o "$OUT/$name"
  if "$OUT/$name" >"$OUT/$name.out" 2>"$OUT/$name.err"; then rc=0; else rc=$?; fi
  return "$rc"
}
run_one O0 -O0 -g
run_one O2 -O2 -g
run_one UBSan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all
for r in O0 O2 UBSan; do
  rg -q '^W34N42 0x8008615C full-body certificate PASS$' "$OUT/$r.out"
  test ! -s "$OUT/$r.err"
done
cmp "$OUT/O0.out" "$OUT/O2.out"; cmp "$OUT/O0.out" "$OUT/UBSan.out"
echo 'CERTIFICATE O0/O2/UBSan PASS; focused warnings clean'
mutants=(
 'M1:WM_8615C_MUTANT_POSITIVE_HEADING:matrix-copy-and-negative-heading'
 'M2:WM_8615C_MUTANT_EIGHT_CLUTS:retail-sixteen-clut-copy'
 'M3:WM_8615C_MUTANT_FOUR_BY_FOUR:retail-five-by-five-validity-and-map-index'
 'M4:WM_8615C_MUTANT_VALIDITY_STRIDE_9:retail-five-by-five-validity-and-map-index'
 'M5:WM_8615C_MUTANT_SKIP_VALIDITY_GATE:retail-five-by-five-validity-and-map-index'
 'M6:WM_8615C_MUTANT_MAP_WIDTH_8:retail-five-by-five-validity-and-map-index'
 'M7:WM_8615C_MUTANT_FIXED_PACKET_ROOT:active-ot-and-interleaved-packet-root'
)
for e in "${mutants[@]}"; do
  l="${e%%:*}"; x="${e#*:}"; d="${x%%:*}"; a="${x#*:}"
  set +e; run_one "$l" -O0 -g -D"$d"; rc=$?; set -e
  if [[ $rc -eq 0 ]] || ! rg -q "^ASSERTION $a$" "$OUT/$l.err"; then
    echo "$l FAILED assertion=$a rc=$rc" >&2; exit 1
  fi
  echo "$l DETECTED; ASSERTION $a"
done
echo 'W34N42 0x8008615C FULL CERTIFICATE PASS; M1-M7 DETECTED'
