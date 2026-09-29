#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root"
out="pc_port/build_tests/w34n25_73398"
mkdir -p "$out"

fixture=disc/world_map.bin
expected_world=4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70
expected_slice=376357b7f2343061544eb3ff7666fd5e2a60cf6009efbf0ada9e7c5df6bc57d9
expected_table=e3862fd70af72043fe9d9436548aaca63fc735694bfb69bf6517d9d65a8d8626
test "$(sha256sum "$fixture" | awk '{print $1}')" = "$expected_world"
test "$(dd if="$fixture" bs=1 skip=$((0x38a8)) count=$((0xb0)) status=none | \
        sha256sum | awk '{print $1}')" = "$expected_slice"
test "$(dd if="$fixture" bs=1 skip=$((0x34)) count=28 status=none | \
        sha256sum | awk '{print $1}')" = "$expected_table"

base=(-DXENO_PC_PORT -DWM_73398_TEST_HOOKS -std=gnu17
      -Wall -Wextra -Werror -Wconversion -Wsign-conversion
      -Iinclude -Ipc_port/src)
src=(pc_port/tests/w34n25_73398_prod_test.c
     pc_port/src/world_map_helper_73398.c)

for regime in O0 O2 UBSan; do
    flags=(-O0)
    [[ "$regime" == O2 ]] && flags=(-O2)
    [[ "$regime" == UBSan ]] && \
        flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all)
    gcc "${base[@]}" "${flags[@]}" "${src[@]}" -o "$out/$regime"
    "$out/$regime"
done

mutants=(
  "M1:W34N25_MUTANT_SKIP_CLEAR:flag.cleared"
  "M2:W34N25_MUTANT_CLEAR_BEFORE_MODE:order.mode_before_clear"
  "M3:W34N25_MUTANT_WRONG_GROUP:mode2.group.direct"
  "M4:W34N25_MUTANT_SIGNED_DIRECT:direct.values.unsigned"
  "M5:W34N25_MUTANT_SWAP_DIRECT:direct.values.mapping"
  "M6:W34N25_MUTANT_WRITE_C5B0:direct.c5b0.preserved"
  "M7:W34N25_MUTANT_SKIP_DFF4:dff4.called"
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

rg -q 'pc_port/src/world_map_helper_73398.c' pc_port/build_port.sh
rg -Fq 'wm_80073398();' pc_port/src/world_map_session_setup_72238.c
rg -Fq 'wm_80073398();' pc_port/src/world_map_init.c

echo 'W34N25 73398 O0/O2/UBSan PASS; strict warnings clean; M1-M7 DETECTED'
