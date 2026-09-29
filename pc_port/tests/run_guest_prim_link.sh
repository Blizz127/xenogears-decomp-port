#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${GUEST_PRIM_LINK_OUT:-$ROOT/pc_port/build_native/guest_prim_link_cert}"
CC="${CC:-clang}"
mkdir -p "$OUT"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -ffunction-sections -fdata-sections
      -fno-pie -Wall -Wextra -Wconversion -Wsign-conversion -Werror
      -Ipc_port/include_shim -Iinclude -Ipc_port/src)
SRC=pc_port/src/guest_prim_link.c
TEST=pc_port/tests/guest_prim_link_prod_test.c

run_case() {
    local name="$1"
    shift
    "$CC" "${BASE[@]}" "$@" "$SRC" "$TEST" -no-pie \
        -Wl,--gc-sections -o "$OUT/$name"
    "$OUT/$name" >"$OUT/$name.stdout" 2>"$OUT/$name.stderr"
}

run_case O0 -O0 -g
run_case O2 -O2
run_case UBSan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all
for regime in O0 O2 UBSan; do
    rg -q '^GUEST PRIM LINK CERTIFICATE PASS$' "$OUT/$regime.stdout"
done
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan.stdout"

for entry in \
    'M1:guest.ot.receives_guest_address' \
    'M2:guest.prim.retains_len_and_predecessor' \
    'M3:guest.ot.receives_guest_address' \
    'M4:native.ot.retains_host_link'; do
    mutant="${entry%%:*}"
    assertion="${entry#*:}"
    set +e
    "$CC" "${BASE[@]}" -O0 -g -D"GUEST_PRIM_LINK_MUTANT_$mutant" \
        "$SRC" "$TEST" -no-pie -Wl,--gc-sections -o "$OUT/$mutant"
    "$OUT/$mutant" >"$OUT/$mutant.stdout" 2>"$OUT/$mutant.stderr"
    rc=$?
    set -e
    if [[ "$rc" -eq 0 ]] ||
       ! rg -q "^ASSERTION $assertion([[:space:]]|$)" "$OUT/$mutant.stderr"; then
        echo "$mutant FAILED expected assertion=$assertion rc=$rc" >&2
        exit 1
    fi
    echo "$mutant DETECTED assertion=$assertion"
done

echo "GUEST PRIM LINK PASS O0/O2/UBSan; M1-M4 DETECTED; strict warnings clean"
