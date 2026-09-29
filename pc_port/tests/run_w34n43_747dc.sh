#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
CC="${CC:-gcc}"
OUT="${W34N43_OUT:-pc_port/build_native/w34n43-747dc-cert}"
TEST="pc_port/tests/w34n43_747dc_prod_test.c"
PROD="pc_port/src/world_map_helper_747dc.c"
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
    rg -q '^W34N43 0x800747DC full-body certificate PASS$' \
        "$OUT/$regime.stdout"
    test ! -s "$OUT/$regime.stderr"
done
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan.stdout"
echo "CERTIFICATE O0/O2/UBSan PASS; strict warnings clean"

mutants=(
  'M1:WM_747DC_MUTANT_FIXED_QUEUE:queue-record-source-and-stride'
  'M2:WM_747DC_MUTANT_DROP_LAST_RECORD:exact-record-count'
  'M3:WM_747DC_MUTANT_RECORD_STRIDE_10:queue-record-source-and-stride'
  'M4:WM_747DC_MUTANT_TERRAIN_Z_FROM_X:queue-record-source-and-stride'
  'M5:WM_747DC_MUTANT_REVERSE_SECOND_CROSS:basis-cross-order'
  'M6:WM_747DC_MUTANT_MODE1_SCALE_1000:mode-scale-values'
  'M7:WM_747DC_MUTANT_SKIP_MODE2_ROTATION:mode2-rotation-chain'
  'M8:WM_747DC_MUTANT_NO_CAMERA_SUBTRACT:camera-relative-position'
  'M9:WM_747DC_MUTANT_WRONG_VERTEX2:projection-vertex-order'
  'M10:WM_747DC_MUTANT_SKIP_FLAG_GATE:negative-flag-stops-fourth-vertex'
  'M11:WM_747DC_MUTANT_MAXIMUM_DEPTH:minimum-depth-and-retail-limit'
  'M12:WM_747DC_MUTANT_DEPTH_LIMIT_0C00:minimum-depth-and-retail-limit'
  'M13:WM_747DC_MUTANT_PACKET_STRIDE_20:compact-packet-cursor'
  'M14:WM_747DC_MUTANT_NO_PUBLICATION:minimum-depth-and-retail-limit'
  'M15:WM_747DC_MUTANT_NO_QUEUE_CLEAR:queue-drained'
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

echo "W34N43 0x800747DC FULL CERTIFICATE PASS; M1-M15 DETECTED"
