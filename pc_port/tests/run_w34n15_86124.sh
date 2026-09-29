#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

repo_root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$repo_root"

fixture=disc/world_map.bin
expected_world=4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70
test "$(sha256sum "$fixture" | awk '{print $1}')" = "$expected_world"

declare -a slices=(
    "0x16634:0x38:261e718bf13923cbb71d4c2941da2d403383b85751a254e7411f778aa8c3e207"
    "0x16bd8:0x38:8946a60aea0b5e5d4c349592835c8d75dc3cc080832884c051f0aada8c0ac061"
    "0x19638:0x38:66d3f7cde11d67c9a569b3bb514dd14121dcd19ba196a667e2155b3210a973b9"
)
for spec in "${slices[@]}"; do
    IFS=: read -r offset length expected <<<"$spec"
    actual="$(dd if="$fixture" bs=1 skip=$((offset)) count=$((length)) status=none | sha256sum | awk '{print $1}')"
    test "$actual" = "$expected"
done

build_dir=pc_port/build_tests/w34n15_86124
mkdir -p "$build_dir"
base=(-DXENO_PC_PORT -std=c11 -Wall -Wextra -Werror -Wconversion -Wsign-conversion
      -Ipc_port -Ipc_port/src -Iinclude)
src=(pc_port/tests/w34n15_86124_prod_test.c
     pc_port/src/psx_memory.c
     pc_port/src/world_map_helper_86124.c)

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
    "m1:W34N15_MUTANT_SWAP_86124:86124.frees.D7EC.then.D7E8"
    "m2:W34N15_MUTANT_WRONG_86124_SECOND:86124.frees.D7EC.then.D7E8"
    "m3:W34N15_MUTANT_SWAP_866C8:866C8.frees.D7FC.then.D7F8"
    "m4:W34N15_MUTANT_WRONG_866C8_SECOND:866C8.frees.D7FC.then.D7F8"
    "m5:W34N15_MUTANT_SWAP_89128:89128.frees.BE1C.then.BE20"
    "m6:W34N15_MUTANT_WRONG_89128_SECOND:89128.frees.BE1C.then.BE20"
)
for spec in "${mutants[@]}"; do
    IFS=: read -r name define assertion <<<"$spec"
    exe="$build_dir/$name"
    log="$build_dir/$name.log"
    gcc "${base[@]}" -O2 -D"$define" "${src[@]}" -o "$exe"
    if "$exe" >"$log" 2>&1; then
        echo "W34N15 MUTANT $name SURVIVED" >&2
        exit 1
    fi
    if ! rg -q "ASSERTION $assertion" "$log"; then
        echo "W34N15 MUTANT $name failed without named assertion $assertion" >&2
        cat "$log" >&2
        exit 1
    fi
    echo "W34N15 MUTANT $name DETECTED by $assertion"
done

echo "W34N15 PAIRED-FREE CERTIFICATE O0/O2/UBSan PASS; strict warnings clean; M1-M6 DETECTED"
