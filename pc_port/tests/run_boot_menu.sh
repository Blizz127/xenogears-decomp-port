#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${BOOT_MENU_BUILD_DIR:-$ROOT/pc_port/build_native/boot_menu}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"
export BOOT_MENU_FIXTURE_DIR="$BUILD_DIR"

BASE=(-std=gnu17 -fno-pie -no-pie -m64 -fno-builtin
      -DXENO_PC_PORT -DBOOT_CERT_NO_STR -DBOOT_CERT_DIRECT_DISC -DSKIP_ASM -D_LANGUAGE_C
      -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -fpermissive -w -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
SRC=(pc_port/tests/boot_menu_prod_test.c
     pc_port/src/boot_menu.c
     pc_port/src/boot_assets.c)

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
    rg -q '^BOOT MENU certificate PASS$' "$BUILD_DIR/$regime.stdout"
    test ! -s "$BUILD_DIR/$regime.stderr"
    rg -v '^\[xeno-port\] PSX RAM emulation:' "$BUILD_DIR/$regime.stdout" \
        >"$BUILD_DIR/$regime.normalized" || true
done
cmp "$BUILD_DIR/O0.normalized" "$BUILD_DIR/O2.normalized"
cmp "$BUILD_DIR/O0.normalized" "$BUILD_DIR/UBSan.normalized"
echo "BOOT MENU O0/O2/UBSAN PASS"

mutants=(
    'M1:BOOT_MUTANT_SKIP_TITLE:boot.title.then.movie.then.menu'
    'M2:BOOT_MUTANT_SKIP_INIT:newgame.init.then.field'
    'M3:BOOT_MUTANT_LAHAN_DEST:newgame.field.dest.map14'
    'M4:BOOT_MUTANT_SKIP_INTRO:boot.intro.str.feed'
    'M5:BOOT_MUTANT_CONTINUE_SNAP:boot.continue.restores.save'
    'M6:BOOT_MUTANT_FONT_PLACEHOLDER:boot.menu.retail.graphic'
    'M7:BOOT_MUTANT_WRONG_STR:boot.intro.str.memcmp'
    'M8:BOOT_MUTANT_OVERRUN_TITLE:boot.menu.retail.graphic'
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
echo "BOOT MENU CERTIFICATE PASS; M1-M8 DETECTED"
