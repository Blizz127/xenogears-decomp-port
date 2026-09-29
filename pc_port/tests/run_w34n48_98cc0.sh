#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
CC="${CC:-gcc}"
OUT="${W34N48_OUT:-pc_port/build_native/w34n48-98cc0-cert}"
TEST="pc_port/tests/w34n48_98cc0_prod_test.c"
PROD="pc_port/src/world_map_helper_98cc0.c"
BASE=(-std=gnu17 -DXENO_PC_PORT -DINCLUDE_ASM_USE_MACRO_INC=0
      -fno-pie -Ipc_port/include_shim -Ipc_port/extern/PsyCross/include
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
    rg -q '^W34N48 0x80098CC0 full-body certificate PASS$' \
        "$OUT/$regime.stdout"
    test ! -s "$OUT/$regime.stderr"
done
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan.stdout"
echo "CERTIFICATE O0/O2/UBSan PASS; strict warnings clean"

mutants=(
  'M1:WM_98CC0_MUTANT_M1:old-window-eviction'
  'M2:WM_98CC0_MUTANT_M2:ready-or-semantics'
  'M3:WM_98CC0_MUTANT_M3:primary-edge-order'
  'M4:WM_98CC0_MUTANT_M4:secondary-edge-order'
  'M5:WM_98CC0_MUTANT_M5:transposed-secondary'
  'M6:WM_98CC0_MUTANT_M6:closure-pass'
  'M7:WM_98CC0_MUTANT_M7:primary-source-priority'
  'M8:WM_98CC0_MUTANT_M8:path-secondary-shift'
  'M9:WM_98CC0_MUTANT_M9:guest-pointer-publication'
  'M10:WM_98CC0_MUTANT_M10:ready-flush-backend'
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

echo "W34N48 0x80098CC0 FULL CERTIFICATE PASS; M1-M10 DETECTED"
