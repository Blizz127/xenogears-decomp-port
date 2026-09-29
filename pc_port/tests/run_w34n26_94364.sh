#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root"
out="pc_port/build_tests/w34n26_94364"
mkdir -p "$out"

fixture=disc/world_map.bin
expected_world=4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70
expected_slice=6fbc417654acf248c20c124e97b8bbc5e4cce2e617c8e12e82d4c6aa69262efd
test "$(sha256sum "$fixture" | awk '{print $1}')" = "$expected_world"
test "$(dd if="$fixture" bs=1 skip=$((0x24874)) count=$((0xd0)) status=none | \
        sha256sum | awk '{print $1}')" = "$expected_slice"

base=(-DXENO_PC_PORT -std=gnu17 -Wall -Wextra -Werror -Wconversion
      -Wsign-conversion -Iinclude -Ipc_port/src)
src=(pc_port/tests/w34n26_94364_prod_test.c
     pc_port/src/world_map_helper_94364.c)

for regime in O0 O2 UBSan; do
    flags=(-O0)
    [[ "$regime" == O2 ]] && flags=(-O2)
    [[ "$regime" == UBSan ]] && \
        flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all)
    gcc "${base[@]}" "${flags[@]}" "${src[@]}" -o "$out/$regime"
    "$out/$regime"
done

mutants=(
  "M1:W34N26_MUTANT_WRONG_LIST_SCALE:list.index.scale"
  "M2:W34N26_MUTANT_MISSING_COORD_MASK:coord.mask"
  "M3:W34N26_MUTANT_EXCLUSIVE_BOUND:bounds.inclusive"
  "M4:W34N26_MUTANT_IGNORE_TYPE:type.match.scan"
  "M5:W34N26_MUTANT_UNSIGNED_TYPE:type.signed"
  "M6:W34N26_MUTANT_WRONG_STRIDE:stride.0x10"
  "M7:W34N26_MUTANT_PUBLISH_ON_MISS:miss.no_publish"
)
for spec in "${mutants[@]}"; do
    IFS=: read -r name define assertion <<<"$spec"
    gcc "${base[@]}" -O2 -D"$define" "${src[@]}" -o "$out/$name"
    set +e
    "$out/$name" >"$out/$name.log" 2>&1
    rc=$?
    set -e
    if [[ $rc -eq 0 ]] ||
       ! rg -q "ASSERTION $assertion FAILED" "$out/$name.log"; then
        echo "$name failed mutant gate rc=$rc expected=$assertion" >&2
        cat "$out/$name.log" >&2
        exit 1
    fi
    echo "$name DETECTED by ASSERTION $assertion"
done

rg -q 'pc_port/src/world_map_helper_94364.c' pc_port/build_port.sh
echo 'W34N26 94364 O0/O2/UBSan PASS; strict warnings clean; M1-M7 DETECTED'
