#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
CC="${CC:-gcc}"
OUT="${W34N40_OUT:-pc_port/build_native/w34n40-737ec-cert}"
TEST="pc_port/tests/w34n40_737ec_prod_test.c"
PROD="pc_port/src/world_map_helper_737ec.c"
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
    rg -q '^W34N40 0x800737EC full-body certificate PASS$' \
        "$OUT/$regime.stdout"
    test ! -s "$OUT/$regime.stderr"
done
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan.stdout"
echo "CERTIFICATE O0/O2/UBSan PASS; strict warnings clean"

mutants=(
  'M1:WM_737EC_MUTANT_HEADING_Z:heading-y-axis'
  'M2:WM_737EC_MUTANT_ROTATION_DEST_COMPOSED:matrix-scratch-addresses'
  'M3:WM_737EC_MUTANT_REVERSE_COMPOSITION:camera-left-composition'
  'M4:WM_737EC_MUTANT_KEEP_ROTATION_TRANSLATION:rotation-translation-cleared'
  'M5:WM_737EC_MUTANT_INSTALL_ROTATION:composed-matrix-installed'
  'M6:WM_737EC_MUTANT_THREE_QUADS:fixed-four-quads'
  'M7:WM_737EC_MUTANT_VERTEX_STRIDE_28:retail-vertex-addresses'
  'M8:WM_737EC_MUTANT_PACKET_BUFFER_48:retail-interleaved-packet-addresses'
  'M9:WM_737EC_MUTANT_PACKET_STRIDE_24:retail-interleaved-packet-addresses'
  'M10:WM_737EC_MUTANT_SKIP_FLAG_GATE:negative-flag-rejects-publication'
  'M11:WM_737EC_MUTANT_NO_DEPTH_SHIFT:retail-depth-shift-and-ot-buckets'
  'M12:WM_737EC_MUTANT_ANY_FLAG_REJECTS:positive-flag-is-not-rejected'
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

echo "W34N40 0x800737EC FULL CERTIFICATE PASS; M1-M12 DETECTED"
