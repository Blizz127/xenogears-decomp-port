#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

repo_root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$repo_root"

fixture=disc/world_map.bin
expected_world=4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70
test "$(sha256sum "$fixture" | awk '{print $1}')" = "$expected_world"
test "$(dd if="$fixture" bs=1 skip=$((0x5dd0)) count=$((0x298)) status=none | sha256sum | awk '{print $1}')" = 90555ee38c3a95ed3839b77a42081a79db4f1d2112b58944103d0e8fd2d864d1
test "$(dd if="$fixture" bs=1 skip=$((0x6068)) count=$((0x1f4)) status=none | sha256sum | awk '{print $1}')" = a3d88489847d346d2544b8f1fc404685904d336f33ac62537037f403d49b1977

build_dir=pc_port/build_tests/w34n17_menu_lifecycle
mkdir -p "$build_dir"
base=(-DXENO_PC_PORT -std=c11 -Wall -Wextra -Werror -Wconversion -Wsign-conversion
      -Ipc_port -Ipc_port/src -Iinclude)
src=(pc_port/tests/w34n17_menu_lifecycle_prod_test.c
     pc_port/src/psx_memory.c pc_port/src/world_map_menu_lifecycle.c)

for regime in o0 o2 ubsan; do
    case "$regime" in
        o0) flags=(-O0) ;;
        o2) flags=(-O2) ;;
        ubsan) flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=undefined) ;;
    esac
    gcc "${base[@]}" "${flags[@]}" "${src[@]}" -o "$build_dir/$regime"
    "$build_dir/$regime"
done

mutants=(
    "m1:W34N17_MUTANT_SWAP_ENTRY_FREE_ORDER:entry.retail.teardown.order"
    "m2:W34N17_MUTANT_WRONG_SCRATCH_SIZE:entry.alloc.sizes.and.scratch.math"
    "m3:W34N17_MUTANT_SKIP_CONDITIONAL_MOVE:entry.conditional.and.fixed.moves"
    "m4:W34N17_MUTANT_WRONG_SYNC_PREDICATE:entry.archive.polls.until.below.two"
    "m5:W34N17_MUTANT_WRONG_OT_COUNT:return.clears.active.guest.ot"
    "m6:W34N17_MUTANT_SWAP_REBUILD_ORDER:return.rebuild.and.reconcile.order"
    "m7:W34N17_MUTANT_SKIP_FINAL_RESTORE:return.restores.final.state"
    "m8:W34N17_MUTANT_SKIP_8440C:return.reenters.8440C"
)
for spec in "${mutants[@]}"; do
    IFS=: read -r name define assertion <<<"$spec"
    exe="$build_dir/$name"; log="$build_dir/$name.log"
    gcc "${base[@]}" -O2 -D"$define" "${src[@]}" -o "$exe"
    if "$exe" >"$log" 2>&1; then
        echo "W34N17 MUTANT $name SURVIVED" >&2; exit 1
    fi
    if ! rg -q "ASSERTION $assertion" "$log"; then
        echo "W34N17 MUTANT $name failed without named assertion $assertion" >&2
        cat "$log" >&2; exit 1
    fi
    echo "W34N17 MUTANT $name DETECTED by $assertion"
done

rg -q '^int wm_8008440C\(void\)$' pc_port/src/world_map_init.c
if rg -q 'already ran this process.*BD20|if \(s_wm8440c_(ran|completed)\)' pc_port/src/world_map_init.c; then
    echo "W34N17 8440C process-wide one-shot guard remains" >&2; exit 1
fi
rg -q 'wm_8008440C\(\);' pc_port/src/world_map_menu_lifecycle.c

echo "W34N17 MENU LIFECYCLE CERTIFICATE O0/O2/UBSan PASS; strict warnings clean; M1-M8 DETECTED; 8440C REENTRY ENABLED"
