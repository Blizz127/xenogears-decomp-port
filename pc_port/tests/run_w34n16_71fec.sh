#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

repo_root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$repo_root"

fixture=disc/world_map.bin
expected_world=4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70
expected_slice=49db0531db51dc7f74ac38402f18ec3877ac6bce69ee745a64f33fa06ec8cc90
test "$(sha256sum "$fixture" | awk '{print $1}')" = "$expected_world"
test "$(dd if="$fixture" bs=1 skip=$((0x24fc)) count=$((0xa4)) status=none | sha256sum | awk '{print $1}')" = "$expected_slice"

build_dir=pc_port/build_tests/w34n16_71fec
mkdir -p "$build_dir"
base=(-DXENO_PC_PORT -std=c11 -Wall -Wextra -Werror -Wconversion -Wsign-conversion
      -Ipc_port -Ipc_port/src -Iinclude)
src=(pc_port/tests/w34n16_71fec_prod_test.c
     pc_port/src/psx_memory.c
     pc_port/src/world_map_helper_71fec.c)

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
    "m1:W34N16_MUTANT_SWAP_DECODE_ORDER:decode.order.26.then.25"
    "m2:W34N16_MUTANT_WRONG_ALLOC_FLAG:alloc.sizes.and.flags.match.retail"
    "m3:W34N16_MUTANT_GUEST_MENU_AUTHORITY:menu.pointer.remains.native.authority"
    "m4:W34N16_MUTANT_RAW_SECOND_POINTER:second.pointer.published.as.guest.address"
    "m5:W34N16_MUTANT_SWAP_QUEUE_PAYLOADS:queue.payloads.match.retail.order"
    "m6:W34N16_MUTANT_MISSING_TERMINATOR:queue.has.zero.terminator"
    "m7:W34N16_MUTANT_WRONG_QUEUE_BASE:queue.submitted.from.D3F8.with.zero.args"
)
for spec in "${mutants[@]}"; do
    IFS=: read -r name define assertion <<<"$spec"
    exe="$build_dir/$name"
    log="$build_dir/$name.log"
    gcc "${base[@]}" -O2 -D"$define" "${src[@]}" -o "$exe"
    if "$exe" >"$log" 2>&1; then
        echo "W34N16 MUTANT $name SURVIVED" >&2
        exit 1
    fi
    if ! rg -q "ASSERTION $assertion" "$log"; then
        echo "W34N16 MUTANT $name failed without named assertion $assertion" >&2
        cat "$log" >&2
        exit 1
    fi
    echo "W34N16 MUTANT $name DETECTED by $assertion"
done

echo "W34N16 71FEC CERTIFICATE O0/O2/UBSan PASS; strict warnings clean; M1-M7 DETECTED"
