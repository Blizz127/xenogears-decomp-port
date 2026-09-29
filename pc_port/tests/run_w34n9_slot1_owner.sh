#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root"
out="pc_port/build_tests/w34n9_slot1"
mkdir -p "$out"

base=(-DXENO_PC_PORT -DWM_72238_TEST_HOOKS -std=gnu17 -Wall -Wextra -Werror -Wconversion
      -Wsign-conversion -Iinclude -Ipc_port/src)
src=(pc_port/tests/w34n9_slot1_owner_prod_test.c
     pc_port/src/world_map_session_setup_72238.c)

for regime in O0 O2 UBSan; do
    flags=(-O0)
    [[ "$regime" == O2 ]] && flags=(-O2)
    [[ "$regime" == UBSan ]] && \
        flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all)
    gcc "${base[@]}" "${flags[@]}" "${src[@]}" -o "$out/$regime"
    "$out/$regime"
done

mutants=(
  "M1:W34N9_MUTANT_SKIP_TRANSITION:retail_order"
  "M2:W34N9_MUTANT_SWAP_POOL_TEMPLATE:retail_order"
  "M3:W34N9_MUTANT_SKIP_WDS_CLEANUP:retail_order"
  "M4:W34N9_MUTANT_SINGLE_CD_DRAIN:cd_drain_repeats"
  "M5:W34N9_MUTANT_SKIP_CONVERGENCE_P2:retail_order"
  "M6:W34N9_MUTANT_SWAP_COMMON_TAIL:retail_order"
  "M7:W34N22_MUTANT_SKIP_AUDIO_TRANSFER:restore.retail_order"
  "M8:W34N22_MUTANT_SKIP_RESTORE_COPY:restore.retail_order"
  "M9:W34N22_MUTANT_SWAP_RESTORE_PAIR:restore.retail_order"
  "M10:W34N22_MUTANT_SWAP_AUDIO_STORES:restore.retail_order"
  "M11:W34N22_MUTANT_CACHE_C894:restore.c894.reloaded"
  "M12:W34N25_MUTANT_SKIP_73398:ee6a.helper.exactly_once"
  "M13:W34N25_MUTANT_73398_FALLTHROUGH:ee6a.exclusive.arm"
  "M14:WM_RESUME_MUTANT_SKIP_BRANCH:restore.event_count"
  "M15:WM_72238_MUTANT_FRESH_TERRAIN_ON_RESUME:restore.retail_order"
)
for spec in "${mutants[@]}"; do
    IFS=: read -r name define assertion <<<"$spec"
    gcc "${base[@]}" -O2 -D"$define" "${src[@]}" -o "$out/$name"
    set +e
    "$out/$name" >"$out/$name.log" 2>&1
    rc=$?
    set -e
    if [[ $rc -eq 0 ]] || ! grep -q "ASSERTION $assertion FAILED" "$out/$name.log"; then
        echo "$name failed mutant gate rc=$rc expected=$assertion" >&2
        cat "$out/$name.log" >&2
        exit 1
    fi
    echo "$name DETECTED by ASSERTION $assertion"
done

if ! sed -n '/case 0x80072238u:/,+5p' \
        pc_port/src/world_map_main_loop_71034.c | grep -q 'wm_80072238\(\)'; then
    echo 'slot-1 dispatcher is not wired to wm_80072238' >&2
    exit 1
fi

echo 'W34N9 SLOT1 OWNER O0/O2/UBSan PASS; strict warnings clean; M1-M15 DETECTED'
