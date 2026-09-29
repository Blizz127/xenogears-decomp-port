#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

repo_root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$repo_root"

fixture=disc/world_map.bin
expected_world=4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70
expected_slice=6f86842c69fe4ea4d262436e341fbdc37fdb49792bb8a19d5a80a1326d0dcb99
test "$(sha256sum "$fixture" | awk '{print $1}')" = "$expected_world"
test "$(dd if="$fixture" bs=1 skip=$((0x625c)) count=$((0x130)) status=none | sha256sum | awk '{print $1}')" = "$expected_slice"

build_dir=pc_port/build_tests/w34n14_75d4c
mkdir -p "$build_dir"
base=(-DXENO_PC_PORT -std=c11 -Wall -Wextra -Werror -Wconversion -Wsign-conversion
      -Ipc_port -Ipc_port/src -Iinclude)
src=(pc_port/tests/w34n14_75d4c_prod_test.c
     pc_port/src/psx_memory.c
     pc_port/src/world_map_helper_75d4c.c)

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
    "m1:W34N14_MUTANT_WRONG_POOL_STRIDE:addition.saves.active.to.backup"
    "m2:W34N14_MUTANT_WRONG_REMOVAL_DIRECTION:removal.restores.backup.to.active"
    "m3:W34N14_MUTANT_SKIP_SLOT_CONTROL_CLEAR:addition.clears.slot.control"
    "m4:W34N14_MUTANT_SKIP_RESYNC_TIMER:addition.arms.resync.timer"
    "m5:W34N14_MUTANT_WRONG_ACTIVE_PREDICATE:active.party.selects.mode2"
    "m6:W34N14_MUTANT_IGNORE_INPUT_FREEZE:input.freeze.preserves.mode"
)

for spec in "${mutants[@]}"; do
    IFS=: read -r name define assertion <<<"$spec"
    exe="$build_dir/$name"
    log="$build_dir/$name.log"
    gcc "${base[@]}" -O2 -D"$define" "${src[@]}" -o "$exe"
    if "$exe" >"$log" 2>&1; then
        echo "W34N14 MUTANT $name SURVIVED" >&2
        exit 1
    fi
    if ! rg -q "ASSERTION $assertion" "$log"; then
        echo "W34N14 MUTANT $name failed without named assertion $assertion" >&2
        cat "$log" >&2
        exit 1
    fi
    echo "W34N14 MUTANT $name DETECTED by $assertion"
done

echo "W34N14 75D4C CERTIFICATE O0/O2/UBSan PASS; strict warnings clean; M1-M6 DETECTED"
