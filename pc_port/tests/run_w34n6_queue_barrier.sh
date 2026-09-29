#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${W34N6_BUILD_DIR:-$ROOT/pc_port/build_native/w34n6}"
mkdir -p "$OUT"
cd "$ROOT"

INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
SRC=(pc_port/tests/w34n6_queue_barrier_prod_test.c
     pc_port/src/psx_memory.c pc_port/src/world_map_helper_96130.c)

run_regime() {
    local name="$1"
    shift
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" "${SRC[@]}" -lm \
        -o "$OUT/$name"
    "$OUT/$name"
}

echo "== focused production regimes =="
run_regime O0 -O0 -g
run_regime O2 -O2
run_regime ubsan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all

echo "== mutants =="
for entry in \
    "M1:WM_96694_MUTANT_PRECHECK:empty_do_while_vsync" \
    "M2:WM_96694_MUTANT_NO_VSYNC:empty_do_while_vsync" \
    "M3:WM_96694_MUTANT_THRESHOLD_TWO:linear_tail_drained" \
    "M4:WM_96694_MUTANT_REVERSED_ORDER:linear_vsync_before_dispatch"
do
    IFS=: read -r mutant define assertion <<<"$entry"
    gcc "${BASE[@]}" "${INC[@]}" -w -O0 -D"$define" "${SRC[@]}" -lm \
        -o "$OUT/$mutant"
    set +e
    "$OUT/$mutant" >"$OUT/$mutant.log" 2>&1
    rc=$?
    set -e
    if [[ "$rc" -eq 0 ]] || ! grep -q "ASSERTION $assertion FAILED" "$OUT/$mutant.log"; then
        echo "$mutant FAILED mutant gate (rc=$rc expected=$assertion)" >&2
        exit 1
    fi
    echo "$mutant: DETECTED ASSERTION $assertion FAILED"
done

echo "W34N6 queue barrier focused certificate PASS"
