#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

repo_root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$repo_root"

build_dir="pc_port/build_tests/w34n8_72db4"
mkdir -p "$build_dir"

base=(-DXENO_PC_PORT -std=c11 -Wall -Wextra -Werror -Wconversion -Wsign-conversion
      -Ipc_port -Ipc_port/src -Iinclude)
src=(pc_port/tests/w34n8_72db4_prod_test.c
     pc_port/src/world_map_helper_72db4.c)

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
    "m1:W34N8_MUTANT_SHORT_LOOP:ASSERT_DRAW_COUNT"
    "m2:W34N8_MUTANT_WRONG_INTENSITY_STEP:ASSERT_G4_INTENSITY"
    "m3:W34N8_MUTANT_G4_WRONG_BUCKET:ASSERT_PACKET_ORDER"
    "m4:W34N8_MUTANT_SKIP_FINAL_FREE:ASSERT_FREE_COUNT"
    "m5:W34N8_MUTANT_NO_ENV_ALTERNATION:ASSERT_DRAW_ENV_ALTERNATION"
    "m6:W34N8_MUTANT_SKIP_DRAW:ASSERT_DRAW_COUNT"
)

for spec in "${mutants[@]}"; do
    IFS=: read -r name define assertion <<<"$spec"
    exe="$build_dir/$name"
    log="$build_dir/$name.log"
    gcc "${base[@]}" -O2 -D"$define" "${src[@]}" -o "$exe"
    if "$exe" >"$log" 2>&1; then
        echo "W34N8 MUTANT $name SURVIVED" >&2
        exit 1
    fi
    if ! rg -q "$assertion" "$log"; then
        echo "W34N8 MUTANT $name failed without named assertion $assertion" >&2
        cat "$log" >&2
        exit 1
    fi
    echo "W34N8 MUTANT $name DETECTED by $assertion"
done

echo "W34N8 72DB4 CERTIFICATE O0/O2/UBSan PASS; strict warnings clean; M1-M6 DETECTED"
