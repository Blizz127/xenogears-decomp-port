#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${FIELD_MENU_BUILD_DIR:-$ROOT/pc_port/build_native/field_menu}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -fno-pie -no-pie -m64 -fno-builtin
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -fpermissive -w -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
SRC=(pc_port/tests/field_menu_prod_test.c
     src/slus_006.64/system/menu.c)

compile_and_run() {
    local name="$1"
    local run_rc=0
    shift
    "$CC" "${BASE[@]}" "${INC[@]}" "$@" "${SRC[@]}" \
        -Wl,--gc-sections \
        -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" \
        2>"$BUILD_DIR/$name.stderr" || run_rc=$?
    return "$run_rc"
}

for regime in O0 O2 UBSan; do
    flags=(-O0 -g)
    if [[ "$regime" == O2 ]]; then flags=(-O2); fi
    if [[ "$regime" == UBSan ]]; then
        flags=(-O2 -g -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    compile_and_run "$regime" "${flags[@]}"
    rg -q '^FIELD MENU certificate PASS$' "$BUILD_DIR/$regime.stdout"
    test ! -s "$BUILD_DIR/$regime.stderr"
    rg -v '^\[xeno-port\] PSX RAM emulation:' "$BUILD_DIR/$regime.stdout" \
        >"$BUILD_DIR/$regime.normalized" || true
done
cmp "$BUILD_DIR/O0.normalized" "$BUILD_DIR/O2.normalized"
cmp "$BUILD_DIR/O0.normalized" "$BUILD_DIR/UBSan.normalized"
echo "FIELD MENU O0/O2/UBSAN PASS"

mutants=(
    'M1:MENU_MUTANT_SKIP_MENUMAIN:request.0x80.reaches.menumain'
    'M2:MENU_MUTANT_NEVER_REQUEST:root.menu.selected'
    'M3:MENU_MUTANT_SKIP_WAIT_CLEAR:menu.close.clears.wait'
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
    if [[ "$rc" -eq 0 ]] || \
       ! rg -q "^ASSERTION ${assertion}$" "$BUILD_DIR/$label.stderr"; then
        echo "$label FAILED named mutant gate rc=$rc" >&2
        sed -n '1,80p' "$BUILD_DIR/$label.stderr" >&2 || true
        exit 1
    fi
    echo "$label DETECTED"
done
echo "FIELD MENU CERTIFICATE PASS; M1-M3 DETECTED"
