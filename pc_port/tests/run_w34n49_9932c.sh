#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
CC="${CC:-gcc}"
OUT="${W34N49_OUT:-pc_port/build_native/w34n49-9932c-cert}"
TEST="pc_port/tests/w34n49_9932c_prod_test.c"
PROD="pc_port/src/world_map_helper_9932c.c"
MEMORY="pc_port/src/psx_memory.c"
BASE=(-std=gnu17 -DXENO_PC_PORT -DINCLUDE_ASM_USE_MACRO_INC=0
      -fno-pie -Ipc_port/include_shim -Ipc_port/extern/PsyCross/include
      -Ipc_port/extern/PsyCross/include/psx -Iinclude -Ipc_port/src)
WARN=(-Wall -Wextra -Werror -Wconversion -Wsign-conversion -Wshadow -Wundef)

mkdir -p "$OUT"

compile_and_run() {
    local name="$1"
    local rc=0
    shift
    rm -f "$OUT/$name" "$OUT/$name.raw" "$OUT/$name.stdout" \
        "$OUT/$name.stderr"
    "$CC" "${BASE[@]}" "${WARN[@]}" "$@" "$TEST" "$PROD" "$MEMORY" \
        -no-pie -o "$OUT/$name"
    if "$OUT/$name" >"$OUT/$name.raw" 2>"$OUT/$name.stderr"; then
        rc=0
    else
        rc=$?
    fi
    rg -v 'PSX RAM emulation' "$OUT/$name.raw" >"$OUT/$name.stdout" || true
    return "$rc"
}

compile_and_run O0 -O0 -g
compile_and_run O2 -O2 -g
compile_and_run UBSan -O2 -g -fsanitize=undefined \
    -fno-sanitize-recover=all
for regime in O0 O2 UBSan; do
    rg -q '^W34N49 0x8009932C exact-side-effect certificate PASS$' \
        "$OUT/$regime.stdout"
    test ! -s "$OUT/$regime.stderr"
done
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan.stdout"
echo "CERTIFICATE O0/O2/UBSan PASS; strict warnings clean"

set +e
compile_and_run M1 -O0 -g -DWM_9932C_MUTANT_SKIPPED_ORIGINS
rc=$?
set -e
if [[ "$rc" -eq 0 ]] ||
   ! rg -q '^ASSERTION skipped-cells-preserve-origin-scratch$' \
       "$OUT/M1.stderr"; then
    echo "M1 FAILED mutant gate rc=$rc" >&2
    exit 1
fi
echo "M1 DETECTED; ASSERTION skipped-cells-preserve-origin-scratch"

echo "W34N49 0x8009932C EXACTNESS CERTIFICATE PASS; M1 DETECTED"
