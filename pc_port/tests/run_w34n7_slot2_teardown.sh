#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${W34N7_BUILD_DIR:-$ROOT/pc_port/build_native/w34n7}"
mkdir -p "$OUT"
cd "$ROOT"

INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -DWM_7299C_PROD_TEST -DWM_7299C_TEST_HOOKS
      -ffunction-sections -fdata-sections)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
SRC=(pc_port/tests/w34n7_slot2_teardown_prod_test.c
     pc_port/src/psx_memory.c pc_port/src/world_map_teardown_7299c.c
     pc_port/src/world_map_helper_7565c.c
     pc_port/src/world_map_helper_86124.c
     pc_port/src/world_map_main_loop_71034.c
     pc_port/tests/world_map_mode_lifecycle_stubs.c)

run_regime() {
    local name="$1"
    shift
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" "${SRC[@]}" -lm \
        -Wl,--gc-sections \
        -o "$OUT/$name"
    "$OUT/$name"
}

echo "== focused production regimes =="
run_regime O0 -O0 -g
run_regime O2 -O2
run_regime ubsan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all

echo "== mutants =="
for entry in \
    "M1:WM_7299C_MUTANT_AUDIO_ALWAYS:state1_no_fade" \
    "M2:WM_7299C_MUTANT_POOL_63:slot63_cleared" \
    "M3:WM_7299C_MUTANT_NO_SLOT_CLEAR:slot63_cleared" \
    "M4:WM_7299C_MUTANT_SKIP_SNAPSHOT:snapshot_pool_copy" \
    "M5:WM_7299C_MUTANT_SWAP_WINDOW_ORDER:window_order" \
    "M6:WM_7299C_MUTANT_SKIP_SECONDARY_PAIR:secondary_pair_freed" \
    "M7:WM_7299C_MUTANT_GUEST_SNAPSHOT:snapshot_native_authority" \
    "M8:WM_7299C_MUTANT_ZERO_SNAPSHOT_HOLES:snapshot_sparse_holes_preserved" \
    "M9:WM_7299C_MUTANT_SWAP_SNAPSHOT_TAIL:snapshot_retail_store_order" \
    "M10:WM_7299C_MUTANT_GUEST_TRANSITION:transition_flag"
do
    IFS=: read -r mutant define assertion <<<"$entry"
    gcc "${BASE[@]}" "${INC[@]}" -w -O0 -D"$define" "${SRC[@]}" -lm \
        -Wl,--gc-sections \
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

grep -q 'case 0x8007299Cu:' pc_port/src/world_map_main_loop_71034.c
grep -A2 'case 0x8007299Cu:' pc_port/src/world_map_main_loop_71034.c | \
    grep -q 'wm_8007299C();'

echo "W34N7 slot2 teardown focused certificate PASS"
