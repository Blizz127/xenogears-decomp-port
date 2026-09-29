#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

repo_root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$repo_root"

fixture=disc/world_map.bin
expected_world=4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70
test "$(sha256sum "$fixture" | awk '{print $1}')" = "$expected_world"
test "$(dd if="$fixture" bs=1 skip=$((0x638c)) count=$((0x21c)) status=none | sha256sum | awk '{print $1}')" = 3a2e295fabc162d78e84c420a53b5795673f86d933bf8756e82f978a35b3f394
test "$(dd if="$fixture" bs=1 skip=$((0x24538)) count=$((0x38)) status=none | sha256sum | awk '{print $1}')" = 135be64ab129d14a479be4074296de5125120704db22375c6a348616baef971b

build_dir=pc_port/build_tests/w34n18_transition_selector
mkdir -p "$build_dir"
base=(-DXENO_PC_PORT -std=c11 -Wall -Wextra -Werror -Wconversion
      -Wsign-conversion -Ipc_port -Ipc_port/src -Iinclude)
src=(pc_port/tests/w34n18_transition_selector_prod_test.c
     pc_port/src/psx_memory.c pc_port/src/world_map_helper_75e7c.c)

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
    "m1:W34N18_MUTANT_CELL_BYTE_OFFSET:selector.cell.byte3.bitfield"
    "m2:W34N18_MUTANT_CELL_SHIFT:selector.cell.byte3.bitfield"
    "m3:W34N18_MUTANT_SKIP_AREA_REMAP:transition.area4.selected.remapped.record"
    "m4:W34N18_MUTANT_WRONG_BUCKET:threshold.retail.bucket"
    "m5:W34N18_MUTANT_COPY_RECORD_AREA:success.copies.base.0x200"
    "m6:W34N18_MUTANT_UNWEIGHTED_PICK:weighted.rand.boundaries"
    "m7:W34N18_MUTANT_SHORT_COPY:success.copies.base.0x200"
    "m8:W34N18_MUTANT_ZERO_SUM_WRITES:zero.sum.selected.unchanged"
)
for spec in "${mutants[@]}"; do
    IFS=: read -r name define assertion <<<"$spec"
    exe="$build_dir/$name"; log="$build_dir/$name.log"
    gcc "${base[@]}" -O2 -D"$define" "${src[@]}" -o "$exe"
    if "$exe" >"$log" 2>&1; then
        echo "W34N18 MUTANT $name SURVIVED" >&2
        exit 1
    fi
    if ! rg -q "ASSERTION $assertion" "$log"; then
        echo "W34N18 MUTANT $name failed without named assertion $assertion" >&2
        cat "$log" >&2
        exit 1
    fi
    echo "W34N18 MUTANT $name DETECTED by $assertion"
done

if rg -q 'worldmap-stub.*80075E7C|static void wm_80075E7C' \
        pc_port/src/world_map_frame_driver_712d0.c; then
    echo "W34N18 stale 75E7C stub remains" >&2
    exit 1
fi
rg -q 'wm_712d0_run_transition_lane\(\);' \
    pc_port/src/world_map_frame_driver_712d0.c
rg -q 'result = wm_80075E7C\(D_8009D55C, \(s32\)fd_lhu\(D_8006EF64\)\);' \
    pc_port/src/world_map_frame_driver_712d0.c

echo "W34N18 TRANSITION SELECTOR CERTIFICATE O0/O2/UBSan PASS; strict warnings clean; M1-M8 DETECTED"
