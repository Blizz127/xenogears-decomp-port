#!/usr/bin/env bash
# Title -> New Game chain certificate (production-linked).
#
# Compiles the shipped field-script handler TUs (src/field/main/misc.c,
# src/field/main/misc11.c) and the shipped title-menu TU (src/menu/main/misc.c)
# in port mode, weakens only the title screen's draw/resource leaves and the
# per-frame pump in the menu object so the test can supply them, and links
# pc_port/tests/title_newgame_chain_prod_test.c against them with
# --gc-sections.  Differential across -O0 / -O2 / UBSan, then five mutants
# in the production sources that must BUILD and then FAIL at runtime with the
# named ASSERTION (a mutant that fails to compile is not a rejection).
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${TITLE_CHAIN_BUILD_DIR:-$ROOT/pc_port/build_native/title_newgame_chain}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -fno-pie -no-pie -m64 -fno-builtin
      -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY -DSKIP_ASM -D_LANGUAGE_C
      -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -fpermissive -w -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)

# Title-screen leaves the test replaces.  Everything else in the menu object
# stays the shipped code (func_801C7D78, func_801C58EC, func_801C531C,
# func_801C8574).
MENU_WEAKEN=(func_801C7BF4 func_801E8474 func_801E8018 func_801E8070
             func_801D22C4 func_801E8044 func_801E8978 func_801D3674
             func_801E3088 func_801D29A8 func_801D1EB0 func_801DE29C
             func_801DBE54 func_801E0F78 func_801E2BE4)

compile_and_run() {
    local name="$1"
    local run_rc=0
    shift
    "$CC" "${BASE[@]}" "${INC[@]}" "$@" -c src/field/main/misc.c \
        -o "$BUILD_DIR/${name}_field_misc.o"
    "$CC" "${BASE[@]}" "${INC[@]}" "$@" -c src/field/main/misc11.c \
        -o "$BUILD_DIR/${name}_field_misc11.o"
    # -fno-inline: objcopy weakens symbols AFTER compilation, so any leaf gcc
    # inlined into func_801C531C / func_801C58EC at -O2 would drag its whole
    # dependency tree into the retained sections.  The rest of -O2 codegen
    # (scheduling, register allocation, constant folding) still differs from
    # -O0, which is what the differential is for.
    "$CC" "${BASE[@]}" "${INC[@]}" "$@" -fno-inline -c src/menu/main/misc.c \
        -o "$BUILD_DIR/${name}_menu_misc.o"
    local weaken_args=()
    local sym
    for sym in "${MENU_WEAKEN[@]}"; do
        weaken_args+=(--weaken-symbol="$sym")
    done
    objcopy "${weaken_args[@]}" "$BUILD_DIR/${name}_menu_misc.o"
    "$CC" "${BASE[@]}" "${INC[@]}" "$@" \
        pc_port/tests/title_newgame_chain_prod_test.c \
        "$BUILD_DIR/${name}_field_misc.o" \
        "$BUILD_DIR/${name}_field_misc11.o" \
        "$BUILD_DIR/${name}_menu_misc.o" \
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
    grep -q '^TITLE NEWGAME CHAIN certificate PASS$' "$BUILD_DIR/$regime.stdout"
    test ! -s "$BUILD_DIR/$regime.stderr"
    cp "$BUILD_DIR/$regime.stdout" "$BUILD_DIR/$regime.normalized"
done
cmp "$BUILD_DIR/O0.normalized" "$BUILD_DIR/O2.normalized"
cmp "$BUILD_DIR/O0.normalized" "$BUILD_DIR/UBSan.normalized"
echo "TITLE NEWGAME CHAIN O0/O2/UBSAN PASS"

mutants=(
    'M1:TITLE_CHAIN_MUTANT_FE60_DROPS_CIRCLE_GATE:fe60.keeps.circle.interrupt.bit'
    'M2:TITLE_CHAIN_MUTANT_FE61_NEVER_RELEASES:fe61.releases.on.flag'
    'M3:TITLE_CHAIN_MUTANT_FE57_WRONG_MENU:fe57.requests.title.menu.2'
    'M4:TITLE_CHAIN_MUTANT_CONFIRM_ON_PRESS:menu.confirm.not.on.circle.press'
    'M5:TITLE_CHAIN_MUTANT_TITLE_UP_IS_DOWN:title.up.moves.continue.to.newgame'
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
    # A mutant that did not even produce a binary failed to COMPILE, which is
    # blind, not a rejection.
    if [[ ! -x "$BUILD_DIR/$label" ]]; then
        echo "$label did not build (blind control) rc=$rc" >&2
        exit 1
    fi
    if [[ "$rc" -eq 0 ]] || \
       ! grep -q "^ASSERTION ${assertion}$" "$BUILD_DIR/$label.stderr"; then
        echo "$label FAILED named mutant gate rc=$rc" >&2
        sed -n '1,80p' "$BUILD_DIR/$label.stderr" >&2 || true
        exit 1
    fi
    echo "$label DETECTED by $assertion"
done
echo "TITLE NEWGAME CHAIN CERTIFICATE PASS; M1-M5 DETECTED"
