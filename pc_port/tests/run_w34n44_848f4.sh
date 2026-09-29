#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
CC="${CC:-gcc}"
OUT="${W34N44_OUT:-pc_port/build_native/w34n44-848f4-cert}"
TEST="pc_port/tests/w34n44_848f4_prod_test.c"
PROD="pc_port/src/world_map_helper_848f4.c"
BASE=(-std=gnu17 -DXENO_PC_PORT -DINCLUDE_ASM_USE_MACRO_INC=0 -fno-pie
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
    rg -q '^W34N44 0x800848F4 full-body certificate PASS$' \
        "$OUT/$regime.stdout"
    test ! -s "$OUT/$regime.stderr"
done
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan.stdout"
echo "CERTIFICATE O0/O2/UBSan PASS; strict warnings clean"

mutants=(
  'M1:WM_848F4_MUTANT_RECORD_STRIDE_04:record-stride-and-state-gate'
  'M2:WM_848F4_MUTANT_SKIP_STATE_GATE:record-stride-and-state-gate'
  'M3:WM_848F4_MUTANT_MATRIX_SOURCE_1C:matrix-copy-source'
  'M4:WM_848F4_MUTANT_SKIP_PARENT_CHAIN:parent-chain-order'
  'M5:WM_848F4_MUTANT_SKIP_PARENT_ROTATION:parent-chain-order'
  'M6:WM_848F4_MUTANT_NO_CAMERA_SUBTRACT:camera-relative-wrap-input'
  'M7:WM_848F4_MUTANT_SKIP_WRAP:wrapped-translation'
  'M8:WM_848F4_MUTANT_SCALE_1000:retail-scale'
  'M9:WM_848F4_MUTANT_REVERSE_COMPOSE:camera-compmatrix-order'
  'M10:WM_848F4_MUTANT_SKIP_FLAG_GATE:flag-and-depth-gates'
  'M11:WM_848F4_MUTANT_DEPTH_LIMIT_1000:flag-and-depth-gates'
  'M12:WM_848F4_MUTANT_FIXED_BUFFER_ZERO:renderer-domain-and-buffer'
  'M13:WM_848F4_MUTANT_VARIANT_UNSIGNED:signed-variant-table'
  'M14:WM_848F4_MUTANT_NO_MODEL_DISPATCH:flag-and-depth-gates'
  'M15:WM_848F4_MUTANT_RAW_GUEST_BUFFER:renderer-domain-and-buffer'
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

echo "W34N44 0x800848F4 FULL CERTIFICATE PASS; M1-M15 DETECTED"
