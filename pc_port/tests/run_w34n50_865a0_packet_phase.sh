#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
CC="${CC:-gcc}"
OUT="${W34N50_OUT:-pc_port/build_native/w34n50-865a0-cert}"
TEST="pc_port/tests/w34b5d_800865a0_prod_test.c"
PROD="pc_port/src/world_map_common_tail.c"
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -fno-pie
      -Ipc_port/include -Iinclude -Ipc_port/src)
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
    rg -q '^=== Results: 68/68 PASS ===$' "$OUT/$regime.stdout"
    ! rg -q '^  FAIL:|runtime error:' \
        "$OUT/$regime.stdout" "$OUT/$regime.stderr"
done
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan.stdout"
echo 'CERTIFICATE O0/O2/UBSan PASS; strict warnings clean'

set +e
compile_and_run M1 -O0 -g -DWM_865A0_MUTANT_SHIFTED_HEADER
rc=$?
set -e
if [[ "$rc" -eq 0 ]] ||
   ! rg -q '^  FAIL: record0 byte\[3\] == 9$' "$OUT/M1.stdout"; then
    echo "M1 FAILED mutant gate assertion=packet-header-at-record-start rc=$rc" >&2
    exit 1
fi
echo 'M1 DETECTED; ASSERTION packet-header-at-record-start'
echo 'W34N50 0x800865A0 PACKET-PHASE CERTIFICATE PASS; M1 DETECTED'
