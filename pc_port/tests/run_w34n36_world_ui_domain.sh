#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34N36_UI_BUILD_DIR:-$ROOT/pc_port/build_native/w34n36_world_ui}"
CC="${CC:-clang}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -fno-pie -no-pie)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/src)
SRC=(pc_port/tests/w34n36_world_ui_domain_prod_test.c
     pc_port/src/psx_memory.c
     pc_port/src/world_map_callback_92c70.c
     pc_port/src/world_map_callback_92fd8.c)

test "$(rg -c 'SYSTEM_ADD_PRIM\(ot, pWindow \+ 0x(3C|30)\)|SYSTEM_ADD_PRIM\(ot, prim \+ 0x48\)' \
    src/slus_006.64/system/system.c)" -eq 3
test "$(rg -c 'PcPort_AddPrimDomainAware\(ot, prim\);' \
    src/slus_006.64/system/temp2.c)" -eq 1
test "$(rg -c 'GetStringEntry\(c70_string_table_pointer\(table\), index\)' \
    pc_port/src/world_map_callback_92c70.c)" -eq 1
test "$(rg -c 'GetStringEntry\(fd_string_table_pointer\(table\), index\)' \
    pc_port/src/world_map_callback_92fd8.c)" -eq 1
echo "SOURCE_INVENTORY=3_DOMAIN_AWARE_SYSTEM_LINKS+TEMP2_LINK+2_STRING_LOOKUPS"

compile_and_run() {
    local name="$1"
    local run_rc=0
    shift
    "$CC" "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" "${SRC[@]}" \
        -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.raw" \
        2>"$BUILD_DIR/$name.stderr" || run_rc=$?
    rg -v 'PSX RAM emulation' "$BUILD_DIR/$name.raw" \
        >"$BUILD_DIR/$name.stdout" || true
    return "$run_rc"
}

compile_and_run O0 -O0 -g
compile_and_run O2 -O2
compile_and_run UBSan -O2 -g -fsanitize=undefined \
    -fno-sanitize-recover=all
for regime in O0 O2 UBSan; do
    rg -q '^W34N36 world UI guest-domain certificate PASS$' \
        "$BUILD_DIR/$regime.stdout"
    test ! -s "$BUILD_DIR/$regime.stderr"
done
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O2.stdout"
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/UBSan.stdout"
echo "CERTIFICATE O0/O2/UBSan PASS; strict warnings clean"

mutants=(
    'M1:WM_UI_MUTANT_RAW_STRING_TABLE:c70-string-table-rebased-from-guest'
    'M2:WM_UI_MUTANT_RAW_OT:c70-guest-ot-rebased-for-system-renderer'
    'M3:WM_UI_MUTANT_SKIP_STRING_LOOKUP:c70-compiled-string-lookup-called'
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
       ! rg -q "^ASSERTION $assertion$" "$BUILD_DIR/$label.stderr"; then
        echo "$label FAILED mutant gate assertion=$assertion rc=$rc" >&2
        exit 1
    fi
    echo "$label DETECTED; ASSERTION $assertion"
done

echo "W34N36 world UI certificate PASS; M1-M3 DETECTED"
