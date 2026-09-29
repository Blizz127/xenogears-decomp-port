#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

CC="${CC:-gcc}"
OUT="${W34N38_OUT:-pc_port/build_native/w34n38-89580-cert}"
TEST="pc_port/tests/w34n38_89580_prod_test.c"
PROD="pc_port/src/world_map_helper_89580.c"
BASE=(
    -std=gnu17 -DXENO_PC_PORT -DINCLUDE_ASM_USE_MACRO_INC=0 -fno-pie
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
)
WARN=(
    -Wall -Wextra -Werror -Wconversion -Wsign-conversion
    -Wshadow -Wundef -Wstrict-prototypes
)

mkdir -p "$OUT"

compile_and_run() {
    local name="$1"
    local rc=0
    shift
    rm -f "$OUT/$name" "$OUT/$name.stdout" "$OUT/$name.stderr"
    "$CC" "${BASE[@]}" "${WARN[@]}" "$@" "$TEST" "$PROD" \
        -no-pie -o "$OUT/$name"
    if "$OUT/$name" >"$OUT/$name.stdout" 2>"$OUT/$name.stderr"; then
        rc=0
    else
        rc=$?
    fi
    return "$rc"
}

compile_and_run O0 -O0 -g
compile_and_run O2 -O2 -g
compile_and_run UBSan -O2 -g -fsanitize=undefined \
    -fno-sanitize-recover=all

for regime in O0 O2 UBSan; do
    rg -q '^W34N38 0x80089580 full-body certificate PASS$' \
        "$OUT/$regime.stdout"
    test ! -s "$OUT/$regime.stderr"
done
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan.stdout"
echo "CERTIFICATE O0/O2/UBSan PASS; strict warnings clean"

mutants=(
    'M1:WM_89580_MUTANT_SKIP_OWNER_GATE:owner-zero-record-is-read-only'
    'M2:WM_89580_MUTANT_REMAINING_FROM_HIGH:remaining-low-halfword-decrements'
    'M3:WM_89580_MUTANT_WRONG_COUNTER_HALF:remaining-low-halfword-decrements'
    'M4:WM_89580_MUTANT_SKIP_SECOND_INTEGRATION:second-vector-integration-is-retail-addu'
    'M5:WM_89580_MUTANT_WRONG_UV_DELTA:uv-pairs-use-38-and-3a-deltas'
    'M6:WM_89580_MUTANT_COLOR_FROM_PACKED:rgb-deltas-come-from-word-40-and-clamp'
    'M7:WM_89580_MUTANT_NO_COLOR_CLAMP:rgb-deltas-come-from-word-40-and-clamp'
    'M8:WM_89580_MUTANT_WRONG_SLOT_STRIDE:expired-record-uses-bcc0-and-54-byte-owner-stride'
    'M9:WM_89580_MUTANT_WRONG_SLOT_GLOBAL:expired-record-uses-bcc0-and-54-byte-owner-stride'
    'M10:WM_89580_MUTANT_PARTICLE_GLOBAL:expired-record-uses-bcc0-and-54-byte-owner-stride'
    'M11:WM_89580_MUTANT_SLOT_OFFSET:expired-record-uses-bcc0-and-54-byte-owner-stride'
    'M12:WM_89580_MUTANT_WRONG_COUNT:all-256-records-use-4c-byte-stride'
    'M13:WM_89580_MUTANT_WRONG_PARTICLE_STRIDE:all-256-records-use-4c-byte-stride'
)

for entry in "${mutants[@]}"; do
    label="${entry%%:*}"
    rest="${entry#*:}"
    define="${rest%%:*}"
    assertion="${rest#*:}"
    set +e
    compile_and_run "$label" -O0 -g -D"$define"
    rc=$?
    set -e
    if [[ "$rc" -eq 0 ]] ||
       ! rg -q "^ASSERTION $assertion$" "$OUT/$label.stderr"; then
        echo "$label FAILED mutant gate assertion=$assertion rc=$rc" >&2
        exit 1
    fi
    echo "$label DETECTED; ASSERTION $assertion"
done

echo "W34N38 0x80089580 FULL CERTIFICATE PASS; M1-M13 DETECTED"
