#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
CC="${CC:-gcc}"
OUT="${W34N47_OUT:-pc_port/build_native/w34n47-86798-cert}"
TEST="pc_port/tests/w34n47_86798_prod_test.c"
PROD="pc_port/src/world_map_helper_86798.c"
BASE=(-std=gnu17 -DXENO_PC_PORT -DINCLUDE_ASM_USE_MACRO_INC=0
      -DWM_86798_TEST_TRACE -fno-pie
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
    rg -q '^W34N47 0x80086798 full-body certificate PASS$' \
        "$OUT/$regime.stdout"
    test ! -s "$OUT/$regime.stderr"
done
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan.stdout"
echo "CERTIFICATE O0/O2/UBSan PASS; strict warnings clean"

mutants=(
  'M1:WM_86798_MUTANT_SKIP_CALLBACK:callback-advance'
  'M2:WM_86798_MUTANT_CAMERA_Z_OVERWRITES_X:camera-tile-slots'
  'M3:WM_86798_MUTANT_REVERSE_COMPOSITION:composition-order'
  'M4:WM_86798_MUTANT_SWAP_LOD:far-path-shape'
  'M5:WM_86798_MUTANT_NO_PUBLICATION:far-links'
  'M6:WM_86798_MUTANT_NO_PACKET_CEILING:packet-ceiling'
  'M7:WM_86798_MUTANT_FAR_STRICT_FLAG:far-sign-only-flag'
  'M8:WM_86798_MUTANT_MID_UV_SHIFTS:mid-uv-layout'
  'M9:WM_86798_MUTANT_NEAR_GRID_STEP_80:near-grid-step'
  'M10:WM_86798_MUTANT_PACKET_STRIDE_36:packet-stride'
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

echo "W34N47 0x80086798 FULL CERTIFICATE PASS; M1-M10 DETECTED"
